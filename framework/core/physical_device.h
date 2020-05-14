/* Copyright (c) 2020, Arm Limited and Contributors
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

#include "core/instance.h"

namespace vkb
{
class Instance;

/**
 * @brief A wrapper class for VkPhysicalDevice
 *
 * This class is responsible for handling gpu features, properties, and queue families for the device creation.
 */
class PhysicalDevice
{
  public:
	PhysicalDevice(Instance &instance, VkPhysicalDevice physical_device);

	PhysicalDevice(const PhysicalDevice &) = delete;

	PhysicalDevice(PhysicalDevice &&) = delete;

	PhysicalDevice &operator=(const PhysicalDevice &) = delete;

	PhysicalDevice &operator=(PhysicalDevice &&) = delete;

	Instance &get_instance() const;

	VkBool32 is_present_supported(VkSurfaceKHR surface, uint32_t queue_family_index) const;

	const VkFormatProperties get_format_properties(VkFormat format) const;

	VkPhysicalDevice get_handle() const;

	const VkPhysicalDeviceFeatures &get_features() const;

	const VkPhysicalDeviceProperties get_properties() const;

	const VkPhysicalDeviceMemoryProperties get_memory_properties() const;

	const std::vector<VkQueueFamilyProperties> &get_queue_family_properties() const;

	uint32_t get_queue_family_performance_query_passes(
	    const VkQueryPoolPerformanceCreateInfoKHR *perf_query_create_info) const;

	void enumerate_queue_family_performance_query_counters(
	    uint32_t                            queue_family_index,
	    uint32_t *                          count,
	    VkPerformanceCounterKHR *           counters,
	    VkPerformanceCounterDescriptionKHR *descriptions) const;

	VkPhysicalDeviceFeatures &get_mutable_requested_features();

	const VkPhysicalDeviceFeatures get_requested_features() const;

	void *get_requested_extension_features() const;

	// Returns the pointer to the extension feature chain to be used for enabling extension features
	void *&get_mutable_requested_extension_features();

	void request_descriptor_indexing_features();

	const VkPhysicalDeviceDescriptorIndexingFeaturesEXT &get_descriptor_indexing_features() const;

	void request_performance_counter_features();

	const VkPhysicalDevicePerformanceQueryFeaturesKHR &get_performance_counter_features() const;

	void request_host_query_reset_features();

	const VkPhysicalDeviceHostQueryResetFeatures &get_host_query_reset_features() const;

  protected:
	/**
	 * @brief Requests a third party extension to be used by the framework
	 *
	 *        To have the features enabled, this function must be called before the logical device
	 *        is created. To do this request sample specific features inside
	 *        VulkanSample::request_gpu_features(vkb::PhysicalDevice &gpu).
	 *
	 *        If the feature extension requires you to ask for certain features to be enabled, you can
	 *        modify the struct returned by this function, it will propegate the changes to the logical
	 *        device.
	 * @param type The VkStructureType for the extension you are requesting
	 * @template T The user defined type for the extension you are requesting
	 * @returns The extension feature struct
	 */
	template <typename T>
	T &request_extension_features(VkStructureType type)
	{
		// We cannot request extension features if the physical device properties 2 instance extension isnt enabled
		if (!instance.is_enabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
		{
			return T{};
		}

		// If the type already exists in the map, dereference the void pointer to get the extension feature struct
		auto extension_features_it = extension_features.find(type);
		if (extension_features_it != extension_features.end())
		{
			return *((T *) extension_features_it->second.get());
		}

		// Get the extension feature
		VkPhysicalDeviceFeatures2KHR physical_device_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR};
		T                            extension{type};
		physical_device_features.pNext = &extension;
		vkGetPhysicalDeviceFeatures2KHR(handle, &physical_device_features);

		// Insert the extension feature into the extension feature map so its ownership is held
		extension_features.insert({type, std::make_shared<T>(extension)});

		// Pull out the dereferenced void pointer, we can assume its type based on the template
		auto &extension_ref = *((T *) extension_features.find(type)->second.get());

		// If an extension feature has already been requested, we shift the linked list down by one
		// Making this current extension the new base pointer
		if (last_requested_extension_feature)
		{
			extension_ref.pNext = last_requested_extension_feature;
		}
		last_requested_extension_feature = &extension_ref;

		return extension_ref;
	}

	template <typename T>
	void chain_extension_features(T &features)
	{
		// If an extension has already been requested, set that to the pNext element
		if (last_requested_extension_feature)
		{
			features.pNext = last_requested_extension_feature;
		}

		// Set the last requested extension to the pointer of the most recently requested extension
		last_requested_extension_feature = &features;
	}

  private:
	// Handle to the Vulkan instance
	Instance &instance;

	// Handle to the Vulkan physical device
	VkPhysicalDevice handle{VK_NULL_HANDLE};

	// The features that this GPU supports
	VkPhysicalDeviceFeatures features{};

	// The GPU properties
	VkPhysicalDeviceProperties properties;

	// The GPU memory properties
	VkPhysicalDeviceMemoryProperties memory_properties;

	// The GPU queue family properties
	std::vector<VkQueueFamilyProperties> queue_family_properties;

	// The features that will be requested to be enabled in the logical device
	VkPhysicalDeviceFeatures requested_features{};

	// The extension feature pointer
	void *last_requested_extension_feature{nullptr};

	VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptor_indexing_features{};

	VkPhysicalDevicePerformanceQueryFeaturesKHR performance_counter_features{};

	VkPhysicalDeviceHostQueryResetFeatures host_query_reset_features{};
};
}        // namespace vkb
