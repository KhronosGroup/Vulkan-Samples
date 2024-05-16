/* Copyright (c) 2022-2024, NVIDIA CORPORATION. All rights reserved.
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

bool validate_layers(std::unordered_map<const char *, bool> &required,
                     const std::vector<vk::LayerProperties> &available)
{
	std::vector<const char *> remove_vec;
	for (auto layer : required)
	{
		bool found = false;
		for (auto &available_layer : available)
		{
			if (strcmp(available_layer.layerName, layer.first) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			if (!layer.second)
			{
				LOGW("Optional Layer {} not found, removing it", layer.first)
				remove_vec.push_back(layer.first);
				continue;
			}
			LOGE("Validation Layer {} not found", layer.first)
			return false;
		}
	}
	for (auto &rem : remove_vec)
	{
		required.erase(rem);
	}
	return true;
}

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
std::vector<const char *> get_optimal_validation_layers(const std::vector<vk::LayerProperties> &supported_instance_layers)
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

Optional<uint32_t> HPPInstance::selected_gpu_index;

namespace
{
bool enable_extension(const char                                 *required_ext_name,
                      const std::vector<vk::ExtensionProperties> &available_exts,
                      std::vector<const char *>                  &enabled_extensions)
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

bool enable_all_extensions(const std::vector<const char *>             required_ext_names,
                           const std::vector<vk::ExtensionProperties> &available_exts,
                           std::vector<const char *>                  &enabled_extensions)
{
	using std::placeholders::_1;

	return std::all_of(required_ext_names.begin(), required_ext_names.end(),
	                   std::bind(enable_extension, _1, available_exts, enabled_extensions));
}

}        // namespace

HPPInstance::HPPInstance(const std::string                            &application_name,
                         const std::unordered_map<const char *, bool> &required_extensions,
                         const std::vector<const char *> &required_validation_layers,
                         const std::unordered_map<const char *, bool> &requested_layers,
                         bool                                          headless,
                         uint32_t                                      api_version)
{
	std::vector<vk::ExtensionProperties> available_instance_extensions = vk::enumerateInstanceExtensionProperties();

#ifdef USE_VALIDATION_LAYERS
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
		std::vector<vk::ExtensionProperties> available_layer_instance_extensions = vk::enumerateInstanceExtensionProperties(std::string("VK_LAYER_KHRONOS_validation"));

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

	std::vector<vk::LayerProperties> supported_validation_layers = vk::enumerateInstanceLayerProperties();

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

	std::unordered_map<const char*, bool> layers = (std::unordered_map<const char*, bool>)(requested_layers);
	if(validate_layers(layers, supported_validation_layers))
	{
		LOGI("Enabled Validation Layers:")
		for (const auto &layer : layers)
		{
			LOGI("	\t{}", layer.first);
		}
	}
	else
	{
		throw std::runtime_error("Required validation layers are missing.");
	}

	vk::ApplicationInfo app_info(application_name.c_str(), 0, "Vulkan Samples", 0, api_version);

	vk::InstanceCreateInfo instance_info({}, &app_info, requested_validation_layers, enabled_extensions);

#ifdef USE_VALIDATION_LAYERS
	vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create_info;
	vk::DebugReportCallbackCreateInfoEXT debug_report_create_info;
	if (has_debug_utils)
	{
		debug_utils_create_info =
		    vk::DebugUtilsMessengerCreateInfoEXT({},
		                                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
		                                         vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
		                                         debug_utils_messenger_callback);

		instance_info.pNext = &debug_utils_create_info;
	}
	else
	{
		debug_report_create_info = vk::DebugReportCallbackCreateInfoEXT(
		    vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::ePerformanceWarning, debug_callback);

		instance_info.pNext = &debug_report_create_info;
	}
#endif

#if (defined(VKB_ENABLE_PORTABILITY))
	instance_info.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
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
	else
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

vkb::core::HPPPhysicalDevice &HPPInstance::get_suitable_gpu(vk::SurfaceKHR surface)
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
	return std::find_if(enabled_extensions.begin(),
	                    enabled_extensions.end(),
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
