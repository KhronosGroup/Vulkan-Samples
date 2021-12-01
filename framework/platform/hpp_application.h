/* Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
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

#include <platform/application.h>

#include <core/hpp_instance.h>
#include <platform/hpp_platform.h>

namespace vkb
{
namespace platform
{
/**
 * @brief facade class around vkb::Application, providing a vulkan.hpp-based interface
 *
 * See vkb::Application for documentation
 */
class HPPApplication : public vkb::Application
{
  public:
	vkb::platform::HPPPlatform &get_platform()
	{
		return *reinterpret_cast<vkb::platform::HPPPlatform *>(platform);
	}

	virtual bool prepare(vkb::Platform &platform) final
	{
		return prepare(reinterpret_cast<vkb::platform::HPPPlatform &>(platform));
	}

	virtual bool prepare(vkb::platform::HPPPlatform &platform)
	{
		return vkb::Application::prepare(reinterpret_cast<vkb::Platform &>(platform));
	}
};
}        // namespace platform
}        // namespace vkb
