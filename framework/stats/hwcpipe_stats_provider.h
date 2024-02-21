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

#pragma once

#include "common/error.h"
#include "common/vk_common.h"

#include "stats_provider.h"

VKBP_DISABLE_WARNINGS()
#include <device/product_id.hpp>
#include <hwcpipe/counter_database.hpp>
#include <hwcpipe/gpu.hpp>
#include <hwcpipe/sampler.hpp>

#include <iomanip>

#include <unistd.h>
VKBP_ENABLE_WARNINGS()

namespace vkb
{
class HWCPipeStatsProvider : public StatsProvider
{
  private:
	struct StatData
	{
		hwcpipe_counter counter;
		StatScaling     scaling;
		hwcpipe_counter divisor;

		StatData() = default;
	};

	using StatDataMap = std::unordered_map<StatIndex, StatData, StatIndexHash>;

  public:
	/**
	 * @brief Constructs a HWCPipeStateProvider
	 * @param requested_stats Set of stats to be collected. Supported stats will be removed from the set.
	 */
	HWCPipeStatsProvider(std::set<StatIndex> &requested_stats);

	/**
	 * @brief Destructor
	 */
	~HWCPipeStatsProvider();

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
	 */
	Counters sample(float delta_time) override;

	/**
	 * @brief Retrieve a new sample set from continuous sampling
	 * @param delta_time Time since last sample
	 */
	Counters continuous_sample(float delta_time) override;

  private:
	std::unique_ptr<hwcpipe::sampler<>> sampler;

	// Only stats which are available and were requested end up in stat_data
	StatDataMap stat_data;

	// Counter sampling configuration
	CounterSamplingConfig sampling_config;
};

}        // namespace vkb
