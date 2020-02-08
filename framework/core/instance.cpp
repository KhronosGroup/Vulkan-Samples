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

bool validate_extensions(const std::vector<const char *> &         required,
                         const std::vector<VkExtensionProperties> &available)
{
	for (auto extension : required)
	{
		bool found = false;
		for (auto &available_extension : available)
		{
			if (strcmp(available_extension.extensionName, extension) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			LOGE("Extension {} not found", extension);
			return false;
		}
	}

	return true;
}

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

Instance::Instance(const std::string &              application_name,
                   const std::vector<const char *> &required_extensions,
                   const std::vector<const char *> &required_validation_layers,
                   bool                             headless) :
    extensions{required_extensions}
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
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
	}
	if (!debug_utils)
	{
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
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
				extensions.push_back(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
			}
		}
		if (!headless_extension)
		{
			LOGW("{} is not available, disabling swapchain creation", VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
		}
	}
	else
	{
		extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	}

	if (!validate_extensions(extensions, available_instance_extensions))
	{
		throw std::runtime_error("Required instance extensions are missing.");
	}

	uint32_t instance_layer_count;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));

	std::vector<VkLayerProperties> instance_layers(instance_layer_count);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers.data()));

	std::vector<const char *> active_instance_layers(required_validation_layers);

#ifdef VKB_VALIDATION_LAYERS
	active_instance_layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

	if (validate_layers(active_instance_layers, instance_layers))
	{
		for (const auto &layer : active_instance_layers)
		{
			LOGI("Enabled Validation Layer {}", layer);
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

	instance_info.enabledExtensionCount   = to_u32(extensions.size());
	instance_info.ppEnabledExtensionNames = extensions.data();

	instance_info.enabledLayerCount   = to_u32(active_instance_layers.size());
	instance_info.ppEnabledLayerNames = active_instance_layers.data();

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
		VkDebugUtilsMessengerCreateInfoEXT info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};

		info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		info.pfnUserCallback = debug_utils_messenger_callback;

		result = vkCreateDebugUtilsMessengerEXT(handle, &info, nullptr, &debug_utils_messenger);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Could not create debug utils messenger.");
		}
	}
	else
	{
		VkDebugReportCallbackCreateInfoEXT info = {VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT};

		info.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		info.pfnCallback = debug_callback;

		result = vkCreateDebugReportCallbackEXT(handle, &info, nullptr, &debug_report_callback);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Could not create debug callback.");
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

	gpus.resize(physical_device_count);

	VK_CHECK(vkEnumeratePhysicalDevices(handle, &physical_device_count, gpus.data()));
}

VkPhysicalDevice Instance::get_gpu()
{
	// Find a discrete GPU
	for (auto gpu : gpus)
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(gpu, &properties);
		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			return gpu;
		}
	}

	// Otherwise just pick the first one
	LOGW("Couldn't find a discrete physical device, using integrated graphics");
	return gpus.at(0);
}

bool Instance::is_enabled(const char *extension)
{
	return std::find(extensions.begin(), extensions.end(), extension) != extensions.end();
}

VkInstance Instance::get_handle()
{
	return handle;
}

const std::vector<const char *> &Instance::get_extensions()
{
	return extensions;
}
}        // namespace vkb
