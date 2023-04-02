/* Copyright (c) 2022, Arm Limited and Contributors
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

#pragma once

#include <string>
#include <vector>

#include <components/common/error.hpp>

VKBP_DISABLE_WARNINGS()
#include <Windows.h>
VKBP_ENABLE_WARNINGS()

#include "components/platform/platform.hpp"

namespace components
{
namespace detail
{
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
}        // namespace detail

class WindowsContext final : public PlatformContext
{
  public:
	WindowsContext()          = default;
	virtual ~WindowsContext() = default;

	virtual std::vector<std::string> arguments() const override
	{
		if (m_arguments.empty())
		{
			int     argc;
			LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
			if (argv)
			{
				for (int i = 0; i < argc; i++)
				{
					m_arguments.push_back(detail::wstr_to_str(std::wstring(argv[i])));
				}
				LocalFree(argv);
			}
		}

		if (m_arguments.empty())
		{
			throw std::runtime_error("Failed to parse command line arguments");
		}

		if (m_arguments.size() == 1)
		{
			// only executable name
			return {};
		}

		// remove executable name
		return {m_arguments.begin() + 1, m_arguments.end()};
	}

	HINSTANCE hInstance;
	HINSTANCE hPrevInstance;
	PSTR      lpCmdLine;
	INT       nCmdShow;

  private:
	mutable std::vector<std::string> m_arguments;
};
}        // namespace components

#define CUSTOM_MAIN(context_name)                                                                    \
	int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) \
	{                                                                                                \
		components::WindowsContext context{};                                                        \
		context.hInstance     = hInstance;                                                           \
		context.hPrevInstance = hPrevInstance;                                                       \
		context.lpCmdLine     = lpCmdLine;                                                           \
		context.nCmdShow      = nCmdShow;                                                            \
                                                                                                     \
		return platform_main(&context);                                                              \
	}                                                                                                \
                                                                                                     \
	int platform_main(const components::PlatformContext *context_name)