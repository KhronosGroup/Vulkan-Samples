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

#include "physical_device.h"

namespace vkb
{
PhysicalDevice::PhysicalDevice(Instance &instance, VkPhysicalDevice physical_device) :
    instance{instance},
    handle{physical_device}
{
	vkGetPhysicalDeviceFeatures(physical_device, &features);
	vkGetPhysicalDeviceProperties(physical_device, &properties);
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

	LOGI("Found GPU: {}", properties.deviceName);

	uint32_t queue_family_properties_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_properties_count, nullptr);
	queue_family_properties = std::vector<VkQueueFamilyProperties>(queue_family_properties_count);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_properties_count, queue_family_properties.data());
}

Instance &PhysicalDevice::get_instance() const
{
	return instance;
}

VkBool32 PhysicalDevice::is_present_supported(VkSurfaceKHR surface, uint32_t queue_family_index) const
{
	VkBool32 present_supported{VK_FALSE};

	if (surface != VK_NULL_HANDLE)
	{
		VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(handle, queue_family_index, surface, &present_supported));
	}

	return present_supported;
}

const VkFormatProperties PhysicalDevice::get_format_properties(VkFormat format) const
{
	VkFormatProperties format_properties;

	vkGetPhysicalDeviceFormatProperties(handle, format, &format_properties);

	return format_properties;
}

VkPhysicalDevice PhysicalDevice::get_handle() const
{
	return handle;
}

const VkPhysicalDeviceFeatures &PhysicalDevice::get_features() const
{
	return features;
}

const VkPhysicalDeviceProperties PhysicalDevice::get_properties() const
{
	return properties;
}

const VkPhysicalDeviceMemoryProperties PhysicalDevice::get_memory_properties() const
{
	return memory_properties;
}

const std::vector<VkQueueFamilyProperties> &PhysicalDevice::get_queue_family_properties() const
{
	return queue_family_properties;
}

uint32_t PhysicalDevice::get_queue_family_performance_query_passes(
    const VkQueryPoolPerformanceCreateInfoKHR *perf_query_create_info) const
{
	uint32_t passes_needed;
	vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(get_handle(), perf_query_create_info,
	                                                        &passes_needed);
	return passes_needed;
}

void PhysicalDevice::enumerate_queue_family_performance_query_counters(
    uint32_t                            queue_family_index,
    uint32_t *                          count,
    VkPerformanceCounterKHR *           counters,
    VkPerformanceCounterDescriptionKHR *descriptions) const
{
	VK_CHECK(vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(
	    get_handle(), queue_family_index, count, counters, descriptions));
}

VkPhysicalDeviceFeatures &PhysicalDevice::get_mutable_requested_features()
{
	return requested_features;
}

const VkPhysicalDeviceFeatures PhysicalDevice::get_requested_features() const
{
	return requested_features;
}

void *PhysicalDevice::get_requested_extension_features() const
{
	return last_requested_extension_feature;
}

void *&PhysicalDevice::get_mutable_requested_extension_features()
{
	return last_requested_extension_feature;
}

void PhysicalDevice::request_descriptor_indexing_features()
{
	// Request the relevant extension
	descriptor_indexing_features = request_extension_features<VkPhysicalDeviceDescriptorIndexingFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT);

	chain_extension_features(descriptor_indexing_features);
}

const VkPhysicalDeviceDescriptorIndexingFeaturesEXT &PhysicalDevice::get_descriptor_indexing_features() const
{
	return descriptor_indexing_features;
}

void PhysicalDevice::request_performance_counter_features()
{
	// Request the relevant extensions
	performance_counter_features = request_extension_features<VkPhysicalDevicePerformanceQueryFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR);

	chain_extension_features(performance_counter_features);
}

const VkPhysicalDevicePerformanceQueryFeaturesKHR &PhysicalDevice::get_performance_counter_features() const
{
	return performance_counter_features;
}

void PhysicalDevice::request_host_query_reset_features()
{
	// Request the relevant extension
	host_query_reset_features = request_extension_features<VkPhysicalDeviceHostQueryResetFeatures>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES);

	chain_extension_features(host_query_reset_features);
}

const VkPhysicalDeviceHostQueryResetFeatures &PhysicalDevice::get_host_query_reset_features() const
{
	return host_query_reset_features;
}

}        // namespace vkb
