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

extern "C"
{
	namespace components
	{
	/**
 * @brief A base context used for platform detection. Components or functions that consume this context can use it to create platform specific functionality.
 */
	struct PlatformContext
	{
		virtual std::vector<std::string> arguments() const = 0;
	};
	}        // namespace components

	/**
 * @brief Forward declare platform_main so that it can be defined elsewhere. CUSTOM_MAIN must only be used once in an executable
 * 
 *  Example Usage:
 * 
 *  CUSTOM_MAIN(context) {
 *      auto* windows = dynamic_cast<WindowsContext>(context);
 *      if (windows) {
 *           ... windows stuff
 *      }
 *
 *      ... other code
 *
 *      return EXIT_SUCCESS;
 *  }
 * 
 * @return int status code
 */
	int platform_main(components::PlatformContext *);

#if defined(_WIN32)

	namespace components
	{
#	include <Windows.h>
	struct WindowsContext : virtual PlatformContext
	{
		HINSTANCE                hInstance;
		HINSTANCE                hPrevInstance;
		PSTR                     lpCmdLine;
		INT                      nCmdShow;
		std::vector<std::string> _arguments;

		virtual std::vector<std::string> arguments() const override
		{
			return _arguments;
		};
	};
	}        // namespace components

// TODO: get arguments from handles
#	define CUSTOM_MAIN(context_name)                                                                    \
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
		int platform_main(components::PlatformContext *context_name)

#elif defined(__ANDROID__)

#	include <android_native_app_glue.h>

	namespace components
	{
	struct AndroidContext : virtual PlatformContext
	{
		android_app *            android_app;
		std::vector<std::string> _arguments;

		virtual std::vector<std::string> arguments() const override
		{
			return _arguments;
		};
	};
	}        // namespace components

// TODO: get arguments from bundle
#	define CUSTOM_MAIN(context_name)               \
		void android_main(android_app *android_app) \
		{                                           \
			components::AndroidContext context{};   \
			context.android_app = android_app;      \
                                                    \
			platform_main(&context);                \
		}                                           \
                                                    \
		int platform_main(components::PlatformContext *context_name)

#elif defined(__APPLE__) || defined(__MACH__)

	namespace components
	{
	struct MacOSXContext : virtual PlatformContext
	{
		std::vector<std::string> _arguments;

		virtual std::vector<std::string> arguments() const override
		{
			return _arguments;
		};
	};
	}        // namespace components

#	define CUSTOM_MAIN(context_name)                                             \
		int main(int argc, char *argv[])                                          \
		{                                                                         \
			components::MacOSXContext context{};                                  \
			context._arguments = std::vector<std::string>{argv + 1, argv + argc}; \
                                                                                  \
			return platform_main(&context);                                       \
		}                                                                         \
                                                                                  \
		int platform_main(components::PlatformContext *context_name)

#elif defined(__linux__)

	namespace components
	{
	struct UnixContext : virtual PlatformContext
	{
		std::vector<std::string> _arguments;

		virtual std::vector<std::string> arguments() const override
		{
			return _arguments;
		};
	};
	}        // namespace components

#	define CUSTOM_MAIN(context_name)                                             \
		int main(int argc, char *argv[])                                          \
		{                                                                         \
			components::UnixContext context{};                                    \
			context._arguments = std::vector<std::string>{argv + 1, argv + argc}; \
                                                                                  \
			return platform_main(&context);                                       \
		}                                                                         \
                                                                                  \
		int platform_main(components::PlatformContext *context_name)

#endif
}
