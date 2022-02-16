/* Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
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

#include <core/vulkan_resource.h>

namespace vkb
{
namespace core
{
class HPPDevice;

/**
 * @brief facade class around vkb::VulkanResource, providing a vulkan.hpp-based interface
 *
 * See vkb::VulkanResource for documentation
 */
template <typename HPPHandle, typename Device = vkb::core::HPPDevice>
class HPPVulkanResource : private vkb::core::VulkanResource<typename HPPHandle::NativeType, static_cast<VkObjectType>(HPPHandle::objectType), Device>
{
  public:
	HPPVulkanResource(HPPHandle handle = nullptr, vkb::core::HPPDevice *device = nullptr) :
	    vkb::core::VulkanResource<typename HPPHandle::NativeType, static_cast<VkObjectType>(HPPHandle::objectType), Device>(handle, device)
	{}

	HPPHandle const &get_handle() const
	{
		return reinterpret_cast<HPPHandle const &>(
		    vkb::core::VulkanResource<typename HPPHandle::NativeType, static_cast<VkObjectType>(HPPHandle::objectType), Device>::get_handle());
	}

  protected:
	void set_handle(HPPHandle hppHandle)
	{
		handle = static_cast<HPPHandle::NativeType>(hppHandle);
	}
};

}        // namespace core
}        // namespace vkb
