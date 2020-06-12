/* Copyright (c) 2018-2020, Arm Limited and Contributors
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

namespace vkb
{
namespace
{
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)

VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                              const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                                                              void *                                      user_data)
{
	// Log debug messge
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

bool validate_layers(const std::vector<const char *> &     required,
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

Instance::Instance(const std::string &                           application_name,
                   const std::unordered_map<const char *, bool> &required_extensions,
                   const std::vector<const char *> &             required_validation_layers,
                   bool                                          headless)
{
	VkResult result = volkInitialize();
	if (result)
	{
		throw VulkanException(result, "Failed to initialize volk.");
	}

	uint32_t instance_extension_count;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr));

	std::vector<VkExtensionProperties> available_instance_extensions(instance_extension_count);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, available_instance_extensions.data()));

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	// Check if VK_EXT_debug_utils is supported, which supersedes VK_EXT_Debug_Report
	bool debug_utils = false;
	for (auto &available_extension : available_instance_extensions)
	{
		if (strcmp(available_extension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
		{
			debug_utils = true;
			LOGI("{} is available, enabling it", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			enabled_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
	}
	if (!debug_utils)
	{
		enabled_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}
#endif

	// Try to enable headless surface extension if it exists
	if (headless)
	{
		bool headless_extension = false;
		for (auto &available_extension : available_instance_extensions)
		{
			if (strcmp(available_extension.extensionName, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME) == 0)
			{
				headless_extension = true;
				LOGI("{} is available, enabling it", VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
				enabled_extensions.push_back(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
			}
		}
		if (!headless_extension)
		{
			LOGW("{} is not available, disabling swapchain creation", VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
		}
	}
	else
	{
		enabled_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	}

	for (auto &available_extension : available_instance_extensions)
	{
		// VK_KHR_get_physical_device_properties2 is a prerequisite of VK_KHR_performance_query
		// which will be used for stats gathering where available.
		if (strcmp(available_extension.extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
		{
			LOGI("{} is available, enabling it", VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
			enabled_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		}
	}

	auto extension_error = false;
	for (auto extension : required_extensions)
	{
		auto extension_name        = extension.first;
		auto extension_is_optional = extension.second;
		if (std::find_if(available_instance_extensions.begin(), available_instance_extensions.end(),
		                 [&extension_name](VkExtensionProperties available_extension) { return strcmp(available_extension.extensionName, extension_name) == 0; }) == available_instance_extensions.end())
		{
			if (extension_is_optional)
			{
				LOGW("Optional instance extension {} not available, some features may be disabled", extension_name);
			}
			else
			{
				LOGE("Required instance extension {} not available, cannot run", extension_name);
			}
			extension_error = !extension_is_optional;
		}
		else
		{
			enabled_extensions.push_back(extension_name);
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

#ifdef VKB_VALIDATION_LAYERS
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
	app_info.apiVersion         = VK_MAKE_VERSION(1, 0, 0);

	VkInstanceCreateInfo instance_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};

	instance_info.pApplicationInfo = &app_info;

	instance_info.enabledExtensionCount   = to_u32(enabled_extensions.size());
	instance_info.ppEnabledExtensionNames = enabled_extensions.data();

	instance_info.enabledLayerCount   = to_u32(requested_validation_layers.size());
	instance_info.ppEnabledLayerNames = requested_validation_layers.data();

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info  = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	VkDebugReportCallbackCreateInfoEXT debug_report_create_info = {VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT};
	if (debug_utils)
	{
		debug_utils_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		debug_utils_create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		debug_utils_create_info.pfnUserCallback = debug_utils_messenger_callback;

		instance_info.pNext = &debug_utils_create_info;
	}
	else
	{
		debug_report_create_info.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		debug_report_create_info.pfnCallback = debug_callback;

		instance_info.pNext = &debug_report_create_info;
	}
#endif

	// Create the Vulkan instance
	result = vkCreateInstance(&instance_info, nullptr, &handle);

	if (result != VK_SUCCESS)
	{
		throw VulkanException(result, "Could not create Vulkan instance");
	}

	volkLoadInstance(handle);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	if (debug_utils)
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
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
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

PhysicalDevice &Instance::get_suitable_gpu()
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
	return *gpus.at(0);
}

bool Instance::is_enabled(const char *extension) const
{
	return std::find_if(enabled_extensions.begin(), enabled_extensions.end(), [extension](const char *enabled_extension) { return strcmp(extension, enabled_extension) == 0; }) != enabled_extensions.end();
}

VkInstance Instance::get_handle()
{
	return handle;
}

const std::vector<const char *> &Instance::get_extensions()
{
	return enabled_extensions;
}
}        // namespace vkb
