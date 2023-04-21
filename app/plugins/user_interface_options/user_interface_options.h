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

#pragma once

#include "platform/plugins/plugin_base.h"

namespace plugins
{
class UserInterfaceOptions;

using UserInterfaceOptionsTags = vkb::PluginBase<UserInterfaceOptions, vkb::tags::Passive>;

/**
 * @brief User interface Options
 *
 * Configure the default user interface
 *
 */
class UserInterfaceOptions : public UserInterfaceOptionsTags
{
  public:
	UserInterfaceOptions();

	virtual ~UserInterfaceOptions() = default;

	virtual bool is_active(const vkb::CommandParser &parser) override;

	virtual void init(const vkb::CommandParser &options) override;

	vkb::FlagCommand hide_ui_flag = {vkb::FlagType::FlagOnly, "hideui", "", "If flag is set, hides the user interface at startup"};

	vkb::CommandGroup user_interface_options_group = {"User interface Options", {&hide_ui_flag}};
};
}        // namespace plugins