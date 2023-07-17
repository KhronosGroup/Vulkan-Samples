/* Copyright (c) 2019-2023, Arm Limited and Contributors
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

#include "unix_platform.h"

#include "common/error.h"

#include "platform/glfw_window.h"
#include "platform/headless_window.h"

VKBP_DISABLE_WARNINGS()
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
VKBP_ENABLE_WARNINGS()

#ifndef VK_MVK_MACOS_SURFACE_EXTENSION_NAME
#	define VK_MVK_MACOS_SURFACE_EXTENSION_NAME "VK_MVK_macos_surface"
#endif

#ifndef VK_KHR_XCB_SURFACE_EXTENSION_NAME
#	define VK_KHR_XCB_SURFACE_EXTENSION_NAME "VK_KHR_xcb_surface"
#endif

#ifndef VK_EXT_METAL_SURFACE_EXTENSION_NAME
#	define VK_EXT_METAL_SURFACE_EXTENSION_NAME "VK_EXT_metal_surface"
#endif

#ifndef VK_KHR_XLIB_SURFACE_EXTENSION_NAME
#	define VK_KHR_XLIB_SURFACE_EXTENSION_NAME "VK_KHR_xlib_surface"
#endif

#ifndef VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
#	define VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME "VK_KHR_wayland_surface"
#endif

namespace vkb
{

namespace fs
{
void create_directory(const std::string &path)
{
	if (!is_directory(path))
	{
		mkdir(path.c_str(), 0777);
	}
}
}        // namespace fs

UnixPlatform::UnixPlatform(const PlatformContext &context, const UnixType &type) :
    Platform{context}, type{type}
{
}

void UnixPlatform::create_window(const Window::Properties &properties)
{
	if (properties.mode == vkb::Window::Mode::Headless)
	{
		window = std::make_unique<HeadlessWindow>(properties);
	}
	else
	{
		window = std::make_unique<GlfwWindow>(this, properties);
	}
}
}        // namespace vkb
