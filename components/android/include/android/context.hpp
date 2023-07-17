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
#include <vector>

#include <core/platform/context.hpp>

#include <game-activity/native_app_glue/android_native_app_glue.h>

namespace vkb
{
/**
 * @brief Android platform context
 *
 * @warning Use in extreme circumstances with code guarded by the PLATFORM__ANDROID define
 */
class AndroidPlatformContext final : public PlatformContext
{
  public:
	AndroidPlatformContext(android_app *app);
	~AndroidPlatformContext() override = default;

	android_app *app{nullptr};

	static std::string              android_external_storage_directory;
	static std::string              android_temp_directory;
	static std::vector<std::string> android_arguments;
};
}        // namespace vkb