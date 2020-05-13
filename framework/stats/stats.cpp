/* Copyright (c) 2018-2020, Arm Limited and Contributors
 * Copyright (c) 2020, Broadcom Inc.
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
#include "common/error.h"
#include "core/device.h"

#include "frame_time_stats_provider.h"
#include "hwcpipe_stats_provider.h"
#include "vulkan_stats_provider.h"

namespace vkb
{
Stats::Stats(Device &device, size_t num_framebuffers, const std::set<StatIndex> &requested_stats,
             CounterSamplingConfig sampling_config, size_t buffer_size) :
    requested_stats(requested_stats),
    sampling_config(sampling_config),
    stop_worker(std::make_unique<std::promise<void>>())
{
	assert(buffer_size >= 2 && "Buffers size should be greater than 2");

	// Copy the requested stats, so they can be changed by the providers below
	std::set<StatIndex> stats = requested_stats;

	// Initialize our list of providers (in priority order)
	// All supported stats will be removed from the given 'stats' set by the provider's constructor
	// so subsequent providers only see requests for stats that aren't already supported.
	providers.emplace_back(std::make_unique<FrameTimeStatsProvider>(stats));
	providers.emplace_back(std::make_unique<HWCPipeStatsProvider>(stats, sampling_config));
	providers.emplace_back(std::make_unique<VulkanStatsProvider>(device, stats, sampling_config, num_framebuffers));

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
		worker_thread = std::thread([this] {
			continuous_sampling_worker(stop_worker->get_future());
		});

		// Reduce smoothing for continuous sampling
		alpha_smoothing = 0.6f;
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
	for (const auto &p : providers)
		if (p->is_available(index))
			return true;
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

void Stats::update(float delta_time, uint32_t active_frame_idx)
{
	switch (sampling_config.mode)
	{
		case CounterSamplingMode::Polling:
		{
			StatsProvider::Counters sample;

			for (auto &p : providers)
			{
				auto s = p->sample(delta_time, active_frame_idx);
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
					pending_samples                  = continuous_samples;
					continuous_samples.clear();
				}
			}

			if (pending_samples.size() == 0)
				return;

			// Ensure the number of pending samples is capped at a reasonable value
			if (pending_samples.size() > 100)
				pending_samples.resize(100);

			// Compute the number of samples to show this frame
			size_t sample_count = static_cast<size_t>(sampling_config.speed * delta_time) * pending_samples.size();

			// Clamp the number of samples
			sample_count = std::max<size_t>(1, std::min<size_t>(sample_count, pending_samples.size()));

			// Get the frame time stats (not a continuous stat)
			StatsProvider::Counters frame_time_sample = frame_time_provider->sample(delta_time, active_frame_idx);

			// Push the samples to circular buffers
			std::for_each(pending_samples.end() - sample_count, pending_samples.end(), [this, frame_time_sample](auto &s) {
				// Write the correct frame time into the continuous stats
				s.insert(frame_time_sample.begin(), frame_time_sample.end());
				// Then push the sample to the counters list
				this->push_sample(s);
			});
			pending_samples.erase(pending_samples.end() - sample_count, pending_samples.end());

			break;
		}
	}
}

void Stats::continuous_sampling_worker(std::future<void> should_terminate)
{
	worker_timer.tick();

	for (auto &p : providers)
		p->continuous_sample(0.0f);

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
			continuous_samples.push_back(sample);
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
			continue;

		float measurement = static_cast<float>(smp->second.result);

		add_smoothed_value(values, measurement, alpha_smoothing);
	}
}

void Stats::command_buffer_begun(CommandBuffer &cb, uint32_t active_frame_idx)
{
	// Inform the providers
	for (auto &p : providers)
		p->command_buffer_begun(cb, active_frame_idx);
}

void Stats::command_buffer_ending(CommandBuffer &cb, uint32_t active_frame_idx)
{
	// Inform the providers
	for (auto &p : providers)
		p->command_buffer_ending(cb, active_frame_idx);
}

const StatGraphData &Stats::get_graph_data(StatIndex index) const
{
	for (auto &p : providers)
	{
		if (p->is_available(index))
			return p->get_graph_data(index);
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
