/* Copyright (c) 2020-2025, Arm Limited and Contributors
 * Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
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
#include "vulkan_type_mapping.h"

namespace vkb
{
struct DriverVersion
{
	uint16_t major;
	uint16_t minor;
	uint16_t patch;
};

namespace core
{
/**
 * @brief A wrapper class for VkPhysicalDevice
 *
 * This class is responsible for handling gpu features, properties, and queue families for the device creation.
 */
template <vkb::BindingType bindingType>
class PhysicalDevice
{
  public:
	using Bool32Type              = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Bool32, VkBool32>::type;
	using FormatPropertiesType    = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::FormatProperties, VkFormatProperties>::type;
	using FormatType              = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Format, VkFormat>::type;
	using MemoryPropertyFlagsType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::MemoryPropertyFlags, VkMemoryPropertyFlags>::type;
	using PerformanceCounterDescriptionKHRType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::PerformanceCounterDescriptionKHR, VkPerformanceCounterDescriptionKHR>::type;
	using PerformanceCounterKHRType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::PerformanceCounterKHR, VkPerformanceCounterKHR>::type;
	using PhysicalDeviceFeaturesType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::PhysicalDeviceFeatures, VkPhysicalDeviceFeatures>::type;
	using PhysicalDeviceMemoryPropertiesType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::PhysicalDeviceMemoryProperties, VkPhysicalDeviceMemoryProperties>::type;
	using PhysicalDevicePropertiesType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::PhysicalDeviceProperties, VkPhysicalDeviceProperties>::type;
	using PhysicalDeviceType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::PhysicalDevice, VkPhysicalDevice>::type;
	using QueueFamilyPropertiesType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::QueueFamilyProperties, VkQueueFamilyProperties>::type;
	using QueryPoolPerformanceCreateInfoKHRType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::QueryPoolPerformanceCreateInfoKHR, VkQueryPoolPerformanceCreateInfoKHR>::type;
	using StructureTypeType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::StructureType, VkStructureType>::type;
	using SurfaceKHRType    = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::SurfaceKHR, VkSurfaceKHR>::type;

  public:
	PhysicalDevice(vkb::core::Instance<bindingType> &instance, PhysicalDeviceType physical_device);

	PhysicalDevice(PhysicalDevice const &)            = delete;
	PhysicalDevice(PhysicalDevice &&)                 = delete;
	PhysicalDevice &operator=(PhysicalDevice const &) = delete;
	PhysicalDevice &operator=(PhysicalDevice &&)      = delete;

	/**
	 * @brief Add an extension features struct to the structure chain used for device creation
	 *
	 *        To have the features enabled, this function must be called before the logical device
	 *        is created. To do this request sample specific features inside
	 *        VulkanSample::request_gpu_features(vkb::core::PhysicalDevice<bindingType> &gpu).
	 *
	 *        If the feature extension requires you to ask for certain features to be enabled, you can
	 *        modify the struct returned by this function, it will propagate the changes to the logical
	 *        device.
	 * @returns A reference to extension feature struct in the structure chain
	 */
	template <typename FeatureType>
	FeatureType &add_extension_features();

	std::pair<std::vector<PerformanceCounterKHRType>, std::vector<PerformanceCounterDescriptionKHRType>>
	    enumerate_queue_family_performance_query_counters(uint32_t queue_family_index) const;

	DriverVersion get_driver_version() const;

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
	 * @returns The extension feature struct
	 */
	template <typename T>
	T get_extension_features();

	PhysicalDeviceFeaturesType const             &get_features() const;
	FormatPropertiesType                          get_format_properties(FormatType format) const;
	PhysicalDeviceType                            get_handle() const;
	vkb::core::Instance<bindingType>             &get_instance() const;
	PhysicalDeviceMemoryPropertiesType const     &get_memory_properties() const;
	uint32_t                                      get_memory_type(uint32_t bits, MemoryPropertyFlagsType properties, Bool32Type *memory_type_found = nullptr) const;
	PhysicalDeviceFeaturesType                   &get_mutable_requested_features();
	PhysicalDevicePropertiesType const           &get_properties() const;
	uint32_t                                      get_queue_family_performance_query_passes(QueryPoolPerformanceCreateInfoKHRType const *perf_query_create_info) const;
	std::vector<QueueFamilyPropertiesType> const &get_queue_family_properties() const;
	PhysicalDeviceFeaturesType const             &get_requested_features() const;

	/**
	 * @brief Returns high priority graphics queue state.
	 * @return High priority state.
	 */
	bool has_high_priority_graphics_queue() const;

	bool       is_extension_supported(const std::string &extension) const;
	Bool32Type is_present_supported(SurfaceKHRType surface, uint32_t queue_family_index) const;

	/**
	 * @brief Request an optional features flag
	 *
	 *        Calls get_extension_features to get the support of the requested flag. If it's supported,
	 *        add_extension_features is called, otherwise a log message is generated.
	 *
	 * @returns true if the requested feature is supported, otherwise false
	 */
	template <typename Feature>
	Bool32Type request_optional_feature(Bool32Type Feature::*flag, std::string const &featureName, std::string const &flagName);

	/**
	 * @brief Request a required features flag
	 *
	 *        Calls get_extension_features to get the support of the requested flag. If it's supported,
	 *        add_extension_features is called, otherwise a runtime_error is thrown.
	 */
	template <typename Feature>
	void request_required_feature(Bool32Type Feature::*flag, std::string const &featureName, std::string const &flagName);

	/**
	 * @brief Sets whether or not the first graphics queue should have higher priority than other queues.
	 * Very specific feature which is used by async compute samples.
	 * @param enable If true, present queue will have prio 1.0 and other queues have prio 0.5.
	 * Default state is false, where all queues have 0.5 priority.
	 */
	void set_high_priority_graphics_queue_enable(bool enable);

  private:
	template <typename FeatureType>
	FeatureType &add_extension_features_impl();
	template <typename FeatureType>
	FeatureType get_extension_features_impl();
	uint32_t    get_memory_type_impl(uint32_t bits, vk::MemoryPropertyFlags properties, vk::Bool32 *memory_type_found = nullptr) const;
	template <typename FeatureType>
	void request_required_feature_impl(vk::Bool32 FeatureType::*flag, std::string const &featureName, std::string const &flagName);

  private:
	std::vector<vk::ExtensionProperties> device_extensions;        // The extensions that this GPU supports
	std::map<vk::StructureType, std::shared_ptr<void>>
	                                       extension_features;        // Holds the extension feature structures, we use a map to retain an order of requested structures
	vk::PhysicalDeviceFeatures             features;                  // The features that this GPU supports
	vk::PhysicalDevice                     handle;                    // Handle to the Vulkan physical device
	bool                                   high_priority_graphics_queue = {};
	vkb::core::InstanceCpp                &instance;                                          // Handle to the Vulkan instance
	void                                  *last_requested_extension_feature = nullptr;        // The extension feature pointer
	vk::PhysicalDeviceMemoryProperties     memory_properties;                                 // The GPU memory properties
	vk::PhysicalDeviceProperties           properties;                                        // The GPU properties
	std::vector<vk::QueueFamilyProperties> queue_family_properties;                           // The GPU queue family properties
	vk::PhysicalDeviceFeatures             requested_features;                                // The features that will be requested to be enabled in the logical device
};

using PhysicalDeviceC   = PhysicalDevice<vkb::BindingType::C>;
using PhysicalDeviceCpp = PhysicalDevice<vkb::BindingType::Cpp>;

#define REQUEST_OPTIONAL_FEATURE(gpu, Feature, flag) gpu.request_optional_feature<Feature>(&Feature::flag, #Feature, #flag)
#define REQUEST_REQUIRED_FEATURE(gpu, Feature, flag) gpu.request_required_feature<Feature>(&Feature::flag, #Feature, #flag)

template <vkb::BindingType bindingType>
inline PhysicalDevice<bindingType>::PhysicalDevice(vkb::core::Instance<bindingType> &instance, PhysicalDeviceType physical_device) :
    instance{instance}, handle{physical_device}
{
	features                = physical_device.getFeatures();
	properties              = physical_device.getProperties();
	memory_properties       = physical_device.getMemoryProperties();
	queue_family_properties = physical_device.getQueueFamilyProperties();
	device_extensions       = physical_device.enumerateDeviceExtensionProperties();

	LOGI("Found GPU: {}", properties.deviceName.data());

	// Display supported extensions
	if (device_extensions.size() > 0)
	{
		LOGD("Device supports the following extensions:");
		for (auto &extension : device_extensions)
		{
			LOGD("  \t{}", extension.extensionName.data());
		}
	}
}

template <vkb::BindingType bindingType>
template <typename FeatureType>
inline FeatureType &PhysicalDevice<bindingType>::add_extension_features()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return add_extension_features_impl<FeatureType>();
	}
	else
	{
		return static_cast<FeatureType &>(add_extension_features_impl<typename vkb::detail::HPPType<FeatureType>::Type>());
	}
}

template <vkb::BindingType bindingType>
template <typename FeatureType>
inline FeatureType &PhysicalDevice<bindingType>::add_extension_features_impl()
{
	// We cannot request extension features if the physical device properties 2 instance extension isn't enabled
	if (!instance.is_enabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
	{
		throw std::runtime_error("Couldn't request feature from device as " + std::string(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) +
		                         " isn't enabled!");
	}

	// Add an (empty) extension features into the map of extension features
	auto [it, added] = extension_features.insert({FeatureType::structureType, std::make_shared<FeatureType>()});
	if (added)
	{
		// if it was actually added, also add it to the structure chain
		if (last_requested_extension_feature)
		{
			static_cast<FeatureType *>(it->second.get())->pNext = last_requested_extension_feature;
		}
		last_requested_extension_feature = it->second.get();
	}

	return *static_cast<FeatureType *>(it->second.get());
}

template <vkb::BindingType bindingType>
inline std::pair<std::vector<typename PhysicalDevice<bindingType>::PerformanceCounterKHRType>,
                 std::vector<typename PhysicalDevice<bindingType>::PerformanceCounterDescriptionKHRType>>
    PhysicalDevice<bindingType>::enumerate_queue_family_performance_query_counters(uint32_t queue_family_index) const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return handle.enumerateQueueFamilyPerformanceQueryCountersKHR(queue_family_index);
	}
	else
	{
		auto [counters, descriptions] = handle.enumerateQueueFamilyPerformanceQueryCountersKHR(queue_family_index);
		return {std::vector<PerformanceCounterKHRType>(counters.begin(), counters.end()),
		        std::vector<PerformanceCounterDescriptionKHRType>(descriptions.begin(), descriptions.end())};
	}
}

template <vkb::BindingType bindingType>
inline DriverVersion PhysicalDevice<bindingType>::get_driver_version() const
{
	DriverVersion version;

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

template <vkb::BindingType bindingType>
inline void *PhysicalDevice<bindingType>::get_extension_feature_chain() const
{
	return last_requested_extension_feature;
}

template <vkb::BindingType bindingType>
template <typename FeatureType>
inline FeatureType PhysicalDevice<bindingType>::get_extension_features()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return get_extension_features_impl<FeatureType>();
	}
	else
	{
		return static_cast<FeatureType>(get_extension_features_impl<typename vkb::detail::HPPType<FeatureType>::Type>());
	}
}

template <vkb::BindingType bindingType>
template <typename FeatureType>
inline FeatureType PhysicalDevice<bindingType>::get_extension_features_impl()
{
	// We cannot request extension features if the physical device properties 2 instance extension isn't enabled
	if (!instance.is_enabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
	{
		throw std::runtime_error("Couldn't request feature from device as " + std::string(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) +
		                         " isn't enabled!");
	}

	// Get the extension feature
	return handle.getFeatures2KHR<vk::PhysicalDeviceFeatures2KHR, FeatureType>().template get<FeatureType>();
}

template <vkb::BindingType bindingType>
inline typename PhysicalDevice<bindingType>::PhysicalDeviceFeaturesType const &PhysicalDevice<bindingType>::get_features() const
{
	return features;
}

template <vkb::BindingType bindingType>
inline typename PhysicalDevice<bindingType>::FormatPropertiesType PhysicalDevice<bindingType>::get_format_properties(FormatType format) const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return handle.getFormatProperties(format);
	}
	else
	{
		return static_cast<FormatPropertiesType>(handle.getFormatProperties(static_cast<vk::Format>(format)));
	}
}

template <vkb::BindingType bindingType>
inline typename PhysicalDevice<bindingType>::PhysicalDeviceType PhysicalDevice<bindingType>::get_handle() const
{
	return handle;
}

template <vkb::BindingType bindingType>
inline vkb::core::Instance<bindingType> &PhysicalDevice<bindingType>::get_instance() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return instance;
	}
	else
	{
		return reinterpret_cast<vkb::core::InstanceC &>(instance);
	}
}

template <vkb::BindingType bindingType>
inline typename PhysicalDevice<bindingType>::PhysicalDeviceMemoryPropertiesType const &PhysicalDevice<bindingType>::get_memory_properties() const
{
	return memory_properties;
}

template <vkb::BindingType bindingType>
inline uint32_t PhysicalDevice<bindingType>::get_memory_type(uint32_t bits, MemoryPropertyFlagsType properties, Bool32Type *memory_type_found) const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return get_memory_type_impl(bits, properties, memory_type_found);
	}
	else
	{
		return get_memory_type_impl(bits, static_cast<vk::MemoryPropertyFlags>(properties), reinterpret_cast<vk::Bool32 *>(memory_type_found));
	}
}

template <vkb::BindingType bindingType>
inline uint32_t PhysicalDevice<bindingType>::get_memory_type_impl(uint32_t bits, vk::MemoryPropertyFlags properties, vk::Bool32 *memory_type_found) const
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

template <vkb::BindingType bindingType>
inline typename PhysicalDevice<bindingType>::PhysicalDeviceFeaturesType &PhysicalDevice<bindingType>::get_mutable_requested_features()
{
	return requested_features;
}

template <vkb::BindingType bindingType>
inline typename PhysicalDevice<bindingType>::PhysicalDevicePropertiesType const &PhysicalDevice<bindingType>::get_properties() const
{
	return properties;
}

template <vkb::BindingType bindingType>
uint32_t
    PhysicalDevice<bindingType>::get_queue_family_performance_query_passes(QueryPoolPerformanceCreateInfoKHRType const *perf_query_create_info) const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return handle.getQueueFamilyPerformanceQueryPassesKHR(perf_query_create_info);
	}
	else
	{
		return handle.getQueueFamilyPerformanceQueryPassesKHR(reinterpret_cast<vk::QueryPoolPerformanceCreateInfoKHR const &>(*perf_query_create_info));
	}
}

template <vkb::BindingType bindingType>
inline std::vector<typename PhysicalDevice<bindingType>::QueueFamilyPropertiesType> const &PhysicalDevice<bindingType>::get_queue_family_properties() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return queue_family_properties;
	}
	else
	{
		return reinterpret_cast<std::vector<QueueFamilyPropertiesType> const &>(queue_family_properties);
	}
}

template <vkb::BindingType bindingType>
inline typename PhysicalDevice<bindingType>::PhysicalDeviceFeaturesType const &PhysicalDevice<bindingType>::get_requested_features() const
{
	return requested_features;
}

template <vkb::BindingType bindingType>
inline bool PhysicalDevice<bindingType>::has_high_priority_graphics_queue() const
{
	return high_priority_graphics_queue;
}

template <vkb::BindingType bindingType>
inline bool PhysicalDevice<bindingType>::is_extension_supported(const std::string &requested_extension) const
{
	return std::ranges::find_if(device_extensions,
	                            [requested_extension](auto &device_extension) { return std::strcmp(device_extension.extensionName, requested_extension.c_str()) == 0; }) != device_extensions.end();
}

template <vkb::BindingType bindingType>
inline typename PhysicalDevice<bindingType>::Bool32Type PhysicalDevice<bindingType>::is_present_supported(SurfaceKHRType surface,
                                                                                                          uint32_t       queue_family_index) const
{
	return surface ? handle.getSurfaceSupportKHR(queue_family_index, surface) : false;
}

template <vkb::BindingType bindingType>
template <typename Feature>
inline typename PhysicalDevice<bindingType>::Bool32Type
    PhysicalDevice<bindingType>::request_optional_feature(Bool32Type Feature::*flag, std::string const &featureName, std::string const &flagName)
{
	Bool32Type supported = get_extension_features<Feature>().*flag;
	if (supported)
	{
		add_extension_features<Feature>().*flag = true;
	}
	else
	{
		LOGI("Requested optional feature <{}::{}> is not supported", featureName, flagName);
	}
	return supported;
}

template <vkb::BindingType bindingType>
template <typename Feature>
inline void
    PhysicalDevice<bindingType>::request_required_feature(Bool32Type Feature::*flag, std::string const &featureName, std::string const &flagName)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		request_required_feature_impl(flag, featureName, flagName);
	}
	else
	{
		request_required_feature_impl<typename vkb::detail::HPPType<Feature>::Type>(
		    reinterpret_cast<vk::Bool32 vkb::detail::HPPType<Feature>::Type::*>(flag), featureName, flagName);
	}
}

template <vkb::BindingType bindingType>
template <typename Feature>
inline void
    PhysicalDevice<bindingType>::request_required_feature_impl(vk::Bool32 Feature::*flag, std::string const &featureName, std::string const &flagName)
{
	if (get_extension_features_impl<Feature>().*flag)
	{
		add_extension_features_impl<Feature>().*flag = true;
	}
	else
	{
		throw std::runtime_error(std::string("Requested required feature <") + featureName + "::" + flagName + "> is not supported");
	}
}

template <vkb::BindingType bindingType>
inline void PhysicalDevice<bindingType>::set_high_priority_graphics_queue_enable(bool enable)
{
	high_priority_graphics_queue = enable;
}

}        // namespace core
}        // namespace vkb
