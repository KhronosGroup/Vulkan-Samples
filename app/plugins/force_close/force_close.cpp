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

#include "force_close.h"

#include <iostream>

namespace plugins
{
ForceClose::ForceClose() :
    ForceCloseTags("Force Close",
                   "Force the application to close if it has been halted before exiting",
                   {}, {&stop_cmd})
{
}

bool ForceClose::is_active(const vkb::CommandParser &parser)
{
	return parser.contains(&stop_cmd);
}

void ForceClose::init(const vkb::CommandParser &parser)
{
}
}        // namespace plugins