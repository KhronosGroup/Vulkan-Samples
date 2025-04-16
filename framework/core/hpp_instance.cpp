/* Copyright (c) 2022-2025, NVIDIA CORPORATION. All rights reserved.
 * Copyright (c) 2024-2025, Arm Limited and Contributors
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

#include <core/hpp_instance.h>

#include <core/hpp_physical_device.h>
#include <core/util/logging.hpp>
#include <volk.h>

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
#	define USE_VALIDATION_LAYERS
#endif

#if defined(USE_VALIDATION_LAYERS) && (defined(VKB_VALIDATION_LAYERS_GPU_ASSISTED) || defined(VKB_VALIDATION_LAYERS_BEST_PRACTICES) || defined(VKB_VALIDATION_LAYERS_SYNCHRONIZATION))
#	define USE_VALIDATION_LAYER_FEATURES
#endif

namespace vkb
{
namespace
{
#ifdef USE_VALIDATION_LAYERS
VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_utils_messenger_callback(vk::DebugUtilsMessageSeverityFlagBitsEXT      message_severity,
                                                                vk::DebugUtilsMessageTypeFlagsEXT             message_type,
                                                                const vk::DebugUtilsMessengerCallbackDataEXT *callback_data,
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
	return VK_FALSE;
}

static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(vk::DebugReportFlagsEXT flags,
                                                       vk::DebugReportObjectTypeEXT /*type*/,
                                                       uint64_t /*object*/,
                                                       size_t /*location*/,
                                                       int32_t /*message_code*/,
                                                       const char *layer_prefix,
                                                       const char *message,
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
	return VK_FALSE;
}
#endif

bool validate_layers(const std::vector<const char *>        &required,
                     const std::vector<vk::LayerProperties> &available)
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

namespace core
{
Optional<uint32_t> HPPInstance::selected_gpu_index;

namespace
{
bool enable_extension(const char                                 *requested_extension,
                      const std::vector<vk::ExtensionProperties> &available_extensions,
                      std::vector<const char *>                  &enabled_extensions)
{
	bool is_available =
	    std::ranges::any_of(available_extensions,
	                        [&requested_extension](auto const &available_extension) { return strcmp(requested_extension, available_extension.extensionName) == 0; });
	if (is_available)
	{
		bool is_already_enabled =
		    std::ranges::any_of(enabled_extensions,
		                        [&requested_extension](auto const &enabled_extension) { return strcmp(requested_extension, enabled_extension) == 0; });
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

bool enable_layer(const char                             *requested_layer,
                  const std::vector<vk::LayerProperties> &available_layers,
                  std::vector<const char *>              &enabled_layers)
{
	bool is_available =
	    std::ranges::any_of(available_layers,
	                        [&requested_layer](auto const &available_layer) { return strcmp(requested_layer, available_layer.layerName) == 0; });
	if (is_available)
	{
		bool is_already_enabled =
		    std::ranges::any_of(enabled_layers,
		                        [&requested_layer](auto const &enabled_layer) { return strcmp(requested_layer, enabled_layer) == 0; });
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
}        // namespace

HPPInstance::HPPInstance(const std::string                            &application_name,
                         const std::unordered_map<const char *, bool> &requested_extensions,
                         const std::unordered_map<const char *, bool> &requested_layers,
                         const std::vector<vk::LayerSettingEXT>       &required_layer_settings,
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

#ifdef USE_VALIDATION_LAYER_FEATURES
	bool validation_features = false;
	{
		std::vector<vk::ExtensionProperties> available_layer_instance_extensions = vk::enumerateInstanceExtensionProperties(std::string("VK_LAYER_KHRONOS_validation"));

		enable_extension(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME, available_layer_instance_extensions, enabled_extensions);
	}
#endif

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

	std::vector<const char *> enabled_layers;

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
	enable_layer("VK_LAYER_KHRONOS_validation", supported_layers, enabled_layers);
#endif

	vk::ApplicationInfo app_info{.pApplicationName = application_name.c_str(), .pEngineName = "Vulkan Samples", .apiVersion = api_version};

	vk::InstanceCreateInfo instance_info{.pApplicationInfo        = &app_info,
	                                     .enabledLayerCount       = static_cast<uint32_t>(enabled_layers.size()),
	                                     .ppEnabledLayerNames     = enabled_layers.data(),
	                                     .enabledExtensionCount   = static_cast<uint32_t>(enabled_extensions.size()),
	                                     .ppEnabledExtensionNames = enabled_extensions.data()};

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

#ifdef USE_VALIDATION_LAYER_FEATURES
	vk::ValidationFeaturesEXT                   validation_features_info;
	std::vector<vk::ValidationFeatureEnableEXT> enable_features{};
	if (validation_features)
	{
#	if defined(VKB_VALIDATION_LAYERS_GPU_ASSISTED)
		enable_features.push_back(vk::ValidationFeatureEnableEXT::eGpuAssistedReserveBindingSlot);
		enable_features.push_back(vk::ValidationFeatureEnableEXT::eGpuAssisted);
#	endif
#	if defined(VKB_VALIDATION_LAYERS_BEST_PRACTICES)
		enable_features.push_back(vk::ValidationFeatureEnableEXT::eBestPractices);
#	endif
		validation_features_info.setEnabledValidationFeatures(enable_features);
		validation_features_info.pNext = instance_info.pNext;
		instance_info.pNext            = &validation_features_info;
	}
#endif

	vk::LayerSettingsCreateInfoEXT layerSettingsCreateInfo;

	// If layer settings are defined, then activate the sample's required layer settings during instance creation
	if (required_layer_settings.size() > 0)
	{
		layerSettingsCreateInfo.settingCount = static_cast<uint32_t>(required_layer_settings.size());
		layerSettingsCreateInfo.pSettings    = required_layer_settings.data();
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

HPPInstance::HPPInstance(vk::Instance instance) :
    handle{instance}
{
	if (handle)
	{
		query_gpus();
	}
	else
	{
		throw std::runtime_error("HPPInstance not valid");
	}
}

HPPInstance::~HPPInstance()
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

const std::vector<const char *> &HPPInstance::get_extensions()
{
	return enabled_extensions;
}

vkb::core::HPPPhysicalDevice &HPPInstance::get_first_gpu()
{
	assert(!gpus.empty() && "No physical devices were found on the system.");

	// Find a discrete GPU
	for (auto &gpu : gpus)
	{
		if (gpu->get_properties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
		{
			return *gpu;
		}
	}

	// Otherwise just pick the first one
	LOGW("Couldn't find a discrete physical device, picking default GPU");
	return *gpus[0];
}

vk::Instance HPPInstance::get_handle() const
{
	return handle;
}

vkb::core::HPPPhysicalDevice &HPPInstance::get_suitable_gpu(vk::SurfaceKHR surface, bool headless_surface)
{
	assert(!gpus.empty() && "No physical devices were found on the system.");

	// A GPU can be explicitly selected via the command line (see plugins/gpu_selection.cpp), this overrides the below GPU selection algorithm
	if (selected_gpu_index.has_value())
	{
		LOGI("Explicitly selecting GPU {}", selected_gpu_index.value());
		if (selected_gpu_index.value() > gpus.size() - 1)
		{
			throw std::runtime_error("Selected GPU index is not within no. of available GPUs");
		}
		return *gpus[selected_gpu_index.value()];
	}
	if (headless_surface)
	{
		LOGW("Using headless surface with multiple GPUs. Considered explicitly selecting the target GPU.")
	}

	// Find a discrete GPU
	for (auto &gpu : gpus)
	{
		if (gpu->get_properties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
		{
			// See if it work with the surface
			size_t queue_count = gpu->get_queue_family_properties().size();
			for (uint32_t queue_idx = 0; static_cast<size_t>(queue_idx) < queue_count; queue_idx++)
			{
				if (gpu->get_handle().getSurfaceSupportKHR(queue_idx, surface))
				{
					return *gpu;
				}
			}
		}
	}

	// Otherwise just pick the first one
	LOGW("Couldn't find a discrete physical device, picking default GPU");
	return *gpus[0];
}

bool HPPInstance::is_enabled(const char *extension) const
{
	return std::ranges::find_if(enabled_extensions,
	                            [extension](const char *enabled_extension) { return strcmp(extension, enabled_extension) == 0; }) != enabled_extensions.end();
}

void HPPInstance::query_gpus()
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
		gpus.push_back(std::make_unique<vkb::core::HPPPhysicalDevice>(*this, physical_device));
	}
}

}        // namespace core
}        // namespace vkb
