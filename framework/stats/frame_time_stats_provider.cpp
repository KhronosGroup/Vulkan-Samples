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

#include "frame_time_stats_provider.h"

namespace vkb
{
FrameTimeStatsProvider::FrameTimeStatsProvider(std::set<StatIndex> &requested_stats)
{
	// We always, and only, support StatIndex::frame_times since it's handled directly by us.
	// Remove from requested set to stop other providers looking for it.
	requested_stats.erase(StatIndex::frame_times);
}

bool FrameTimeStatsProvider::is_available(StatIndex index) const
{
	// We only support StatIndex::frame_times
	return index == StatIndex::frame_times;
}

StatsProvider::Counters FrameTimeStatsProvider::sample(float delta_time, uint32_t active_frame_idx)
{
	Counters res;
	// frame_times comes directly from delta_time
	res[StatIndex::frame_times].result = delta_time;
	return res;
}

}        // namespace vkb
