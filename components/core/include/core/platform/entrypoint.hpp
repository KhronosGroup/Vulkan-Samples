/* Copyright (c) 2023-2024, Thomas Atkinson
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

#include "core/platform/context.hpp"

// Platform specific entrypoint definitions
// Applications should use CUSTOM_MAIN to define their own main function
// Definitions added by core/CMakeLists.txt

#if defined(PLATFORM__MACOS)
#include <TargetConditionals.h>
#endif

#if defined(PLATFORM__ANDROID)
#	include <game-activity/native_app_glue/android_native_app_glue.h>
extern std::unique_ptr<vkb::PlatformContext> create_platform_context(android_app *state);

#	define CUSTOM_MAIN(context_name)                      \
		int  platform_main(const vkb::PlatformContext &);  \
		void android_main(android_app *state)              \
		{                                                  \
			auto context = create_platform_context(state); \
			platform_main(*context);                       \
		}                                                  \
		int platform_main(const vkb::PlatformContext &context_name)
#elif defined(PLATFORM__WINDOWS)
#	include <Windows.h>
extern std::unique_ptr<vkb::PlatformContext> create_platform_context(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow);

#	define CUSTOM_MAIN(context_name)                                                                    \
		int          platform_main(const vkb::PlatformContext &);                                        \
		int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) \
		{                                                                                                \
			auto context = create_platform_context(hInstance, hPrevInstance, lpCmdLine, nCmdShow);       \
			return platform_main(*context);                                                              \
		}                                                                                                \
		int platform_main(const vkb::PlatformContext &context_name)
#elif defined(PLATFORM__LINUX) || defined(PLATFORM__MACOS)
extern std::unique_ptr<vkb::PlatformContext> create_platform_context(int argc, char **argv);

#	define CUSTOM_MAIN(context_name)                           \
		int platform_main(const vkb::PlatformContext &);        \
		int main(int argc, char *argv[])                        \
		{                                                       \
			auto context = create_platform_context(argc, argv); \
			return platform_main(*context);                     \
		}                                                       \
		int platform_main(const vkb::PlatformContext &context_name)

#else
#	include <stdexcept>
#	define CUSTOM_MAIN(context_name)                           \
		int main(int argc, char *argv[])                        \
		{                                                       \
			throw std::runtime_error{"platform not supported"}; \
		}                                                       \
		int unused(const vkb::PlatformContext &context_name)
#endif
