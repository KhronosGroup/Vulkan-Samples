/* Copyright (c) 2020-2024, Arm Limited and Contributors
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
class WindowOptions;

using WindowOptionsTags = vkb::PluginBase<WindowOptions, vkb::tags::Passive>;

/**
 * @brief Window Options
 * 
 * Configure the window used when running Vulkan Samples.
 * 
 * Usage: vulkan_samples sample instancing --width 500 --height 500 --vsync OFF
 * 
 */
class WindowOptions : public WindowOptionsTags
{
  public:
	WindowOptions();

	virtual ~WindowOptions() = default;

	virtual bool is_active(const vkb::CommandParser &parser) override;

	virtual void init(const vkb::CommandParser &options) override;

	vkb::FlagCommand width_flag      = {vkb::FlagType::OneValue, "width", "", "Initial window width"};
	vkb::FlagCommand height_flag     = {vkb::FlagType::OneValue, "height", "", "Initial window height"};
	vkb::FlagCommand fullscreen_flag = {vkb::FlagType::FlagOnly, "fullscreen", "", "Run in fullscreen mode"};
	vkb::FlagCommand headless_flag   = {vkb::FlagType::FlagOnly, "headless_surface", "", "Run in headless surface mode. A Surface and swap-chain is still created using VK_EXT_headless_surface."};
	vkb::FlagCommand borderless_flag = {vkb::FlagType::FlagOnly, "borderless", "", "Run in borderless mode"};
	vkb::FlagCommand stretch_flag    = {vkb::FlagType::FlagOnly, "stretch", "", "Stretch window to fullscreen (direct-to-display only)"};
	vkb::FlagCommand vsync_flag      = {vkb::FlagType::OneValue, "vsync", "", "Force vsync {ON | OFF}. If not set samples decide how vsync is set"};

	vkb::CommandGroup window_options_group = {"Window Options", {&width_flag, &height_flag, &vsync_flag, &fullscreen_flag, &borderless_flag, &stretch_flag, &headless_flag}};
};
}        // namespace plugins