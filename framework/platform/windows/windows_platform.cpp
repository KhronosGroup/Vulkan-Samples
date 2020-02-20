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

#include "windows_platform.h"

#include <Windows.h>
#include <iostream>
#include <shellapi.h>
#include <stdexcept>

#include "common/error.h"

VKBP_DISABLE_WARNINGS()
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
VKBP_ENABLE_WARNINGS()

#include "platform/glfw_window.h"
#include "platform/headless_window.h"

namespace vkb
{
namespace
{
inline const std::string get_temp_path_from_environment()
{
	std::string temp_path = "temp/";

	TCHAR temp_buffer[MAX_PATH];
	DWORD temp_path_ret = GetTempPath(MAX_PATH, temp_buffer);
	if (temp_path_ret > MAX_PATH || temp_path_ret == 0)
	{
		temp_path = "temp/";
	}
	else
	{
		temp_path = std::string(temp_buffer) + "/";
	}

	return temp_path;
}

/// @brief Converts wstring to string using Windows specific function
/// @param wstr Wide string to convert
/// @return A converted utf8 string
std::string wstr_to_str(const std::wstring &wstr)
{
	if (wstr.empty())
	{
		return {};
	}

	auto wstr_len = static_cast<int>(wstr.size());
	auto str_len  = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr_len, NULL, 0, NULL, NULL);

	std::string str(str_len, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr_len, &str[0], str_len, NULL, NULL);

	return str;
}

inline std::vector<std::string> get_args()
{
	LPWSTR *argv;
	int     argc;

	argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	// Ignore the first argument containing the application full path
	std::vector<std::wstring> arg_strings(argv + 1, argv + argc);
	std::vector<std::string>  args;

	for (auto &arg : arg_strings)
	{
		args.push_back(wstr_to_str(arg));
	}

	return args;
}
}        // namespace

namespace fs
{
void create_directory(const std::string &path)
{
	if (!is_directory(path))
	{
		CreateDirectory(path.c_str(), NULL);
	}
}
}        // namespace fs

WindowsPlatform::WindowsPlatform(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                                 PSTR lpCmdLine, INT nCmdShow)
{
	if (!AllocConsole())
	{
		throw std::runtime_error{"AllocConsole error"};
	}

	FILE *fp;
	freopen_s(&fp, "conin$", "r", stdin);
	freopen_s(&fp, "conout$", "w", stdout);
	freopen_s(&fp, "conout$", "w", stderr);

	Platform::set_arguments(get_args());
	Platform::set_temp_directory(get_temp_path_from_environment());
}

bool WindowsPlatform::initialize(std::unique_ptr<Application> &&app)
{
	return Platform::initialize(std::move(app)) && prepare();
}

void WindowsPlatform::create_window()
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

void WindowsPlatform::terminate(ExitCode code)
{
	Platform::terminate(code);

	if (code != ExitCode::Success || benchmark_mode)
	{
		std::cout << "Press enter to close...\n";
		std::cin.get();
	}

	FreeConsole();
}

const char *WindowsPlatform::get_surface_extension()
{
	return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
}

std::vector<spdlog::sink_ptr> WindowsPlatform::get_platform_sinks()
{
	std::vector<spdlog::sink_ptr> sinks;
	sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	return sinks;
}
}        // namespace vkb
