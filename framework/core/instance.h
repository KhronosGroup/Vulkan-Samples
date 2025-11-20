/* Copyright (c) 2018-2025, Arm Limited and Contributors
 * Copyright (c) 2022-2025, NVIDIA CORPORATION. All rights reserved.
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

#include "common/vk_common.h"
#include <ranges>
#include <vulkan/vulkan.hpp>

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
#	define USE_VALIDATION_LAYERS
#endif

#if defined(USE_VALIDATION_LAYERS) && \
    (defined(VKB_VALIDATION_LAYERS_GPU_ASSISTED) || defined(VKB_VALIDATION_LAYERS_BEST_PRACTICES) || defined(VKB_VALIDATION_LAYERS_SYNCHRONIZATION))
#	define USE_VALIDATION_LAYER_FEATURES
#endif

namespace vkb
{
namespace core
{
template <vkb::BindingType bindingType>
class PhysicalDevice;
using PhysicalDeviceC   = PhysicalDevice<vkb::BindingType::C>;
using PhysicalDeviceCpp = PhysicalDevice<vkb::BindingType::Cpp>;

/**
 * @brief A wrapper class for InstanceType
 *
 * This class is responsible for initializing the dispatcher, enumerating over all available extensions and validation layers
 * enabling them if they exist, setting up debug messaging and querying all the physical devices existing on the machine.
 */
template <vkb::BindingType bindingType>
class Instance
{
  public:
	using InstanceType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Instance, VkInstance>::type;
	using SurfaceType  = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::SurfaceKHR, VkSurfaceKHR>::type;

  public:
	/**
	 * @brief Can be set from the GPU selection plugin to explicitly select a GPU instead
	 */
	inline static uint32_t selected_gpu_index = ~0;

	/**
	 * @brief Initializes the connection to Vulkan
	 * @param application_name The name of the application
	 * @param requested_extensions The extensions requested to be enabled
	 * @param requested_layers The validation layers to be enabled
	 * @param requested_layer_settings The layer settings to be enabled
	 * @param api_version The Vulkan API version that the instance will be using
	 * @throws runtime_error if the required extensions and validation layers are not found
	 */
	Instance(std::string const                            &application_name,
	         std::unordered_map<char const *, bool> const &requested_extensions     = {},
	         std::unordered_map<char const *, bool> const &requested_layers         = {},
	         const std::vector<vk::LayerSettingEXT>       &requested_layer_settings = {},
	         uint32_t                                      api_version              = VK_API_VERSION_1_1);
	Instance(std::string const                            &application_name,
	         std::unordered_map<char const *, bool> const &requested_extensions,
	         std::unordered_map<char const *, bool> const &requested_layers,
	         std::vector<VkLayerSettingEXT> const         &requested_layer_settings,
	         uint32_t                                      api_version = VK_API_VERSION_1_1);

	/**
	 * @brief Queries the GPUs of a InstanceType that is already created
	 * @param instance A valid InstanceType
	 * @param externally_enabled_extensions List of extensions that have been enabled, used for following checks e.g. during device creation
	 * @param needsToInitializeDispatcher If the sample uses the C-bindings and some "non-standard" initialization, the dispatcher needs to be initialized
	 */
	Instance(vk::Instance instance, const std::vector<const char *> &externally_enabled_extensions = {}, bool needsToInitializeDispatcher = false);
	Instance(VkInstance instance, const std::vector<const char *> &externally_enabled_extensions = {});

	Instance(Instance const &) = delete;
	Instance(Instance &&)      = delete;

	~Instance();

	Instance &operator=(Instance const &) = delete;
	Instance &operator=(Instance &&)      = delete;

	const std::vector<const char *> &get_extensions();

	/**
	 * @brief Tries to find the first available discrete GPU
	 * @returns A valid physical device
	 */
	PhysicalDevice<bindingType> &get_first_gpu();

	InstanceType get_handle() const;

	/**
	 * @brief Tries to find the first available discrete GPU that can render to the given surface
	 * @param surface to test against
	 * @param headless_surface Is surface created with VK_EXT_headless_surface
	 * @returns A valid physical device
	 */
	PhysicalDevice<bindingType> &get_suitable_gpu(SurfaceType surface, bool headless_surface);

	/**
	 * @brief Checks if the given extension is enabled in the InstanceType
	 * @param extension An extension to check
	 */
	bool is_enabled(char const *extension) const;

  private:
	/**
	 * @brief Queries the instance for the physical devices on the machine
	 */
	void query_gpus();

  private:
	std::vector<const char *>                                  enabled_extensions;        // The enabled extensions
	std::vector<std::unique_ptr<vkb::core::PhysicalDeviceCpp>> gpus;                      // The physical devices found on the machine
	vk::Instance                                               handle;                    // The Vulkan instance

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	vk::DebugReportCallbackEXT debug_report_callback;        // The debug report callback
	vk::DebugUtilsMessengerEXT debug_utils_messenger;        // Debug utils messenger callback for VK_EXT_Debug_Utils
#endif
};

using InstanceC   = Instance<vkb::BindingType::C>;
using InstanceCpp = Instance<vkb::BindingType::Cpp>;
}        // namespace core
}        // namespace vkb

#include "core/physical_device.h"

namespace vkb
{
namespace core
{
namespace
{
#ifdef USE_VALIDATION_LAYERS
inline VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_utils_messenger_callback(vk::DebugUtilsMessageSeverityFlagBitsEXT      message_severity,
                                                                       vk::DebugUtilsMessageTypeFlagsEXT             message_type,
                                                                       vk::DebugUtilsMessengerCallbackDataEXT const *callback_data,
                                                                       void                                         *user_data)
{
	// Log debug message
	if (message_severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
	{
		LOGW("{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	else if (message_severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
	{
		LOGE("{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	return false;
}

inline VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(vk::DebugReportFlagsEXT flags,
                                                       vk::DebugReportObjectTypeEXT /*type*/,
                                                       uint64_t /*object*/,
                                                       size_t /*location*/,
                                                       int32_t /*message_code*/,
                                                       char const *layer_prefix,
                                                       char const *message,
                                                       void * /*user_data*/)
{
	if (flags & vk::DebugReportFlagBitsEXT::eError)
	{
		LOGE("{}: {}", layer_prefix, message);
	}
	else if (flags & vk::DebugReportFlagBitsEXT::eWarning)
	{
		LOGW("{}: {}", layer_prefix, message);
	}
	else if (flags & vk::DebugReportFlagBitsEXT::ePerformanceWarning)
	{
		LOGW("{}: {}", layer_prefix, message);
	}
	else
	{
		LOGI("{}: {}", layer_prefix, message);
	}
	return false;
}
#endif

inline bool enable_extension(char const                                 *requested_extension,
                             std::vector<vk::ExtensionProperties> const &available_extensions,
                             std::vector<char const *>                  &enabled_extensions)
{
	bool is_available = std::ranges::any_of(available_extensions,
	                                        [&requested_extension](auto const &available_extension) { return strcmp(requested_extension, available_extension.extensionName) == 0; });
	if (is_available)
	{
		bool is_already_enabled = std::ranges::any_of(
		    enabled_extensions, [&requested_extension](auto const &enabled_extension) { return strcmp(requested_extension, enabled_extension) == 0; });
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

inline bool enable_layer(char const *requested_layer, std::vector<vk::LayerProperties> const &available_layers, std::vector<char const *> &enabled_layers)
{
	bool is_available = std::ranges::any_of(
	    available_layers, [&requested_layer](auto const &available_layer) { return strcmp(requested_layer, available_layer.layerName) == 0; });
	if (is_available)
	{
		bool is_already_enabled =
		    std::ranges::any_of(enabled_layers, [&requested_layer](auto const &enabled_layer) { return strcmp(requested_layer, enabled_layer) == 0; });
		if (!is_already_enabled)
		{
			LOGI("Layer {} available, enabling it", requested_layer);
			enabled_layers.emplace_back(requested_layer);
		}
	}
	else
	{
		LOGI("Layer {} not available", requested_layer);
	}

	return is_available;
}

inline bool enable_layer_setting(vk::LayerSettingEXT const        &requested_layer_setting,
                                 std::vector<char const *> const  &enabled_layers,
                                 std::vector<vk::LayerSettingEXT> &enabled_layer_settings)
{
	// We are checking if the layer is available.
	// Vulkan does not provide a reflection API for layer settings. Layer settings are described in each layer JSON manifest.
	bool is_available = std::ranges::any_of(
	    enabled_layers, [&requested_layer_setting](auto const &available_layer) { return strcmp(available_layer, requested_layer_setting.pLayerName) == 0; });
#if defined(PLATFORM__MACOS)
	// On Apple platforms the MoltenVK layer is implicitly enabled and available, and cannot be explicitly added or checked via enabled_layers.
	is_available = is_available || strcmp(requested_layer_setting.pLayerName, "MoltenVK") == 0;
#endif

	if (!is_available)
	{
		LOGW("Layer: {} not found. Disabling layer setting: {}", requested_layer_setting.pLayerName, requested_layer_setting.pSettingName);
		return false;
	}

	bool is_already_enabled = std::ranges::any_of(enabled_layer_settings,
	                                              [&requested_layer_setting](VkLayerSettingEXT const &enabled_layer_setting) {
		                                              return (strcmp(requested_layer_setting.pLayerName, enabled_layer_setting.pLayerName) == 0) &&
		                                                     (strcmp(requested_layer_setting.pSettingName, enabled_layer_setting.pSettingName) == 0);
	                                              });

	if (is_already_enabled)
	{
		LOGW("Ignoring duplicated layer setting {} in layer {}.", requested_layer_setting.pSettingName, requested_layer_setting.pLayerName);
		return false;
	}

	LOGI("Enabling layer setting {} in layer {}.", requested_layer_setting.pSettingName, requested_layer_setting.pLayerName);
	enabled_layer_settings.push_back(requested_layer_setting);
	return true;
}

inline bool enable_layer_setting(VkLayerSettingEXT const          &requested_layer_setting,
                                 std::vector<char const *> const  &enabled_layers,
                                 std::vector<vk::LayerSettingEXT> &enabled_layer_settings)
{
	return enable_layer_setting(reinterpret_cast<vk::LayerSettingEXT const &>(requested_layer_setting), enabled_layers, enabled_layer_settings);
}

inline bool validate_layers(std::vector<char const *> const &required, std::vector<vk::LayerProperties> const &available)
{
	for (auto layer : required)
	{
		bool found = false;
		for (auto &available_layer : available)
		{
			if (strcmp(available_layer.layerName, layer) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			LOGE("Validation Layer {} not found", layer);
			return false;
		}
	}

	return true;
}
}        // namespace

template <vkb::BindingType bindingType>
inline Instance<bindingType>::Instance(std::string const                            &application_name,
                                       std::unordered_map<char const *, bool> const &requested_extensions,
                                       std::unordered_map<char const *, bool> const &requested_layers,
                                       std::vector<vk::LayerSettingEXT> const       &requested_layer_settings,
                                       uint32_t                                      api_version)
{
	std::vector<vk::ExtensionProperties> available_instance_extensions = vk::enumerateInstanceExtensionProperties();

#ifdef USE_VALIDATION_LAYERS
	// Check if VK_EXT_debug_utils is supported, which supersedes VK_EXT_Debug_Report
	const bool has_debug_utils  = enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, available_instance_extensions, enabled_extensions);
	bool       has_debug_report = false;

	if (!has_debug_utils)
	{
		has_debug_report = enable_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, available_instance_extensions, enabled_extensions);
		if (!has_debug_report)
		{
			LOGW("Neither of {} or {} are available; disabling debug reporting", VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}
	}
#endif

#if (defined(VKB_ENABLE_PORTABILITY))
	enable_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, available_instance_extensions, enabled_extensions);
	bool portability_enumeration_available = enable_extension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME, available_instance_extensions, enabled_extensions);
#endif

#ifdef USE_VALIDATION_LAYERS
	const char *validation_layer_name = "VK_LAYER_KHRONOS_validation";
#	ifdef USE_VALIDATION_LAYER_FEATURES
	bool validation_features = false;
	{
		std::vector<vk::ExtensionProperties> available_layer_instance_extensions = vk::enumerateInstanceExtensionProperties(std::string(validation_layer_name));
		validation_features                                                      = enable_extension(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME, available_layer_instance_extensions, enabled_extensions);
	}
#	endif        // USE_VALIDATION_LAYER_FEATURES
#endif            // USE_VALIDATION_LAYERS

	// Specific surface extensions are obtained from  Window::get_required_surface_extensions
	// They are already added to requested_extensions by VulkanSample::prepare

	// Even for a headless surface a swapchain is still required
	enable_extension(VK_KHR_SURFACE_EXTENSION_NAME, available_instance_extensions, enabled_extensions);

	// VK_KHR_get_physical_device_properties2 is a prerequisite of VK_KHR_performance_query
	// which will be used for stats gathering where available.
	enable_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, available_instance_extensions, enabled_extensions);

	for (auto requested_extension : requested_extensions)
	{
		auto const &extension_name        = requested_extension.first;
		auto        extension_is_optional = requested_extension.second;
		if (!enable_extension(extension_name, available_instance_extensions, enabled_extensions))
		{
			if (extension_is_optional)
			{
				LOGW("Optional instance extension {} not available, some features may be disabled", extension_name);
			}
			else
			{
				LOGE("Required instance extension {} not available, cannot run", extension_name);
				throw std::runtime_error("Required instance extensions are missing.");
			}
		}
	}

	std::vector<vk::LayerProperties> supported_layers = vk::enumerateInstanceLayerProperties();

	std::vector<char const *> enabled_layers;

	auto layer_error = false;
	for (auto const &requested_layer : requested_layers)
	{
		auto const &layer_name        = requested_layer.first;
		auto        layer_is_optional = requested_layer.second;
		if (!enable_layer(layer_name, supported_layers, enabled_layers))
		{
			if (layer_is_optional)
			{
				LOGW("Optional layer {} not available, some features may be disabled", layer_name);
			}
			else
			{
				LOGE("Required layer {} not available, cannot run", layer_name);
				throw std::runtime_error("Required layers are missing.");
			}
		}
	}

#ifdef USE_VALIDATION_LAYERS
	// NOTE: It's important to have the validation layer as the last one here!!!!
	//			 Otherwise, device creation fails !?!
	enable_layer(validation_layer_name, supported_layers, enabled_layers);
#endif

	vk::ApplicationInfo app_info{.pApplicationName = application_name.c_str(), .pEngineName = "Vulkan Samples", .apiVersion = api_version};

	vk::InstanceCreateInfo instance_info{.pApplicationInfo        = &app_info,
	                                     .enabledLayerCount       = static_cast<uint32_t>(enabled_layers.size()),
	                                     .ppEnabledLayerNames     = enabled_layers.data(),
	                                     .enabledExtensionCount   = static_cast<uint32_t>(enabled_extensions.size()),
	                                     .ppEnabledExtensionNames = enabled_extensions.data()};

	std::vector<vk::LayerSettingEXT> enabled_layer_settings;

	for (auto const &layer_setting : requested_layer_settings)
	{
		enable_layer_setting(layer_setting, enabled_layers, enabled_layer_settings);
	}

#ifdef USE_VALIDATION_LAYERS
	vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create_info;
	vk::DebugReportCallbackCreateInfoEXT debug_report_create_info;
	if (has_debug_utils)
	{
		debug_utils_create_info = vk::DebugUtilsMessengerCreateInfoEXT{
		    .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
		    .messageType     = vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
		    .pfnUserCallback = debug_utils_messenger_callback};

		instance_info.pNext = &debug_utils_create_info;
	}
	else if (has_debug_report)
	{
		debug_report_create_info = {.flags =
		                                vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::ePerformanceWarning,
		                            .pfnCallback = debug_callback};

		instance_info.pNext = &debug_report_create_info;
	}
#endif

#if (defined(VKB_ENABLE_PORTABILITY))
	if (portability_enumeration_available)
	{
		instance_info.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
	}
#endif

	// Some of the specialized layers need to be enabled explicitly
	// The validation layer does not need to be enabled in code and it can also be configured using the vulkan configurator.
#ifdef USE_VALIDATION_LAYER_FEATURES

#	if defined(VKB_VALIDATION_LAYERS_GPU_ASSISTED)
	const vk::Bool32 setting_validate_gpuav = true;
	if (validation_features)
	{
		enable_layer_setting(vk::LayerSettingEXT(validation_layer_name, "gpuav_enable", vk::LayerSettingTypeEXT::eBool32, 1, &setting_validate_gpuav), enabled_layers, enabled_layer_settings);
	}
#	endif

#	if defined(VKB_VALIDATION_LAYERS_BEST_PRACTICES)
	const vk::Bool32 setting_validate_best_practices        = true;
	const vk::Bool32 setting_validate_best_practices_arm    = true;
	const vk::Bool32 setting_validate_best_practices_amd    = true;
	const vk::Bool32 setting_validate_best_practices_img    = true;
	const vk::Bool32 setting_validate_best_practices_nvidia = true;
	if (validation_features)
	{
		enable_layer_setting(
		    vk::LayerSettingEXT(validation_layer_name, "validate_best_practices", vk::LayerSettingTypeEXT::eBool32, 1, &setting_validate_best_practices),
		    enabled_layers,
		    enabled_layer_settings);
		enable_layer_setting(
		    vk::LayerSettingEXT(validation_layer_name, "validate_best_practices_arm", vk::LayerSettingTypeEXT::eBool32, 1, &setting_validate_best_practices_arm),
		    enabled_layers,
		    enabled_layer_settings);
		enable_layer_setting(
		    vk::LayerSettingEXT(validation_layer_name, "validate_best_practices_amd", vk::LayerSettingTypeEXT::eBool32, 1, &setting_validate_best_practices_amd),
		    enabled_layers,
		    enabled_layer_settings);
		enable_layer_setting(
		    vk::LayerSettingEXT(validation_layer_name, "validate_best_practices_img", vk::LayerSettingTypeEXT::eBool32, 1, &setting_validate_best_practices_img),
		    enabled_layers,
		    enabled_layer_settings);
		enable_layer_setting(
		    vk::LayerSettingEXT(
		        validation_layer_name, "validate_best_practices_nvidia", vk::LayerSettingTypeEXT::eBool32, 1, &setting_validate_best_practices_nvidia),
		    enabled_layers,
		    enabled_layer_settings);
	}
#	endif

#	if defined(VKB_VALIDATION_LAYERS_SYNCHRONIZATION)
	const vk::Bool32 setting_validate_sync            = true;
	const vk::Bool32 setting_validate_sync_heuristics = true;
	if (validation_features)
	{
		enable_layer_setting(vk::LayerSettingEXT(validation_layer_name, "validate_sync", vk::LayerSettingTypeEXT::eBool32, 1, &setting_validate_sync),
		                     enabled_layers,
		                     enabled_layer_settings);
		enable_layer_setting(
		    vk::LayerSettingEXT(
		        validation_layer_name, "syncval_shader_accesses_heuristic", vk::LayerSettingTypeEXT::eBool32, 1, &setting_validate_sync_heuristics),
		    enabled_layers,
		    enabled_layer_settings);
	}
#	endif
#endif

	vk::LayerSettingsCreateInfoEXT layerSettingsCreateInfo;

	// If layer settings are defined, then activate the sample's required layer settings during instance creation
	if (enabled_layer_settings.size() > 0)
	{
		layerSettingsCreateInfo.settingCount = static_cast<uint32_t>(enabled_layer_settings.size());
		layerSettingsCreateInfo.pSettings    = enabled_layer_settings.data();
		layerSettingsCreateInfo.pNext        = instance_info.pNext;
		instance_info.pNext                  = &layerSettingsCreateInfo;
	}

	// Create the Vulkan instance
	handle = vk::createInstance(instance_info);

	// initialize the Vulkan-Hpp default dispatcher on the instance
	VULKAN_HPP_DEFAULT_DISPATCHER.init(handle);

	// Need to load volk for all the not-yet Vulkan-Hpp calls
	volkLoadInstance(handle);

#ifdef USE_VALIDATION_LAYERS
	if (has_debug_utils)
	{
		debug_utils_messenger = handle.createDebugUtilsMessengerEXT(debug_utils_create_info);
	}
	else if (has_debug_report)
	{
		debug_report_callback = handle.createDebugReportCallbackEXT(debug_report_create_info);
	}
#endif

	query_gpus();
}

template <vkb::BindingType bindingType>
inline Instance<bindingType>::Instance(std::string const                            &application_name,
                                       std::unordered_map<char const *, bool> const &requested_extensions,
                                       std::unordered_map<char const *, bool> const &requested_layers,
                                       std::vector<VkLayerSettingEXT> const         &requested_layer_settings,
                                       uint32_t                                      api_version) :
    Instance<bindingType>(application_name,
                          requested_extensions,
                          requested_layers,
                          reinterpret_cast<std::vector<vk::LayerSettingEXT> const &>(requested_layer_settings),
                          api_version)
{}

template <vkb::BindingType bindingType>
inline Instance<bindingType>::Instance(vk::Instance instance, const std::vector<const char *> &externally_enabled_extensions, bool needsToInitializeDispatcher) :
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

	if (handle)
	{
		query_gpus();
	}
	else
	{
		throw std::runtime_error("Instance not valid");
	}
}

template <vkb::BindingType bindingType>
inline Instance<bindingType>::Instance(VkInstance instance, const std::vector<const char *> &externally_enabled_extensions) :
    Instance<bindingType>(static_cast<vk::Instance>(instance), externally_enabled_extensions, true)
{}

template <vkb::BindingType bindingType>
inline Instance<bindingType>::~Instance()
{
#ifdef USE_VALIDATION_LAYERS
	if (debug_utils_messenger)
	{
		handle.destroyDebugUtilsMessengerEXT(debug_utils_messenger);
	}
	if (debug_report_callback)
	{
		handle.destroyDebugReportCallbackEXT(debug_report_callback);
	}
#endif

	if (handle)
	{
		handle.destroy();
	}
}

#if defined(USE_VALIDATION_LAYERS)
#	undef USE_VALIDATION_LAYERS
#endif

#if defined(USE_VALIDATION_LAYER_FEATURES)
#	undef USE_VALIDATION_LAYER_FEATURES
#endif

template <vkb::BindingType bindingType>
inline PhysicalDevice<bindingType> &Instance<bindingType>::get_first_gpu()
{
	assert(!gpus.empty() && "No physical devices were found on the system.");

	// Find a discrete GPU
	auto gpuIt = std::ranges::find_if(gpus, [](auto const &gpu) { return gpu->get_properties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu; });
	if (gpuIt == gpus.end())
	{
		LOGW("Couldn't find a discrete physical device, picking default GPU");
		gpuIt = gpus.begin();
	}
	if constexpr (bindingType == BindingType::Cpp)
	{
		return **gpuIt;
	}
	else
	{
		return reinterpret_cast<vkb::core::PhysicalDeviceC &>(**gpuIt);
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
	return handle;
}

template <vkb::BindingType bindingType>
inline PhysicalDevice<bindingType> &Instance<bindingType>::get_suitable_gpu(SurfaceType surface, bool headless_surface)
{
	assert(!gpus.empty() && "No physical devices were found on the system.");

	// A GPU can be explicitly selected via the command line (see plugins/gpu_selection.cpp), this overrides the below GPU selection algorithm
	auto gpuIt = gpus.begin();
	if (selected_gpu_index != ~0)
	{
		LOGI("Explicitly selecting GPU {}", selected_gpu_index);
		if (selected_gpu_index > gpus.size() - 1)
		{
			throw std::runtime_error("Selected GPU index is not within no. of available GPUs");
		}
		gpuIt = gpus.begin() + selected_gpu_index;
	}
	else
	{
		if (headless_surface)
		{
			LOGW("Using headless surface with multiple GPUs. Considered explicitly selecting the target GPU.")
		}

		// Find a discrete GPU
#if 0
		gpuIt = std::ranges::find_if(gpus,
		                             [&surface](auto const &gpu) {
			                             return (gpu->get_properties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) &&
			                                    std::ranges::any_of(std::views::iota(size_t(0), gpu->get_queue_family_properties().size()),
			                                                        [&gpu, &surface](auto const &queue_idx) {
				                                                        return gpu->get_handle().getSurfaceSupportKHR(static_cast<uint32_t>(queue_idx), surface);
			                                                        });
		                             });
#else
		gpuIt = std::ranges::find_if(gpus,
		                             [&surface](auto const &gpu) {
			                             auto gpu_supports_surface = [&gpu, &surface]() {
				                             for (uint32_t queue_idx = 0; queue_idx < gpu->get_queue_family_properties().size(); ++queue_idx)
				                             {
					                             if (gpu->get_handle().getSurfaceSupportKHR(queue_idx, surface))
					                             {
						                             return true;
					                             }
				                             }
				                             return false;
			                             };
			                             return (gpu->get_properties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) && gpu_supports_surface();
		                             });
#endif
		if (gpuIt == gpus.end())
		{
			LOGW("Couldn't find a discrete physical device that supports the surface, picking default GPU");
			gpuIt = gpus.begin();
		}
	}

	if constexpr (bindingType == BindingType::Cpp)
	{
		return **gpuIt;
	}
	else
	{
		return reinterpret_cast<vkb::core::PhysicalDeviceC &>(**gpuIt);
	}
}

template <vkb::BindingType bindingType>
inline bool Instance<bindingType>::is_enabled(char const *extension) const
{
	return std::ranges::find_if(enabled_extensions, [extension](char const *enabled_extension) { return strcmp(extension, enabled_extension) == 0; }) !=
	       enabled_extensions.end();
}

template <vkb::BindingType bindingType>
inline void Instance<bindingType>::query_gpus()
{
	// Querying valid physical devices on the machine
	std::vector<vk::PhysicalDevice> physical_devices = handle.enumeratePhysicalDevices();
	if (physical_devices.empty())
	{
		throw std::runtime_error("Couldn't find a physical device that supports Vulkan.");
	}

	// Create gpus wrapper objects from the vk::PhysicalDevice's
	for (auto &physical_device : physical_devices)
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			gpus.push_back(std::make_unique<vkb::core::PhysicalDeviceCpp>(*this, physical_device));
		}
		else
		{
			gpus.push_back(std::make_unique<vkb::core::PhysicalDeviceCpp>(*reinterpret_cast<vkb::core::InstanceCpp *>(this), physical_device));
		}
	}
}

template <vkb::BindingType bindingType>
inline std::vector<char const *> const &Instance<bindingType>::get_extensions()
{
	return enabled_extensions;
}
}        // namespace core
}        // namespace vkb
