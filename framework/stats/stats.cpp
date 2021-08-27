/* Copyright (c) 2018-2021, Arm Limited and Contributors
 * Copyright (c) 2020-2021, Broadcom Inc.
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

#include <utility>
#include "common/error.h"
#include "core/device.h"

#include "frame_time_stats_provider.h"
#include "hwcpipe_stats_provider.h"
#include "vulkan_stats_provider.h"

namespace vkb
{
Stats::Stats(RenderContext &render_context, size_t buffer_size) :
    render_context(render_context),
    frame_time_provider(nullptr),
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
	if (!providers.empty())
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
	providers.emplace_back(std::make_unique<HWCPipeStatsProvider>(stats));
	providers.emplace_back(std::make_unique<VulkanStatsProvider>(stats, sampling_config, render_context));

	// In continuous sampling mode we still need to update the frame times as if we are polling
	// Store the frame time provider here, so we can easily access it later.
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
			LOGW(vkb::StatsProvider::default_graph_data(stat_index).name + " : not available")
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

void Stats::update(float delta_time)
{
	switch (sampling_config.mode)
	{
		case CounterSamplingMode::Polling: {
			StatsProvider::Counters sample;

			for (auto &p : providers)
			{
				auto s = p->sample(delta_time);
				sample.insert(s.begin(), s.end());
			}
			push_sample(sample);
			break;
		}
		case CounterSamplingMode::Continuous: {
			// Check that we have no pending samples to be shown
			if (pending_samples.empty())
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

			if (pending_samples.empty())
				return;

			// Ensure the number of pending samples is capped at a reasonable value
			if (pending_samples.size() > 100)
			{
				// Prefer later samples to new samples.
				std::move(pending_samples.end() - 100, pending_samples.end(), pending_samples.begin());
				pending_samples.erase(pending_samples.begin() + 100, pending_samples.end());

				// If we get to this point, we're not reading samples fast enough, nudge a little ahead.
				fractional_pending_samples += 1.0f;
			}

			// Compute the number of samples to show this frame
			float floating_sample_count = sampling_config.speed * delta_time * float(buffer_size) + fractional_pending_samples;

			// Keep track of the fractional value to avoid speeding up or slowing down too much due to rounding errors.
			// Generally we push very few samples per frame, so this matters.
			fractional_pending_samples = floating_sample_count - std::floor(floating_sample_count);

			auto sample_count = static_cast<size_t>(floating_sample_count);

			// Clamp the number of samples
			sample_count = std::max<size_t>(1, std::min<size_t>(sample_count, pending_samples.size()));

			// Get the frame time stats (not a continuous stat)
			StatsProvider::Counters frame_time_sample = frame_time_provider->sample(delta_time);

			// Push the samples to circular buffers
			std::for_each(pending_samples.begin(), pending_samples.begin() + static_cast<int>(sample_count), [this, frame_time_sample](auto &s) {
				// Write the correct frame time into the continuous stats
				s.insert(frame_time_sample.begin(), frame_time_sample.end());
				// Then push the sample to the counters list
				this->push_sample(s);
			});
			pending_samples.erase(pending_samples.begin(), pending_samples.begin() + static_cast<int>(sample_count));

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
			continue;

		auto measurement = static_cast<float>(smp->second.result);

		add_smoothed_value(values, measurement, alpha_smoothing);
	}
}

void Stats::begin_sampling(CommandBuffer &cb)
{
	// Inform the providers
	for (auto &p : providers)
		p->begin_sampling(cb);
}

void Stats::end_sampling(CommandBuffer &cb)
{
	// Inform the providers
	for (auto &p : providers)
		p->end_sampling(cb);
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

StatGraphData::StatGraphData(std::string name,
                             std::string graph_label_format,
                             float              scale_factor,
                             bool               has_fixed_max,
                             float              max_value) :
    name(std::move(name)),
    format{std::move(graph_label_format)},
    scale_factor{scale_factor},
    has_fixed_max{has_fixed_max},
    max_value{max_value}
{
}

}        // namespace vkb
