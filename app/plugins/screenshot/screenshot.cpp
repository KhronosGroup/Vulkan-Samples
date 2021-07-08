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

#include "screenshot.h"

#include <chrono>
#include <iomanip>

#include "rendering/render_context.h"

namespace plugins
{
Screenshot::Screenshot() :
    ScreenshotTags("Screenshot",
                   "Save a screenshot of a specific frame",
                   {vkb::Hook::OnUpdate, vkb::Hook::OnAppStart, vkb::Hook::PostDraw},
                   {&screenshot_flag, &screenshot_output_flag})
{
}

bool Screenshot::is_active(const vkb::CommandParser &parser)
{
	return parser.contains(&screenshot_flag);
}

void Screenshot::init(const vkb::CommandParser &parser)
{
	if (parser.contains(&screenshot_flag))
	{
		frame_number = parser.as<int>(&screenshot_flag);

		if (parser.contains(&screenshot_output_flag))
		{
			output_path     = parser.as<std::string>(&screenshot_output_flag);
			output_path_set = true;
		}
	}
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

void Screenshot::on_post_draw(vkb::RenderContext &context)
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

		screenshot(context, output_path);
	}
}
}        // namespace plugins