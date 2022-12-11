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

#include "components/vulkan/context/context_builder.hpp"
#include "debug.h"
#include "macros.hpp"

#include <algorithm>
#include <cstring>

#define KHORNOS_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"

namespace components
{
namespace vulkan
{
InstanceBuilder InstanceBuilder::enable_portability()
{
	required_layer(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	required_layer(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME, [](VkInstanceCreateInfo &create_info) {
		create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	});
	return *this;
}

InstanceBuilder &InstanceBuilder::enable_debug_logger()
{
	VkDebugReportCallbackCreateInfoEXT debug_report_create_info = {VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT};

	debug_utils_create_info.pNext = &debug_report_create_info;

	optional_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, [this](VkInstanceCreateInfo &create_info) {
		configure_extension_struct<VkDebugUtilsMessengerCreateInfoEXT>([](VkDebugUtilsMessengerCreateInfoEXT &info) {
			info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
			info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			info.pfnUserCallback = debug_utils_messenger_callback;
		});

		_has_debug_utils = true;
	});

	optional_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, [this](VkInstanceCreateInfo &create_info) {
		if (_has_debug_utils)
		{
			std::remove_if(_enabed_extensions.begin(), _enabled_extensions.end(), [](const char *name) {
				return strcmp(name, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0;
			});
			return;
		}

		configure_extension_struct<VkDebugReportCallbackCreateInfoEXT>([](VkDebugReportCallbackCreateInfoEXT &info) {
			info.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
			info.pfnCallback = debug_callback;
		});

		_has_debug_report = true;
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

	// TODO: required layer stuff
	require_extension(KHRONOS_VALIDATION_LAYER_NAME, VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);

	static const VkValidationFeatureEnableEXT enable_features[2] = {
	    VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
	    VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
	};

	configure_extension_struct<VkValidationFeaturesEXT>([enable_features](VkValidationFeaturesEXT &info) {
		// pNextChain handles chaining and VkStructureType
		validation_features_info.enabledValidationFeatureCount = 2;
		validation_features_info.pEnabledValidationFeatures    = enable_features;
	});

	std::set<std::string_view> layers{required_layers.begin(), required_layers.end()};

	// require layers in priority order
	for (const char *validation : validation_layer_priority_list)
	{
		if (std::find_if(layers.begin(), layers.end(), [validation](std::string_view view) { view == validation }) != layers.end())
		{
			required_layer(KHRONOS_VALIDATION_LAYER_NAME, validation);
			layers.erase(validation);
		}
	}

	// require any layers not listed in the priority list
	for (auto &layer : layers)
	{
		require_layer(KHRONOS_VALIDATION_LAYER_NAME, layer);
	}

	return *this;
}

std::vector<std::string_view> InstanceBuilder::collect_enabled_extensions(VkInstanceCreateInfo &info) const
{
	std::vector<std::string_view> enabled_extensions;

	// enable a single extension
	constexpr auto enable_one = [this, &info](const InstanceBuilder::LabelledCallback & extension,
	                                          const std::vector<VkExtensionProperties> &available_exts,
	                                          std::vector<const char *> *               enabled_extensions) {
		for (auto &avail_ext_it : available_exts)
		{
			if (strcmp(avail_ext_it.extensionName, extension.name) == 0)
			{
				auto it = std::find_if(enabled_extensions->begin(), enabled_extensions->end(),
				                       [&extension](const char *enabled_ext_name) {
					                       return strcmp(enabled_ext_name, extension.name) == 0;
				                       });
				if (it != enabled_extensions->end())
				{
					// Extension is already enabled
				}
				else
				{
					// LOGI("Extension {} found, enabling it", required_ext_name);
					enabled_extensions->emplace_back(extension.extension_name);
					extension.callback(info);
				}
				return true;
			}
		}

		return false;
	};

	// enable all extensions
	constexpr auto enable_all = [](const std::vector<std::string_view> &     required_ext_names,
	                               const std::vector<VkExtensionProperties> &available_exts,
	                               std::vector<const char *> *               enabled_extensions) {
		using std::placeholders::_1;

		return std::all_of(required_ext_names.begin(), required_ext_names.end(),
		                   std::bind(enable_extension, _1, available_exts, enabled_extensions));
	};

	for (auto extension_set : _optional_extensions)
	{
		if (extension.layer_name)
		{
			uint32_t layer_extension_count;
			VK_CHECK(vkEnumerateInstanceExtensionProperties(extension_set->first, &layer_extension_count, nullptr));
			std::vector<VkExtensionProperties> available_layer_extensions{layer_extension_count};
			VK_CHECK(vkEnumerateInstanceExtensionProperties(extension_set->first, &layer_extension_count, available_layer_extensions.data()));

			enable_all(extension_set->second, available_layer_extensions, &enabled_extensions);
		}
	}

	for (auto extension_set : _required_extensions)
	{
		if (extension.layer_name)
		{
			uint32_t layer_extension_count;
			VK_CHECK(vkEnumerateInstanceExtensionProperties(extension_set->first, &layer_extension_count, nullptr));
			std::vector<VkExtensionProperties> available_layer_extensions{layer_extension_count};
			VK_CHECK(vkEnumerateInstanceExtensionProperties(extension_set->first, &layer_extension_count, available_layer_extensions.data()));

			if (!enable_all(extension_set->second, available_layer_extensions, &enabled_extensions))
			{
				throw "failed to enable one or more required extensions";
			}
		}
	}

	return enabled_extensions;
}

std::vector<std::string_view> InstanceBuilder::collect_enabled_layers(VkInstanceCreateInfo &info) const
{
	std::vector<std::string_view> emabled_layers;

	uint32_t instance_layer_count;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));

	std::vector<VkLayerProperties> supported_validation_layers(instance_layer_count);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, supported_validation_layers.data()));

	std::vector<const char *> requested_validation_layers(required_validation_layers);

	// TODO: process layers

	return enabled_layers;
}

InstanceBuilder::Instance InstanceBuilder::Build()
{
	VkInstanceCreateInfo instance_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};

	auto enabled_layers     = collect_enabled_layers(instance_info);
	auto enabled_extensions = collect_enabled_extensions(instance_info);

	instance_info.pApplicationInfo = &_application_info;

	std::vector<const char *> enabled_extensions_cstr{enabled_extensions.begin(), enabled_extensions.end()};
	instance_info.enabledExtensionCount   = to_u32(enabled_extensions_cstr.size());
	instance_info.ppEnabledExtensionNames = enabled_extensions_cstr.data();

	std::vector<const char *> enabled_layers_cstr{enabled_layers.begin(), enabled_layers.end()};
	instance_info.enabledLayerCount   = to_u32(enabled_layers_cstr.size());
	instance_info.ppEnabledLayerNames = enabled_layers_cstr.data();

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

	if (_has_debug_utils)
	{
		result = vkCreateDebugUtilsMessengerEXT(instance, &debug_utils_create_info, nullptr, &debug_utils_messenger);
		if (result != VK_SUCCESS)
		{
			throw "Could not create debug utils messenger";
		}
	}
	else if (_has_debug_report)
	{
		result = vkCreateDebugReportCallbackEXT(instance, &debug_report_create_info, nullptr, &debug_report_callback);
		if (result != VK_SUCCESS)
		{
			throw "Could not create debug report callback";
		}
	}

	return {instance, {debug_utils_messenger, debug_report_callback}};
}

}        // namespace vulkan
}        // namespace components