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

#include "screenshot.h"

#include <chrono>
#include <iomanip>

#include "common/utils.h"
#include "rendering/render_context.h"

namespace plugins
{
Screenshot::Screenshot() :
    ScreenshotTags("Screenshot",
                   "Save a screenshot of a specific frame",
                   {vkb::Hook::OnUpdate, vkb::Hook::OnAppStart, vkb::Hook::PostDraw},
                   {},
                   {{"screenshot", "Take a screenshot at a given frame"}, {"screenshot-output", "Declare an output name for the image"}})
{
}

bool Screenshot::handle_option(std::deque<std::string> &arguments)
{
	assert(!arguments.empty() && (arguments[0].substr(0, 2) == "--"));
	std::string option = arguments[0].substr(2);
	if (option == "screenshot")
	{
		if (arguments.size() < 2)
		{
			LOGE("Option \"screenshot\" is missing the frame index to take a screenshot!");
			return false;
		}
		frame_number = static_cast<uint32_t>(std::stoul(arguments[1]));

		arguments.pop_front();
		arguments.pop_front();
		return true;
	}
	else if (option == "screenshot-output")
	{
		if (arguments.size() < 2)
		{
			LOGE("Option \"screenshot-output\" is missing the filename to store the screenshot!");
			return false;
		}
		output_path     = arguments[1];
		output_path_set = true;

		arguments.pop_front();
		arguments.pop_front();
		return true;
	}
	return false;
}

void Screenshot::on_update(float delta_time)
{
	current_frame++;
}

void Screenshot::on_app_start(const std::string &name)
{
	current_app_name = name;
	current_frame    = 0;
}

void Screenshot::on_post_draw(vkb::rendering::RenderContextC &context)
{
	if (current_frame == frame_number)
	{
		if (!output_path_set)
		{
			// Create generic image path. <app name>-<current timestamp>.png
			auto        timestamp = std::chrono::system_clock::now();
			std::time_t now_tt    = std::chrono::system_clock::to_time_t(timestamp);
			std::tm     tm        = *std::localtime(&now_tt);

			char buffer[30];
			strftime(buffer, sizeof(buffer), "%G-%m-%d---%H-%M-%S", &tm);

			std::stringstream stream;
			stream << current_app_name << "-" << buffer;

			output_path = stream.str();
		}

		vkb::screenshot(context, output_path);
	}
}
}        // namespace plugins