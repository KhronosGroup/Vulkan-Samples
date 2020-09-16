/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include "unix_d2d_platform.h"

#include "common/error.h"

VKBP_DISABLE_WARNINGS()
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
VKBP_ENABLE_WARNINGS()

#include "headless/headless.h"
#include "platform/headless_window.h"
#include "platform/unix/direct_window.h"

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

UnixD2DPlatform::UnixD2DPlatform(int argc, char **argv)
{
	Platform::set_arguments({argv + 1, argv + argc});
	Platform::set_temp_directory(get_temp_path_from_environment());
}

bool UnixD2DPlatform::initialize(const std::vector<extensions::Extension *> &extensions)
{
	return Platform::initialize(extensions) && prepare();
}

void UnixD2DPlatform::create_window()
{
	if (using_extension<extensions::Headless>())
	{
		window = std::make_unique<HeadlessWindow>(*this, width, height);
	}
	else
	{
		window = std::make_unique<DirectWindow>(*this, width, height);
	}
}

const char *UnixD2DPlatform::get_surface_extension()
{
	return VK_KHR_DISPLAY_EXTENSION_NAME;
}

std::vector<spdlog::sink_ptr> UnixD2DPlatform::get_platform_sinks()
{
	std::vector<spdlog::sink_ptr> sinks;
	sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	return sinks;
}
}        // namespace vkb
