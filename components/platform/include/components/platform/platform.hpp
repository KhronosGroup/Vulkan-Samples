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
#include <string_view>
#include <vector>

namespace components
{
class WindowsContext;
class AndroidContext;
class MacOSXContext;
class UnixContext;

/**
 * @brief A base context used for platform detection. Components or functions that consume this context can use it to create platform specific functionality.
 */
class PlatformContext
{
  public:
	PlatformContext()          = default;
	virtual ~PlatformContext() = default;

	virtual std::vector<std::string_view> arguments() const = 0;

#if defined(_WIN32)
	inline const WindowsContext *cast() const;
#elif defined(__ANDROID__)
	inline const AndroidContext *cast() const;
#elif defined(__APPLE__) || defined(__MACH__)
	inline const MacOSXContext *cast() const;
#elif defined(__linux__)
	inline const UnixContext *cast() const;
#endif
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
int platform_main(const components::PlatformContext *);

#if defined(_WIN32)
#	include <components/platform/platforms/windows.hpp>
#elif defined(__ANDROID__)
#	include <components/platform/platforms/android.hpp>
#elif defined(__APPLE__) || defined(__MACH__)
#	include <components/platform/platforms/macos.hpp>
#elif defined(__linux__)
#	include <components/platform/platforms/unix.hpp>
#endif

namespace components
{
#if defined(_WIN32)
inline const WindowsContext *PlatformContext::cast() const
{
	return dynamic_cast<const WindowsContext *>(this);
}
#elif defined(__ANDROID__)
inline const AndroidContext *PlatformContext::cast() const
{
	return dynamic_cast<const AndroidContext *>(this);
}
#elif defined(__APPLE__) || defined(__MACH__)
inline const MacOSXContext *PlatformContext::cast() const
{
	return dynamic_cast<const MacOSXContext *>(this);
}
#elif defined(__linux__)
inline const UnixContext *PlatformContext::cast() const
{
	return dynamic_cast<const UnixContext *>(this);
}
#endif
}        // namespace components
