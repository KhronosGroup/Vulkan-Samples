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

#include "window_options.h"

#include <algorithm>

#include "platform/platform.h"
#include "platform/window.h"

namespace plugins
{
WindowOptions::WindowOptions() :
    WindowOptionsTags("Window Options",
                      "A collection of flags to configure window used when running the application. Implementation may differ between platforms", {}, {},
                      {{"borderless", "Run in borderless mode"},
                       {"fullscreen", "Run in fullscreen mode"},
                       {"headless-surface", "Run in headless surface mode. A Surface and swap-chain is still created using VK_EXT_headless_surface."},
                       {"height", "Initial window height"},
                       {"stretch", "Stretch window to fullscreen (direct-to-display only)"},
                       {"vsync", "Force vsync {ON | OFF}. If not set samples decide how vsync is set"},
                       {"width", "Initial window width"}})
{}

bool WindowOptions::handle_option(std::deque<std::string> &arguments)
{
	assert(!arguments.empty() && (arguments[0].substr(0, 2) == "--"));
	std::string option = arguments[0].substr(2);

	vkb::Window::OptionalProperties properties;
	if (option == "borderless")
	{
		properties.mode = vkb::Window::Mode::FullscreenBorderless;
		platform->set_window_properties(properties);

		arguments.pop_front();
		return true;
	}
	else if (option == "fullscreen")
	{
		properties.mode = vkb::Window::Mode::Fullscreen;
		platform->set_window_properties(properties);

		arguments.pop_front();
		return true;
	}
	else if (option == "headless-surface")
	{
		properties.mode = vkb::Window::Mode::Headless;
		platform->set_window_properties(properties);

		arguments.pop_front();
		return true;
	}
	else if (option == "height")
	{
		if (arguments.size() < 2)
		{
			LOGE("Option \"height\" is missing the actual height!");
			return false;
		}
		uint32_t height = static_cast<uint32_t>(std::stoul(arguments[1]));
		if (height < platform->MIN_WINDOW_HEIGHT)
		{
			LOGD("[Window Options] {} is smaller than the minimum height {}, resorting to minimum height", height, platform->MIN_WINDOW_HEIGHT);
			height = platform->MIN_WINDOW_HEIGHT;
		}
		properties.extent.height = height;
		platform->set_window_properties(properties);

		arguments.pop_front();
		arguments.pop_front();
		return true;
	}
	else if (option == "stretch")
	{
		properties.mode = vkb::Window::Mode::FullscreenStretch;
		platform->set_window_properties(properties);

		arguments.pop_front();
		return true;
	}
	else if (option == "vsync")
	{
		if (arguments.size() < 2)
		{
			LOGE("Option \"vsync\" is missing the actual setting!");
			return false;
		}
		std::string value = arguments[1];
		std::transform(value.begin(), value.end(), value.begin(), ::tolower);
		if (value == "on")
		{
			properties.vsync = vkb::Window::Vsync::ON;
		}
		else if (value == "off")
		{
			properties.vsync = vkb::Window::Vsync::OFF;
		}
		platform->set_window_properties(properties);

		arguments.pop_front();
		arguments.pop_front();
		return true;
	}
	else if (option == "width")
	{
		if (arguments.size() < 2)
		{
			LOGE("Option \"width\" is missing the actual width!");
			return false;
		}
		uint32_t width = static_cast<uint32_t>(std::stoul(arguments[1]));
		if (width < platform->MIN_WINDOW_WIDTH)
		{
			LOGD("[Window Options] {} is smaller than the minimum width {}, resorting to minimum width", width, platform->MIN_WINDOW_WIDTH);
			width = platform->MIN_WINDOW_WIDTH;
		}
		properties.extent.width = width;
		platform->set_window_properties(properties);

		arguments.pop_front();
		arguments.pop_front();
		return true;
	}
	return false;
}
}        // namespace plugins