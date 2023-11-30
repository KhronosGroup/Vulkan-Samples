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

#include "windows_platform.h"

#include <Windows.h>
#include <iostream>
#include <shellapi.h>
#include <stdexcept>

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

WindowsPlatform::WindowsPlatform(const PlatformContext &context) :
    Platform(context)
{
}

void WindowsPlatform::create_window(const Window::Properties &properties)
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
