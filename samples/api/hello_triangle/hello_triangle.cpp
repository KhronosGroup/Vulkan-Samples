/* Copyright (c) 2018-2025, Arm Limited and Contributors
 * Copyright (c) 2025, Sascha Willems
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

#include "hello_triangle.h"

#include "common/vk_common.h"
#include "core/util/logging.hpp"
#include "filesystem/legacy.h"
#include "platform/window.h"

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
/// @brief A debug callback used to report messages from the validation layers. See instance creation for details on how this is set up
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                     const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                                                     void                                       *user_data)
{
	(void) user_data;

	if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		LOGE("{} Validation Layer: Error: {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage)
	}
	else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		LOGE("{} Validation Layer: Warning: {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage)
	}
	else if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
	{
		LOGI("{} Validation Layer: Performance warning: {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage)
	}
	else
	{
		LOGI("{} Validation Layer: Information: {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage)
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
bool HelloTriangle::validate_extensions(const std::vector<const char *>          &required,
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
			return false;
		}
	}

	return true;
}

/**
 * @brief Initializes the Vulkan instance.
 */
void HelloTriangle::init_instance()
{
	LOGI("Initializing vulkan instance.");

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
	// Validation layers help finding wrong api usage, we enable them when explicitly requested or in debug builds
	// For this we use the debug utils extension if it is supported
	bool has_debug_utils = false;
	for (const auto &ext : available_instance_extensions)
	{
		if (strcmp(ext.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
		{
			has_debug_utils = true;
			required_instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			break;
		}
	}
	if (!has_debug_utils)
	{
		LOGW("{} not supported or available", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		LOGW("Make sure to compile the sample in debug mode and/or enable the validation layers");
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
	    .pApplicationName = "Hello Triangle",
	    .pEngineName      = "Vulkan Samples",
	    .apiVersion       = VK_API_VERSION_1_1};

	VkInstanceCreateInfo instance_info{
	    .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
	    .pApplicationInfo        = &app,
	    .enabledLayerCount       = static_cast<uint32_t>(requested_instance_layers.size()),
	    .ppEnabledLayerNames     = requested_instance_layers.data(),
	    .enabledExtensionCount   = static_cast<uint32_t>(required_instance_extensions.size()),
	    .ppEnabledExtensionNames = required_instance_extensions.data()};

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	// Validation layers help finding wrong api usage, we enable them when explicitly requested or in debug builds
	// For this we use the debug utils extension if it is supported
	VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info = {.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	if (has_debug_utils)
	{
		debug_utils_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		debug_utils_create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		debug_utils_create_info.pfnUserCallback = debug_callback;

		instance_info.pNext = &debug_utils_create_info;
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
		VK_CHECK(vkCreateDebugUtilsMessengerEXT(context.instance, &debug_utils_create_info, nullptr, &context.debug_callback));
	}
#endif
}

/**
 * @brief Initializes the Vulkan physical device and logical device.
 */
void HelloTriangle::init_device()
{
	LOGI("Initializing vulkan device.");

	uint32_t gpu_count = 0;
	VK_CHECK(vkEnumeratePhysicalDevices(context.instance, &gpu_count, nullptr));

	if (gpu_count < 1)
	{
		throw std::runtime_error("No physical device found.");
	}

	// For simplicity, the sample selects the first gpu that has a graphics and present queue
	std::vector<VkPhysicalDevice> gpus(gpu_count);
	VK_CHECK(vkEnumeratePhysicalDevices(context.instance, &gpu_count, gpus.data()));

	for (size_t i = 0; i < gpu_count && (context.graphics_queue_index < 0); i++)
	{
		context.gpu = gpus[i];

		uint32_t queue_family_count;
		vkGetPhysicalDeviceQueueFamilyProperties(context.gpu, &queue_family_count, nullptr);

		if (queue_family_count < 1)
		{
			throw std::runtime_error("No queue family found.");
		}

		std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(context.gpu, &queue_family_count, queue_family_properties.data());

		for (uint32_t i = 0; i < queue_family_count; i++)
		{
			VkBool32 supports_present;
			vkGetPhysicalDeviceSurfaceSupportKHR(context.gpu, i, context.surface, &supports_present);

			// Find a queue family which supports graphics and presentation.
			if ((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && supports_present)
			{
				context.graphics_queue_index = i;
				break;
			}
		}
	}

	if (context.graphics_queue_index < 0)
	{
		throw std::runtime_error("Did not find suitable device with a queue that supports graphics and presentation.");
	}

	uint32_t device_extension_count;
	VK_CHECK(vkEnumerateDeviceExtensionProperties(context.gpu, nullptr, &device_extension_count, nullptr));
	std::vector<VkExtensionProperties> device_extensions(device_extension_count);
	VK_CHECK(vkEnumerateDeviceExtensionProperties(context.gpu, nullptr, &device_extension_count, device_extensions.data()));

	// Since this sample has visual output, the device needs to support the swapchain extension
	std::vector<const char *> required_device_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	// Shaders generated by Slang require a certain SPIR-V environment that can't be satisfied by Vulkan 1.0, so we need to expliclity up that to at least 1.1 and enable some required extensions
	if (get_shading_language() == vkb::ShadingLanguage::SLANG)
	{
		required_device_extensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
		required_device_extensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
		required_device_extensions.push_back(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME);
	}

	if (!validate_extensions(required_device_extensions, device_extensions))
	{
		throw std::runtime_error("Required device extensions are missing.");
	}

#if (defined(VKB_ENABLE_PORTABILITY))
	// VK_KHR_portability_subset must be enabled if present in the implementation (e.g on macOS/iOS with beta extensions enabled)
	if (std::ranges::any_of(device_extensions,
	                        [](VkExtensionProperties const &extension) { return strcmp(extension.extensionName, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) == 0; }))
	{
		required_device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
	}
#endif

	// The sample uses a single graphics queue
	const float queue_priority = 1.0f;

	VkDeviceQueueCreateInfo queue_info{
	    .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
	    .queueFamilyIndex = static_cast<uint32_t>(context.graphics_queue_index),
	    .queueCount       = 1,
	    .pQueuePriorities = &queue_priority};

	VkDeviceCreateInfo device_info{
	    .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	    .queueCreateInfoCount    = 1,
	    .pQueueCreateInfos       = &queue_info,
	    .enabledExtensionCount   = static_cast<uint32_t>(required_device_extensions.size()),
	    .ppEnabledExtensionNames = required_device_extensions.data()};

	VK_CHECK(vkCreateDevice(context.gpu, &device_info, nullptr, &context.device));
	volkLoadDevice(context.device);

	vkGetDeviceQueue(context.device, context.graphics_queue_index, 0, &context.queue);

	// This sample uses the Vulkan Memory Alloctor (VMA), which needs to be set up
	VmaVulkanFunctions vma_vulkan_func{
	    .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
	    .vkGetDeviceProcAddr   = vkGetDeviceProcAddr};

	VmaAllocatorCreateInfo allocator_info{
	    .physicalDevice   = context.gpu,
	    .device           = context.device,
	    .pVulkanFunctions = &vma_vulkan_func,
	    .instance         = context.instance};

	VkResult result = vmaCreateAllocator(&allocator_info, &context.vma_allocator);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create allocator for VMA allocator");
	}
}

/**
 * @brief Initializes the vertex buffer by creating it, allocating memory, binding the memory, and uploading vertex data.
 * @note This function must be called after the Vulkan device has been initialized.
 * @throws std::runtime_error if any Vulkan operation fails.
 */
void HelloTriangle::init_vertex_buffer()
{
	// Vertex data for a single colored triangle
	const std::vector<Vertex> vertices = {
	    {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
	    {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
	    {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};

	const VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

	// Copy Vertex data to a buffer accessible by the device

	VkBufferCreateInfo buffer_info{
	    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	    .size  = buffer_size,
	    .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT};

	// We use the Vulkan Memory Allocator to find a memory type that can be written and mapped from the host
	// On most setups this will return a memory type that resides in VRAM and is accessible from the host
	VmaAllocationCreateInfo buffer_alloc_ci{
	    .flags         = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
	    .usage         = VMA_MEMORY_USAGE_AUTO,
	    .requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

	VmaAllocationInfo buffer_alloc_info{};
	vmaCreateBuffer(context.vma_allocator, &buffer_info, &buffer_alloc_ci, &vertex_buffer, &vertex_buffer_allocation, &buffer_alloc_info);
	if (buffer_alloc_info.pMappedData)
	{
		memcpy(buffer_alloc_info.pMappedData, vertices.data(), buffer_size);
	}
	else
	{
		throw std::runtime_error("Could not map vertex buffer.");
	}
}

/**
 * @brief Initializes per frame data.
 * @param per_frame The data of a frame.
 */
void HelloTriangle::init_per_frame(PerFrame &per_frame)
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
void HelloTriangle::teardown_per_frame(PerFrame &per_frame)
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
void HelloTriangle::init_swapchain()
{
	VkSurfaceCapabilitiesKHR surface_properties;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.gpu, context.surface, &surface_properties));

	VkSurfaceFormatKHR format = vkb::select_surface_format(context.gpu, context.surface);

	VkExtent2D swapchain_size{};
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

	// Find a supported composite type.
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
	    .surface          = context.surface,
	    .minImageCount    = desired_swapchain_images,
	    .imageFormat      = format.format,
	    .imageColorSpace  = format.colorSpace,
	    .imageExtent      = swapchain_size,
	    .imageArrayLayers = 1,
	    .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    .preTransform     = pre_transform,
	    .compositeAlpha   = composite,
	    .presentMode      = swapchain_present_mode,
	    .clipped          = true,
	    .oldSwapchain     = old_swapchain};

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
 * @brief Initializes the Vulkan render pass.
 */
void HelloTriangle::init_render_pass()
{
	VkAttachmentDescription attachment{
	    .format         = context.swapchain_dimensions.format,        // Backbuffer format.
	    .samples        = VK_SAMPLE_COUNT_1_BIT,                      // Not multisampled.
	    .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,                // When starting the frame, we want tiles to be cleared.
	    .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,               // When ending the frame, we want tiles to be written out.
	    .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,            // Don't care about stencil since we're not using it.
	    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,           // Don't care about stencil since we're not using it.
	    .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,                  // The image layout will be undefined when the render pass begins.
	    .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR             // After the render pass is complete, we will transition to PRESENT_SRC_KHR layout.
	};

	// We have one subpass. This subpass has one color attachment.
	// While executing this subpass, the attachment will be in attachment optimal layout.
	VkAttachmentReference color_ref = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

	// We will end up with two transitions.
	// The first one happens right before we start subpass #0, where
	// UNDEFINED is transitioned into COLOR_ATTACHMENT_OPTIMAL.
	// The final layout in the render pass attachment states PRESENT_SRC_KHR, so we
	// will get a final transition from COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR.
	VkSubpassDescription subpass{
	    .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
	    .colorAttachmentCount = 1,
	    .pColorAttachments    = &color_ref,
	};

	// Create a dependency to external events.
	// We need to wait for the WSI semaphore to signal.
	// Only pipeline stages which depend on COLOR_ATTACHMENT_OUTPUT_BIT will
	// actually wait for the semaphore, so we must also wait for that pipeline stage.
	VkSubpassDependency dependency{
	    .srcSubpass   = VK_SUBPASS_EXTERNAL,
	    .dstSubpass   = 0,
	    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	// Since we changed the image layout, we need to make the memory visible to
	// color attachment to modify.
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// Finally, create the renderpass.
	VkRenderPassCreateInfo rp_info{
	    .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
	    .attachmentCount = 1,
	    .pAttachments    = &attachment,
	    .subpassCount    = 1,
	    .pSubpasses      = &subpass,
	    .dependencyCount = 1,
	    .pDependencies   = &dependency};

	VK_CHECK(vkCreateRenderPass(context.device, &rp_info, nullptr, &context.render_pass));
}

/**
 * @brief Helper function to load a shader module from an offline-compiled SPIR-V file
 * @param path The path for the shader (relative to the assets directory).
 * @returns A VkShaderModule handle. Aborts execution if shader creation fails.
 */
VkShaderModule HelloTriangle::load_shader_module(const std::string &path)
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
void HelloTriangle::init_pipeline()
{
	// Create a blank pipeline layout.
	// We are not binding any resources to the pipeline in this first sample.
	VkPipelineLayoutCreateInfo layout_info{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	VK_CHECK(vkCreatePipelineLayout(context.device, &layout_info, nullptr, &context.pipeline_layout));

	// The Vertex input properties define the interface between the vertex buffer and the vertex shader.

	// Specify we will use triangle lists to draw geometry.
	VkPipelineInputAssemblyStateCreateInfo input_assembly{
	    .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
	    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};

	// Define the vertex input binding.
	VkVertexInputBindingDescription binding_description{
	    .binding   = 0,
	    .stride    = sizeof(Vertex),
	    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};

	// Define the vertex input attribute.
	std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions{
	    {{.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, position)},
	     {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, color)}}};

	// Define the pipeline vertex input.
	VkPipelineVertexInputStateCreateInfo vertex_input{
	    .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	    .vertexBindingDescriptionCount   = 1,
	    .pVertexBindingDescriptions      = &binding_description,
	    .vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size()),
	    .pVertexAttributeDescriptions    = attribute_descriptions.data()};

	// Specify rasterization state.
	VkPipelineRasterizationStateCreateInfo raster{
	    .sType     = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
	    .cullMode  = VK_CULL_MODE_BACK_BIT,
	    .frontFace = VK_FRONT_FACE_CLOCKWISE,
	    .lineWidth = 1.0f};

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
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};

	// No multisampling.
	VkPipelineMultisampleStateCreateInfo multisample{
	    .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
	    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};

	// Specify that these states will be dynamic, i.e. not part of pipeline state object.
	std::array<VkDynamicState, 2> dynamics{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamic{
	    .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
	    .dynamicStateCount = static_cast<uint32_t>(dynamics.size()),
	    .pDynamicStates    = dynamics.data()};

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

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};

	// Vertex stage of the pipeline
	shader_stages[0] = {
	    .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	    .stage  = VK_SHADER_STAGE_VERTEX_BIT,
	    .module = load_shader_module("hello_triangle/" + shader_folder + "/triangle.vert.spv"),
	    .pName  = "main"};

	// Fragment stage of the pipeline
	shader_stages[1] = {
	    .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	    .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
	    .module = load_shader_module("hello_triangle/" + shader_folder + "/triangle.frag.spv"),
	    .pName  = "main"};

	VkGraphicsPipelineCreateInfo pipe{
	    .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
	    .stageCount          = static_cast<uint32_t>(shader_stages.size()),
	    .pStages             = shader_stages.data(),
	    .pVertexInputState   = &vertex_input,
	    .pInputAssemblyState = &input_assembly,
	    .pViewportState      = &viewport,
	    .pRasterizationState = &raster,
	    .pMultisampleState   = &multisample,
	    .pDepthStencilState  = &depth_stencil,
	    .pColorBlendState    = &blend,
	    .pDynamicState       = &dynamic,
	    .layout              = context.pipeline_layout,        // We need to specify the pipeline layout up front
	    .renderPass          = context.render_pass             // We need to specify the render pass up front
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
VkResult HelloTriangle::acquire_next_image(uint32_t *image)
{
	VkSemaphore acquire_semaphore;
	if (context.recycled_semaphores.empty())
	{
		VkSemaphoreCreateInfo info = {
		    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
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
void HelloTriangle::render_triangle(uint32_t swapchain_index)
{
	// Render to this framebuffer.
	VkFramebuffer framebuffer = context.swapchain_framebuffers[swapchain_index];

	// Allocate or re-use a primary command buffer.
	VkCommandBuffer cmd = context.per_frame[swapchain_index].primary_command_buffer;

	// We will only submit this once before it's recycled.
	VkCommandBufferBeginInfo begin_info{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
	// Begin command recording
	vkBeginCommandBuffer(cmd, &begin_info);

	// Set clear color values.
	VkClearValue clear_value{
	    .color = {{0.01f, 0.01f, 0.033f, 1.0f}}};

	// Begin the render pass.
	VkRenderPassBeginInfo rp_begin{
	    .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
	    .renderPass      = context.render_pass,
	    .framebuffer     = framebuffer,
	    .renderArea      = {.extent = {.width = context.swapchain_dimensions.width, .height = context.swapchain_dimensions.height}},
	    .clearValueCount = 1,
	    .pClearValues    = &clear_value};
	// We will add draw commands in the same command buffer.
	vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

	// Bind the graphics pipeline.
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, context.pipeline);

	VkViewport vp{
	    .width    = static_cast<float>(context.swapchain_dimensions.width),
	    .height   = static_cast<float>(context.swapchain_dimensions.height),
	    .minDepth = 0.0f,
	    .maxDepth = 1.0f};
	// Set viewport dynamically
	vkCmdSetViewport(cmd, 0, 1, &vp);

	VkRect2D scissor{
	    .extent = {.width = context.swapchain_dimensions.width, .height = context.swapchain_dimensions.height}};
	// Set scissor dynamically
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	// Bind the vertex buffer to source the draw calls from.
	VkDeviceSize offset = {0};
	vkCmdBindVertexBuffers(cmd, 0, 1, &vertex_buffer, &offset);

	// Draw three vertices with one instance from the currently bound vertex bound.
	vkCmdDraw(cmd, 3, 1, 0, 0);

	// Complete render pass.
	vkCmdEndRenderPass(cmd);

	// Complete the command buffer.
	VK_CHECK(vkEndCommandBuffer(cmd));

	// Submit it to the queue with a release semaphore.
	if (context.per_frame[swapchain_index].swapchain_release_semaphore == VK_NULL_HANDLE)
	{
		VkSemaphoreCreateInfo semaphore_info{
		    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		VK_CHECK(vkCreateSemaphore(context.device, &semaphore_info, nullptr, &context.per_frame[swapchain_index].swapchain_release_semaphore));
	}

	VkPipelineStageFlags wait_stage{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

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
 * @param index The swapchain index previously obtained from @ref acquire_next_image.
 * @returns Vulkan result code
 */
VkResult HelloTriangle::present_image(uint32_t index)
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
 * @brief Initializes the Vulkan framebuffers.
 */
void HelloTriangle::init_framebuffers()
{
	context.swapchain_framebuffers.clear();

	// Create framebuffer for each swapchain image view
	for (auto &image_view : context.swapchain_image_views)
	{
		// Build the framebuffer.
		VkFramebufferCreateInfo fb_info{
		    .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		    .renderPass      = context.render_pass,
		    .attachmentCount = 1,
		    .pAttachments    = &image_view,
		    .width           = context.swapchain_dimensions.width,
		    .height          = context.swapchain_dimensions.height,
		    .layers          = 1};

		VkFramebuffer framebuffer;
		VK_CHECK(vkCreateFramebuffer(context.device, &fb_info, nullptr, &framebuffer));

		context.swapchain_framebuffers.push_back(framebuffer);
	}
}

HelloTriangle::HelloTriangle()
{
}

HelloTriangle::~HelloTriangle()
{
	// When destroying the application, we need to make sure the GPU is no longer accessing any resources
	// This is done by doing a device wait idle, which blocks until the GPU signals
	vkDeviceWaitIdle(context.device);

	for (auto &framebuffer : context.swapchain_framebuffers)
	{
		vkDestroyFramebuffer(context.device, framebuffer, nullptr);
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

	if (context.render_pass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(context.device, context.render_pass, nullptr);
	}

	for (VkImageView image_view : context.swapchain_image_views)
	{
		vkDestroyImageView(context.device, image_view, nullptr);
	}

	if (context.swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(context.device, context.swapchain, nullptr);
	}

	if (context.surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
	}

	if (vertex_buffer_allocation != VK_NULL_HANDLE)
	{
		vmaDestroyBuffer(context.vma_allocator, vertex_buffer, vertex_buffer_allocation);
	}

	if (context.vma_allocator != VK_NULL_HANDLE)
	{
		vmaDestroyAllocator(context.vma_allocator);
	}

	if (context.device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(context.device, nullptr);
	}

	if (context.debug_callback != VK_NULL_HANDLE)
	{
		vkDestroyDebugUtilsMessengerEXT(context.instance, context.debug_callback, nullptr);
	}

	vk_instance.reset();
}

bool HelloTriangle::prepare(const vkb::ApplicationOptions &options)
{
	// Headless is not supported to keep this sample as simple as possible
	assert(options.window != nullptr);
	assert(options.window->get_window_mode() != vkb::Window::Mode::Headless);

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
	init_render_pass();
	init_pipeline();
	init_framebuffers();

	return true;
}

void HelloTriangle::update(float delta_time)
{
	uint32_t index;

	auto res = acquire_next_image(&index);

	// Handle outdated error in acquire.
	if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
	{
		resize(context.swapchain_dimensions.width, context.swapchain_dimensions.height);
		res = acquire_next_image(&index);
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
		resize(context.swapchain_dimensions.width, context.swapchain_dimensions.height);
	}
	else if (res != VK_SUCCESS)
	{
		LOGE("Failed to present swapchain image.");
	}
}

bool HelloTriangle::resize(const uint32_t, const uint32_t)
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

	for (auto &framebuffer : context.swapchain_framebuffers)
	{
		vkDestroyFramebuffer(context.device, framebuffer, nullptr);
	}

	init_swapchain();
	init_framebuffers();
	return true;
}

std::unique_ptr<vkb::Application> create_hello_triangle()
{
	return std::make_unique<HelloTriangle>();
}
