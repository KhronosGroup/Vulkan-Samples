/* Copyright (c) 2020-2025, Arm Limited and Contributors
 * Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
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

#include "stop_after.h"

namespace plugins
{
StopAfter::StopAfter() :
    StopAfterTags("Stop After X", "A collection of flags to stop the running application after a set period.", {vkb::Hook::OnUpdate}, {},
                  {{"stop-after-frame", "Stop the application after a certain number of frames"}})
{}

bool StopAfter::handle_option(std::deque<std::string> &arguments)
{
	assert(!arguments.empty() && (arguments[0].substr(0, 2) == "--"));
	std::string option = arguments[0].substr(2);
	if (option == "stop-after-frame")
	{
		if (arguments.size() < 2)
		{
			LOGE("Option \"stop-after-frame\" is missing the actual frame index to stop after!");
			return false;
		}
		remaining_frames = static_cast<uint32_t>(std::stoul(arguments[1]));

		arguments.pop_front();
		arguments.pop_front();
		return true;
	}
	return false;
}

void StopAfter::on_update(float delta_time)
{
	remaining_frames--;

	if (remaining_frames <= 0)
	{
		platform->close();
	}
}
}        // namespace plugins