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

#include "common/error.h"
#include "common/vk_common.h"

VKBP_DISABLE_WARNINGS()
#include <hwcpipe.h>
VKBP_ENABLE_WARNINGS()

#include "stats_provider.h"

namespace vkb
{
class HWCPipeStatsProvider : public StatsProvider
{
  private:
	enum class StatType
	{
		Cpu,
		Gpu
	};

	struct StatData
	{
		StatType            type;
		StatScaling         scaling;
		hwcpipe::CpuCounter cpu_counter;
		hwcpipe::CpuCounter divisor_cpu_counter;
		hwcpipe::GpuCounter gpu_counter;
		hwcpipe::GpuCounter divisor_gpu_counter;

		StatData() = default;

		/**
	 * @brief Constructor for CPU counters
	 * @param c The CPU counter to be gathered
	 * @param stat_scaling The scaling to be applied to the stat
	 * @param divisor The CPU counter to be used as divisor if scaling is ByCounter
	 */
		StatData(hwcpipe::CpuCounter c,
		         StatScaling         stat_scaling = StatScaling::ByDeltaTime,
		         hwcpipe::CpuCounter divisor      = hwcpipe::CpuCounter::MaxValue) :
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
		StatData(hwcpipe::GpuCounter c,
		         StatScaling         stat_scaling = StatScaling::ByDeltaTime,
		         hwcpipe::GpuCounter divisor      = hwcpipe::GpuCounter::MaxValue) :
		    type(StatType::Gpu),
		    scaling(stat_scaling),
		    gpu_counter(c),
		    divisor_gpu_counter(divisor)
		{}
	};

	using StatDataMap = std::unordered_map<StatIndex, StatData, StatIndexHash>;

  public:
	/**
	 * @brief Constructs a HWCPipeStateProvider
	 * @param requested_stats Set of stats to be collected. Supported stats will be removed from the set.
	 */
	HWCPipeStatsProvider(std::set<StatIndex> &requested_stats, CounterSamplingConfig sampling_config);

	/**
	 * @brief Checks if this provider can supply the given enabled stat
	 * @param index The stat index
	 * @return True if the stat is available, false otherwise
	 */
	bool is_available(StatIndex index) const override;

	/**
	 * @brief Retrieve graphing data for the given enabled stat
	 * @param index The stat index
	 */
	const StatGraphData &get_graph_data(StatIndex index) const override;

	/**
	 * @brief Retrieve a new sample set from polled sampling
	 * @param delta_time Time since last sample
	 * @param active_frame_idx Which of the framebuffers is active
	 */
	Sample sample(float delta_time, uint32_t active_frame_idx) override;

	/**
	 * @brief Retrieve a new sample set from continuous sampling
 	 * @param delta_time Time since last sample
	 */
	Sample continuous_sample(float delta_time) override;

  private:
	// The hwcpipe instance
	std::unique_ptr<hwcpipe::HWCPipe> hwcpipe{};

	// Only stats which are available and were requested end up in stat_data
	StatDataMap stat_data;

	// Counter sampling configuration
	CounterSamplingConfig sampling_config;
};

}        // namespace vkb
