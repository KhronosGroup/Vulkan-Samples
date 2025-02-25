/* Copyright (c) 2023-2025, Sascha Willems
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

#include "user_interface_options.h"

#include <algorithm>

#include "gui.h"
#include "hpp_gui.h"

namespace plugins
{
UserInterfaceOptions::UserInterfaceOptions() :
    UserInterfaceOptionsTags("User interface options",
                             "A collection of flags to configure the user interface",
                             {},
                             {},
                             {{"hideui", "If flag is set, hides the user interface at startup"}})
{
}

bool UserInterfaceOptions::handle_option(std::deque<std::string> &arguments)
{
	assert(!arguments.empty() && (arguments[0].substr(0, 2) == "--"));
	std::string option = arguments[0].substr(2);
	if (option == "hideui")
	{
		vkb::Gui::visible    = false;
		vkb::HPPGui::visible = false;

		arguments.pop_front();
		return true;
	}
	return false;
}
}        // namespace plugins