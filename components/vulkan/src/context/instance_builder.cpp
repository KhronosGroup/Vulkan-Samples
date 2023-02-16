/* Copyright (c) 2022, Arm Limited and Contributors
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

#include "components/vulkan/context/instance_builder.hpp"

#include "debug_loggers.h"
#include "macros.hpp"

#include <algorithm>
#include <cstring>
#include <set>
#include <sstream>
#include <string_view>
#include <vector>

#include <components/common/logging.hpp>
#include <components/common/strings.hpp>

#define KHRONOS_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"

namespace components
{
namespace vulkan
{

InstanceBuilder &InstanceBuilder::enable_portability()
{
	required_layer(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	required_layer(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME, [](VkInstanceCreateInfo &create_info) {
		create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	});
	return *this;
}

InstanceBuilder &InstanceBuilder::enable_debug_logger()
{
	// attempt to create debug utils messenger first
	optional_extension("", VK_EXT_DEBUG_UTILS_EXTENSION_NAME, [this](VkInstanceCreateInfo &create_info) {
		// if we created utils then append the struct to the chain
		configure_extension_struct<VkDebugUtilsMessengerCreateInfoEXT>([](VkDebugUtilsMessengerCreateInfoEXT &info) {
			info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
			info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			info.pfnUserCallback = debug::debug_utils_messenger_callback;
		});
	});

	// attempt to create debug report messenger second
	optional_extension("", VK_EXT_DEBUG_REPORT_EXTENSION_NAME, [this](VkInstanceCreateInfo &create_info) {
		// if we created report then disable utils
		if (has_extension_in_chain<VkDebugUtilsMessengerCreateInfoEXT>())
		{
			// remove debug utils extension from required extensions
			disable_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}

		// use report instead by appending its struct
		configure_extension_struct<VkDebugReportCallbackCreateInfoEXT>([](VkDebugReportCallbackCreateInfoEXT &info) {
			info.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
			info.pfnCallback = debug::debug_report_callback;
		});
	});

	return *this;
}

InstanceBuilder &InstanceBuilder::enable_validation_layers(const std::vector<std::string_view> &required_layers)
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

	required_extension(KHRONOS_VALIDATION_LAYER_NAME, VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);

	configure_extension_struct<VkValidationFeaturesEXT>([&](VkValidationFeaturesEXT &info) {
		// we already know which features we want to enable, so we can just hardcode them here
		static const std::vector<VkValidationFeatureEnableEXT> enable_features{
		    {
		        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
		        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
		    }};
		info.enabledValidationFeatureCount = static_cast<uint32_t>(enable_features.size());
		info.pEnabledValidationFeatures    = enable_features.data();
	});

	for (auto layer : required_layers)
	{
		required_layer(layer);
	}

	auto supported_instance_layers = get_available_layers();

	for (auto &layer_priority_list : validation_layer_priority_list)
	{
		bool all_layers_found = true;
		for (auto &layer_name : layer_priority_list)
		{
			if (std::find_if(supported_instance_layers.begin(), supported_instance_layers.end(), [&](const VkLayerProperties &layer) {
				    return strcmp(layer.layerName, layer_name) == 0;
			    }) == supported_instance_layers.end())
			{
				all_layers_found = false;
				break;
			}
		}

		if (all_layers_found)
		{
			for (auto &layer_name : layer_priority_list)
			{
				required_layer(layer_name);
			}
		}
	}

	return *this;
}

InstanceBuilder::Output InstanceBuilder::build()
{
	VkInstanceCreateInfo instance_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};

	instance_info.pApplicationInfo = &_application_info;

	std::vector<const char *> enabled_extensions_cstr = strings::to_cstr(collect_enabled_extensions(instance_info));
	instance_info.enabledExtensionCount               = static_cast<uint32_t>(enabled_extensions_cstr.size());
	instance_info.ppEnabledExtensionNames             = enabled_extensions_cstr.data();

	std::vector<const char *> enabled_layers_cstr = strings::to_cstr(collect_enabled_layers(instance_info));
	instance_info.enabledLayerCount               = static_cast<uint32_t>(enabled_layers_cstr.size());
	instance_info.ppEnabledLayerNames             = enabled_layers_cstr.data();

	VkInstance instance;

	// Create the Vulkan instance
	VkResult result = vkCreateInstance(&instance_info, nullptr, &instance);

	if (result != VK_SUCCESS)
	{
		throw "Could not create Vulkan instance";
	}

	volkLoadInstance(instance);

	VkDebugUtilsMessengerEXT debug_utils_messenger{VK_NULL_HANDLE};
	VkDebugReportCallbackEXT debug_report_callback{VK_NULL_HANDLE};

	if (has_extension_in_chain<VkDebugUtilsMessengerCreateInfoEXT>())
	{
		auto debug_utils_create_info = _chain.get<VkDebugUtilsMessengerCreateInfoEXT>();
		result                       = vkCreateDebugUtilsMessengerEXT(instance, &debug_utils_create_info, nullptr, &debug_utils_messenger);
		if (result != VK_SUCCESS)
		{
			throw "Could not create debug utils messenger";
		}
	}
	else if (has_extension_in_chain<VkDebugReportCallbackCreateInfoEXT>())
	{
		auto debug_report_create_info = _chain.get<VkDebugReportCallbackCreateInfoEXT>();
		result                        = vkCreateDebugReportCallbackEXT(instance, &debug_report_create_info, nullptr, &debug_report_callback);
		if (result != VK_SUCCESS)
		{
			throw "Could not create debug report callback";
		}
	}

	return {instance, {debug_utils_messenger, debug_report_callback}};
}

}        // namespace vulkan
}        // namespace components