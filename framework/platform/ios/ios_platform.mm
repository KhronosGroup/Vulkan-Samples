/* Copyright (c) 2023, Holochip Inc.
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

#include "ios_platform.h"

#include "common/error.h"

#include "platform/headless_window.h"
#include "platform/ios/ios_window.h"

VKBP_DISABLE_WARNINGS()
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
VKBP_ENABLE_WARNINGS()

#import <objc/message.h>
#import <objc/runtime.h>

// This is equivalent to creating a @class with one public variable named 'window'.
struct AppDel
{
	Class isa;

	id window;
};

#ifndef VK_MVK_MACOS_SURFACE_EXTENSION_NAME
#	define VK_MVK_MACOS_SURFACE_EXTENSION_NAME "VK_MVK_macos_surface"
#endif

#ifndef VK_EXT_METAL_SURFACE_EXTENSION_NAME
#	define VK_EXT_METAL_SURFACE_EXTENSION_NAME "VK_EXT_metal_surface"
#endif

namespace vkb
{
namespace
{
inline const std::string get_temp_path_from_environment()
{
	std::string temp_path = "/tmp/";

	if (const char *env_ptr = std::getenv("TMPDIR"))
	{
		temp_path = std::string(env_ptr) + "/";
	}

	return temp_path;
}
}        // namespace

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

IosPlatform::IosPlatform(const PlatformContext &context)
    : Platform{context}
{
//	Platform::set_arguments({argv + 1, argv + argc});
//	Platform::set_temp_directory(get_temp_path_from_environment());
	return VK_EXT_METAL_SURFACE_EXTENSION_NAME;
}

const char *IosPlatform::get_surface_extension()
{
	return VK_EXT_METAL_SURFACE_EXTENSION_NAME;
}

void IosPlatform::create_window(const Window::Properties &properties)
{
	if (properties.mode == vkb::Window::Mode::Headless)
	{
		window = std::make_unique<HeadlessWindow>(properties);
	}
	else
	{
		window = std::make_unique<IosWindow>(this, properties);
	}
}
}        // namespace vkb
