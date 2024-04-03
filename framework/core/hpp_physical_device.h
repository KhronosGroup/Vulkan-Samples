/* Copyright (c) 2023-2024, NVIDIA CORPORATION. All rights reserved.
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

#include <core/hpp_instance.h>
#include <map>
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
 * @brief A wrapper class for vk::PhysicalDevice
 *
 * This class is responsible for handling gpu features, properties, and queue families for the device creation.
 */
class HPPPhysicalDevice
{
  public:
	HPPPhysicalDevice(HPPInstance &instance, vk::PhysicalDevice physical_device);

	HPPPhysicalDevice(const HPPPhysicalDevice &) = delete;

	HPPPhysicalDevice(HPPPhysicalDevice &&) = delete;

	HPPPhysicalDevice &operator=(const HPPPhysicalDevice &) = delete;

	HPPPhysicalDevice &operator=(HPPPhysicalDevice &&) = delete;

	/**
	 * @return The version of the driver
	 */
	DriverVersion get_driver_version() const;

	/**
	 * @brief Used at logical device creation to pass the extensions feature chain to vkCreateDevice
	 * @returns A void pointer to the start of the extension linked list
	 */
	void *get_extension_feature_chain() const;

	bool is_extension_supported(const std::string &requested_extension) const;

	const vk::PhysicalDeviceFeatures &get_features() const;

	vk::PhysicalDevice get_handle() const;

	vkb::core::HPPInstance &get_instance() const;

	const vk::PhysicalDeviceMemoryProperties &get_memory_properties() const;

	/**
	 * @brief Checks that a given memory type is supported by the GPU
	 * @param bits The memory requirement type bits
	 * @param properties The memory property to search for
	 * @param memory_type_found True if found, false if not found
	 * @returns The memory type index of the found memory type
	 */
	uint32_t get_memory_type(uint32_t bits, vk::MemoryPropertyFlags properties, vk::Bool32 *memory_type_found = nullptr) const;

	const vk::PhysicalDeviceProperties &get_properties() const;

	const std::vector<vk::QueueFamilyProperties> &get_queue_family_properties() const;

	const vk::PhysicalDeviceFeatures get_requested_features() const;

	vk::PhysicalDeviceFeatures &get_mutable_requested_features();

	/**
	 * @brief Get an extension features struct
	 *
	 *        Gets the actual extension features struct with the supported flags set.
	 *        The flags you're interested in can be set in a corresponding struct in the structure chain
	 *        by calling PhysicalDevice::add_extension_features()
	 * @returns The extension feature struct
	 */
	template <typename HPPStructureType>
	HPPStructureType get_extension_features()
	{
		// We cannot request extension features if the physical device properties 2 instance extension isn't enabled
		if (!instance.is_enabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
		{
			throw std::runtime_error("Couldn't request feature from device as " + std::string(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) +
			                         " isn't enabled!");
		}

		// Get the extension feature
		return handle.getFeatures2KHR<vk::PhysicalDeviceFeatures2KHR, HPPStructureType>().template get<HPPStructureType>();
	}

	/**
	 * @brief Add an extension features struct to the structure chain used for device creation
	 *
	 *        To have the features enabled, this function must be called before the logical device
	 *        is created. To do this request sample specific features inside
	 *        VulkanSample::request_gpu_features(vkb::PhysicalDevice &gpu).
	 *
	 *        If the feature extension requires you to ask for certain features to be enabled, you can
	 *        modify the struct returned by this function, it will propagate the changes to the logical
	 *        device.
	 * @returns A reference to extension feature struct in the structure chain
	 */
	template <typename HPPStructureType>
	HPPStructureType &add_extension_features()
	{
		// We cannot request extension features if the physical device properties 2 instance extension isn't enabled
		if (!instance.is_enabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
		{
			throw std::runtime_error("Couldn't request feature from device as " + std::string(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) +
			                         " isn't enabled!");
		}

		// Add an (empty) extension features into the map of extension features
		auto [it, added] = extension_features.insert({HPPStructureType::structureType, std::make_shared<HPPStructureType>()});
		if (added)
		{
			// if it was actually added, also add it to the structure chain
			if (last_requested_extension_feature)
			{
				static_cast<HPPStructureType *>(it->second.get())->pNext = last_requested_extension_feature;
			}
			last_requested_extension_feature = it->second.get();
		}

		return *static_cast<HPPStructureType *>(it->second.get());
	}

	/**
	 * @brief Sets whether or not the first graphics queue should have higher priority than other queues.
	 * Very specific feature which is used by async compute samples.
	 * @param enable If true, present queue will have prio 1.0 and other queues have prio 0.5.
	 * Default state is false, where all queues have 0.5 priority.
	 */
	void set_high_priority_graphics_queue_enable(bool enable)
	{
		high_priority_graphics_queue = enable;
	}

	/**
	 * @brief Returns high priority graphics queue state.
	 * @return High priority state.
	 */
	bool has_high_priority_graphics_queue() const
	{
		return high_priority_graphics_queue;
	}

  private:
	// Handle to the Vulkan instance
	HPPInstance &instance;

	// Handle to the Vulkan physical device
	vk::PhysicalDevice handle{nullptr};

	// The features that this GPU supports
	vk::PhysicalDeviceFeatures features;

	// The extensions that this GPU supports
	std::vector<vk::ExtensionProperties> device_extensions;

	// The GPU properties
	vk::PhysicalDeviceProperties properties;

	// The GPU memory properties
	vk::PhysicalDeviceMemoryProperties memory_properties;

	// The GPU queue family properties
	std::vector<vk::QueueFamilyProperties> queue_family_properties;

	// The features that will be requested to be enabled in the logical device
	vk::PhysicalDeviceFeatures requested_features;

	// The extension feature pointer
	void *last_requested_extension_feature{nullptr};

	// Holds the extension feature structures, we use a map to retain an order of requested structures
	std::map<vk::StructureType, std::shared_ptr<void>> extension_features;

	bool high_priority_graphics_queue{false};
};
}        // namespace core
}        // namespace vkb
