/* Copyright (c) 2018-2025, Arm Limited and Contributors
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

#include "instance.h"

#include <algorithm>
#include <functional>

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

VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                              const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                                                              void                                       *user_data)
{
	// Log debug message
	if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		LOGW("{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		LOGE("{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	return VK_FALSE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT /*type*/,
                                                     uint64_t /*object*/, size_t /*location*/, int32_t /*message_code*/,
                                                     const char *layer_prefix, const char *message, void * /*user_data*/)
{
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		LOGE("{}: {}", layer_prefix, message);
	}
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		LOGW("{}: {}", layer_prefix, message);
	}
	else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
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

bool validate_layers(const std::vector<const char *>      &required,
                     const std::vector<VkLayerProperties> &available)
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

Optional<uint32_t> Instance::selected_gpu_index;

namespace
{
bool enable_extension(const char                               *requested_extension,
                      const std::vector<VkExtensionProperties> &available_extensions,
                      std::vector<const char *>                &enabled_extensions)
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

bool enable_layer(const char                           *requested_layer,
                  const std::vector<VkLayerProperties> &available_layers,
                  std::vector<const char *>            &enabled_layers)
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

Instance::Instance(const std::string                            &application_name,
                   const std::unordered_map<const char *, bool> &requested_extensions,
                   const std::unordered_map<const char *, bool> &requested_layers,
                   const std::vector<VkLayerSettingEXT>         &required_layer_settings,
                   uint32_t                                      api_version)
{
	uint32_t instance_extension_count;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr));

	std::vector<VkExtensionProperties> available_instance_extensions(instance_extension_count);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, available_instance_extensions.data()));

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
		uint32_t layer_instance_extension_count;
		VK_CHECK(vkEnumerateInstanceExtensionProperties("VK_LAYER_KHRONOS_validation", &layer_instance_extension_count, nullptr));

		std::vector<VkExtensionProperties> available_layer_instance_extensions(layer_instance_extension_count);
		VK_CHECK(vkEnumerateInstanceExtensionProperties("VK_LAYER_KHRONOS_validation", &layer_instance_extension_count, available_layer_instance_extensions.data()));

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

	uint32_t instance_layer_count;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));

	std::vector<VkLayerProperties> supported_layers(instance_layer_count);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, supported_layers.data()));

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

	VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};

	app_info.pApplicationName   = application_name.c_str();
	app_info.applicationVersion = 0;
	app_info.pEngineName        = "Vulkan Samples";
	app_info.engineVersion      = 0;
	app_info.apiVersion         = api_version;

	VkInstanceCreateInfo instance_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};

	instance_info.pApplicationInfo = &app_info;

	instance_info.enabledExtensionCount   = to_u32(enabled_extensions.size());
	instance_info.ppEnabledExtensionNames = enabled_extensions.data();

	instance_info.enabledLayerCount   = to_u32(enabled_layers.size());
	instance_info.ppEnabledLayerNames = enabled_layers.data();

#ifdef USE_VALIDATION_LAYERS
	VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info  = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	VkDebugReportCallbackCreateInfoEXT debug_report_create_info = {VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT};
	if (has_debug_utils)
	{
		debug_utils_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		debug_utils_create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debug_utils_create_info.pfnUserCallback = debug_utils_messenger_callback;

		instance_info.pNext = &debug_utils_create_info;
	}
	else if (has_debug_report)
	{
		debug_report_create_info.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		debug_report_create_info.pfnCallback = debug_callback;

		instance_info.pNext = &debug_report_create_info;
	}
#endif

#if (defined(VKB_ENABLE_PORTABILITY))
	if (portability_enumeration_available)
	{
		instance_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	}
#endif

	// Some of the specialized layers need to be enabled explicitly
#ifdef USE_VALIDATION_LAYER_FEATURES
	VkValidationFeaturesEXT                   validation_features_info = {VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
	std::vector<VkValidationFeatureEnableEXT> enable_features{};
	if (validation_features)
	{
#	if defined(VKB_VALIDATION_LAYERS_GPU_ASSISTED)
		enable_features.push_back(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT);
		enable_features.push_back(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT);
#	endif
#	if defined(VKB_VALIDATION_LAYERS_BEST_PRACTICES)
		enable_features.push_back(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);
#	endif
#	if defined(VKB_VALIDATION_LAYERS_SYNCHRONIZATION)
		enable_features.push_back(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT);
#	endif
		validation_features_info.enabledValidationFeatureCount = static_cast<uint32_t>(enable_features.size());
		validation_features_info.pEnabledValidationFeatures    = enable_features.data();
		validation_features_info.pNext                         = instance_info.pNext;
		instance_info.pNext                                    = &validation_features_info;
	}
#endif

	VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo{VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT};

	// If layer settings are defined, then activate the sample's required layer settings during instance creation
	if (required_layer_settings.size() > 0)
	{
		layerSettingsCreateInfo.settingCount = static_cast<uint32_t>(required_layer_settings.size());
		layerSettingsCreateInfo.pSettings    = required_layer_settings.data();
		layerSettingsCreateInfo.pNext        = instance_info.pNext;
		instance_info.pNext                  = &layerSettingsCreateInfo;
	}

	// Create the Vulkan instance
	VkResult result = vkCreateInstance(&instance_info, nullptr, &handle);

	if (result != VK_SUCCESS)
	{
		throw VulkanException(result, "Could not create Vulkan instance");
	}

	volkLoadInstance(handle);

#ifdef USE_VALIDATION_LAYERS
	if (has_debug_utils)
	{
		result = vkCreateDebugUtilsMessengerEXT(handle, &debug_utils_create_info, nullptr, &debug_utils_messenger);
		if (result != VK_SUCCESS)
		{
			throw VulkanException(result, "Could not create debug utils messenger");
		}
	}
	else if (has_debug_report)
	{
		result = vkCreateDebugReportCallbackEXT(handle, &debug_report_create_info, nullptr, &debug_report_callback);
		if (result != VK_SUCCESS)
		{
			throw VulkanException(result, "Could not create debug report callback");
		}
	}
#endif

	query_gpus();
}

Instance::Instance(VkInstance                       instance,
                   const std::vector<const char *> &externally_enabled_extensions) :
    handle{instance}
{
	// Some parts of the framework will check for certain extensions to be enabled
	// To make those work we need to copy over externally enabled extensions into this class
	for (auto extension : externally_enabled_extensions)
	{
		enabled_extensions.push_back(extension);
	}

	if (handle != VK_NULL_HANDLE)
	{
		query_gpus();
	}
	else
	{
		throw std::runtime_error("Instance not valid");
	}
}

Instance::~Instance()
{
#ifdef USE_VALIDATION_LAYERS
	if (debug_utils_messenger != VK_NULL_HANDLE)
	{
		vkDestroyDebugUtilsMessengerEXT(handle, debug_utils_messenger, nullptr);
	}
	if (debug_report_callback != VK_NULL_HANDLE)
	{
		vkDestroyDebugReportCallbackEXT(handle, debug_report_callback, nullptr);
	}
#endif

	if (handle != VK_NULL_HANDLE)
	{
		vkDestroyInstance(handle, nullptr);
	}
}

void Instance::query_gpus()
{
	// Querying valid physical devices on the machine
	uint32_t physical_device_count{0};
	VK_CHECK(vkEnumeratePhysicalDevices(handle, &physical_device_count, nullptr));

	if (physical_device_count < 1)
	{
		throw std::runtime_error("Couldn't find a physical device that supports Vulkan.");
	}

	std::vector<VkPhysicalDevice> physical_devices;
	physical_devices.resize(physical_device_count);
	VK_CHECK(vkEnumeratePhysicalDevices(handle, &physical_device_count, physical_devices.data()));

	// Create gpus wrapper objects from the VkPhysicalDevice's
	for (auto &physical_device : physical_devices)
	{
		gpus.push_back(std::make_unique<PhysicalDevice>(*this, physical_device));
	}
}

PhysicalDevice &Instance::get_first_gpu()
{
	assert(!gpus.empty() && "No physical devices were found on the system.");

	// Find a discrete GPU
	for (auto &gpu : gpus)
	{
		if (gpu->get_properties().deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			return *gpu;
		}
	}

	// Otherwise just pick the first one
	LOGW("Couldn't find a discrete physical device, picking default GPU");
	return *gpus[0];
}

PhysicalDevice &Instance::get_suitable_gpu(VkSurfaceKHR surface, bool headless_surface)
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
		if (gpu->get_properties().deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			// See if it work with the surface
			size_t queue_count = gpu->get_queue_family_properties().size();
			for (uint32_t queue_idx = 0; static_cast<size_t>(queue_idx) < queue_count; queue_idx++)
			{
				if (gpu->is_present_supported(surface, queue_idx))
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

bool Instance::is_enabled(const char *extension) const
{
	return std::ranges::find_if(enabled_extensions, [extension](const char *enabled_extension) { return strcmp(extension, enabled_extension) == 0; }) != enabled_extensions.end();
}

VkInstance Instance::get_handle() const
{
	return handle;
}

const std::vector<const char *> &Instance::get_extensions()
{
	return enabled_extensions;
}
}        // namespace vkb
