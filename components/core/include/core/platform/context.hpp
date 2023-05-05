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

#pragma once

#include <memory>
#include <string_view>
#include <vector>

namespace vkb
{
/**
 * @brief A platform context contains abstract platform specific operations
 *
 *        A platform can be thought as the physical device and operating system configuration that the application
 *        is running on.
 *
 *        Some platforms can be reused across different hardware configurations, such as Linux and Macos as both
 *        are POSIX compliant. However, some platforms are more specific such as Android and Windows
 */
class PlatformContext
{
  public:
	PlatformContext()          = default;
	virtual ~PlatformContext() = default;

	virtual std::vector<std::string_view> arguments() const = 0;

	virtual std::string_view external_storage_directory() const = 0;

	virtual std::string_view temp_directory() const = 0;
};
}        // namespace vkb