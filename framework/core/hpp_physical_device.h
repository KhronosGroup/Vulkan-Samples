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

#include <core/physical_device.h>
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace core
{
/**
 * @brief facade class around vkb::PhysicalDevice, providing a vulkan.hpp-based interface
 *
 * See vkb::PhysicalDevice for documentation
 */
class HPPPhysicalDevice : protected vkb::PhysicalDevice
{
  public:
	vk::PhysicalDeviceFeatures const &get_features() const
	{
		return *reinterpret_cast<vk::PhysicalDeviceFeatures const *>(&vkb::PhysicalDevice::get_features());
	}

	vk::PhysicalDevice get_handle() const
	{
		return vkb::PhysicalDevice::get_handle();
	}

	vk::PhysicalDeviceFeatures &get_mutable_requested_features()
	{
		return *reinterpret_cast<vk::PhysicalDeviceFeatures *>(&vkb::PhysicalDevice::get_mutable_requested_features());
	}

	vk::PhysicalDeviceProperties const &get_properties() const
	{
		return *reinterpret_cast<vk::PhysicalDeviceProperties const *>(&vkb::PhysicalDevice::get_properties());
	}
};
}        // namespace core
}        // namespace vkb
