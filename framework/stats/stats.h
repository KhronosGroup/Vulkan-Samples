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

#pragma once

#include <cstdint>
#include <ctime>
#include <future>
#include <map>
#include <set>
#include <vector>

#include "common/error.h"

#include "stats_common.h"
#include "stats_provider.h"
#include "timer.h"

namespace vkb
{
class Device;
class CommandBuffer;

/*
 * @brief Helper class for querying statistics about the CPU and the GPU
 */
class Stats
{
  public:
	/**
	 * @brief Constructs a Stats object
	 * @param device Device on which to collect stats
	 * @param requested_stats Set of stats to be collected if available
	 * @param sampling_config Sampling mode configuration (polling or continuous)
	 * @param buffer_size Size of the circular buffers
	 */
	Stats(Device &                   device,
	      size_t                     num_framebuffers,
	      const std::set<StatIndex> &requested_stats,
	      CounterSamplingConfig      sampling_config = {CounterSamplingMode::Polling},
	      size_t                     buffer_size     = 16);

	/**
	 * @brief Destroys the Stats object
	 */
	~Stats();

	/**
	 * @brief Resizes the stats buffers according to the width of the screen
	 * @param width The width of the screen
	 */
	void resize(size_t width);

	/**
	 * @brief Checks if an enabled stat is available in the current platform
	 * @param index The stat index
	 * @return True if the stat is available, false otherwise
	 */
	bool is_available(StatIndex index) const;

	/**
	 * @brief Returns data relevant for graphing a specific statistic
	 * @param index The stat index of the data requested
	 * @return The data of the specified stat
	 */
	const StatGraphData &get_graph_data(StatIndex index) const;

	/**
	 * @brief Returns the collected data for a specific statistic
	 * @param index The stat index of the data requested
	 * @return The data of the specified stat
	 */
	const std::vector<float> &get_data(StatIndex index) const
	{
		return counters.at(index);
	};

	/**
	 * @return The requested stats
	 */
	const std::set<StatIndex> &get_requested_stats() const
	{
		return requested_stats;
	}

	/**
	 * @brief Update statistics, must be called after every frame
	 */
	void update(float delta_time, uint32_t active_frame_idx);

	/**
	 * @brief A command buffer that we want stats about has just begun
	 * @param cb The command buffer
	 */
	void command_buffer_begun(CommandBuffer &cb, uint32_t active_frame_idx);

	/**
	 * @brief A command buffer that we want stats about is about to be ended
	 * @param cb The command buffer
	 */
	void command_buffer_ending(CommandBuffer &cb, uint32_t active_frame_idx);

  private:
	/// Stats that were requested - they may not all be available
	std::set<StatIndex> requested_stats;

	/// Provider that tracks frame times
	StatsProvider *frame_time_provider;

	/// A list of stats providers to use in priority order
	std::vector<std::unique_ptr<StatsProvider>> providers;

	/// Counter sampling configuration
	CounterSamplingConfig sampling_config;

	/// Timer used in the main thread to compute delta time
	Timer main_timer;

	/// Timer used by the worker thread to throttle counter sampling
	Timer worker_timer;

	/// Alpha smoothing for running average
	float alpha_smoothing{0.2f};

	/// Circular buffers for counter data
	std::map<StatIndex, std::vector<float>> counters{};

	/// Worker thread for continuous sampling
	std::thread worker_thread;

	/// Promise to stop the worker thread
	std::unique_ptr<std::promise<void>> stop_worker;

	/// A mutex for accessing measurements during continuous sampling
	std::mutex continuous_sampling_mutex;

	/// The samples read during continuous sampling
	std::vector<StatsProvider::Sample> continuous_samples;

	/// A flag specifying if the worker thread should add entries to continuous_samples
	bool should_add_to_continuous_samples{false};

	/// The samples waiting to be displayed
	std::vector<StatsProvider::Sample> pending_samples;

	/// The worker thread function for continuous sampling;
	/// it adds a new entry to continuous_samples at every interval
	void continuous_sampling_worker(std::future<void> should_terminate);

	/// Updates circular buffers for CPU and GPU counters
	void push_sample(const StatsProvider::Sample &sample);
};

}        // namespace vkb
