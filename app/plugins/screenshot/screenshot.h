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

#include "platform/filesystem.h"
#include "platform/plugins/plugin_base.h"

namespace plugins
{
class Screenshot;

using ScreenshotTags = vkb::PluginBase<Screenshot, vkb::tags::Passive>;

/**
 * @brief Screenshot
 * 
 * Capture a screen shot of the last rendered image at a given frame. The output can also be named
 * 
 * Usage: vulkan_sample sample afbc --screenshot 1 --screenshot-output afbc-screenshot
 * 
 */
class Screenshot : public ScreenshotTags
{
  public:
	Screenshot();

	virtual ~Screenshot() = default;

	virtual bool is_active(const vkb::CommandParser &parser) override;

	virtual void init(const vkb::CommandParser &parser) override;

	virtual void on_update(float delta_time) override;

	virtual void on_app_start(const std::string &app_info) override;

	virtual void on_post_draw(vkb::RenderContext &context) override;

	vkb::FlagCommand screenshot_flag        = {vkb::FlagType::OneValue, "screenshot", "", "Take a screenshot at a given frame"};
	vkb::FlagCommand screenshot_output_flag = {vkb::FlagType::OneValue, "screenshot-output", "", "Declare an output name for the image"};

  private:
	uint32_t    current_frame = 0;
	uint32_t    frame_number;
	std::string current_app_name;

	bool        output_path_set = false;
	std::string output_path;
};
}        // namespace plugins