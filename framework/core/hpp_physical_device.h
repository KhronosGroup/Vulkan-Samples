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

#include <core/physical_device.h>

#include <core/hpp_instance.h>
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace core
{
class HPPInstance;

struct DriverVersion
{
	uint16_t major;
	uint16_t minor;
	uint16_t patch;
};

/**
 * @brief facade class around vkb::PhysicalDevice, providing a vulkan.hpp-based interface
 *
 * See vkb::PhysicalDevice for documentation
 */
class HPPPhysicalDevice : private vkb::PhysicalDevice
{
  public:
	using vkb::PhysicalDevice::get_extension_feature_chain;
	using vkb::PhysicalDevice::has_high_priority_graphics_queue;
	using vkb::PhysicalDevice::set_high_priority_graphics_queue_enable;

	/**
   * @return The version of the driver
   */
	DriverVersion get_driver_version() const
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

	vk::PhysicalDeviceFeatures const &get_features() const
	{
		return *reinterpret_cast<vk::PhysicalDeviceFeatures const *>(&vkb::PhysicalDevice::get_features());
	}

	vk::PhysicalDevice get_handle() const
	{
		return vkb::PhysicalDevice::get_handle();
	}

	vkb::core::HPPInstance &get_instance() const
	{
		return reinterpret_cast<vkb::core::HPPInstance &>(vkb::PhysicalDevice::get_instance());
	}

	vk::PhysicalDeviceMemoryProperties const &get_memory_properties() const
	{
		return reinterpret_cast<vk::PhysicalDeviceMemoryProperties const &>(vkb::PhysicalDevice::get_memory_properties());
	}

	/**
   * @brief Checks that a given memory type is supported by the GPU
   * @param bits The memory requirement type bits
   * @param properties The memory property to search for
   * @param memory_type_found True if found, false if not found
   * @returns The memory type index of the found memory type
   */
	uint32_t get_memory_type(uint32_t bits, vk::MemoryPropertyFlags properties, vk::Bool32 *memory_type_found = nullptr) const
	{
		vk::PhysicalDeviceMemoryProperties const &memory_properties = get_memory_properties();
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

	vk::PhysicalDeviceFeatures &get_mutable_requested_features()
	{
		return reinterpret_cast<vk::PhysicalDeviceFeatures &>(vkb::PhysicalDevice::get_mutable_requested_features());
	}

	vk::PhysicalDeviceProperties const &get_properties() const
	{
		return reinterpret_cast<vk::PhysicalDeviceProperties const &>(vkb::PhysicalDevice::get_properties());
	}

	std::vector<vk::QueueFamilyProperties> const &get_queue_family_properties() const
	{
		return reinterpret_cast<std::vector<vk::QueueFamilyProperties> const &>(vkb::PhysicalDevice::get_queue_family_properties());
	}

	/**
   * @return Whether an image format is supported by the GPU
   */
	bool is_image_format_supported(vk::Format format) const
	{
		try
		{
			get_handle().getImageFormatProperties(format, vk::ImageType::e2D, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled);
		}
		catch (vk::SystemError &err)
		{
			return err.code() != vk::make_error_code(vk::Result::eErrorFormatNotSupported);
		}
		return true;
	}

	vk::Bool32 is_present_supported(vk::SurfaceKHR surface, uint32_t queue_family_index) const
	{
		return static_cast<vk::Bool32>(vkb::PhysicalDevice::is_present_supported(static_cast<VkSurfaceKHR>(surface), queue_family_index));
	}

	template <typename HPPStructureType>
	HPPStructureType &request_extension_features()
	{
		return reinterpret_cast<HPPStructureType &>(vkb::PhysicalDevice::request_extension_features<typename HPPStructureType::NativeType>(
		    static_cast<VkStructureType>(HPPStructureType::structureType)));
	}
};
}        // namespace core
}        // namespace vkb
