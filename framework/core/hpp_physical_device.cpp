/* Copyright (c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
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

#include <core/hpp_physical_device.h>

namespace vkb
{
namespace core
{
HPPPhysicalDevice::HPPPhysicalDevice(HPPInstance &instance, vk::PhysicalDevice physical_device) :
    instance{instance},
    handle{physical_device}
{
	features          = physical_device.getFeatures();
	properties        = physical_device.getProperties();
	memory_properties = physical_device.getMemoryProperties();

	LOGI("Found GPU: {}", properties.deviceName.data());

	queue_family_properties = physical_device.getQueueFamilyProperties();
}

DriverVersion HPPPhysicalDevice::get_driver_version() const
{
	DriverVersion version;

	vk::PhysicalDeviceProperties const &properties = get_properties();
	switch (properties.vendorID)
	{
		case 0x10DE:
			// Nvidia
			version.major = (properties.driverVersion >> 22) & 0x3ff;
			version.minor = (properties.driverVersion >> 14) & 0x0ff;
			version.patch = (properties.driverVersion >> 6) & 0x0ff;
			// Ignoring optional tertiary info in lower 6 bits
			break;
		case 0x8086:
			version.major = (properties.driverVersion >> 14) & 0x3ffff;
			version.minor = properties.driverVersion & 0x3ffff;
			break;
		default:
			version.major = VK_VERSION_MAJOR(properties.driverVersion);
			version.minor = VK_VERSION_MINOR(properties.driverVersion);
			version.patch = VK_VERSION_PATCH(properties.driverVersion);
			break;
	}

	return version;
}

void *HPPPhysicalDevice::get_extension_feature_chain() const
{
	return last_requested_extension_feature;
}

const vk::PhysicalDeviceFeatures &HPPPhysicalDevice::get_features() const
{
	return features;
}

vk::PhysicalDevice HPPPhysicalDevice::get_handle() const
{
	return handle;
}

vkb::core::HPPInstance &HPPPhysicalDevice::get_instance() const
{
	return instance;
}

const vk::PhysicalDeviceMemoryProperties &HPPPhysicalDevice::get_memory_properties() const
{
	return memory_properties;
}

uint32_t HPPPhysicalDevice::get_memory_type(uint32_t bits, vk::MemoryPropertyFlags properties, vk::Bool32 *memory_type_found) const
{
	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
	{
		if ((bits & 1) == 1)
		{
			if ((memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				if (memory_type_found)
				{
					*memory_type_found = true;
				}
				return i;
			}
		}
		bits >>= 1;
	}

	if (memory_type_found)
	{
		*memory_type_found = false;
		return ~0;
	}
	else
	{
		throw std::runtime_error("Could not find a matching memory type");
	}
}

const vk::PhysicalDeviceProperties &HPPPhysicalDevice::get_properties() const
{
	return properties;
}

const std::vector<vk::QueueFamilyProperties> &HPPPhysicalDevice::get_queue_family_properties() const
{
	return queue_family_properties;
}

const vk::PhysicalDeviceFeatures HPPPhysicalDevice::get_requested_features() const
{
	return requested_features;
}

vk::PhysicalDeviceFeatures &HPPPhysicalDevice::get_mutable_requested_features()
{
	return requested_features;
}

}        // namespace core
}        // namespace vkb
