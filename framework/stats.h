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

#pragma once

#include <cstdint>
#include <ctime>
#include <future>
#include <map>
#include <set>
#include <vector>

#include "common/error.h"

VKBP_DISABLE_WARNINGS()
#include <hwcpipe.h>
VKBP_ENABLE_WARNINGS()

#include "timer.h"

namespace vkb
{
/**
 * @brief Handles of stats to be optionally enabled in @ref Stats
 */
enum class StatIndex
{
	frame_times,
	cpu_cycles,
	cpu_instructions,
	cache_miss_ratio,
	branch_miss_ratio,
	gpu_cycles,
	vertex_compute_cycles,
	tiles,
	killed_tiles,
	fragment_jobs,
	fragment_cycles,
	load_store_cycles,
	l2_reads_lookups,
	l2_ext_reads,
	l2_writes_lookups,
	l2_ext_writes,
	l2_ext_read_stalls,
	l2_ext_write_stalls,
	l2_ext_read_bytes,
	l2_ext_write_bytes,
	tex_cycles
};

struct StatIndexHash
{
	template <typename T>
	std::size_t operator()(T t) const
	{
		return static_cast<std::size_t>(t);
	}
};

enum class StatType
{
	Cpu,
	Gpu,
	Other
};

enum class StatScaling
{
	// The stat is not scaled
	None,

	// The stat is scaled by delta time, useful for per-second values
	ByDeltaTime,

	// The stat is scaled by another counter, useful for ratios
	ByCounter
};

struct StatData
{
	StatType            type;
	StatScaling         scaling;
	hwcpipe::CpuCounter cpu_counter;
	hwcpipe::CpuCounter divisor_cpu_counter;
	hwcpipe::GpuCounter gpu_counter;
	hwcpipe::GpuCounter divisor_gpu_counter;

	/**
	 * @brief Constructor for simple stats that do not use any counter
	 * @param stat_scaling The scaling to be applied to the stat
	 */
	StatData(StatScaling stat_scaling = StatScaling::ByDeltaTime) :
	    type(StatType::Other),
	    scaling(stat_scaling)
	{}

	/**
	 * @brief Constructor for CPU counters
	 * @param c The CPU counter to be gathered
	 * @param stat_scaling The scaling to be applied to the stat
	 * @param divisor The CPU counter to be used as divisor if scaling is ByCounter
	 */
	StatData(hwcpipe::CpuCounter c, StatScaling stat_scaling = StatScaling::ByDeltaTime,
	         hwcpipe::CpuCounter divisor = hwcpipe::CpuCounter::MaxValue) :
	    type(StatType::Cpu),
	    scaling(stat_scaling),
	    cpu_counter(c),
	    divisor_cpu_counter(divisor)
	{}

	/**
	 * @brief Constructor for GPU counters
	 * @param c The GPU counter to be gathered
	 * @param stat_scaling The scaling to be applied to the stat
	 * @param divisor The GPU counter to be used as divisor if scaling is ByCounter
	 */
	StatData(hwcpipe::GpuCounter c, StatScaling stat_scaling = StatScaling::ByDeltaTime,
	         hwcpipe::GpuCounter divisor = hwcpipe::GpuCounter::MaxValue) :
	    type(StatType::Gpu),
	    scaling(stat_scaling),
	    gpu_counter(c),
	    divisor_gpu_counter(divisor)
	{}
};

using StatDataMap = std::unordered_map<StatIndex, StatData, StatIndexHash>;

enum class CounterSamplingMode
{
	/// Sample counters only when calling update()
	Polling,
	/// Sample counters continuously, update circular buffers when calling update()
	Continuous
};

struct CounterSamplingConfig
{
	/// Sampling mode (polling or continuous)
	CounterSamplingMode mode;

	/// Sampling interval in continuous mode
	std::chrono::milliseconds interval{1};

	/// Speed of circular buffer updates in continuous mode;
	/// at speed = 1.0f a new sample is displayed over 1 second.
	float speed{0.5f};
};

/*
 * @brief Helper class for querying statistics about the CPU and the GPU
 */
class Stats
{
  public:
	/**
	 * @brief Constructs a Stats object
	 * @param enabled_stats Set of stats to be collected
	 * @param sampling_config Sampling mode configuration (polling or continuous)
	 * @param buffer_size Size of the circular buffers
	 */
	Stats(const std::set<StatIndex> &enabled_stats,
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
	 * @param index The stat index of the data requested
	 * @return The data of the specified stat
	 */
	const std::vector<float> &get_data(StatIndex index) const
	{
		return counters.at(index);
	};

	/**
	 * @return The enabled stats
	 */
	const std::set<StatIndex> &get_enabled_stats() const
	{
		return enabled_stats;
	}

	/**
	 * @brief Update statistics, must be called after every frame
	 */
	void update(float delta_time);

  private:
	struct MeasurementSample
	{
		hwcpipe::CpuMeasurements cpu{};
		hwcpipe::GpuMeasurements gpu{};
		float                    delta_time{0.0f};
	};

	/// Stats to be enabled
	std::set<StatIndex> enabled_stats;

	/// Counter sampling configuration
	CounterSamplingConfig sampling_config;

	/// Mapping of stats to their availability and value getters
	StatDataMap stat_data;

	/// Timer used in the main thread to compute delta time
	Timer main_timer;

	/// Timer used by the worker thread to throttle counter sampling
	Timer worker_timer;

	/// Alpha smoothing for running average
	float alpha_smoothing{0.2f};

	/// Circular buffers for counter data
	std::map<StatIndex, std::vector<float>> counters{};

	/// Profiler to gather CPU and GPU performance data
	std::unique_ptr<hwcpipe::HWCPipe> hwcpipe{};

	/// Worker thread for continuous sampling
	std::thread worker_thread;

	/// Promise to stop the worker thread
	std::unique_ptr<std::promise<void>> stop_worker;

	/// A mutex for accessing measurements during continuous sampling
	std::mutex continuous_sampling_mutex;

	/// The samples read during continuous sampling
	std::vector<MeasurementSample> continuous_samples;

	/// A flag specifying if the worker thread should add entries to continuous_samples
	bool should_add_to_continuous_samples{false};

	/// The samples waiting to be displayed
	std::vector<MeasurementSample> pending_samples;

	/// The worker thread function for continuous sampling;
	/// it adds a new entry to continuous_samples at every interval
	void continuous_sampling_worker(std::future<void> should_terminate);

	/// Updates circular buffers for CPU and GPU counters
	void push_sample(const MeasurementSample &sample);
};

}        // namespace vkb
