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
	PhysicalDevice(const Instance &instance, VkPhysicalDevice physical_device);

	PhysicalDevice(const PhysicalDevice &) = delete;

	PhysicalDevice(PhysicalDevice &&) = delete;

	PhysicalDevice &operator=(const PhysicalDevice &) = delete;

	PhysicalDevice &operator=(PhysicalDevice &&) = delete;

	const Instance &get_instance() const;

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
	template <typename T>
	const T request_extension_features(VkStructureType type)
	{
		if (!instance.is_enabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
		{
			return {};
		}

		VkPhysicalDeviceFeatures2KHR extended_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR};
		T                            ext{type};

		extended_features.pNext = &ext;

		vkGetPhysicalDeviceFeatures2KHR(handle, &extended_features);

		return ext;
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
	const Instance &instance;

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
