/* Copyright (c) 2018-2023, Arm Limited and Contributors
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
#	define USE_VALIDATION_LAYERS 1
#endif

#if defined(USE_VALIDATION_LAYERS) && (defined(VKB_VALIDATION_LAYERS_GPU_ASSISTED) || defined(VKB_VALIDATION_LAYERS_BEST_PRACTICES) || defined(VKB_VALIDATION_LAYERS_SYNCHRONIZATION))
#	define USE_VALIDATION_LAYER_FEATURES 1
#endif

#ifdef USE_VALIDATION_LAYERS
#	define USE_VULKAN_LOGGER
#endif

namespace vkb
{
namespace
{
#ifdef USE_VULKAN_LOGGER

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

std::vector<const char *> get_optimal_validation_layers(const std::vector<VkLayerProperties> &supported_instance_layers)
{
	std::vector<std::vector<const char *>> validation_layer_priority_list =
	    {
	        // The preferred validation layer is "VK_LAYER_KHRONOS_validation"
	        {"VK_LAYER_KHRONOS_validation"},

	        // Otherwise we fallback to using the LunarG meta layer
	        {"VK_LAYER_LUNARG_standard_validation"},

	        // Otherwise we attempt to enable the individual layers that compose the LunarG meta layer since it doesn't exist
	        {
	            "VK_LAYER_GOOGLE_threading",
	            "VK_LAYER_LUNARG_parameter_validation",
	            "VK_LAYER_LUNARG_object_tracker",
	            "VK_LAYER_LUNARG_core_validation",
	            "VK_LAYER_GOOGLE_unique_objects",
	        },

	        // Otherwise as a last resort we fallback to attempting to enable the LunarG core layer
	        {"VK_LAYER_LUNARG_core_validation"}};

	for (auto &validation_layers : validation_layer_priority_list)
	{
		if (validate_layers(validation_layers, supported_instance_layers))
		{
			return validation_layers;
		}

		LOGW("Couldn't enable validation layers (see log for error) - falling back");
	}

	// Else return nothing
	return {};
}

namespace
{
bool enable_extension(const char                               *required_ext_name,
                      const std::vector<VkExtensionProperties> &available_exts,
                      std::vector<const char *>                &enabled_extensions)
{
	for (auto &avail_ext_it : available_exts)
	{
		if (strcmp(avail_ext_it.extensionName, required_ext_name) == 0)
		{
			auto it = std::find_if(enabled_extensions.begin(), enabled_extensions.end(),
			                       [required_ext_name](const char *enabled_ext_name) {
				                       return strcmp(enabled_ext_name, required_ext_name) == 0;
			                       });
			if (it != enabled_extensions.end())
			{
				// Extension is already enabled
			}
			else
			{
				LOGI("Extension {} found, enabling it", required_ext_name);
				enabled_extensions.emplace_back(required_ext_name);
			}
			return true;
		}
	}

	LOGI("Extension {} not found", required_ext_name);
	return false;
}

bool enable_all_extensions(const std::vector<const char *>           required_ext_names,
                           const std::vector<VkExtensionProperties> &available_exts,
                           std::vector<const char *>                &enabled_extensions)
{
	using std::placeholders::_1;

	return std::all_of(required_ext_names.begin(), required_ext_names.end(),
	                   std::bind(enable_extension, _1, available_exts, enabled_extensions));
}

}        // namespace

Instance::Instance(const std::string                            &application_name,
                   const std::unordered_map<const char *, bool> &required_extensions,
                   const std::vector<const char *>              &required_validation_layers,
                   bool                                          headless,
                   uint32_t                                      api_version)
{
	uint32_t instance_extension_count;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr));

	std::vector<VkExtensionProperties> available_instance_extensions(instance_extension_count);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, available_instance_extensions.data()));

#ifdef USE_VULKAN_LOGGER
	// Check if VK_EXT_debug_utils is supported, which supersedes VK_EXT_Debug_Report
	const bool has_debug_utils  = enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	                                               available_instance_extensions, enabled_extensions);
	bool       has_debug_report = false;

	if (!has_debug_utils)
	{
		has_debug_report = enable_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		                                    available_instance_extensions, enabled_extensions);
		if (!has_debug_report)
		{
			LOGW("Neither of {} or {} are available; disabling debug reporting",
			     VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}
	}
#endif

#if (defined(VKB_ENABLE_PORTABILITY))
	enable_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, available_instance_extensions, enabled_extensions);
	enable_extension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME, available_instance_extensions, enabled_extensions);
#endif

#ifdef USE_VALIDATION_LAYER_FEATURES
	bool validation_features = false;
	{
		uint32_t layer_instance_extension_count;
		VK_CHECK(vkEnumerateInstanceExtensionProperties("VK_LAYER_KHRONOS_validation", &layer_instance_extension_count, nullptr));

		std::vector<VkExtensionProperties> available_layer_instance_extensions(layer_instance_extension_count);
		VK_CHECK(vkEnumerateInstanceExtensionProperties("VK_LAYER_KHRONOS_validation", &layer_instance_extension_count, available_layer_instance_extensions.data()));

		for (auto &available_extension : available_layer_instance_extensions)
		{
			if (strcmp(available_extension.extensionName, VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME) == 0)
			{
				validation_features = true;
				LOGI("{} is available, enabling it", VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
				enabled_extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
			}
		}
	}
#endif

	// Try to enable headless surface extension if it exists
	if (headless)
	{
		const bool has_headless_surface = enable_extension(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME,
		                                                   available_instance_extensions, enabled_extensions);
		if (!has_headless_surface)
		{
			LOGW("{} is not available, disabling swapchain creation", VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
		}
	}
	else
	{
		enabled_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	}

	// VK_KHR_get_physical_device_properties2 is a prerequisite of VK_KHR_performance_query
	// which will be used for stats gathering where available.
	enable_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
	                 available_instance_extensions, enabled_extensions);

	auto extension_error = false;
	for (auto extension : required_extensions)
	{
		auto extension_name        = extension.first;
		auto extension_is_optional = extension.second;
		if (!enable_extension(extension_name, available_instance_extensions, enabled_extensions))
		{
			if (extension_is_optional)
			{
				LOGW("Optional instance extension {} not available, some features may be disabled", extension_name);
			}
			else
			{
				LOGE("Required instance extension {} not available, cannot run", extension_name);
				extension_error = true;
			}
			extension_error = extension_error || !extension_is_optional;
		}
	}

	if (extension_error)
	{
		throw std::runtime_error("Required instance extensions are missing.");
	}

	uint32_t instance_layer_count;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));

	std::vector<VkLayerProperties> supported_validation_layers(instance_layer_count);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, supported_validation_layers.data()));

	std::vector<const char *> requested_validation_layers(required_validation_layers);

#ifdef USE_VALIDATION_LAYERS
	// Determine the optimal validation layers to enable that are necessary for useful debugging
	std::vector<const char *> optimal_validation_layers = get_optimal_validation_layers(supported_validation_layers);
	requested_validation_layers.insert(requested_validation_layers.end(), optimal_validation_layers.begin(), optimal_validation_layers.end());
#endif

	if (validate_layers(requested_validation_layers, supported_validation_layers))
	{
		LOGI("Enabled Validation Layers:")
		for (const auto &layer : requested_validation_layers)
		{
			LOGI("	\t{}", layer);
		}
	}
	else
	{
		throw std::runtime_error("Required validation layers are missing.");
	}

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

	instance_info.enabledLayerCount   = to_u32(requested_validation_layers.size());
	instance_info.ppEnabledLayerNames = requested_validation_layers.data();

#ifdef USE_VULKAN_LOGGER
	VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info  = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	VkDebugReportCallbackCreateInfoEXT debug_report_create_info = {VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT};
	if (has_debug_utils)
	{
		debug_utils_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		debug_utils_create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debug_utils_create_info.pfnUserCallback = debug_utils_messenger_callback;

		instance_info.pNext = &debug_utils_create_info;
	}
	else
	{
		debug_report_create_info.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		debug_report_create_info.pfnCallback = debug_callback;

		instance_info.pNext = &debug_report_create_info;
	}
#endif

#if (defined(VKB_ENABLE_PORTABILITY))
	instance_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
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

	// Create the Vulkan instance
	VkResult result = vkCreateInstance(&instance_info, nullptr, &handle);

	if (result != VK_SUCCESS)
	{
		throw VulkanException(result, "Could not create Vulkan instance");
	}

	volkLoadInstance(handle);

#ifdef USE_VULKAN_LOGGER
	if (has_debug_utils)
	{
		result = vkCreateDebugUtilsMessengerEXT(handle, &debug_utils_create_info, nullptr, &debug_utils_messenger);
		if (result != VK_SUCCESS)
		{
			throw VulkanException(result, "Could not create debug utils messenger");
		}
	}
	else
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

Instance::Instance(VkInstance instance) :
    handle{instance}
{
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
#ifdef USE_VULKAN_LOGGER
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

PhysicalDevice &Instance::get_suitable_gpu(VkSurfaceKHR surface)
{
	assert(!gpus.empty() && "No physical devices were found on the system.");

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
	return std::find_if(enabled_extensions.begin(), enabled_extensions.end(), [extension](const char *enabled_extension) { return strcmp(extension, enabled_extension) == 0; }) != enabled_extensions.end();
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
