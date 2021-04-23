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

#include "window_options.h"

#include <algorithm>

#include "platform/window.h"

namespace plugins
{
WindowOptions::WindowOptions() :
    WindowOptionsTags("Window Options",
                      "A collection of flags to configure window used when running the application. Implementation may differ between platforms",
                      {}, {&window_options_group})
{
}

bool WindowOptions::is_active(const vkb::CommandParser &parser)
{
	return true;
}

void WindowOptions::init(const vkb::CommandParser &parser)
{
	vkb::Window::OptionalProperties properties;

	if (parser.contains(&width_flag))
	{
		auto width = parser.as<uint32_t>(&width_flag);
		if (width < vkb::Platform::MIN_WINDOW_WIDTH)
		{
			LOGD("[Window Options] {} is smaller than the minimum width {}, resorting to minimum width", width, vkb::Platform::MIN_WINDOW_WIDTH);
			width = vkb::Platform::MIN_WINDOW_WIDTH;
		}
		properties.extent.width = width;
	}

	if (parser.contains(&height_flag))
	{
		auto height = parser.as<uint32_t>(&height_flag);
		if (height < vkb::Platform::MIN_WINDOW_HEIGHT)
		{
			LOGD("[Window Options] {} is smaller than the minimum height {}, resorting to minimum height", height, vkb::Platform::MIN_WINDOW_HEIGHT);
			height = vkb::Platform::MIN_WINDOW_HEIGHT;
		}
		properties.extent.height = height;
	}

	if (parser.contains(&headless_flag))
	{
		properties.mode = vkb::Window::Mode::Headless;
	}
	else if (parser.contains(&fullscreen_flag))
	{
		properties.mode = vkb::Window::Mode::Fullscreen;
	}
	else if (parser.contains(&borderless_flag))
	{
		properties.mode = vkb::Window::Mode::FullscreenBorderless;
	}

	if (parser.contains(&vsync_flag))
	{
		std::string value = parser.as<std::string>(&vsync_flag);
		std::transform(value.begin(), value.end(), value.begin(), ::tolower);
		if (value == "on")
		{
			properties.vsync = vkb::Window::Vsync::ON;
		}
		else if (value == "off")
		{
			properties.vsync = vkb::Window::Vsync::OFF;
		}
	}

	platform->set_window_properties(properties);
}
}        // namespace plugins