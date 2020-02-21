/* Copyright (c) 2019, Arm Limited and Contributors
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

#include "timer.h"

namespace vkb
{
Timer::Timer() :
    start_time{Clock::now()},
    previous_tick{Clock::now()}
{
}

void Timer::start()
{
	if (!running)
	{
		running    = true;
		start_time = Clock::now();
	}
}

void Timer::lap()
{
	lapping  = true;
	lap_time = Clock::now();
}

bool Timer::is_running() const
{
	return running;
}
}        // namespace vkb
