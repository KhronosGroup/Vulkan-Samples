/* Copyright (c) 2020-2024, Arm Limited and Contributors
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

	bool is_extension_supported(const std::string &extension) const;

	const VkFormatProperties get_format_properties(VkFormat format) const;

	VkPhysicalDevice get_handle() const;

	const VkPhysicalDeviceFeatures &get_features() const;

	const VkPhysicalDeviceProperties &get_properties() const;

	const VkPhysicalDeviceMemoryProperties &get_memory_properties() const;

	const std::vector<VkQueueFamilyProperties> &get_queue_family_properties() const;

	uint32_t get_queue_family_performance_query_passes(
	    const VkQueryPoolPerformanceCreateInfoKHR *perf_query_create_info) const;

	void enumerate_queue_family_performance_query_counters(
	    uint32_t                            queue_family_index,
	    uint32_t                           *count,
	    VkPerformanceCounterKHR            *counters,
	    VkPerformanceCounterDescriptionKHR *descriptions) const;

	const VkPhysicalDeviceFeatures get_requested_features() const;

	VkPhysicalDeviceFeatures &get_mutable_requested_features();

	/**
	 * @brief Used at logical device creation to pass the extensions feature chain to vkCreateDevice
	 * @returns A void pointer to the start of the extension linked list
	 */
	void *get_extension_feature_chain() const;

	/**
	 * @brief Get an extension features struct
	 *
	 *        Gets the actual extension features struct with the supported flags set.
	 *        The flags you're interested in can be set in a corresponding struct in the structure chain
	 *        by calling PhysicalDevice::add_extension_features()
	 * @param type The VkStructureType for the extension you are requesting
	 * @returns The extension feature struct
	 */
	template <typename T>
	T get_extension_features(VkStructureType type)
	{
		// We cannot request extension features if the physical device properties 2 instance extension isn't enabled
		if (!instance.is_enabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
		{
			throw std::runtime_error("Couldn't request feature from device as " + std::string(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) +
			                         " isn't enabled!");
		}

		// Get the extension features
		T                            features{type};
		VkPhysicalDeviceFeatures2KHR physical_device_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR};
		physical_device_features.pNext = &features;
		vkGetPhysicalDeviceFeatures2KHR(handle, &physical_device_features);

		return features;
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
	 * @param type The VkStructureType for the extension you are requesting
	 * @returns A reference to extension feature struct in the structure chain
	 */
	template <typename T>
	T &add_extension_features(VkStructureType type)
	{
		// We cannot request extension features if the physical device properties 2 instance extension isn't enabled
		if (!instance.is_enabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
		{
			throw std::runtime_error("Couldn't request feature from device as " + std::string(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) +
			                         " isn't enabled!");
		}

		// Add an (empty) extension features into the map of extension features
		auto [it, added] = extension_features.insert({type, std::make_shared<T>(T{type})});
		if (added)
		{
			// if it was actually added, also add it to the structure chain
			if (last_requested_extension_feature)
			{
				static_cast<T *>(it->second.get())->pNext = last_requested_extension_feature;
			}
			last_requested_extension_feature = it->second.get();
		}

		return *static_cast<T *>(it->second.get());
	}

	/**
	 * @brief Request an optional features flag
	 *
	 *        Calls get_extension_features to get the support of the requested flag. If it's supported,
	 *        add_extension_features is called, otherwise a log message is generated.
	 *
	 * @returns true if the requested feature is supported, otherwise false
	 */
	template <typename Feature>
	VkBool32 request_optional_feature(VkStructureType type, VkBool32 Feature::*flag, std::string const &featureName, std::string const &flagName)
	{
		VkBool32 supported = get_extension_features<Feature>(type).*flag;
		if (supported)
		{
			add_extension_features<Feature>(type).*flag = true;
		}
		else
		{
			LOGI("Requested optional feature <{}::{}> is not supported", featureName, flagName);
		}
		return supported;
	}

	/**
	 * @brief Request a required features flag
	 *
	 *        Calls get_extension_features to get the support of the requested flag. If it's supported,
	 *        add_extension_features is called, otherwise a runtime_error is thrown.
	 */
	template <typename Feature>
	void request_required_feature(VkStructureType type, VkBool32 Feature::*flag, std::string const &featureName, std::string const &flagName)
	{
		if (get_extension_features<Feature>(type).*flag)
		{
			add_extension_features<Feature>(type).*flag = true;
		}
		else
		{
			throw std::runtime_error(std::string("Requested required feature <") + featureName + "::" + flagName + "> is not supported");
		}
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
	Instance &instance;

	// Handle to the Vulkan physical device
	VkPhysicalDevice handle{VK_NULL_HANDLE};

	// The features that this GPU supports
	VkPhysicalDeviceFeatures features{};

	// The extensions that this GPU supports
	std::vector<VkExtensionProperties> device_extensions;

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

	// Holds the extension feature structures, we use a map to retain an order of requested structures
	std::map<VkStructureType, std::shared_ptr<void>> extension_features;

	bool high_priority_graphics_queue{};
};

#define REQUEST_OPTIONAL_FEATURE(gpu, Feature, type, flag) gpu.request_optional_feature<Feature>(type, &Feature::flag, #Feature, #flag)
#define REQUEST_REQUIRED_FEATURE(gpu, Feature, type, flag) gpu.request_required_feature<Feature>(type, &Feature::flag, #Feature, #flag)

}        // namespace vkb
