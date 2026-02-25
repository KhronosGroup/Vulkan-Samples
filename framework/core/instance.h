/* Copyright (c) 2018-2026, Arm Limited and Contributors
 * Copyright (c) 2022-2026, NVIDIA CORPORATION. All rights reserved.
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

#include "common/helpers.h"
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace core
{
/**
 * @brief A wrapper class for InstanceType
 *
 * This class is responsible for checking the API version, checking for required and optional extensions and layers, creating the Vulkan
 * instance and initializing the default dispatcher.
 */
template <vkb::BindingType bindingType>
class Instance
{
  public:
	using InstanceCreateFlagsType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::InstanceCreateFlags, VkInstanceCreateFlags>::type;
	using InstanceType            = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Instance, VkInstance>::type;

  public:
	/**
	 * @brief Initializes the connection to Vulkan
	 * @param application_name The name of the application
	 * @param api_version The Vulkan API version that the instance will be using
	 * @param requested_layers The requested layers to be enabled
	 * @param requested_extensions The requested extensions to be enabled
	 * @param get_pNext A function pointer returning the pNext pointer for the InstanceCreateInfo
	 * @param get_create_flags A function pointer returning the InstanceCreateFlags for the InstanceCreateInfo
	 * @throws runtime_error if a required layer or extension is not available
	 */
	Instance(
	    std::string const                                                                                     &application_name,
	    uint32_t                                                                                               api_version          = VK_API_VERSION_1_1,
	    std::unordered_map<std::string, vkb::RequestMode> const                                               &requested_layers     = {},
	    std::unordered_map<std::string, vkb::RequestMode> const                                               &requested_extensions = {},
	    std::function<void const *(std::vector<std::string> const &, std::vector<std::string> const &)> const &get_pNext =
	        [](std::vector<std::string> const &, std::vector<std::string> const &) { return nullptr; },
	    std::function<InstanceCreateFlagsType(std::vector<std::string> const &)> const &get_create_flags =
	        [](std::vector<std::string> const &) {
		        if constexpr (bindingType == vkb::BindingType::Cpp)
		        {
			        return vk::InstanceCreateFlags{};
		        }
		        else
		        {
			        return 0;
		        }
	        });

	Instance(vk::Instance instance, std::vector<char const *> const &externally_enabled_extensions = {}, bool needsToInitializeDispatcher = false);
	Instance(VkInstance instance, std::vector<char const *> const &externally_enabled_extensions = {});

	Instance(Instance const &) = delete;
	Instance(Instance &&)      = delete;

	~Instance();

	Instance &operator=(Instance const &) = delete;
	Instance &operator=(Instance &&)      = delete;

	std::vector<std::string> const &get_enabled_extensions();

	InstanceType get_handle() const;

	/**
	 * @brief Checks if the given extension is enabled in the InstanceType
	 * @param extension An extension to check
	 */
	bool is_extension_enabled(char const *extension) const;

  private:
	std::vector<std::string> enabled_extensions;        // The enabled extensions
	vk::Instance             handle;                    // The Vulkan instance
};

using InstanceC   = Instance<vkb::BindingType::C>;
using InstanceCpp = Instance<vkb::BindingType::Cpp>;

namespace
{
inline bool enable_extension(std::string const                          &requested_extension,
                             std::vector<vk::ExtensionProperties> const &available_extensions,
                             std::vector<std::string>                   &enabled_extensions)
{
	bool is_available = std::ranges::any_of(
	    available_extensions, [&requested_extension](auto const &available_extension) { return requested_extension == available_extension.extensionName; });
	if (is_available)
	{
		bool is_already_enabled =
		    std::ranges::any_of(enabled_extensions, [&requested_extension](auto const &enabled_extension) { return requested_extension == enabled_extension; });
		if (!is_already_enabled)
		{
			LOGI("Extension {} available, enabling it", requested_extension);
			enabled_extensions.emplace_back(requested_extension);
		}
	}
	else
	{
		LOGI("Extension {} not available", requested_extension);
	}

	return is_available;
}

inline bool
    enable_layer(std::string const &requested_layer, std::vector<vk::LayerProperties> const &available_layers, std::vector<std::string> &enabled_layers)
{
	bool is_available =
	    std::ranges::any_of(available_layers, [&requested_layer](auto const &available_layer) { return requested_layer == available_layer.layerName; });
	if (is_available)
	{
		bool is_already_enabled =
		    std::ranges::any_of(enabled_layers, [&requested_layer](auto const &enabled_layer) { return requested_layer == enabled_layer; });
		if (!is_already_enabled)
		{
			LOGI("Layer {} available, enabling it", requested_layer);
			enabled_layers.emplace_back(requested_layer.c_str());
		}
	}
	else
	{
		LOGI("Layer {} not available", requested_layer);
	}

	return is_available;
}
}        // namespace

template <vkb::BindingType bindingType>
inline Instance<bindingType>::Instance(std::string const                                                                                     &application_name,
                                       uint32_t                                                                                               api_version,
                                       std::unordered_map<std::string, vkb::RequestMode> const                                               &requested_layers,
                                       std::unordered_map<std::string, vkb::RequestMode> const                                               &requested_extensions,
                                       std::function<void const *(std::vector<std::string> const &, std::vector<std::string> const &)> const &get_pNext,
                                       std::function<InstanceCreateFlagsType(std::vector<std::string> const &)> const                        &get_create_flags)
{
	// check API version
	LOGI("Requesting Vulkan API version {}.{}", VK_VERSION_MAJOR(api_version), VK_VERSION_MINOR(api_version));
	if (api_version < VK_API_VERSION_1_1)
	{
		LOGE("Vulkan API version {}.{} is requested but version 1.1 or higher is required.", VK_VERSION_MAJOR(api_version), VK_VERSION_MINOR(api_version));
		throw std::runtime_error("Requested Vulkan API version is too low.");
	}
	uint32_t instance_api_version = vk::enumerateInstanceVersion();
	LOGI("Vulkan instance supports API version {}.{}", VK_VERSION_MAJOR(instance_api_version), VK_VERSION_MINOR(instance_api_version));
	if (instance_api_version < api_version)
	{
		LOGE("Vulkan API version {}.{} is requested but only version {}.{} is supported.",
		     VK_VERSION_MAJOR(api_version),
		     VK_VERSION_MINOR(api_version),
		     VK_VERSION_MAJOR(instance_api_version),
		     VK_VERSION_MINOR(instance_api_version));
		throw std::runtime_error("Requested Vulkan API version is too high.");
	}

	// Check for optional and required layers
	std::vector<vk::LayerProperties> available_layers = vk::enumerateInstanceLayerProperties();
	std::vector<std::string>         enabled_layers;
	for (auto const &requested_layer : requested_layers)
	{
		if (!enable_layer(requested_layer.first, available_layers, enabled_layers))
		{
			if (requested_layer.second == vkb::RequestMode::Optional)
			{
				LOGW("Optional layer {} not available, some features may be disabled", requested_layer.first);
			}
			else
			{
				LOGE("Required layer {} not available, cannot run", requested_layer.first);
				throw std::runtime_error("Required layers are missing.");
			}
		}
	}
	std::vector<char const *> enabled_layers_cstr;
	for (auto &layer : enabled_layers)
	{
		enabled_layers_cstr.push_back(layer.c_str());
	}

	// Check for optional and required extensions
	std::vector<vk::ExtensionProperties> available_extensions = vk::enumerateInstanceExtensionProperties();

	if (contains(enabled_layers, "VK_LAYER_KHRONOS_validation"))
	{
		std::string const                    validation_layer_name               = "VK_LAYER_KHRONOS_validation";
		std::vector<vk::ExtensionProperties> available_layer_instance_extensions = vk::enumerateInstanceExtensionProperties(validation_layer_name);
		available_extensions.insert(available_extensions.end(),
		                            available_layer_instance_extensions.begin(),
		                            available_layer_instance_extensions.end());
	}

	for (auto const &requested_extension : requested_extensions)
	{
		if (!enable_extension(requested_extension.first, available_extensions, enabled_extensions))
		{
			if (requested_extension.second == vkb::RequestMode::Optional)
			{
				LOGW("Optional instance extension {} not available, some features may be disabled", requested_extension.first);
			}
			else
			{
				LOGE("Required instance extension {} not available, cannot run", requested_extension.first);
				throw std::runtime_error("Required instance extensions are missing.");
			}
		}
	}
	std::vector<char const *> enabled_extensions_cstr;
	for (auto &extension : enabled_extensions)
	{
		enabled_extensions_cstr.push_back(extension.c_str());
	}

	vk::ApplicationInfo app_info{.pApplicationName = application_name.c_str(), .pEngineName = "Vulkan Samples", .apiVersion = api_version};

	vk::InstanceCreateInfo create_info{.pNext                   = get_pNext(enabled_layers, enabled_extensions),
	                                   .flags                   = static_cast<vk::InstanceCreateFlags>(get_create_flags(enabled_extensions)),
	                                   .pApplicationInfo        = &app_info,
	                                   .enabledLayerCount       = static_cast<uint32_t>(enabled_layers_cstr.size()),
	                                   .ppEnabledLayerNames     = enabled_layers_cstr.data(),
	                                   .enabledExtensionCount   = static_cast<uint32_t>(enabled_extensions_cstr.size()),
	                                   .ppEnabledExtensionNames = enabled_extensions_cstr.data()};

	// Create the Vulkan instance
	handle = vk::createInstance(create_info);

	// initialize the Vulkan-Hpp default dispatcher on the instance
	VULKAN_HPP_DEFAULT_DISPATCHER.init(handle);

	// Need to load volk for all the not-yet Vulkan-Hpp calls
	volkLoadInstance(handle);
}

template <vkb::BindingType bindingType>
inline Instance<bindingType>::Instance(vk::Instance instance, std::vector<char const *> const &externally_enabled_extensions, bool needsToInitializeDispatcher) :
    handle{instance}
{
	if (needsToInitializeDispatcher)
	{
#if defined(_HPP_VULKAN_LIBRARY)
		static vk::detail::DynamicLoader dl(_HPP_VULKAN_LIBRARY);
#else
		static vk::detail::DynamicLoader dl;
#endif
		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
		VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
		VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
	}

	// Some parts of the framework will check for certain extensions to be enabled
	// To make those work we need to copy over externally enabled extensions into this class
	for (auto extension : externally_enabled_extensions)
	{
		enabled_extensions.push_back(extension);
	}
}

template <vkb::BindingType bindingType>
inline Instance<bindingType>::Instance(VkInstance instance, std::vector<char const *> const &externally_enabled_extensions) :
    Instance<bindingType>(static_cast<vk::Instance>(instance), externally_enabled_extensions, true)
{}

template <vkb::BindingType bindingType>
inline Instance<bindingType>::~Instance()
{
	if (handle)
	{
		handle.destroy();
	}
}

template <vkb::BindingType bindingType>
inline typename Instance<bindingType>::InstanceType Instance<bindingType>::get_handle() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return handle;
	}
	else
	{
		return static_cast<VkInstance>(handle);
	}
}

template <vkb::BindingType bindingType>
inline bool Instance<bindingType>::is_extension_enabled(char const *extension) const
{
	return std::ranges::any_of(enabled_extensions, [extension](std::string const &enabled_extension) { return enabled_extension == extension; });
}

template <vkb::BindingType bindingType>
inline std::vector<std::string> const &Instance<bindingType>::get_enabled_extensions()
{
	return enabled_extensions;
}
}        // namespace core
}        // namespace vkb
