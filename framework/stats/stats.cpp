/* Copyright (c) 2018-2024, Arm Limited and Contributors
 * Copyright (c) 2020-2024, Broadcom Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "stats/stats.h"

#include <core/util/profiling.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "core/device.h"
#include "frame_time_stats_provider.h"
#ifdef VK_USE_PLATFORM_ANDROID_KHR
#	include "hwcpipe_stats_provider.h"
#endif
#include "vulkan_stats_provider.h"

namespace vkb
{
Stats::Stats(RenderContext &render_context, size_t buffer_size) :
    render_context(render_context),
    buffer_size(buffer_size)
{
	assert(buffer_size >= 2 && "Buffers size should be greater than 2");
}

Stats::~Stats()
{
	if (stop_worker)
	{
		stop_worker->set_value();
	}

	if (worker_thread.joinable())
	{
		worker_thread.join();
	}
}

void Stats::request_stats(const std::set<StatIndex> &wanted_stats,
                          CounterSamplingConfig      config)
{
	if (providers.size() != 0)
	{
		throw std::runtime_error("Stats must only be requested once");
	}

	requested_stats = wanted_stats;
	sampling_config = config;

	// Copy the requested stats, so they can be changed by the providers below
	std::set<StatIndex> stats = requested_stats;

	// Initialize our list of providers (in priority order)
	// All supported stats will be removed from the given 'stats' set by the provider's constructor
	// so subsequent providers only see requests for stats that aren't already supported.
	providers.emplace_back(std::make_unique<FrameTimeStatsProvider>(stats));
#ifdef VK_USE_PLATFORM_ANDROID_KHR
	providers.emplace_back(std::make_unique<HWCPipeStatsProvider>(stats));
#endif
	providers.emplace_back(std::make_unique<VulkanStatsProvider>(stats, sampling_config, render_context));

	// In continuous sampling mode we still need to update the frame times as if we are polling
	// Store the frame time provider here so we can easily access it later.
	frame_time_provider = providers[0].get();

	for (const auto &stat : requested_stats)
	{
		counters[stat] = std::vector<float>(buffer_size, 0);
	}

	if (sampling_config.mode == CounterSamplingMode::Continuous)
	{
		// Start a thread for continuous sample capture
		stop_worker = std::make_unique<std::promise<void>>();

		worker_thread = std::thread([this] {
			continuous_sampling_worker(stop_worker->get_future());
		});

		// Reduce smoothing for continuous sampling
		alpha_smoothing = 0.6f;
	}

	for (const auto &stat_index : requested_stats)
	{
		if (!is_available(stat_index))
		{
			LOGW(vkb::StatsProvider::default_graph_data(stat_index).name + " : not available");
		}
	}
}

void Stats::resize(const size_t width)
{
	// The circular buffer size will be 1/16th of the width of the screen
	// which means every sixteen pixels represent one graph value
	buffer_size = width >> 4;

	for (auto &counter : counters)
	{
		counter.second.resize(buffer_size);
		counter.second.shrink_to_fit();
	}
}

bool Stats::is_available(const StatIndex index) const
{
	for (const auto &p : providers)
	{
		if (p->is_available(index))
		{
			return true;
		}
	}
	return false;
}

static void add_smoothed_value(std::vector<float> &values, float value, float alpha)
{
	assert(values.size() >= 2 && "Buffers size should be greater than 2");

	if (values.size() == values.capacity())
	{
		// Shift values to the left to make space at the end and update counters
		std::rotate(values.begin(), values.begin() + 1, values.end());
	}

	// Use an exponential moving average to smooth values
	values.back() = value * alpha + *(values.end() - 2) * (1.0f - alpha);
}

void Stats::update(float delta_time)
{
	switch (sampling_config.mode)
	{
		case CounterSamplingMode::Polling:
		{
			StatsProvider::Counters sample;

			for (auto &p : providers)
			{
				auto s = p->sample(delta_time);
				sample.insert(s.begin(), s.end());
			}
			push_sample(sample);
			break;
		}
		case CounterSamplingMode::Continuous:
		{
			// Check that we have no pending samples to be shown
			if (pending_samples.size() == 0)
			{
				std::unique_lock<std::mutex> lock(continuous_sampling_mutex);
				if (!should_add_to_continuous_samples)
				{
					// If we have no pending samples, we let the worker thread
					// capture samples for the next frame
					should_add_to_continuous_samples = true;
				}
				else
				{
					// The worker thread has captured a frame, so we stop it
					// and read the samples
					should_add_to_continuous_samples = false;
					pending_samples.clear();
					std::swap(pending_samples, continuous_samples);
				}
			}

			if (pending_samples.size() == 0)
			{
				return;
			}

			// Ensure the number of pending samples is capped at a reasonable value
			if (pending_samples.size() > 100)
			{
				// Prefer later samples over new samples.
				std::move(pending_samples.end() - 100, pending_samples.end(), pending_samples.begin());
				pending_samples.erase(pending_samples.begin() + 100, pending_samples.end());

				// If we get to this point, we're not reading samples fast enough, nudge a little ahead.
				fractional_pending_samples += 1.0f;
			}

			// Compute the number of samples to show this frame
			float floating_sample_count = sampling_config.speed * delta_time * static_cast<float>(buffer_size) + fractional_pending_samples;

			// Keep track of the fractional value to avoid speeding up or slowing down too much due to rounding errors.
			// Generally we push very few samples per frame, so this matters.
			fractional_pending_samples = floating_sample_count - std::floor(floating_sample_count);

			auto sample_count = static_cast<size_t>(floating_sample_count);

			// Clamp the number of samples
			sample_count = std::max<size_t>(1, std::min<size_t>(sample_count, pending_samples.size()));

			// Get the frame time stats (not a continuous stat)
			StatsProvider::Counters frame_time_sample = frame_time_provider->sample(delta_time);

			// Push the samples to circular buffers
			std::for_each(pending_samples.begin(), pending_samples.begin() + sample_count, [this, frame_time_sample](auto &s) {
				// Write the correct frame time into the continuous stats
				s.insert(frame_time_sample.begin(), frame_time_sample.end());
				// Then push the sample to the counters list
				this->push_sample(s);
			});
			pending_samples.erase(pending_samples.begin(), pending_samples.begin() + sample_count);

			break;
		}
	}

	profile_counters();
}

void Stats::continuous_sampling_worker(std::future<void> should_terminate)
{
	worker_timer.tick();

	for (auto &p : providers)
	{
		p->continuous_sample(0.0f);
	}

	while (should_terminate.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
	{
		auto delta_time = static_cast<float>(worker_timer.tick());
		auto interval   = std::chrono::duration_cast<std::chrono::duration<float>>(sampling_config.interval).count();

		// Ensure we wait for the interval specified in config
		if (delta_time < interval)
		{
			std::this_thread::sleep_for(std::chrono::duration<float>(interval - delta_time));
			delta_time += static_cast<float>(worker_timer.tick());
		}

		// Sample counters
		StatsProvider::Counters sample;
		for (auto &p : providers)
		{
			StatsProvider::Counters s = p->continuous_sample(delta_time);
			sample.insert(s.begin(), s.end());
		}

		// Add the new sample to the vector of continuous samples
		{
			std::unique_lock<std::mutex> lock(continuous_sampling_mutex);
			if (should_add_to_continuous_samples)
			{
				continuous_samples.push_back(sample);
			}
		}
	}
}

void Stats::push_sample(const StatsProvider::Counters &sample)
{
	for (auto &c : counters)
	{
		StatIndex           idx    = c.first;
		std::vector<float> &values = c.second;

		// Find the counter matching this StatIndex in the Sample
		const auto &smp = sample.find(idx);
		if (smp == sample.end())
		{
			continue;
		}

		float measurement = static_cast<float>(smp->second.result);

		add_smoothed_value(values, measurement, alpha_smoothing);
	}
}

namespace
{
// For now names are taken from the stats_provider.cpp file
const char *to_string(StatIndex index)
{
	switch (index)
	{
		case StatIndex::frame_times:
			return "Frame Times (ms)";
		case StatIndex::cpu_cycles:
			return "CPU Cycles (M/s)";
		case StatIndex::cpu_instructions:
			return "CPU Instructions (M/s)";
		case StatIndex::cpu_cache_miss_ratio:
			return "Cache Miss Ratio (%)";
		case StatIndex::cpu_branch_miss_ratio:
			return "Branch Miss Ratio (%)";
		case StatIndex::cpu_l1_accesses:
			return "CPU L1 Accesses (M/s)";
		case StatIndex::cpu_instr_retired:
			return "CPU Instructions Retired (M/s)";
		case StatIndex::cpu_l2_accesses:
			return "CPU L2 Accesses (M/s)";
		case StatIndex::cpu_l3_accesses:
			return "CPU L3 Accesses (M/s)";
		case StatIndex::cpu_bus_reads:
			return "CPU Bus Read Beats (M/s)";
		case StatIndex::cpu_bus_writes:
			return "CPU Bus Write Beats (M/s)";
		case StatIndex::cpu_mem_reads:
			return "CPU Memory Read Instructions (M/s)";
		case StatIndex::cpu_mem_writes:
			return "CPU Memory Write Instructions (M/s)";
		case StatIndex::cpu_ase_spec:
			return "CPU Speculatively Exec. SIMD Instructions (M/s)";
		case StatIndex::cpu_vfp_spec:
			return "CPU Speculatively Exec. FP Instructions (M/s)";
		case StatIndex::cpu_crypto_spec:
			return "CPU Speculatively Exec. Crypto Instructions (M/s)";
		case StatIndex::gpu_cycles:
			return "GPU Cycles (M/s)";
		case StatIndex::gpu_vertex_cycles:
			return "Vertex Cycles (M/s)";
		case StatIndex::gpu_load_store_cycles:
			return "Load Store Cycles (k/s)";
		case StatIndex::gpu_tiles:
			return "Tiles (k/s)";
		case StatIndex::gpu_killed_tiles:
			return "Tiles killed by CRC match (k/s)";
		case StatIndex::gpu_fragment_jobs:
			return "Fragment Jobs (s)";
		case StatIndex::gpu_fragment_cycles:
			return "Fragment Cycles (M/s)";
		case StatIndex::gpu_tex_cycles:
			return "Shader Texture Cycles (k/s)";
		case StatIndex::gpu_ext_reads:
			return "External Reads (M/s)";
		case StatIndex::gpu_ext_writes:
			return "External Writes (M/s)";
		case StatIndex::gpu_ext_read_stalls:
			return "External Read Stalls (M/s)";
		case StatIndex::gpu_ext_write_stalls:
			return "External Write Stalls (M/s)";
		case StatIndex::gpu_ext_read_bytes:
			return "External Read Bytes (MiB/s)";
		case StatIndex::gpu_ext_write_bytes:
			return "External Write Bytes (MiB/s)";
		default:
			return nullptr;
	}
}
}        // namespace

void Stats::profile_counters() const
{
	static std::chrono::high_resolution_clock::time_point last_time = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point        now       = std::chrono::high_resolution_clock::now();

	if (now - last_time < std::chrono::milliseconds(100))
	{
		return;
	}

	last_time = now;

	for (auto &c : counters)
	{
		StatIndex idx        = c.first;
		auto     &graph_data = get_graph_data(idx);

		if (c.second.empty())
		{
			continue;
		}

		float average = 0.0f;
		for (auto &v : c.second)
		{
			average += v;
		}
		average /= c.second.size();

		if (auto *index_name = to_string(idx))
		{
			Plot<float>::plot(index_name, average * graph_data.scale_factor);
		}
	}

	static std::vector<std::string> labels;

	auto        &device    = render_context.get_device();
	VmaAllocator allocator = device.get_memory_allocator();

	VmaBudget heap_budgets[VK_MAX_MEMORY_HEAPS];
	vmaGetHeapBudgets(allocator, heap_budgets);

	// We know that we will only ever have one device in the system, so we can cache the labels
	if (labels.size() == 0)
	{
		VkPhysicalDeviceMemoryProperties memory_properties;
		vkGetPhysicalDeviceMemoryProperties(device.get_gpu().get_handle(), &memory_properties);

		labels.reserve(memory_properties.memoryHeapCount);

		for (size_t heap = 0; heap < memory_properties.memoryHeapCount; heap++)
		{
			VkMemoryPropertyFlags flags = memory_properties.memoryHeaps[heap].flags;
			labels.push_back("Heap " + std::to_string(heap) + " " + vk::to_string(vk::MemoryPropertyFlags{flags}));
		}
	}

	for (size_t heap = 0; heap < labels.size(); heap++)
	{
		Plot<float, PlotType::Memory>::plot(labels[heap].c_str(), heap_budgets[heap].usage / (1024.0f * 1024.0f));
	}
}

void Stats::begin_sampling(CommandBuffer &cb)
{
	// Inform the providers
	for (auto &p : providers)
	{
		p->begin_sampling(cb);
	}
}

void Stats::end_sampling(CommandBuffer &cb)
{
	// Inform the providers
	for (auto &p : providers)
	{
		p->end_sampling(cb);
	}
}

const StatGraphData &Stats::get_graph_data(StatIndex index) const
{
	for (auto &p : providers)
	{
		if (p->is_available(index))
		{
			return p->get_graph_data(index);
		}
	}
	return StatsProvider::default_graph_data(index);
}

StatGraphData::StatGraphData(const std::string &name,
                             const std::string &graph_label_format,
                             float              scale_factor,
                             bool               has_fixed_max,
                             float              max_value) :
    name(name),
    format{graph_label_format},
    scale_factor{scale_factor},
    has_fixed_max{has_fixed_max},
    max_value{max_value}
{
}

}        // namespace vkb
