/* Copyright (c) 2021-2022, NVIDIA CORPORATION. All rights reserved.
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

#include <core/hpp_physical_device.h>
#include <core/instance.h>
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace core
{
/**
 * @brief facade class around vkb::Instance, providing a vulkan.hpp-based interface
 *
 * See vkb::Instance for documentation
 */
class HPPInstance : protected vkb::Instance
{
  public:
	using vkb::Instance::is_enabled;

	HPPInstance(const std::string &                           application_name,
	            const std::unordered_map<const char *, bool> &required_extensions        = {},
	            const std::vector<const char *> &             required_validation_layers = {},
	            bool                                          headless                   = false,
	            uint32_t                                      api_version                = VK_API_VERSION_1_0) :
	    vkb::Instance(application_name, required_extensions, required_validation_layers, headless, api_version)
	{}

	vk::Instance get_handle() const
	{
		return vkb::Instance::get_handle();
	}

	vkb::core::HPPPhysicalDevice &get_suitable_gpu(vk::SurfaceKHR surface)
	{
		return reinterpret_cast<vkb::core::HPPPhysicalDevice &>(vkb::Instance::get_suitable_gpu(surface));
	}
};
}        // namespace core
}        // namespace vkb
