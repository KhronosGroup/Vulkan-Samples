/* Copyright (c) 2020-2021, Arm Limited and Contributors
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

#include "stop_on_close.h"

#include <iostream>

namespace plugins
{
StopOnClose::StopOnClose() :
    StopOnCloseTags("Stop on Close",
                    "Halt the application before exiting. (Desktop Only)",
                    {vkb::Hook::OnPlatformClose}, {&stop_cmd})
{
}

bool StopOnClose::is_active(const vkb::CommandParser &parser)
{
	return parser.contains(&stop_cmd);
}

void StopOnClose::init(const vkb::CommandParser &parser)
{
}

void StopOnClose::on_platform_close()
{
#ifndef ANDROID
	std::cout << "Press any key to continue";
	std::cin.get();
#endif
}
}        // namespace plugins