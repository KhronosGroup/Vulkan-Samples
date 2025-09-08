/* Copyright (c) 2024-2025, Huawei Technologies Co., Ltd.
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

#include "hello_triangle_1_3.h"

#include "common/vk_common.h"
#include "core/util/logging.hpp"
#include "filesystem/legacy.h"
#include "platform/window.h"

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
/// @brief A debug callback called from Vulkan validation layers.
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
                                                     VkDebugUtilsMessageTypeFlagsEXT             message_types,
                                                     const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                                                     void                                       *user_data)
{
	if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		LOGE("{} Validation Layer: Error: {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		LOGW("{} Validation Layer: Warning: {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		LOGI("{} Validation Layer: Information: {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	else if (message_types & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
	{
		LOGI("{} Validation Layer: Performance warning: {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
	{
		LOGD("{} Validation Layer: Verbose: {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	return VK_FALSE;
}
#endif

/**
 * @brief Validates a list of required extensions, comparing it with the available ones.
 *
 * @param required A vector containing required extension names.
 * @param available A VkExtensionProperties object containing available extensions.
 * @return true if all required extensions are available
 * @return false otherwise
 */
bool HelloTriangleV13::validate_extensions(const std::vector<const char *>          &required,
                                           const std::vector<VkExtensionProperties> &available)
{
	bool all_found = true;

	for (const auto *extension_name : required)
	{
		bool found = false;
		for (const auto &available_extension : available)
		{
			if (strcmp(available_extension.extensionName, extension_name) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			// Output an error message for the missing extension
			LOGE("Error: Required extension not found: {}", extension_name);
			all_found = false;
		}
	}

	return all_found;
}

/**
 * @brief Initializes the Vulkan instance.
 */
void HelloTriangleV13::init_instance()
{
	LOGI("Initializing Vulkan instance.");

	if (volkInitialize())
	{
		throw std::runtime_error("Failed to initialize volk.");
	}

	uint32_t instance_extension_count;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr));

	std::vector<VkExtensionProperties> available_instance_extensions(instance_extension_count);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, available_instance_extensions.data()));

	std::vector<const char *> required_instance_extensions{VK_KHR_SURFACE_EXTENSION_NAME};

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	bool has_debug_utils = false;
	for (const auto &ext : available_instance_extensions)
	{
		if (strncmp(ext.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME, strlen(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) == 0)
		{
			has_debug_utils = true;
			break;
		}
	}
	if (has_debug_utils)
	{
		required_instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	else
	{
		LOGW("{} is not available; disabling debug utils messenger", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
#endif

#if (defined(VKB_ENABLE_PORTABILITY))
	required_instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	bool portability_enumeration_available = false;
	if (std::ranges::any_of(available_instance_extensions,
	                        [](VkExtensionProperties const &extension) { return strcmp(extension.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0; }))
	{
		required_instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
		portability_enumeration_available = true;
	}
#endif

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	required_instance_extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
	required_instance_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
	required_instance_extensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	required_instance_extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
	required_instance_extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	required_instance_extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
	required_instance_extensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
#else
#	pragma error Platform not supported
#endif

	if (!validate_extensions(required_instance_extensions, available_instance_extensions))
	{
		throw std::runtime_error("Required instance extensions are missing.");
	}

	std::vector<const char *> requested_instance_layers{};

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	char const *validationLayer = "VK_LAYER_KHRONOS_validation";

	uint32_t instance_layer_count;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));

	std::vector<VkLayerProperties> supported_instance_layers(instance_layer_count);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, supported_instance_layers.data()));

	if (std::ranges::any_of(supported_instance_layers, [&validationLayer](auto const &lp) { return strcmp(lp.layerName, validationLayer) == 0; }))
	{
		requested_instance_layers.push_back(validationLayer);
		LOGI("Enabled Validation Layer {}", validationLayer);
	}
	else
	{
		LOGW("Validation Layer {} is not available", validationLayer);
	}
#endif

	VkApplicationInfo app{
	    .sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO,
	    .pApplicationName = "Hello Triangle V1.3",
	    .pEngineName      = "Vulkan Samples",
	    .apiVersion       = VK_API_VERSION_1_3};

	VkInstanceCreateInfo instance_info{
	    .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
	    .pApplicationInfo        = &app,
	    .enabledLayerCount       = vkb::to_u32(requested_instance_layers.size()),
	    .ppEnabledLayerNames     = requested_instance_layers.data(),
	    .enabledExtensionCount   = vkb::to_u32(required_instance_extensions.size()),
	    .ppEnabledExtensionNames = required_instance_extensions.data()};

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	if (has_debug_utils)
	{
		debug_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		debug_messenger_create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		debug_messenger_create_info.pfnUserCallback = debug_callback;

		instance_info.pNext = &debug_messenger_create_info;
	}
#endif

#if (defined(VKB_ENABLE_PORTABILITY))
	if (portability_enumeration_available)
	{
		instance_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	}
#endif

	// Create the Vulkan instance
	VK_CHECK(vkCreateInstance(&instance_info, nullptr, &context.instance));

	volkLoadInstance(context.instance);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	if (has_debug_utils)
	{
		VK_CHECK(vkCreateDebugUtilsMessengerEXT(context.instance, &debug_messenger_create_info, nullptr, &context.debug_callback));
	}
#endif
}

/**
 * @brief Initializes the Vulkan physical device and logical device.
 */
void HelloTriangleV13::init_device()
{
	LOGI("Initializing Vulkan device.");

	uint32_t gpu_count = 0;
	VK_CHECK(vkEnumeratePhysicalDevices(context.instance, &gpu_count, nullptr));

	if (gpu_count < 1)
	{
		throw std::runtime_error("No physical device found.");
	}

	std::vector<VkPhysicalDevice> gpus(gpu_count);
	VK_CHECK(vkEnumeratePhysicalDevices(context.instance, &gpu_count, gpus.data()));

	for (const auto &physical_device : gpus)
	{
		// Check if the device supports Vulkan 1.3
		VkPhysicalDeviceProperties device_properties;
		vkGetPhysicalDeviceProperties(physical_device, &device_properties);

		if (device_properties.apiVersion < VK_API_VERSION_1_3)
		{
			LOGW("Physical device '{}' does not support Vulkan 1.3, skipping.", device_properties.deviceName);
			continue;
		}

		// Find a queue family that supports graphics and presentation
		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

		std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_properties.data());

		for (uint32_t i = 0; i < queue_family_count; i++)
		{
			VkBool32 supports_present = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, context.surface, &supports_present);

			if ((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && supports_present)
			{
				context.graphics_queue_index = i;
				break;
			}
		}

		if (context.graphics_queue_index >= 0)
		{
			context.gpu = physical_device;
			break;
		}
	}

	if (context.graphics_queue_index < 0)
	{
		throw std::runtime_error("Failed to find a suitable GPU with Vulkan 1.3 support.");
	}

	uint32_t device_extension_count;

	VK_CHECK(vkEnumerateDeviceExtensionProperties(context.gpu, nullptr, &device_extension_count, nullptr));

	std::vector<VkExtensionProperties> device_extensions(device_extension_count);

	VK_CHECK(vkEnumerateDeviceExtensionProperties(context.gpu, nullptr, &device_extension_count, device_extensions.data()));

	// Since this sample has visual output, the device needs to support the swapchain extension
	std::vector<const char *> required_device_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

	if (!validate_extensions(required_device_extensions, device_extensions))
	{
		throw std::runtime_error("Required device extensions are missing");
	}

#if (defined(VKB_ENABLE_PORTABILITY))
	// VK_KHR_portability_subset must be enabled if present in the implementation (e.g on macOS/iOS with beta extensions enabled)
	if (std::ranges::any_of(device_extensions,
	                        [](VkExtensionProperties const &extension) { return strcmp(extension.extensionName, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) == 0; }))
	{
		required_device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
	}
#endif

	// Query for Vulkan 1.3 features
	VkPhysicalDeviceFeatures2                       query_device_features2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
	VkPhysicalDeviceVulkan13Features                query_vulkan13_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT query_extended_dynamic_state_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT};
	query_device_features2.pNext  = &query_vulkan13_features;
	query_vulkan13_features.pNext = &query_extended_dynamic_state_features;

	vkGetPhysicalDeviceFeatures2(context.gpu, &query_device_features2);

	// Check if Physical device supports Vulkan 1.3 features
	if (!query_vulkan13_features.dynamicRendering)
	{
		throw std::runtime_error("Dynamic Rendering feature is missing");
	}

	if (!query_vulkan13_features.synchronization2)
	{
		throw std::runtime_error("Synchronization2 feature is missing");
	}

	if (!query_extended_dynamic_state_features.extendedDynamicState)
	{
		throw std::runtime_error("Extended Dynamic State feature is missing");
	}

	// Enable only specific Vulkan 1.3 features

	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT enable_extended_dynamic_state_features = {
	    .sType                = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
	    .extendedDynamicState = VK_TRUE};

	VkPhysicalDeviceVulkan13Features enable_vulkan13_features = {
	    .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
	    .pNext            = &enable_extended_dynamic_state_features,
	    .synchronization2 = VK_TRUE,
	    .dynamicRendering = VK_TRUE,
	};

	VkPhysicalDeviceFeatures2 enable_device_features2{
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
	    .pNext = &enable_vulkan13_features};
	// Create the logical device

	float queue_priority = 1.0f;

	// Create one queue
	VkDeviceQueueCreateInfo queue_info{
	    .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
	    .queueFamilyIndex = static_cast<uint32_t>(context.graphics_queue_index),
	    .queueCount       = 1,
	    .pQueuePriorities = &queue_priority};

	VkDeviceCreateInfo device_info{
	    .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	    .pNext                   = &enable_device_features2,
	    .queueCreateInfoCount    = 1,
	    .pQueueCreateInfos       = &queue_info,
	    .enabledExtensionCount   = vkb::to_u32(required_device_extensions.size()),
	    .ppEnabledExtensionNames = required_device_extensions.data()};

	VK_CHECK(vkCreateDevice(context.gpu, &device_info, nullptr, &context.device));
	volkLoadDevice(context.device);

	vkGetDeviceQueue(context.device, context.graphics_queue_index, 0, &context.queue);
}

/**
 * @brief Initializes the vertex buffer by creating it, allocating memory, binding the memory, and uploading vertex data.
 * @note This function must be called after the Vulkan device has been initialized.
 * @throws std::runtime_error if any Vulkan operation fails.
 */
void HelloTriangleV13::init_vertex_buffer()
{
	VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

	// Create the vertex buffer
	VkBufferCreateInfo vertext_buffer_info{
	    .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	    .flags       = 0,
	    .size        = buffer_size,
	    .usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

	VK_CHECK(vkCreateBuffer(context.device, &vertext_buffer_info, nullptr, &context.vertex_buffer));

	// Get memory requirements
	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(context.device, context.vertex_buffer, &memory_requirements);

	// Allocate memory for the buffer
	VkMemoryAllocateInfo alloc_info{
	    .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	    .allocationSize  = memory_requirements.size,
	    .memoryTypeIndex = find_memory_type(context.gpu, memory_requirements.memoryTypeBits,
	                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)};

	VK_CHECK(vkAllocateMemory(context.device, &alloc_info, nullptr, &context.vertex_buffer_memory));

	// Bind the buffer with the allocated memory
	VK_CHECK(vkBindBufferMemory(context.device, context.vertex_buffer, context.vertex_buffer_memory, 0));

	// Map the memory and copy the vertex data
	void *data;
	VK_CHECK(vkMapMemory(context.device, context.vertex_buffer_memory, 0, buffer_size, 0, &data));
	memcpy(data, vertices.data(), static_cast<size_t>(buffer_size));
	vkUnmapMemory(context.device, context.vertex_buffer_memory);
}

/**
 * @brief Finds a suitable memory type index for allocating memory.
 *
 * This function searches through the physical device's memory types to find one that matches
 * the requirements specified by `type_filter` and `properties`. It's typically used when allocating
 * memory for buffers or images, ensuring that the memory type supports the desired properties.
 *
 * @param physical_device The Vulkan physical device to query for memory properties.
 * @param type_filter A bitmask specifying the acceptable memory types.
 *                    This is usually obtained from `VkMemoryRequirements::memoryTypeBits`.
 * @param properties A bitmask specifying the desired memory properties,
 *                   such as `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT` or `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT`.
 * @return The index of a suitable memory type.
 * @throws std::runtime_error if no suitable memory type is found.
 */
uint32_t HelloTriangleV13::find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	// Structure to hold the physical device's memory properties
	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

	// Iterate over all memory types available on the physical device
	for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
	{
		// Check if the current memory type is acceptable based on the type_filter
		// The type_filter is a bitmask where each bit represents a memory type that is suitable
		if (type_filter & (1 << i))
		{
			// Check if the memory type has all the desired property flags
			// properties is a bitmask of the required memory properties
			if ((mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				// Found a suitable memory type; return its index
				return i;
			}
		}
	}

	// If no suitable memory type was found, throw an exception
	throw std::runtime_error("Failed to find suitable memory type.");
}
/**
 * @brief Initializes per frame data.
 * @param per_frame The data of a frame.
 */
void HelloTriangleV13::init_per_frame(PerFrame &per_frame)
{
	VkFenceCreateInfo info{
	    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	    .flags = VK_FENCE_CREATE_SIGNALED_BIT};
	VK_CHECK(vkCreateFence(context.device, &info, nullptr, &per_frame.queue_submit_fence));

	VkCommandPoolCreateInfo cmd_pool_info{
	    .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	    .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
	    .queueFamilyIndex = static_cast<uint32_t>(context.graphics_queue_index)};
	VK_CHECK(vkCreateCommandPool(context.device, &cmd_pool_info, nullptr, &per_frame.primary_command_pool));

	VkCommandBufferAllocateInfo cmd_buf_info{
	    .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	    .commandPool        = per_frame.primary_command_pool,
	    .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	    .commandBufferCount = 1};
	VK_CHECK(vkAllocateCommandBuffers(context.device, &cmd_buf_info, &per_frame.primary_command_buffer));
}

/**
 * @brief Tears down the frame data.
 * @param per_frame The data of a frame.
 */
void HelloTriangleV13::teardown_per_frame(PerFrame &per_frame)
{
	if (per_frame.queue_submit_fence != VK_NULL_HANDLE)
	{
		vkDestroyFence(context.device, per_frame.queue_submit_fence, nullptr);

		per_frame.queue_submit_fence = VK_NULL_HANDLE;
	}

	if (per_frame.primary_command_buffer != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(context.device, per_frame.primary_command_pool, 1, &per_frame.primary_command_buffer);

		per_frame.primary_command_buffer = VK_NULL_HANDLE;
	}

	if (per_frame.primary_command_pool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(context.device, per_frame.primary_command_pool, nullptr);

		per_frame.primary_command_pool = VK_NULL_HANDLE;
	}

	if (per_frame.swapchain_acquire_semaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(context.device, per_frame.swapchain_acquire_semaphore, nullptr);

		per_frame.swapchain_acquire_semaphore = VK_NULL_HANDLE;
	}

	if (per_frame.swapchain_release_semaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(context.device, per_frame.swapchain_release_semaphore, nullptr);

		per_frame.swapchain_release_semaphore = VK_NULL_HANDLE;
	}
}

/**
 * @brief Initializes the Vulkan swapchain.
 */
void HelloTriangleV13::init_swapchain()
{
	VkSurfaceCapabilitiesKHR surface_properties;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.gpu, context.surface, &surface_properties));

	VkSurfaceFormatKHR format = vkb::select_surface_format(context.gpu, context.surface);

	VkExtent2D swapchain_size;
	if (surface_properties.currentExtent.width == 0xFFFFFFFF)
	{
		swapchain_size.width  = context.swapchain_dimensions.width;
		swapchain_size.height = context.swapchain_dimensions.height;
	}
	else
	{
		swapchain_size = surface_properties.currentExtent;
	}

	// FIFO must be supported by all implementations.
	VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;

	// Determine the number of VkImage's to use in the swapchain.
	// Ideally, we desire to own 1 image at a time, the rest of the images can
	// either be rendered to and/or being queued up for display.
	uint32_t desired_swapchain_images = surface_properties.minImageCount + 1;
	if ((surface_properties.maxImageCount > 0) && (desired_swapchain_images > surface_properties.maxImageCount))
	{
		// Application must settle for fewer images than desired.
		desired_swapchain_images = surface_properties.maxImageCount;
	}

	// Figure out a suitable surface transform.
	VkSurfaceTransformFlagBitsKHR pre_transform;
	if (surface_properties.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		pre_transform = surface_properties.currentTransform;
	}

	VkSwapchainKHR old_swapchain = context.swapchain;

	// one bitmask needs to be set according to the priority of presentation engine
	VkCompositeAlphaFlagBitsKHR composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	if (surface_properties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
	{
		composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	}
	else if (surface_properties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
	{
		composite = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	}
	else if (surface_properties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
	{
		composite = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
	}
	else if (surface_properties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
	{
		composite = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
	}

	VkSwapchainCreateInfoKHR info{
	    .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
	    .surface          = context.surface,                            // The surface onto which images will be presented
	    .minImageCount    = desired_swapchain_images,                   // Minimum number of images in the swapchain (number of buffers)
	    .imageFormat      = format.format,                              // Format of the swapchain images (e.g., VK_FORMAT_B8G8R8A8_SRGB)
	    .imageColorSpace  = format.colorSpace,                          // Color space of the images (e.g., VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
	    .imageExtent      = swapchain_size,                             // Resolution of the swapchain images (width and height)
	    .imageArrayLayers = 1,                                          // Number of layers in each image (usually 1 unless stereoscopic)
	    .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,        // How the images will be used (as color attachments)
	    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,                  // Access mode of the images (exclusive to one queue family)
	    .preTransform     = pre_transform,                              // Transform to apply to images (e.g., rotation)
	    .compositeAlpha   = composite,                                  // Alpha blending to apply (e.g., opaque, pre-multiplied)
	    .presentMode      = swapchain_present_mode,                     // Presentation mode (e.g., vsync settings)
	    .clipped          = true,                                       // Whether to clip obscured pixels (improves performance)
	    .oldSwapchain     = old_swapchain                               // Handle to the old swapchain, if replacing an existing one
	};

	VK_CHECK(vkCreateSwapchainKHR(context.device, &info, nullptr, &context.swapchain));

	if (old_swapchain != VK_NULL_HANDLE)
	{
		for (VkImageView image_view : context.swapchain_image_views)
		{
			vkDestroyImageView(context.device, image_view, nullptr);
		}

		for (auto &per_frame : context.per_frame)
		{
			teardown_per_frame(per_frame);
		}

		context.swapchain_image_views.clear();

		vkDestroySwapchainKHR(context.device, old_swapchain, nullptr);
	}

	context.swapchain_dimensions = {swapchain_size.width, swapchain_size.height, format.format};

	uint32_t image_count;
	VK_CHECK(vkGetSwapchainImagesKHR(context.device, context.swapchain, &image_count, nullptr));

	/// The swapchain images.
	std::vector<VkImage> swapchain_images(image_count);
	VK_CHECK(vkGetSwapchainImagesKHR(context.device, context.swapchain, &image_count, swapchain_images.data()));

	// Store swapchain images
	context.swapchain_images = swapchain_images;

	// Initialize per-frame resources.
	// Every swapchain image has its own command pool and fence manager.
	// This makes it very easy to keep track of when we can reset command buffers and such.
	context.per_frame.clear();
	context.per_frame.resize(image_count);

	for (size_t i = 0; i < image_count; i++)
	{
		init_per_frame(context.per_frame[i]);
	}

	for (size_t i = 0; i < image_count; i++)
	{
		// Create an image view which we can render into.
		VkImageViewCreateInfo view_info{
		    .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		    .pNext            = nullptr,
		    .flags            = 0,
		    .image            = swapchain_images[i],
		    .viewType         = VK_IMAGE_VIEW_TYPE_2D,
		    .format           = context.swapchain_dimensions.format,
		    .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};

		VkImageView image_view;
		VK_CHECK(vkCreateImageView(context.device, &view_info, nullptr, &image_view));

		context.swapchain_image_views.push_back(image_view);
	}
}
/**
 * @brief Helper function to load a shader module.
 * @param path The path for the shader (relative to the assets directory).
 * @param shader_stage The shader stage flag specifying the type of shader (e.g., VK_SHADER_STAGE_VERTEX_BIT).
 * @returns A VkShaderModule handle. Aborts execution if shader creation fails.
 */
VkShaderModule HelloTriangleV13::load_shader_module(const std::string &path, VkShaderStageFlagBits shader_stage)
{
	auto spirv = vkb::fs::read_shader_binary_u32(path);

	VkShaderModuleCreateInfo module_info{
	    .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	    .codeSize = spirv.size() * sizeof(uint32_t),
	    .pCode    = spirv.data()};

	VkShaderModule shader_module;
	VK_CHECK(vkCreateShaderModule(context.device, &module_info, nullptr, &shader_module));

	return shader_module;
}

/**
 * @brief Initializes the Vulkan pipeline.
 */
void HelloTriangleV13::init_pipeline()
{
	// Create a blank pipeline layout.
	// We are not binding any resources to the pipeline in this first sample.
	VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	VK_CHECK(vkCreatePipelineLayout(context.device, &layout_info, nullptr, &context.pipeline_layout));

	// Define the vertex input binding description
	VkVertexInputBindingDescription binding_description{
	    .binding   = 0,
	    .stride    = sizeof(Vertex),
	    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};

	// Define the vertex input attribute descriptions
	std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions = {{
	    {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, position)},        // position
	    {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, color)}         // color
	}};

	// Create the vertex input state
	VkPipelineVertexInputStateCreateInfo vertex_input{.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	                                                  .vertexBindingDescriptionCount   = 1,
	                                                  .pVertexBindingDescriptions      = &binding_description,
	                                                  .vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size()),
	                                                  .pVertexAttributeDescriptions    = attribute_descriptions.data()};

	// Specify we will use triangle lists to draw geometry.
	VkPipelineInputAssemblyStateCreateInfo input_assembly{
	    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
	    .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	    .primitiveRestartEnable = VK_FALSE};

	// Specify rasterization state.
	VkPipelineRasterizationStateCreateInfo raster{
	    .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
	    .depthClampEnable        = VK_FALSE,
	    .rasterizerDiscardEnable = VK_FALSE,
	    .polygonMode             = VK_POLYGON_MODE_FILL,
	    .depthBiasEnable         = VK_FALSE,
	    .lineWidth               = 1.0f};

	// Specify that these states will be dynamic, i.e. not part of pipeline state object.
	std::vector<VkDynamicState> dynamic_states = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR,
	    VK_DYNAMIC_STATE_CULL_MODE,
	    VK_DYNAMIC_STATE_FRONT_FACE,
	    VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY};

	// Our attachment will write to all color channels, but no blending is enabled.
	VkPipelineColorBlendAttachmentState blend_attachment{
	    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

	VkPipelineColorBlendStateCreateInfo blend{
	    .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
	    .attachmentCount = 1,
	    .pAttachments    = &blend_attachment};

	// We will have one viewport and scissor box.
	VkPipelineViewportStateCreateInfo viewport{
	    .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
	    .viewportCount = 1,
	    .scissorCount  = 1};

	// Disable all depth testing.
	VkPipelineDepthStencilStateCreateInfo depth_stencil{
	    .sType          = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
	    .depthCompareOp = VK_COMPARE_OP_ALWAYS};

	// No multisampling.
	VkPipelineMultisampleStateCreateInfo multisample{
	    .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
	    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};

	VkPipelineDynamicStateCreateInfo dynamic_state_info{
	    .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
	    .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
	    .pDynamicStates    = dynamic_states.data()};

	// Load our SPIR-V shaders.

	// Samples support different shading languages, all of which are offline compiled to SPIR-V, the shader format that Vulkan uses.
	// The shading language to load for can be selected via command line
	std::string shader_folder{""};
	switch (get_shading_language())
	{
		case vkb::ShadingLanguage::HLSL:
			shader_folder = "hlsl";
			break;
		case vkb::ShadingLanguage::SLANG:
			shader_folder = "slang";
			break;
		default:
			shader_folder = "glsl";
	}

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {{
	    {.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	     .stage  = VK_SHADER_STAGE_VERTEX_BIT,
	     .module = load_shader_module("hello_triangle_1_3/" + shader_folder + "/triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
	     .pName  = "main"},        // Vertex shader stage
	    {
	        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	        .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
	        .module = load_shader_module("hello_triangle_1_3/" + shader_folder + "/triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
	        .pName  = "main"}        // Fragment shader stage
	}};

	// Pipeline rendering info (for dynamic rendering).
	VkPipelineRenderingCreateInfo pipeline_rendering_info{
	    .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
	    .colorAttachmentCount    = 1,
	    .pColorAttachmentFormats = &context.swapchain_dimensions.format};

	// Create the graphics pipeline.
	VkGraphicsPipelineCreateInfo pipe{
	    .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
	    .pNext               = &pipeline_rendering_info,
	    .stageCount          = vkb::to_u32(shader_stages.size()),
	    .pStages             = shader_stages.data(),
	    .pVertexInputState   = &vertex_input,
	    .pInputAssemblyState = &input_assembly,
	    .pViewportState      = &viewport,
	    .pRasterizationState = &raster,
	    .pMultisampleState   = &multisample,
	    .pDepthStencilState  = &depth_stencil,
	    .pColorBlendState    = &blend,
	    .pDynamicState       = &dynamic_state_info,
	    .layout              = context.pipeline_layout,        // We need to specify the pipeline layout description up front as well.
	    .renderPass          = VK_NULL_HANDLE,                 // Since we are using dynamic rendering this will set as null
	    .subpass             = 0,
	};

	VK_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipe, nullptr, &context.pipeline));

	// Pipeline is baked, we can delete the shader modules now.
	vkDestroyShaderModule(context.device, shader_stages[0].module, nullptr);
	vkDestroyShaderModule(context.device, shader_stages[1].module, nullptr);
}

/**
 * @brief Acquires an image from the swapchain.
 * @param[out] image The swapchain index for the acquired image.
 * @returns Vulkan result code
 */
VkResult HelloTriangleV13::acquire_next_swapchain_image(uint32_t *image)
{
	VkSemaphore acquire_semaphore;
	if (context.recycled_semaphores.empty())
	{
		VkSemaphoreCreateInfo info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		VK_CHECK(vkCreateSemaphore(context.device, &info, nullptr, &acquire_semaphore));
	}
	else
	{
		acquire_semaphore = context.recycled_semaphores.back();
		context.recycled_semaphores.pop_back();
	}

	VkResult res = vkAcquireNextImageKHR(context.device, context.swapchain, UINT64_MAX, acquire_semaphore, VK_NULL_HANDLE, image);

	if (res != VK_SUCCESS)
	{
		context.recycled_semaphores.push_back(acquire_semaphore);
		return res;
	}

	// If we have outstanding fences for this swapchain image, wait for them to complete first.
	// After begin frame returns, it is safe to reuse or delete resources which
	// were used previously.
	//
	// We wait for fences which completes N frames earlier, so we do not stall,
	// waiting for all GPU work to complete before this returns.
	// Normally, this doesn't really block at all,
	// since we're waiting for old frames to have been completed, but just in case.
	if (context.per_frame[*image].queue_submit_fence != VK_NULL_HANDLE)
	{
		vkWaitForFences(context.device, 1, &context.per_frame[*image].queue_submit_fence, true, UINT64_MAX);
		vkResetFences(context.device, 1, &context.per_frame[*image].queue_submit_fence);
	}

	if (context.per_frame[*image].primary_command_pool != VK_NULL_HANDLE)
	{
		vkResetCommandPool(context.device, context.per_frame[*image].primary_command_pool, 0);
	}

	// Recycle the old semaphore back into the semaphore manager.
	VkSemaphore old_semaphore = context.per_frame[*image].swapchain_acquire_semaphore;

	if (old_semaphore != VK_NULL_HANDLE)
	{
		context.recycled_semaphores.push_back(old_semaphore);
	}

	context.per_frame[*image].swapchain_acquire_semaphore = acquire_semaphore;

	return VK_SUCCESS;
}

/**
 * @brief Renders a triangle to the specified swapchain image.
 * @param swapchain_index The swapchain index for the image being rendered.
 */
void HelloTriangleV13::render_triangle(uint32_t swapchain_index)
{
	// Allocate or re-use a primary command buffer.
	VkCommandBuffer cmd = context.per_frame[swapchain_index].primary_command_buffer;

	// We will only submit this once before it's recycled.
	VkCommandBufferBeginInfo begin_info{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

	// Begin command recording
	VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

	// Before starting rendering, transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
	transition_image_layout(
	    cmd,
	    context.swapchain_images[swapchain_index],
	    VK_IMAGE_LAYOUT_UNDEFINED,
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	    0,                                                     // srcAccessMask (no need to wait for previous operations)
	    VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,                // dstAccessMask
	    VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,                   // srcStage
	    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT        // dstStage
	);
	// Set clear color values.
	VkClearValue clear_value{
	    .color = {{0.01f, 0.01f, 0.033f, 1.0f}}};

	// Set up the rendering attachment info
	VkRenderingAttachmentInfo color_attachment{
	    .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
	    .imageView   = context.swapchain_image_views[swapchain_index],
	    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	    .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
	    .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
	    .clearValue  = clear_value};

	// Begin rendering
	VkRenderingInfo rendering_info{
	    .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
	    .renderArea           = {                         // Initialize the nested `VkRect2D` structure
	                             .offset = {0, 0},        // Initialize the `VkOffset2D` inside `renderArea`
	                             .extent = {              // Initialize the `VkExtent2D` inside `renderArea`
	                                        .width  = context.swapchain_dimensions.width,
	                                        .height = context.swapchain_dimensions.height}},
	              .layerCount = 1,
	    .colorAttachmentCount = 1,
	    .pColorAttachments    = &color_attachment};

	vkCmdBeginRendering(cmd, &rendering_info);

	// Bind the graphics pipeline.
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, context.pipeline);

	// Set dynamic states

	// Set viewport dynamically
	VkViewport vp{
	    .width    = static_cast<float>(context.swapchain_dimensions.width),
	    .height   = static_cast<float>(context.swapchain_dimensions.height),
	    .minDepth = 0.0f,
	    .maxDepth = 1.0f};

	vkCmdSetViewport(cmd, 0, 1, &vp);

	// Set scissor dynamically
	VkRect2D scissor{
	    .extent = {
	        .width  = context.swapchain_dimensions.width,
	        .height = context.swapchain_dimensions.height}};

	vkCmdSetScissor(cmd, 0, 1, &scissor);

	// Since we declared VK_DYNAMIC_STATE_CULL_MODE as dynamic in the pipeline,
	// we need to set the cull mode here. VK_CULL_MODE_NONE disables face culling,
	// meaning both front and back faces will be rendered.
	vkCmdSetCullMode(cmd, VK_CULL_MODE_NONE);

	// Since we declared VK_DYNAMIC_STATE_FRONT_FACE as dynamic,
	// we need to specify the winding order considered as the front face.
	// VK_FRONT_FACE_CLOCKWISE indicates that vertices defined in clockwise order
	// are considered front-facing.
	vkCmdSetFrontFace(cmd, VK_FRONT_FACE_CLOCKWISE);

	// Since we declared VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY as dynamic,
	// we need to set the primitive topology here. VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
	// tells Vulkan that the input vertex data should be interpreted as a list of triangles.
	vkCmdSetPrimitiveTopology(cmd, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	// Bind the vertex buffer
	VkDeviceSize offset = {0};
	vkCmdBindVertexBuffers(cmd, 0, 1, &context.vertex_buffer, &offset);

	// Draw three vertices with one instance.
	vkCmdDraw(cmd, vertices.size(), 1, 0, 0);

	// Complete rendering.
	vkCmdEndRendering(cmd);

	// After rendering , transition the swapchain image to PRESENT_SRC
	transition_image_layout(
	    cmd,
	    context.swapchain_images[swapchain_index],
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	    VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,                 // srcAccessMask
	    0,                                                      // dstAccessMask
	    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,        // srcStage
	    VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT                  // dstStage
	);

	// Complete the command buffer.
	VK_CHECK(vkEndCommandBuffer(cmd));

	// Submit it to the queue with a release semaphore.
	if (context.per_frame[swapchain_index].swapchain_release_semaphore == VK_NULL_HANDLE)
	{
		VkSemaphoreCreateInfo semaphore_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		VK_CHECK(vkCreateSemaphore(context.device, &semaphore_info, nullptr,
		                           &context.per_frame[swapchain_index].swapchain_release_semaphore));
	}

	// Using TOP_OF_PIPE here to ensure that the command buffer does not begin executing any pipeline stages
	// (including the layout transition) until the swapchain image is actually acquired (signaled by the semaphore).
	// This prevents the GPU from starting operations too early and guarantees that the image is ready
	// before any rendering commands run.
	VkPipelineStageFlags wait_stage{VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT};

	VkSubmitInfo info{
	    .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	    .waitSemaphoreCount   = 1,
	    .pWaitSemaphores      = &context.per_frame[swapchain_index].swapchain_acquire_semaphore,
	    .pWaitDstStageMask    = &wait_stage,
	    .commandBufferCount   = 1,
	    .pCommandBuffers      = &cmd,
	    .signalSemaphoreCount = 1,
	    .pSignalSemaphores    = &context.per_frame[swapchain_index].swapchain_release_semaphore};

	// Submit command buffer to graphics queue
	VK_CHECK(vkQueueSubmit(context.queue, 1, &info, context.per_frame[swapchain_index].queue_submit_fence));
}

/**
 * @brief Presents an image to the swapchain.
 * @param index The swapchain index previously obtained from @ref acquire_next_swapchain_image.
 * @returns Vulkan result code
 */
VkResult HelloTriangleV13::present_image(uint32_t index)
{
	VkPresentInfoKHR present{
	    .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
	    .waitSemaphoreCount = 1,
	    .pWaitSemaphores    = &context.per_frame[index].swapchain_release_semaphore,
	    .swapchainCount     = 1,
	    .pSwapchains        = &context.swapchain,
	    .pImageIndices      = &index,
	};

	// Present swapchain image
	return vkQueuePresentKHR(context.queue, &present);
}

/**
 * @brief Transitions an image layout in a Vulkan command buffer.
 * @param cmd The command buffer to record the barrier into.
 * @param image The Vulkan image to transition.
 * @param oldLayout The current layout of the image.
 * @param newLayout The desired new layout of the image.
 * @param srcAccessMask The source access mask, specifying which access types are being transitioned from.
 * @param dstAccessMask The destination access mask, specifying which access types are being transitioned to.
 * @param srcStage The pipeline stage that must happen before the transition.
 * @param dstStage The pipeline stage that must happen after the transition.
 */
void HelloTriangleV13::transition_image_layout(
    VkCommandBuffer       cmd,
    VkImage               image,
    VkImageLayout         oldLayout,
    VkImageLayout         newLayout,
    VkAccessFlags2        srcAccessMask,
    VkAccessFlags2        dstAccessMask,
    VkPipelineStageFlags2 srcStage,
    VkPipelineStageFlags2 dstStage)
{
	// Initialize the VkImageMemoryBarrier2 structure
	VkImageMemoryBarrier2 image_barrier{
	    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,

	    // Specify the pipeline stages and access masks for the barrier
	    .srcStageMask  = srcStage,             // Source pipeline stage mask
	    .srcAccessMask = srcAccessMask,        // Source access mask
	    .dstStageMask  = dstStage,             // Destination pipeline stage mask
	    .dstAccessMask = dstAccessMask,        // Destination access mask

	    // Specify the old and new layouts of the image
	    .oldLayout = oldLayout,        // Current layout of the image
	    .newLayout = newLayout,        // Target layout of the image

	    // We are not changing the ownership between queues
	    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

	    // Specify the image to be affected by this barrier
	    .image = image,

	    // Define the subresource range (which parts of the image are affected)
	    .subresourceRange = {
	        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,        // Affects the color aspect of the image
	        .baseMipLevel   = 0,                                // Start at mip level 0
	        .levelCount     = 1,                                // Number of mip levels affected
	        .baseArrayLayer = 0,                                // Start at array layer 0
	        .layerCount     = 1                                 // Number of array layers affected
	    }};

	// Initialize the VkDependencyInfo structure
	VkDependencyInfo dependency_info{
	    .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	    .dependencyFlags         = 0,                    // No special dependency flags
	    .imageMemoryBarrierCount = 1,                    // Number of image memory barriers
	    .pImageMemoryBarriers    = &image_barrier        // Pointer to the image memory barrier(s)
	};

	// Record the pipeline barrier into the command buffer
	vkCmdPipelineBarrier2(cmd, &dependency_info);
}

HelloTriangleV13::~HelloTriangleV13()
{
	// Don't release anything until the GPU is completely idle.
	if (context.device != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(context.device);
	}

	for (auto &per_frame : context.per_frame)
	{
		teardown_per_frame(per_frame);
	}

	context.per_frame.clear();

	for (auto semaphore : context.recycled_semaphores)
	{
		vkDestroySemaphore(context.device, semaphore, nullptr);
	}

	if (context.pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(context.device, context.pipeline, nullptr);
	}

	if (context.pipeline_layout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(context.device, context.pipeline_layout, nullptr);
	}

	for (VkImageView image_view : context.swapchain_image_views)
	{
		vkDestroyImageView(context.device, image_view, nullptr);
	}

	if (context.swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(context.device, context.swapchain, nullptr);
		context.swapchain = VK_NULL_HANDLE;
	}

	if (context.surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
		context.surface = VK_NULL_HANDLE;
	}

	if (context.vertex_buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(context.device, context.vertex_buffer, nullptr);
		context.vertex_buffer = VK_NULL_HANDLE;
	}

	if (context.vertex_buffer_memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(context.device, context.vertex_buffer_memory, nullptr);
		context.vertex_buffer_memory = VK_NULL_HANDLE;
	}

	if (context.device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(context.device, nullptr);
		context.device = VK_NULL_HANDLE;
	}

	if (context.debug_callback != VK_NULL_HANDLE)
	{
		vkDestroyDebugUtilsMessengerEXT(context.instance, context.debug_callback, nullptr);
		context.debug_callback = VK_NULL_HANDLE;
	}

	vk_instance.reset();
}

bool HelloTriangleV13::prepare(const vkb::ApplicationOptions &options)
{
	assert(options.window != nullptr);

	init_instance();

	vk_instance = std::make_unique<vkb::core::InstanceC>(context.instance);

	context.surface                     = options.window->create_surface(*vk_instance);
	auto &extent                        = options.window->get_extent();
	context.swapchain_dimensions.width  = extent.width;
	context.swapchain_dimensions.height = extent.height;

	if (!context.surface)
	{
		throw std::runtime_error("Failed to create window surface.");
	}

	init_device();

	init_vertex_buffer();

	init_swapchain();

	// Create the necessary objects for rendering.
	init_pipeline();

	return true;
}

void HelloTriangleV13::update(float delta_time)
{
	uint32_t index;

	auto res = acquire_next_swapchain_image(&index);

	// Handle outdated error in acquire.
	if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
	{
		if (!resize(context.swapchain_dimensions.width, context.swapchain_dimensions.height))
		{
			LOGI("Resize failed");
		}
		res = acquire_next_swapchain_image(&index);
	}

	if (res != VK_SUCCESS)
	{
		vkQueueWaitIdle(context.queue);
		return;
	}

	render_triangle(index);
	res = present_image(index);

	// Handle Outdated error in present.
	if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
	{
		if (!resize(context.swapchain_dimensions.width, context.swapchain_dimensions.height))
		{
			LOGI("Resize failed");
		}
	}
	else if (res != VK_SUCCESS)
	{
		LOGE("Failed to present swapchain image.");
	}
}

bool HelloTriangleV13::resize(const uint32_t, const uint32_t)
{
	if (context.device == VK_NULL_HANDLE)
	{
		return false;
	}

	VkSurfaceCapabilitiesKHR surface_properties;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.gpu, context.surface, &surface_properties));

	// Only rebuild the swapchain if the dimensions have changed
	if (surface_properties.currentExtent.width == context.swapchain_dimensions.width &&
	    surface_properties.currentExtent.height == context.swapchain_dimensions.height)
	{
		return false;
	}

	vkDeviceWaitIdle(context.device);

	init_swapchain();
	return true;
}

std::unique_ptr<vkb::Application> create_hello_triangle_1_3()
{
	return std::make_unique<HelloTriangleV13>();
}
