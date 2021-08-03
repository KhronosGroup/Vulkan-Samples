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

#pragma once

#include "platform/plugins/plugin_base.h"

namespace plugins
{
class ForceClose;

// Passive behaviour
using ForceCloseTags = vkb::PluginBase<ForceClose, vkb::tags::Passive>;

/**
 * @brief Force Close
 * 
 * Force the close of the application if halted before exiting
 * 
 * The plugin is used as a boolean with platform->using_plugin<ForceClose>();
 * 
 * Usage: vulkan_sample sample afbc --force-close
 * 
 */
class ForceClose : public ForceCloseTags
{
  public:
	ForceClose();

	virtual ~ForceClose() = default;

	virtual bool is_active(const vkb::CommandParser &parser) override;

	virtual void init(const vkb::CommandParser &parser) override;

	vkb::FlagCommand stop_cmd = {vkb::FlagType::FlagOnly, "force-close", "", "Force the close of the application if halted before exiting"};
};
}        // namespace plugins