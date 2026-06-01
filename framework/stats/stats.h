/* Copyright (c) 2018-2026, Arm Limited and Contributors
 * Copyright (c) 2020-2026, Broadcom Inc.
 * Copyright (c) 2026, NVIDIA CORPORATION. All rights reserved.
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

#pragma once

#include <future>

#include "core/util/profiling.hpp"
#include "stats/frame_time_stats_provider.h"
#include "stats/stats_common.h"
#include "stats/stats_provider.h"
#include "stats/vulkan_stats_provider.h"
#include "timer.h"
#ifdef VK_USE_PLATFORM_ANDROID_KHR
#	include "stats/hwcpipe_stats_provider.h"
#endif

namespace vkb
{
namespace stats
{
/*
 * @brief Helper class for querying statistics about the CPU and the GPU
 */
template <vkb::BindingType bindingType>
class Stats
{
  public:
	/**
	 * @brief Constructs a Stats object
	 * @param render_context The RenderContext for this sample
	 * @param buffer_size Size of the circular buffers
	 */
	explicit Stats(vkb::rendering::RenderContext<bindingType> &render_context, size_t buffer_size = 16);

	/**
	 * @brief Destroys the Stats object
	 */
	~Stats();

	/**
	 * @brief A command buffer that we want to collect stats about has just begun
	 *
	 * Some stats providers (like the Vulkan extension one) can only collect stats
	 * about the execution of a specific command buffer. In those cases we need to
	 * know when a command buffer has begun and when it's about to end so that we
	 * can inject some extra commands into the command buffer to control the stats
	 * collection. This method tells the stats provider that a command buffer has
	 * begun so that can happen. The command buffer must be in a recording state
	 * when this method is called.
	 * @param cb The command buffer
	 */
	void begin_sampling(vkb::core::CommandBuffer<bindingType> &cb);

	/**
	 * @brief A command buffer that we want to collect stats about is about to be ended
	 *
	 * Some stats providers (like the Vulkan extension one) can only collect stats
	 * about the execution of a specific command buffer. In those cases we need to
	 * know when a command buffer has begun and when it's about to end so that we
	 * can inject some extra commands into the command buffer to control the stats
	 * collection. This method tells the stats provider that a command buffer is
	 * about to be ended so that can happen. The command buffer must be in a recording
	 * state when this method is called.
	 * @param cb The command buffer
	 */
	void end_sampling(vkb::core::CommandBuffer<bindingType> &cb);

	/**
	 * @brief Returns the collected data for a specific statistic
	 * @param index The stat index of the data requested
	 * @return The data of the specified stat
	 */
	std::vector<float> const &get_data(StatIndex index) const;

	/**
	 * @brief Returns data relevant for graphing a specific statistic
	 * @param index The stat index of the data requested
	 * @return The data of the specified stat
	 */
	StatGraphData const &get_graph_data(StatIndex index) const;

	/**
	 * @return The requested stats
	 */
	std::set<StatIndex> const &get_requested_stats() const;

	/**
	 * @brief Checks if an enabled stat is available in the current platform
	 * @param index The stat index
	 * @return True if the stat is available, false otherwise
	 */
	bool is_available(StatIndex index) const;

	/**
	 * @brief Request specific set of stats to be collected
	 * @param requested_stats Set of stats to be collected if available
	 * @param sampling_config Sampling mode configuration (polling or continuous)
	 */
	void request_stats(const std::set<StatIndex> &requested_stats,
	                   CounterSamplingConfig      sampling_config = {CounterSamplingMode::Polling});

	/**
	 * @brief Resizes the stats buffers according to the width of the screen
	 * @param width The width of the screen
	 */
	void resize(size_t width);

	/**
	 * @brief Update statistics, must be called after every frame
	 * @param delta_time Time since last update
	 */
	void update(float delta_time);

  private:
	/// The worker thread function for continuous sampling;
	/// it adds a new entry to continuous_samples at every interval
	void continuous_sampling_worker(std::future<void> should_terminate);

	// Push counters to external profilers
	void profile_counters() const;

	/// Updates circular buffers for CPU and GPU counters
	void push_sample(const vkb::StatsProvider::Counters &sample);

  private:
	float                                            alpha_smoothing = 0.2f;                          // Alpha smoothing for running average
	size_t                                           buffer_size;                                     // Size of the circular buffers
	std::vector<vkb::StatsProvider::Counters>        continuous_samples;                              // The samples read during continuous sampling
	std::mutex                                       continuous_sampling_mutex;                       // A mutex for accessing measurements during continuous sampling
	std::map<StatIndex, std::vector<float>>          counters;                                        // Circular buffers for counter data
	float                                            fractional_pending_samples = 0.0f;               // A value which helps keep a steady pace of continuous samples output.
	vkb::StatsProvider                              *frame_time_provider;                             // Provider that tracks frame times
	vkb::Timer                                       main_timer;                                      // vkb::Timer used in the main thread to compute delta time
	std::vector<vkb::StatsProvider::Counters>        pending_samples;                                 // The samples waiting to be displayed
	std::vector<std::unique_ptr<vkb::StatsProvider>> providers;                                       // A list of stats providers to use in priority order
	vkb::rendering::RenderContextCpp                &render_context;                                  // The render context
	std::set<StatIndex>                              requested_stats;                                 // Stats that were requested - they may not all be available
	CounterSamplingConfig                            sampling_config;                                 // Counter sampling configuration
	bool                                             should_add_to_continuous_samples = false;        // A flag specifying if the worker thread should add entries to continuous_samples
	std::unique_ptr<std::promise<void>>              stop_worker;                                     // Promise to stop the worker thread
	std::thread                                      worker_thread;                                   // Worker thread for continuous sampling
	vkb::Timer                                       worker_timer;                                    // vkb::Timer used by the worker thread to throttle counter sampling
};

using StatsC   = Stats<vkb::BindingType::C>;
using StatsCpp = Stats<vkb::BindingType::Cpp>;

namespace
{
static inline void add_smoothed_value(std::vector<float> &values, float value, float alpha)
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

// For now names are taken from the stats_provider.cpp file
static inline char const *to_string(StatIndex index)
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

// Member function definitions

template <>
inline Stats<vkb::BindingType::Cpp>::Stats(vkb::rendering::RenderContextCpp &render_context, size_t buffer_size) :
    render_context(render_context),
    buffer_size(buffer_size)
{
	assert(buffer_size >= 2 && "Buffers size should be greater than 2");
}

template <>
inline Stats<vkb::BindingType::C>::Stats(vkb::rendering::RenderContextC &render_context, size_t buffer_size) :
    render_context(reinterpret_cast<vkb::rendering::RenderContextCpp &>(render_context)),
    buffer_size(buffer_size)
{
	assert(buffer_size >= 2 && "Buffers size should be greater than 2");
}

template <vkb::BindingType bindingType>
inline Stats<bindingType>::~Stats()
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

template <vkb::BindingType bindingType>
inline void Stats<bindingType>::begin_sampling(vkb::core::CommandBuffer<bindingType> &cb)
{
	// Inform the providers
	for (auto &p : providers)
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			p->begin_sampling(reinterpret_cast<vkb::core::CommandBufferC &>(cb));
		}
		else
		{
			p->begin_sampling(cb);
		}
	}
}

template <vkb::BindingType bindingType>
inline void Stats<bindingType>::continuous_sampling_worker(std::future<void> should_terminate)
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
		vkb::StatsProvider::Counters sample;
		for (auto &p : providers)
		{
			vkb::StatsProvider::Counters s = p->continuous_sample(delta_time);
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

template <vkb::BindingType bindingType>
inline void Stats<bindingType>::end_sampling(vkb::core::CommandBuffer<bindingType> &cb)
{
	// Inform the providers
	for (auto &p : providers)
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			p->end_sampling(reinterpret_cast<vkb::core::CommandBufferC &>(cb));
		}
		else
		{
			p->end_sampling(cb);
		}
	}
}

template <vkb::BindingType bindingType>
inline std::vector<float> const &Stats<bindingType>::get_data(StatIndex index) const
{
	return counters.at(index);
};

template <vkb::BindingType bindingType>
inline StatGraphData const &Stats<bindingType>::get_graph_data(StatIndex index) const
{
	for (auto &p : providers)
	{
		if (p->is_available(index))
		{
			return p->get_graph_data(index);
		}
	}
	return vkb::StatsProvider::default_graph_data(index);
}

template <vkb::BindingType bindingType>
inline std::set<StatIndex> const &Stats<bindingType>::get_requested_stats() const
{
	return requested_stats;
}

template <vkb::BindingType bindingType>
inline bool Stats<bindingType>::is_available(const StatIndex index) const
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

template <vkb::BindingType bindingType>
inline void Stats<bindingType>::profile_counters() const
{
#if VKB_PROFILING
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
	VmaAllocator allocator = allocated::get_memory_allocator();

	VmaBudget heap_budgets[VK_MAX_MEMORY_HEAPS];
	vmaGetHeapBudgets(allocator, heap_budgets);

	// We know that we will only ever have one device in the system, so we can cache the labels
	if (labels.size() == 0)
	{
		vk::PhysicalDeviceMemoryProperties memory_properties = device.get_gpu().get_handle().getMemoryProperties();

		labels.reserve(memory_properties.memoryHeapCount);

		for (size_t heap = 0; heap < memory_properties.memoryHeapCount; heap++)
		{
			vk::MemoryHeapFlags flags = memory_properties.memoryHeaps[heap].flags;
			labels.push_back("Heap " + std::to_string(heap) + " " + vk::to_string(flags));
		}
	}

	for (size_t heap = 0; heap < labels.size(); heap++)
	{
		Plot<float, PlotType::Memory>::plot(labels[heap].c_str(), heap_budgets[heap].usage / (1024.0f * 1024.0f));
	}
#endif
}

template <vkb::BindingType bindingType>
inline void Stats<bindingType>::push_sample(const vkb::StatsProvider::Counters &sample)
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

template <vkb::BindingType bindingType>
inline void Stats<bindingType>::request_stats(const std::set<StatIndex> &wanted_stats, CounterSamplingConfig config)
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
	providers.emplace_back(std::make_unique<vkb::FrameTimeStatsProvider>(stats));
#ifdef VK_USE_PLATFORM_ANDROID_KHR
	providers.emplace_back(std::make_unique<HWCPipeStatsProvider>(stats));
#endif
	providers.emplace_back(std::make_unique<vkb::VulkanStatsProvider>(stats, sampling_config, reinterpret_cast<vkb::rendering::RenderContextC &>(render_context)));

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

template <vkb::BindingType bindingType>
inline void Stats<bindingType>::resize(const size_t width)
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

template <vkb::BindingType bindingType>
inline void Stats<bindingType>::update(float delta_time)
{
	switch (sampling_config.mode)
	{
		case CounterSamplingMode::Polling:
		{
			vkb::StatsProvider::Counters sample;

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
			vkb::StatsProvider::Counters frame_time_sample = frame_time_provider->sample(delta_time);

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

}        // namespace stats
}        // namespace vkb