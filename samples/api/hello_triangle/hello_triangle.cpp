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

#include "hello_triangle.h"

#include "common/logging.h"
#include "common/vk_common.h"
#include "platform/filesystem.h"
#include "platform/window.h"
#include "shader_compiler.h"

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
/// @brief A debug callback called from Vulkan validation layers.
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT type,
                                                     uint64_t object, size_t location, int32_t message_code,
                                                     const char *layer_prefix, const char *message, void *user_data)
{
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		LOGE("Validation Layer: Error: {}: {}", layer_prefix, message);
	}
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		LOGE("Validation Layer: Warning: {}: {}", layer_prefix, message);
	}
	else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
	{
		LOGI("Validation Layer: Performance warning: {}: {}", layer_prefix, message);
	}
	else
	{
		LOGI("Validation Layer: Information: {}: {}", layer_prefix, message);
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
bool HelloTriangle::validate_extensions(const std::vector<const char *> &         required,
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
 * @brief Validates a list of required layers, comparing it with the available ones.
 *
 * @param required A vector containing required layer names.
 * @param available A VkLayerProperties object containing available layers.
 * @return true if all required extensions are available
 * @return false otherwise
 */
bool HelloTriangle::validate_layers(const std::vector<const char *> &     required,
                                    const std::vector<VkLayerProperties> &available)
{
	for (auto extension : required)
	{
		bool found = false;
		for (auto &available_extension : available)
		{
			if (strcmp(available_extension.layerName, extension) == 0)
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
 * @brief Find the vulkan shader stage for a given a string.
 *
 * @param ext A string containing the shader stage name.
 * @return VkShaderStageFlagBits The shader stage mapping from the given string, VK_SHADER_STAGE_VERTEX_BIT otherwise.
 */
VkShaderStageFlagBits HelloTriangle::find_shader_stage(const std::string &ext)
{
	if (ext == "vert")
	{
		return VK_SHADER_STAGE_VERTEX_BIT;
	}
	else if (ext == "frag")
	{
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	}
	else if (ext == "comp")
	{
		return VK_SHADER_STAGE_COMPUTE_BIT;
	}
	else if (ext == "geom")
	{
		return VK_SHADER_STAGE_GEOMETRY_BIT;
	}
	else if (ext == "tesc")
	{
		return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	}
	else if (ext == "tese")
	{
		return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	}

	throw std::runtime_error("No Vulkan shader stage found for the file extension name.");
};

/**
 * @brief Initializes the Vulkan instance.
 *
 * @param context A newly created Vulkan context.
 * @param required_instance_extensions The required Vulkan instance extensions.
 * @param required_validation_layers The required Vulkan validation layers
 */
void HelloTriangle::init_instance(Context &                        context,
                                  const std::vector<const char *> &required_instance_extensions,
                                  const std::vector<const char *> &required_validation_layers)
{
	LOGI("Initializing vulkan instance.");

	if (volkInitialize())
	{
		throw std::runtime_error("Failed to initialize volk.");
	}

	uint32_t instance_extension_count;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr));

	std::vector<VkExtensionProperties> instance_extensions(instance_extension_count);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, instance_extensions.data()));

	std::vector<const char *> active_instance_extensions(required_instance_extensions);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	active_instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

#if (defined(VKB_ENABLE_PORTABILITY))
	active_instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	active_instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	active_instance_extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
	active_instance_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
	active_instance_extensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	active_instance_extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
	active_instance_extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	active_instance_extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
	active_instance_extensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
#else
#	pragma error Platform not supported
#endif

	if (!validate_extensions(active_instance_extensions, instance_extensions))
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
	std::vector<const char *> optimal_validation_layers = vkb::get_optimal_validation_layers(supported_validation_layers);
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

	VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
	app.pApplicationName = "Hello Triangle";
	app.pEngineName      = "Vulkan Samples";
	app.apiVersion       = VK_MAKE_VERSION(1, 0, 0);

	VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	instance_info.pApplicationInfo        = &app;
	instance_info.enabledExtensionCount   = vkb::to_u32(active_instance_extensions.size());
	instance_info.ppEnabledExtensionNames = active_instance_extensions.data();
	instance_info.enabledLayerCount       = vkb::to_u32(requested_validation_layers.size());
	instance_info.ppEnabledLayerNames     = requested_validation_layers.data();

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	VkDebugReportCallbackCreateInfoEXT debug_report_create_info = {VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT};
	debug_report_create_info.flags                              = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	debug_report_create_info.pfnCallback                        = debug_callback;

	instance_info.pNext = &debug_report_create_info;
#endif

#if (defined(VKB_ENABLE_PORTABILITY))
	instance_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

	// Create the Vulkan instance
	VK_CHECK(vkCreateInstance(&instance_info, nullptr, &context.instance));

	volkLoadInstance(context.instance);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	VK_CHECK(vkCreateDebugReportCallbackEXT(context.instance, &debug_report_create_info, nullptr, &context.debug_callback));
#endif
}

/**
 * @brief Initializes the Vulkan physical device and logical device.
 *
 * @param context A Vulkan context with an instance already set up.
 * @param required_device_extensions The required Vulkan device extensions.
 */
void HelloTriangle::init_device(Context &                        context,
                                const std::vector<const char *> &required_device_extensions)
{
	LOGI("Initializing vulkan device.");

	uint32_t gpu_count = 0;
	VK_CHECK(vkEnumeratePhysicalDevices(context.instance, &gpu_count, nullptr));

	if (gpu_count < 1)
	{
		throw std::runtime_error("No physical device found.");
	}

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
		LOGE("Did not find suitable queue which supports graphics, compute and presentation.");
	}

	uint32_t device_extension_count;
	VK_CHECK(vkEnumerateDeviceExtensionProperties(context.gpu, nullptr, &device_extension_count, nullptr));
	std::vector<VkExtensionProperties> device_extensions(device_extension_count);
	VK_CHECK(vkEnumerateDeviceExtensionProperties(context.gpu, nullptr, &device_extension_count, device_extensions.data()));

	if (!validate_extensions(required_device_extensions, device_extensions))
	{
		throw std::runtime_error("Required device extensions are missing, will try without.");
	}

	float queue_priority = 1.0f;

	// Create one queue
	VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
	queue_info.queueFamilyIndex = context.graphics_queue_index;
	queue_info.queueCount       = 1;
	queue_info.pQueuePriorities = &queue_priority;

	VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
	device_info.queueCreateInfoCount    = 1;
	device_info.pQueueCreateInfos       = &queue_info;
	device_info.enabledExtensionCount   = vkb::to_u32(required_device_extensions.size());
	device_info.ppEnabledExtensionNames = required_device_extensions.data();

	VK_CHECK(vkCreateDevice(context.gpu, &device_info, nullptr, &context.device));
	volkLoadDevice(context.device);

	vkGetDeviceQueue(context.device, context.graphics_queue_index, 0, &context.queue);
}

/**
 * @brief Initializes per frame data.
 * @param context A newly created Vulkan context.
 * @param per_frame The data of a frame.
 */
void HelloTriangle::init_per_frame(Context &context, PerFrame &per_frame)
{
	VkFenceCreateInfo info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
	info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	VK_CHECK(vkCreateFence(context.device, &info, nullptr, &per_frame.queue_submit_fence));

	VkCommandPoolCreateInfo cmd_pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	cmd_pool_info.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	cmd_pool_info.queueFamilyIndex = context.graphics_queue_index;
	VK_CHECK(vkCreateCommandPool(context.device, &cmd_pool_info, nullptr, &per_frame.primary_command_pool));

	VkCommandBufferAllocateInfo cmd_buf_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	cmd_buf_info.commandPool        = per_frame.primary_command_pool;
	cmd_buf_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd_buf_info.commandBufferCount = 1;
	VK_CHECK(vkAllocateCommandBuffers(context.device, &cmd_buf_info, &per_frame.primary_command_buffer));

	per_frame.device      = context.device;
	per_frame.queue_index = context.graphics_queue_index;
}

/**
 * @brief Tears down the frame data.
 * @param context The Vulkan context.
 * @param per_frame The data of a frame.
 */
void HelloTriangle::teardown_per_frame(Context &context, PerFrame &per_frame)
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

	per_frame.device      = VK_NULL_HANDLE;
	per_frame.queue_index = -1;
}

/**
 * @brief Initializes the Vulkan swapchain.
 * @param context A Vulkan context with a physical device already set up.
 */
void HelloTriangle::init_swapchain(Context &context)
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

	VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
	info.surface            = context.surface;
	info.minImageCount      = desired_swapchain_images;
	info.imageFormat        = format.format;
	info.imageColorSpace    = format.colorSpace;
	info.imageExtent.width  = swapchain_size.width;
	info.imageExtent.height = swapchain_size.height;
	info.imageArrayLayers   = 1;
	info.imageUsage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	info.imageSharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	info.preTransform       = pre_transform;
	info.compositeAlpha     = composite;
	info.presentMode        = swapchain_present_mode;
	info.clipped            = true;
	info.oldSwapchain       = old_swapchain;

	VK_CHECK(vkCreateSwapchainKHR(context.device, &info, nullptr, &context.swapchain));

	if (old_swapchain != VK_NULL_HANDLE)
	{
		for (VkImageView image_view : context.swapchain_image_views)
		{
			vkDestroyImageView(context.device, image_view, nullptr);
		}

		uint32_t image_count;
		VK_CHECK(vkGetSwapchainImagesKHR(context.device, old_swapchain, &image_count, nullptr));

		for (size_t i = 0; i < image_count; i++)
		{
			teardown_per_frame(context, context.per_frame[i]);
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
		init_per_frame(context, context.per_frame[i]);
	}

	for (size_t i = 0; i < image_count; i++)
	{
		// Create an image view which we can render into.
		VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		view_info.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format                      = context.swapchain_dimensions.format;
		view_info.image                       = swapchain_images[i];
		view_info.subresourceRange.levelCount = 1;
		view_info.subresourceRange.layerCount = 1;
		view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view_info.components.r                = VK_COMPONENT_SWIZZLE_R;
		view_info.components.g                = VK_COMPONENT_SWIZZLE_G;
		view_info.components.b                = VK_COMPONENT_SWIZZLE_B;
		view_info.components.a                = VK_COMPONENT_SWIZZLE_A;

		VkImageView image_view;
		VK_CHECK(vkCreateImageView(context.device, &view_info, nullptr, &image_view));

		context.swapchain_image_views.push_back(image_view);
	}
}

/**
 * @brief Initializes the Vulkan render pass.
 * @param context A Vulkan context with a device already set up.
 */
void HelloTriangle::init_render_pass(Context &context)
{
	VkAttachmentDescription attachment = {0};
	// Backbuffer format.
	attachment.format = context.swapchain_dimensions.format;
	// Not multisampled.
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// When starting the frame, we want tiles to be cleared.
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// When ending the frame, we want tiles to be written out.
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	// Don't care about stencil since we're not using it.
	attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// The image layout will be undefined when the render pass begins.
	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// After the render pass is complete, we will transition to PRESENT_SRC_KHR layout.
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// We have one subpass. This subpass has one color attachment.
	// While executing this subpass, the attachment will be in attachment optimal layout.
	VkAttachmentReference color_ref = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

	// We will end up with two transitions.
	// The first one happens right before we start subpass #0, where
	// UNDEFINED is transitioned into COLOR_ATTACHMENT_OPTIMAL.
	// The final layout in the render pass attachment states PRESENT_SRC_KHR, so we
	// will get a final transition from COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR.
	VkSubpassDescription subpass = {0};
	subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments    = &color_ref;

	// Create a dependency to external events.
	// We need to wait for the WSI semaphore to signal.
	// Only pipeline stages which depend on COLOR_ATTACHMENT_OUTPUT_BIT will
	// actually wait for the semaphore, so we must also wait for that pipeline stage.
	VkSubpassDependency dependency = {0};
	dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass          = 0;
	dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	// Since we changed the image layout, we need to make the memory visible to
	// color attachment to modify.
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// Finally, create the renderpass.
	VkRenderPassCreateInfo rp_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
	rp_info.attachmentCount        = 1;
	rp_info.pAttachments           = &attachment;
	rp_info.subpassCount           = 1;
	rp_info.pSubpasses             = &subpass;
	rp_info.dependencyCount        = 1;
	rp_info.pDependencies          = &dependency;

	VK_CHECK(vkCreateRenderPass(context.device, &rp_info, nullptr, &context.render_pass));
}

/**
 * @brief Helper function to load a shader module.
 * @param context A Vulkan context with a device.
 * @param path The path for the shader (relative to the assets directory).
 * @returns A VkShaderModule handle. Aborts execution if shader creation fails.
 */
VkShaderModule HelloTriangle::load_shader_module(Context &context, const char *path)
{
	vkb::ShaderCompiler shader_compiler;

	auto buffer = vkb::fs::read_shader_binary(path);

	std::string file_ext = path;

	// Extract extension name from the glsl shader file
	file_ext = file_ext.substr(file_ext.find_last_of(".") + 1);

	std::vector<uint32_t> spirv;
	std::string           info_log;

	// Compile the GLSL source
	if (!shader_compiler.compile_to_spirv(find_shader_stage(file_ext), buffer, "main", {}, spirv, info_log))
	{
		LOGE("Failed to compile shader, Error: {}", info_log.c_str());
		return VK_NULL_HANDLE;
	}

	VkShaderModuleCreateInfo module_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
	module_info.codeSize = spirv.size() * sizeof(uint32_t);
	module_info.pCode    = spirv.data();

	VkShaderModule shader_module;
	VK_CHECK(vkCreateShaderModule(context.device, &module_info, nullptr, &shader_module));

	return shader_module;
}

/**
 * @brief Initializes the Vulkan pipeline.
 * @param context A Vulkan context with a device and a render pass already set up.
 */
void HelloTriangle::init_pipeline(Context &context)
{
	// Create a blank pipeline layout.
	// We are not binding any resources to the pipeline in this first sample.
	VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	VK_CHECK(vkCreatePipelineLayout(context.device, &layout_info, nullptr, &context.pipeline_layout));

	VkPipelineVertexInputStateCreateInfo vertex_input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

	// Specify we will use triangle lists to draw geometry.
	VkPipelineInputAssemblyStateCreateInfo input_assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Specify rasterization state.
	VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	raster.cullMode  = VK_CULL_MODE_BACK_BIT;
	raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
	raster.lineWidth = 1.0f;

	// Our attachment will write to all color channels, but no blending is enabled.
	VkPipelineColorBlendAttachmentState blend_attachment{};
	blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
	blend.attachmentCount = 1;
	blend.pAttachments    = &blend_attachment;

	// We will have one viewport and scissor box.
	VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	viewport.viewportCount = 1;
	viewport.scissorCount  = 1;

	// Disable all depth testing.
	VkPipelineDepthStencilStateCreateInfo depth_stencil{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};

	// No multisampling.
	VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Specify that these states will be dynamic, i.e. not part of pipeline state object.
	std::array<VkDynamicState, 2> dynamics{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
	dynamic.pDynamicStates    = dynamics.data();
	dynamic.dynamicStateCount = vkb::to_u32(dynamics.size());

	// Load our SPIR-V shaders.
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};

	// Vertex stage of the pipeline
	shader_stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
	shader_stages[0].module = load_shader_module(context, "triangle.vert");
	shader_stages[0].pName  = "main";

	// Fragment stage of the pipeline
	shader_stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_stages[1].module = load_shader_module(context, "triangle.frag");
	shader_stages[1].pName  = "main";

	VkGraphicsPipelineCreateInfo pipe{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
	pipe.stageCount          = vkb::to_u32(shader_stages.size());
	pipe.pStages             = shader_stages.data();
	pipe.pVertexInputState   = &vertex_input;
	pipe.pInputAssemblyState = &input_assembly;
	pipe.pRasterizationState = &raster;
	pipe.pColorBlendState    = &blend;
	pipe.pMultisampleState   = &multisample;
	pipe.pViewportState      = &viewport;
	pipe.pDepthStencilState  = &depth_stencil;
	pipe.pDynamicState       = &dynamic;

	// We need to specify the pipeline layout and the render pass description up front as well.
	pipe.renderPass = context.render_pass;
	pipe.layout     = context.pipeline_layout;

	VK_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipe, nullptr, &context.pipeline));

	// Pipeline is baked, we can delete the shader modules now.
	vkDestroyShaderModule(context.device, shader_stages[0].module, nullptr);
	vkDestroyShaderModule(context.device, shader_stages[1].module, nullptr);
}

/**
 * @brief Acquires an image from the swapchain.
 * @param context A Vulkan context with a swapchain already set up.
 * @param[out] image The swapchain index for the acquired image.
 * @returns Vulkan result code
 */
VkResult HelloTriangle::acquire_next_image(Context &context, uint32_t *image)
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
 * @param context A Vulkan context set up for rendering.
 * @param swapchain_index The swapchain index for the image being rendered.
 */
void HelloTriangle::render_triangle(Context &context, uint32_t swapchain_index)
{
	// Render to this framebuffer.
	VkFramebuffer framebuffer = context.swapchain_framebuffers[swapchain_index];

	// Allocate or re-use a primary command buffer.
	VkCommandBuffer cmd = context.per_frame[swapchain_index].primary_command_buffer;

	// We will only submit this once before it's recycled.
	VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	// Begin command recording
	vkBeginCommandBuffer(cmd, &begin_info);

	// Set clear color values.
	VkClearValue clear_value;
	clear_value.color = {{0.01f, 0.01f, 0.033f, 1.0f}};

	// Begin the render pass.
	VkRenderPassBeginInfo rp_begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	rp_begin.renderPass               = context.render_pass;
	rp_begin.framebuffer              = framebuffer;
	rp_begin.renderArea.extent.width  = context.swapchain_dimensions.width;
	rp_begin.renderArea.extent.height = context.swapchain_dimensions.height;
	rp_begin.clearValueCount          = 1;
	rp_begin.pClearValues             = &clear_value;
	// We will add draw commands in the same command buffer.
	vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

	// Bind the graphics pipeline.
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, context.pipeline);

	VkViewport vp{};
	vp.width    = static_cast<float>(context.swapchain_dimensions.width);
	vp.height   = static_cast<float>(context.swapchain_dimensions.height);
	vp.minDepth = 0.0f;
	vp.maxDepth = 1.0f;
	// Set viewport dynamically
	vkCmdSetViewport(cmd, 0, 1, &vp);

	VkRect2D scissor{};
	scissor.extent.width  = context.swapchain_dimensions.width;
	scissor.extent.height = context.swapchain_dimensions.height;
	// Set scissor dynamically
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	// Draw three vertices with one instance.
	vkCmdDraw(cmd, 3, 1, 0, 0);

	// Complete render pass.
	vkCmdEndRenderPass(cmd);

	// Complete the command buffer.
	VK_CHECK(vkEndCommandBuffer(cmd));

	// Submit it to the queue with a release semaphore.
	if (context.per_frame[swapchain_index].swapchain_release_semaphore == VK_NULL_HANDLE)
	{
		VkSemaphoreCreateInfo semaphore_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		VK_CHECK(vkCreateSemaphore(context.device, &semaphore_info, nullptr,
		                           &context.per_frame[swapchain_index].swapchain_release_semaphore));
	}

	VkPipelineStageFlags wait_stage{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	VkSubmitInfo info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
	info.commandBufferCount   = 1;
	info.pCommandBuffers      = &cmd;
	info.waitSemaphoreCount   = 1;
	info.pWaitSemaphores      = &context.per_frame[swapchain_index].swapchain_acquire_semaphore;
	info.pWaitDstStageMask    = &wait_stage;
	info.signalSemaphoreCount = 1;
	info.pSignalSemaphores    = &context.per_frame[swapchain_index].swapchain_release_semaphore;
	// Submit command buffer to graphics queue
	VK_CHECK(vkQueueSubmit(context.queue, 1, &info, context.per_frame[swapchain_index].queue_submit_fence));
}

/**
 * @brief Presents an image to the swapchain.
 * @param context The Vulkan context, with a swapchain and per-frame resources already set up.
 * @param index The swapchain index previously obtained from @ref acquire_next_image.
 * @returns Vulkan result code
 */
VkResult HelloTriangle::present_image(Context &context, uint32_t index)
{
	VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
	present.swapchainCount     = 1;
	present.pSwapchains        = &context.swapchain;
	present.pImageIndices      = &index;
	present.waitSemaphoreCount = 1;
	present.pWaitSemaphores    = &context.per_frame[index].swapchain_release_semaphore;
	// Present swapchain image
	return vkQueuePresentKHR(context.queue, &present);
}

/**
 * @brief Initializes the Vulkan framebuffers.
 * @param context A Vulkan context with the render pass already set up.
 */
void HelloTriangle::init_framebuffers(Context &context)
{
	VkDevice device = context.device;

	// Create framebuffer for each swapchain image view
	for (auto &image_view : context.swapchain_image_views)
	{
		// Build the framebuffer.
		VkFramebufferCreateInfo fb_info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
		fb_info.renderPass      = context.render_pass;
		fb_info.attachmentCount = 1;
		fb_info.pAttachments    = &image_view;
		fb_info.width           = context.swapchain_dimensions.width;
		fb_info.height          = context.swapchain_dimensions.height;
		fb_info.layers          = 1;

		VkFramebuffer framebuffer;
		VK_CHECK(vkCreateFramebuffer(device, &fb_info, nullptr, &framebuffer));

		context.swapchain_framebuffers.push_back(framebuffer);
	}
}

/**
 * @brief Tears down the framebuffers. If our swapchain changes, we will call this, and create a new swapchain.
 * @param context The Vulkan context.
 */
void HelloTriangle::teardown_framebuffers(Context &context)
{
	// Wait until device is idle before teardown.
	vkQueueWaitIdle(context.queue);

	for (auto &framebuffer : context.swapchain_framebuffers)
	{
		vkDestroyFramebuffer(context.device, framebuffer, nullptr);
	}

	context.swapchain_framebuffers.clear();
}

/**
 * @brief Tears down the Vulkan context.
 * @param context The Vulkan context.
 */
void HelloTriangle::teardown(Context &context)
{
	// Don't release anything until the GPU is completely idle.
	vkDeviceWaitIdle(context.device);

	teardown_framebuffers(context);

	for (auto &per_frame : context.per_frame)
	{
		teardown_per_frame(context, per_frame);
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
		context.swapchain = VK_NULL_HANDLE;
	}

	if (context.surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
		context.surface = VK_NULL_HANDLE;
	}

	if (context.device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(context.device, nullptr);
		context.device = VK_NULL_HANDLE;
	}

	if (context.debug_callback != VK_NULL_HANDLE)
	{
		vkDestroyDebugReportCallbackEXT(context.instance, context.debug_callback, nullptr);
		context.debug_callback = VK_NULL_HANDLE;
	}

	vk_instance.reset();
}

HelloTriangle::HelloTriangle()
{
}

HelloTriangle::~HelloTriangle()
{
	teardown(context);
}

bool HelloTriangle::prepare(const vkb::ApplicationOptions &options)
{
	assert(options.window != nullptr);

	init_instance(context, {VK_KHR_SURFACE_EXTENSION_NAME}, {});

	vk_instance = std::make_unique<vkb::Instance>(context.instance);

	context.surface                     = options.window->create_surface(*vk_instance);
	auto &extent                        = options.window->get_extent();
	context.swapchain_dimensions.width  = extent.width;
	context.swapchain_dimensions.height = extent.height;

	if (!context.surface)
	{
		throw std::runtime_error("Failed to create window surface.");
	}

	init_device(context, {"VK_KHR_swapchain"});

	init_swapchain(context);

	// Create the necessary objects for rendering.
	init_render_pass(context);
	init_pipeline(context);
	init_framebuffers(context);

	return true;
}

void HelloTriangle::update(float delta_time)
{
	uint32_t index;

	auto res = acquire_next_image(context, &index);

	// Handle outdated error in acquire.
	if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
	{
		resize(context.swapchain_dimensions.width, context.swapchain_dimensions.height);
		res = acquire_next_image(context, &index);
	}

	if (res != VK_SUCCESS)
	{
		vkQueueWaitIdle(context.queue);
		return;
	}

	render_triangle(context, index);
	res = present_image(context, index);

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
	teardown_framebuffers(context);

	init_swapchain(context);
	init_framebuffers(context);
	return true;
}

std::unique_ptr<vkb::Application> create_hello_triangle()
{
	return std::make_unique<HelloTriangle>();
}
