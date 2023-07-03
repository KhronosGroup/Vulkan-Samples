/* Copyright (c) 2023, Thomas Atkinson
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

#include "windows/context.hpp"

#include <Windows.h>
#include <cassert>
#include <stdexcept>

namespace vkb
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
inline std::string wstr_to_str(const std::wstring &wstr)
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

WindowsPlatformContext::WindowsPlatformContext(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
	_temp_directory = get_temp_path_from_environment();
	_arguments      = get_args();

	// allocate a console for this app
	// TODO: do we really need to do this?
	if (!AllocConsole())
	{
		throw std::runtime_error{"AllocConsole error"};
	}

	FILE *fp;
	freopen_s(&fp, "conin$", "r", stdin);
	freopen_s(&fp, "conout$", "w", stdout);
	freopen_s(&fp, "conout$", "w", stderr);
}

std::vector<std::string> WindowsPlatformContext::arguments() const
{
	return _arguments;
}

std::string WindowsPlatformContext::external_storage_directory() const
{
	return _external_storage_directory;
}

std::string WindowsPlatformContext::temp_directory() const
{
	return _temp_directory;
}
}        // namespace vkb