/* Copyright (c) 2020, Broadcom Inc. and Contributors
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

#include "stats_provider.h"

namespace vkb
{
class FrameTimeStatsProvider : public StatsProvider
{
  public:
	/**
	 * @brief Constructs a FrameTimeStatsProvider
	 * @param requested_stats Set of stats to be collected. Supported stats will be removed from the set.
	 */
	FrameTimeStatsProvider(std::set<StatIndex> &requested_stats);
	/**
	 * @brief Checks if this provider can supply the given enabled stat
	 * @param index The stat index
	 * @return True if the stat is available, false otherwise
	 */
	bool is_available(StatIndex index) const override;

	/**
	 * @brief Retrieve a new sample set
	 * @param delta_time Time since last sample
	 * @param active_frame_idx Which of the framebuffers is active
	 */
	Counters sample(float delta_time, uint32_t active_frame_idx) override;
};
}        // namespace vkb
