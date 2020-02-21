/* Copyright (c) 2019, Arm Limited and Contributors
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

VKBP_DISABLE_WARNINGS()
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
VKBP_ENABLE_WARNINGS()

#include "platform/glfw_window.h"
#include "platform/headless_window.h"

#ifndef VK_MVK_MACOS_SURFACE_EXTENSION_NAME
#	define VK_MVK_MACOS_SURFACE_EXTENSION_NAME "VK_MVK_macos_surface"
#endif

#ifndef VK_KHR_XCB_SURFACE_EXTENSION_NAME
#	define VK_KHR_XCB_SURFACE_EXTENSION_NAME "VK_KHR_xcb_surface"
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

UnixPlatform::UnixPlatform(const UnixType &type, int argc, char **argv) :
    type{type}
{
	Platform::set_arguments({argv + 1, argv + argc});
	Platform::set_temp_directory(get_temp_path_from_environment());
}

bool UnixPlatform::initialize(std::unique_ptr<Application> &&app)
{
	return Platform::initialize(std::move(app)) && prepare();
}

void UnixPlatform::create_window()
{
	if (active_app->is_headless())
	{
		window = std::make_unique<HeadlessWindow>(*this);
	}
	else
	{
		window = std::make_unique<GlfwWindow>(*this);
	}
}

const char *UnixPlatform::get_surface_extension()
{
	if (type == UnixType::Mac)
	{
		return VK_MVK_MACOS_SURFACE_EXTENSION_NAME;
	}
	else
	{
		return VK_KHR_XCB_SURFACE_EXTENSION_NAME;
	}
}

std::vector<spdlog::sink_ptr> UnixPlatform::get_platform_sinks()
{
	std::vector<spdlog::sink_ptr> sinks;
	sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	return sinks;
}
}        // namespace vkb
