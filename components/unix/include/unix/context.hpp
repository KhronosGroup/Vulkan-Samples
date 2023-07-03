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

#include <string>

#include <core/platform/context.hpp>

namespace vkb
{
/**
 * @brief Unix platform context
 *
 * @warning Use in extreme circumstances with code guarded by the PLATFORM__UNIX define
 */
class UnixPlatformContext final : public PlatformContext
{
  public:
	UnixPlatformContext(int argc, char **argv);
	~UnixPlatformContext() override = default;

	std::vector<std::string> arguments() const override;
	std::string              external_storage_directory() const override;
	std::string              temp_directory() const override;

  private:
	std::vector<std::string> _arguments{};
	std::string                   _external_storage_directory{""};
	std::string                   _temp_directory{""};
};
}        // namespace vkb