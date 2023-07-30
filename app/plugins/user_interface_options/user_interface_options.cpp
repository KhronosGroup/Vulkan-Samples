/* Copyright (c) 2023, Sascha Willems
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

#include "user_interface_options.h"

#include <algorithm>

#include "gui.h"
#include "hpp_gui.h"

namespace plugins
{
UserInterfaceOptions::UserInterfaceOptions() :
    UserInterfaceOptionsTags("User interface options",
                             "A collection of flags to configure the user interface",
                             {}, {&user_interface_options_group})
{
}

bool UserInterfaceOptions::is_active(const vkb::CommandParser &parser)
{
	return true;
}

void UserInterfaceOptions::init(const vkb::CommandParser &parser)
{
	if (parser.contains(&hide_ui_flag))
	{
		vkb::Gui::visible    = false;
		vkb::HPPGui::visible = false;
	}
}
}        // namespace plugins