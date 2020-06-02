/* Copyright (c) 2018-2020, Arm Limited and Contributors
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

#include "stats.h"
#include "gui.h"

#include "common/error.h"

namespace vkb
{
Stats::Stats(const std::set<StatIndex> &enabled_stats, CounterSamplingConfig sampling_config,
             const size_t buffer_size) :
    enabled_stats(enabled_stats),
    sampling_config(sampling_config),
    stop_worker(std::make_unique<std::promise<void>>())
{
	assert(buffer_size >= 2 && "Buffers size should be greater than 2");

	for (const auto &stat : enabled_stats)
	{
		counters[stat] = std::vector<float>(buffer_size, 0);
	}

	stat_data = {
	    {StatIndex::frame_times, {StatScaling::None}},
	    {StatIndex::cpu_cycles, {hwcpipe::CpuCounter::Cycles}},
	    {StatIndex::cpu_instructions, {hwcpipe::CpuCounter::Instructions}},
	    {StatIndex::cache_miss_ratio, {hwcpipe::CpuCounter::CacheMisses, StatScaling::ByCounter, hwcpipe::CpuCounter::CacheReferences}},
	    {StatIndex::branch_miss_ratio, {hwcpipe::CpuCounter::BranchMisses, StatScaling::ByCounter, hwcpipe::CpuCounter::BranchInstructions}},
	    {StatIndex::gpu_cycles, {hwcpipe::GpuCounter::GpuCycles}},
	    {StatIndex::vertex_compute_cycles, {hwcpipe::GpuCounter::VertexComputeCycles}},
	    {StatIndex::tiles, {hwcpipe::GpuCounter::Tiles}},
	    {StatIndex::killed_tiles, {hwcpipe::GpuCounter::TransactionEliminations}},
	    {StatIndex::fragment_cycles, {hwcpipe::GpuCounter::FragmentCycles}},
	    {StatIndex::fragment_jobs, {hwcpipe::GpuCounter::FragmentJobs}},
	    {StatIndex::l2_reads_lookups, {hwcpipe::GpuCounter::CacheReadLookups}},
	    {StatIndex::l2_ext_reads, {hwcpipe::GpuCounter::ExternalMemoryReadAccesses}},
	    {StatIndex::l2_writes_lookups, {hwcpipe::GpuCounter::CacheWriteLookups}},
	    {StatIndex::l2_ext_writes, {hwcpipe::GpuCounter::ExternalMemoryWriteAccesses}},
	    {StatIndex::l2_ext_read_stalls, {hwcpipe::GpuCounter::ExternalMemoryReadStalls}},
	    {StatIndex::l2_ext_write_stalls, {hwcpipe::GpuCounter::ExternalMemoryWriteStalls}},
	    {StatIndex::l2_ext_read_bytes, {hwcpipe::GpuCounter::ExternalMemoryReadBytes}},
	    {StatIndex::l2_ext_write_bytes, {hwcpipe::GpuCounter::ExternalMemoryWriteBytes}},
	    {StatIndex::tex_cycles, {hwcpipe::GpuCounter::ShaderTextureCycles}},
	};

	hwcpipe::CpuCounterSet enabled_cpu_counters{};
	hwcpipe::GpuCounterSet enabled_gpu_counters{};

	for (const auto &stat : enabled_stats)
	{
		auto res = stat_data.find(stat);
		if (res != stat_data.end())
		{
			switch (res->second.type)
			{
				case StatType::Cpu:
					enabled_cpu_counters.insert(res->second.cpu_counter);

					if (res->second.divisor_cpu_counter != hwcpipe::CpuCounter::MaxValue)
					{
						enabled_cpu_counters.insert(res->second.divisor_cpu_counter);
					}
					break;
				case StatType::Gpu:
					enabled_gpu_counters.insert(res->second.gpu_counter);

					if (res->second.divisor_gpu_counter != hwcpipe::GpuCounter::MaxValue)
					{
						enabled_gpu_counters.insert(res->second.divisor_gpu_counter);
					}
					break;
				default:
					break;
			}
		}
	}

	hwcpipe = std::make_unique<hwcpipe::HWCPipe>(enabled_cpu_counters, enabled_gpu_counters);
	hwcpipe->run();

	if (sampling_config.mode == CounterSamplingMode::Continuous)
	{
		worker_thread = std::thread([this] {
			continuous_sampling_worker(stop_worker->get_future());
		});

		// Reduce smoothing for continuous sampling
		alpha_smoothing = 0.6f;
	}

	for (const auto &stat_index : enabled_stats)
	{
		if (!is_available(stat_index))
		{
			// Find the graph data of this stat index
			vkb::Gui::StatsView stats_view;
			auto &              graph_data = stats_view.graph_map.find(stat_index)->second;
			LOGW(graph_data.name + " : not available");
		}
	}
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

void Stats::resize(const size_t width)
{
	// The circular buffer size will be 1/16th of the width of the screen
	// which means every sixteen pixels represent one graph value
	size_t buffers_size = width >> 4;

	for (auto &counter : counters)
	{
		counter.second.resize(buffers_size);
		counter.second.shrink_to_fit();
	}
}

bool Stats::is_available(const StatIndex index) const
{
	const auto &data = stat_data.find(index);
	if (data == stat_data.end())
	{
		return false;
	}

	switch (data->second.type)
	{
		case StatType::Cpu:
		{
			if (hwcpipe->cpu_profiler())
			{
				const auto &cpu_supp = hwcpipe->cpu_profiler()->supported_counters();
				return cpu_supp.find(data->second.cpu_counter) != cpu_supp.end();
			}
			break;
		}
		case StatType::Gpu:
		{
			if (hwcpipe->gpu_profiler())
			{
				const auto &gpu_supp = hwcpipe->gpu_profiler()->supported_counters();
				return gpu_supp.find(data->second.gpu_counter) != gpu_supp.end();
			}
			break;
		}
		case StatType::Other:
		{
			return true;
		}
	}

	return false;
}

void add_smoothed_value(std::vector<float> &values, float value, float alpha)
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
			auto m          = hwcpipe->sample();
			pending_samples = {{m.cpu ? *m.cpu : hwcpipe::CpuMeasurements{},
			                    m.gpu ? *m.gpu : hwcpipe::GpuMeasurements{},
			                    delta_time}};
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
					pending_samples                  = continuous_samples;
					continuous_samples.clear();
				}
			}

			// Ensure the number of pending samples is capped at a reasonable value
			if (pending_samples.size() > 100)
			{
				pending_samples.resize(100);
			}
			break;
		}
	}

	// Handle delta time counter
	auto delta_time_counter = counters.find(StatIndex::frame_times);
	if (delta_time_counter != counters.end())
	{
		add_smoothed_value(delta_time_counter->second, delta_time, alpha_smoothing);
	}

	if (pending_samples.size() == 0)
	{
		return;
	}

	// Compute the number of samples to show this frame
	size_t sample_count = static_cast<size_t>(sampling_config.speed * delta_time) * pending_samples.size();

	// Clamp the number of samples
	sample_count = std::max<size_t>(1, std::min<size_t>(sample_count, pending_samples.size()));

	// Push the samples to circular buffers
	std::for_each(pending_samples.end() - sample_count, pending_samples.end(), [this](const auto &s) {
		this->push_sample(s);
	});
	pending_samples.erase(pending_samples.end() - sample_count, pending_samples.end());
}

void Stats::continuous_sampling_worker(std::future<void> should_terminate)
{
	worker_timer.tick();
	hwcpipe->sample();

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
		const auto measurements = hwcpipe->sample();

		// Add the new sample to the vector of continuous samples
		{
			std::unique_lock<std::mutex> lock(continuous_sampling_mutex);
			continuous_samples.push_back({measurements.cpu ? *measurements.cpu : hwcpipe::CpuMeasurements{},
			                              measurements.gpu ? *measurements.gpu : hwcpipe::GpuMeasurements{},
			                              delta_time});
		}
	}
}

void Stats::push_sample(const MeasurementSample &sample)
{
	for (auto &c : counters)
	{
		auto &values = c.second;

		const auto data = stat_data.find(c.first);
		if (data == stat_data.end())
		{
			continue;
		}

		float measurement = 0;
		switch (data->second.type)
		{
			case StatType::Cpu:
			{
				const auto &cpu_res = sample.cpu.find(data->second.cpu_counter);
				if (cpu_res != sample.cpu.end())
				{
					measurement = cpu_res->second.get<float>();
				}

				if (data->second.scaling == StatScaling::ByCounter)
				{
					const auto &divisor_cpu_res = sample.cpu.find(data->second.divisor_cpu_counter);
					if (divisor_cpu_res != sample.cpu.end())
					{
						measurement /= divisor_cpu_res->second.get<float>();
					}
					else
					{
						measurement = 0;
					}
				}
				break;
			}
			case StatType::Gpu:
			{
				const auto &gpu_res = sample.gpu.find(data->second.gpu_counter);
				if (gpu_res != sample.gpu.end())
				{
					measurement = gpu_res->second.get<float>();
				}

				if (data->second.scaling == StatScaling::ByCounter)
				{
					const auto &divisor_gpu_res = sample.gpu.find(data->second.divisor_gpu_counter);
					if (divisor_gpu_res != sample.gpu.end())
					{
						measurement /= divisor_gpu_res->second.get<float>();
					}
					else
					{
						measurement = 0;
					}
				}
				break;
			}
			default:
			{
				// Skip to next counter
				continue;
			}
		}

		if (data->second.scaling == StatScaling::ByDeltaTime)
		{
			measurement /= sample.delta_time;
		}

		add_smoothed_value(values, measurement, alpha_smoothing);
	}
}

}        // namespace vkb
