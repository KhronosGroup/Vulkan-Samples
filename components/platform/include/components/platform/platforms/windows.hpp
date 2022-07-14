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

#include <Windows.h>

#include "components/platform/platform.hpp"

namespace components
{
class WindowsContext final : public PlatformContext
{
  public:
	WindowsContext()          = default;
	virtual ~WindowsContext() = default;

	virtual std::vector<std::string_view> arguments() const override
	{
		return {m_arguments.begin(), m_arguments.end()};
	}

	HINSTANCE hInstance;
	HINSTANCE hPrevInstance;
	PSTR      lpCmdLine;
	INT       nCmdShow;

  private:
	std::vector<std::string> m_arguments;
};
}        // namespace components

// TODO: get arguments from handles
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