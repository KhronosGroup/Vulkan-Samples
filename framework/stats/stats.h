/* Copyright (c) 2018-2022, Arm Limited and Contributors
 * Copyright (c) 2020-2022, Broadcom Inc.
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

#include "stats_common.h"
#include "stats_provider.h"
#include "timer.h"

namespace vkb
{
class Device;
class CommandBuffer;
class RenderContext;

/*
 * @brief Helper class for querying statistics about the CPU and the GPU
 */
class Stats
{
  public:
	/**
	 * @brief Constructs a Stats object
	 * @param render_context The RenderContext for this sample
	 * @param buffer_size Size of the circular buffers
	 */
	explicit Stats(RenderContext &render_context, size_t buffer_size = 16);

	/**
	 * @brief Destroys the Stats object
	 */
	~Stats();

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
	 * @param delta_time Time since last update
	 */
	void update(float delta_time);

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
	void begin_sampling(CommandBuffer &cb);

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
	void end_sampling(CommandBuffer &cb);

  private:
	/// The render context
	RenderContext &render_context;

	/// Stats that were requested - they may not all be available
	std::set<StatIndex> requested_stats;

	/// Provider that tracks frame times
	StatsProvider *frame_time_provider;

	/// A list of stats providers to use in priority order
	std::vector<std::unique_ptr<StatsProvider>> providers;

	/// Counter sampling configuration
	CounterSamplingConfig sampling_config;

	/// Size of the circular buffers
	size_t buffer_size;

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
	std::vector<StatsProvider::Counters> continuous_samples;

	/// A flag specifying if the worker thread should add entries to continuous_samples
	bool should_add_to_continuous_samples{false};

	/// The samples waiting to be displayed
	std::vector<StatsProvider::Counters> pending_samples;

	/// A value which helps keep a steady pace of continuous samples output.
	float fractional_pending_samples{0.0f};

	/// The worker thread function for continuous sampling;
	/// it adds a new entry to continuous_samples at every interval
	void continuous_sampling_worker(std::future<void> should_terminate);

	/// Updates circular buffers for CPU and GPU counters
	void push_sample(const StatsProvider::Counters &sample);
};

}        // namespace vkb
