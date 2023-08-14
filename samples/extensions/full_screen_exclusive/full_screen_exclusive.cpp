/* Copyright (c) 2023, Holochip Corporation
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

#include "full_screen_exclusive.h"

#include "glsl_compiler.h"
#include "platform/filesystem.h"
#include "platform/window.h"

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
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

bool FullScreenExclusive::validate_extensions(const std::vector<const char *> &required, const std::vector<VkExtensionProperties> &available)
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
			return false;
	}

	return true;
}

bool FullScreenExclusive::validate_layers(const std::vector<const char *> &required, const std::vector<VkLayerProperties> &available)
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

VkShaderStageFlagBits FullScreenExclusive::find_shader_stage(const std::string &ext)
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
}

void FullScreenExclusive::init_instance(const std::vector<const char *> &required_instance_extensions, const std::vector<const char *> &required_validation_layers)
{
	LOGI("Initializing vulkan instance.")

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
	active_instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

#if (defined(VKB_ENABLE_PORTABILITY))
	active_instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	active_instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

	// Add Instance extensions for full screen exclusive
	active_instance_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	active_instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	active_instance_extensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
	// Get the HWND application window handle and stores it in the class variable
	initialize_windows();

	if (!validate_extensions(active_instance_extensions, instance_extensions))
	{
		throw std::runtime_error("Required instance extensions are missing.");
	}
	uint32_t instance_layer_count;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));
	std::vector<VkLayerProperties> supported_validation_layers(instance_layer_count);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, supported_validation_layers.data()));
	std::vector<const char *> requested_validation_layers(required_validation_layers);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	// Determine the optimal validation layers to enable that are necessary for useful debugging
	std::vector<const char *> optimal_validation_layers = vkb::get_optimal_validation_layers(supported_validation_layers);
	requested_validation_layers.insert(requested_validation_layers.end(), optimal_validation_layers.begin(), optimal_validation_layers.end());
#endif

	if (validate_layers(requested_validation_layers, supported_validation_layers))
	{
		LOGI("Enabled Validation Layers:")
		for (const auto &layer : requested_validation_layers)
		{
			LOGI(" \t{}", layer)
		}
	}
	else
	{
		throw std::runtime_error("Required validation layers are missing.");
	}

	VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
	app.pApplicationName = "Full Screen Exclusive";
	app.pEngineName      = "Vulkan Samples";
	app.apiVersion       = VK_MAKE_VERSION(1, 0, 0);

	VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	instance_info.pApplicationInfo        = &app;
	instance_info.enabledExtensionCount   = vkb::to_u32(active_instance_extensions.size());
	instance_info.ppEnabledExtensionNames = active_instance_extensions.data();
	instance_info.enabledLayerCount       = vkb::to_u32(requested_validation_layers.size());
	instance_info.ppEnabledLayerNames     = requested_validation_layers.data();

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	debug_utils_create_info.messageSeverity                    = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	debug_utils_create_info.messageType                        = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debug_utils_create_info.pfnUserCallback                    = debug_callback;

	instance_info.pNext = &debug_utils_create_info;
#endif

#if (defined(VKB_ENABLE_PORTABILITY))
	instance_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

	VK_CHECK(vkCreateInstance(&instance_info, nullptr, &context.instance));
	volkLoadInstance(context.instance);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	VK_CHECK(vkCreateDebugUtilsMessengerEXT(context.instance, &debug_utils_create_info, nullptr, &context.debug_callback));
#endif
}

void FullScreenExclusive::init_device(const std::vector<const char *> &required_device_extensions)
{
	LOGI("Initializing vulkan device.")

	uint32_t gpu_count = 0;
	VK_CHECK(vkEnumeratePhysicalDevices(context.instance, &gpu_count, nullptr));

	if (gpu_count < 1)
	{
		throw std::runtime_error("No physical device found.");
	}

	std::vector<VkPhysicalDevice> gpus(gpu_count);
	VK_CHECK(vkEnumeratePhysicalDevices(context.instance, &gpu_count, gpus.data()));

	for (uint32_t i = 0; i < gpu_count && (context.graphics_queue_index < 0); i++)
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

		for (uint32_t j = 0; j < queue_family_count; j++)
		{
			VkBool32 supports_present;
			vkGetPhysicalDeviceSurfaceSupportKHR(context.gpu, j, context.surface, &supports_present);

			if ((queue_family_properties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) && supports_present)
			{
				context.graphics_queue_index = j;
				break;
			}
		}
	}

	if (context.graphics_queue_index < 0)
	{
		LOGE("Did not find suitable queue which supports graphics, and presentation.")
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

	VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
	queue_info.queueFamilyIndex = context.graphics_queue_index;
	queue_info.queueCount       = 1;
	queue_info.pQueuePriorities = &queue_priority;

	VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
	device_info.queueCreateInfoCount = 1;
	device_info.pQueueCreateInfos    = &queue_info;

	device_info.enabledExtensionCount   = vkb::to_u32(required_device_extensions.size());
	device_info.ppEnabledExtensionNames = required_device_extensions.data();

	VK_CHECK(vkCreateDevice(context.gpu, &device_info, nullptr, &context.device));
	volkLoadDevice(context.device);

	vkGetDeviceQueue(context.device, context.graphics_queue_index, 0, &context.queue);

	LOGI("Press the F1-F3 keys to control this sample.  Here's what they do:")
	LOGI("F1 sets Windowed view.")
	LOGI("F2 sets Borderless Fullscreen.")
	LOGI("F3 sets Exclusive Fullscreen.")
}

void FullScreenExclusive::init_per_frame(PerFrame &per_frame) const
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

void FullScreenExclusive::teardown_per_frame(PerFrame &per_frame) const
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

void FullScreenExclusive::init_swapchain()
{
	VkSurfaceCapabilitiesKHR surface_properties;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.gpu, context.surface, &surface_properties));

	VkSurfaceFormatKHR format = vkb::select_surface_format(context.gpu, context.surface);

	VkExtent2D swapchain_size;
	if (is_full_screen_exclusive)
	{
		swapchain_size = surface_properties.maxImageExtent;
	}
	else
	{
		if (surface_properties.currentExtent.width == 0xFFFFFFFF)
		{
			swapchain_size = context.swapchain_dimensions;
		}
		else
		{
			swapchain_size = surface_properties.currentExtent;
		}
	}

	VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;

	uint32_t desired_swapchain_images = surface_properties.minImageCount + 1;
	if ((surface_properties.maxImageCount > 0) && (desired_swapchain_images > surface_properties.maxImageCount))
	{
		desired_swapchain_images = surface_properties.maxImageCount;
	}

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

	VkSwapchainCreateInfoKHR info{};        // initialize the swapchain create info without adding pNext info
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

	info.pNext = &surface_full_screen_exclusive_info_EXT;        // syncing the full screen exclusive info.

	info.surface          = context.surface;
	info.minImageCount    = desired_swapchain_images;
	info.imageFormat      = format.format;
	info.imageColorSpace  = format.colorSpace;
	info.imageExtent      = swapchain_size;
	info.imageArrayLayers = 1;
	info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.preTransform     = pre_transform;
	info.compositeAlpha   = composite;
	info.presentMode      = swapchain_present_mode;
	info.clipped          = true;
	info.oldSwapchain     = old_swapchain;

	VkResult result = vkCreateSwapchainKHR(context.device, &info, nullptr, &context.swapchain);
	if (result == VK_ERROR_INITIALIZATION_FAILED)
	{
		LOGE("this mode doesn't work with the current setup.")
		abort();
	}
	VK_CHECK(result);

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
			teardown_per_frame(context.per_frame[i]);
		}

		context.swapchain_image_views.clear();

		vkDestroySwapchainKHR(context.device, old_swapchain, nullptr);
	}

	context.swapchain_dimensions = swapchain_size;
	context.swapchain_format     = format.format;

	uint32_t image_count;
	VK_CHECK(vkGetSwapchainImagesKHR(context.device, context.swapchain, &image_count, nullptr));

	std::vector<VkImage> swapchain_images(image_count);
	VK_CHECK(vkGetSwapchainImagesKHR(context.device, context.swapchain, &image_count, swapchain_images.data()));

	context.per_frame.clear();
	context.per_frame.resize(image_count);

	for (size_t i = 0; i < image_count; i++)
	{
		init_per_frame(context.per_frame[i]);
	}

	for (size_t i = 0; i < image_count; i++)
	{
		VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		view_info.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format                      = context.swapchain_format;
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

void FullScreenExclusive::init_render_pass()
{
	VkAttachmentDescription attachment = {0};
	attachment.format                  = context.swapchain_format;
	attachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_ref = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

	VkSubpassDescription subpass = {0};
	subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments    = &color_ref;

	VkSubpassDependency dependency = {0};
	dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass          = 0;
	dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo rp_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
	rp_info.attachmentCount        = 1;
	rp_info.pAttachments           = &attachment;
	rp_info.subpassCount           = 1;
	rp_info.pSubpasses             = &subpass;
	rp_info.dependencyCount        = 1;
	rp_info.pDependencies          = &dependency;

	VK_CHECK(vkCreateRenderPass(context.device, &rp_info, nullptr, &context.render_pass));
}

VkShaderModule FullScreenExclusive::load_shader_module(const char *path) const
{
	vkb::GLSLCompiler glsl_compiler;

	auto buffer = vkb::fs::read_shader_binary(path);

	std::string file_ext = path;

	file_ext = file_ext.substr(file_ext.find_last_of('.') + 1);

	std::vector<uint32_t> spirv;
	std::string           info_log;

	if (!glsl_compiler.compile_to_spirv(find_shader_stage(file_ext), buffer, "main", {}, spirv, info_log))
	{
		LOGE("Failed to compile shader, Error: {}", info_log.c_str())
		return VK_NULL_HANDLE;
	}

	VkShaderModuleCreateInfo module_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
	module_info.codeSize = spirv.size() * sizeof(uint32_t);
	module_info.pCode    = spirv.data();

	VkShaderModule shader_module;
	VK_CHECK(vkCreateShaderModule(context.device, &module_info, nullptr, &shader_module));

	return shader_module;
}

void FullScreenExclusive::init_pipeline()
{
	VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	VK_CHECK(vkCreatePipelineLayout(context.device, &layout_info, nullptr, &context.pipeline_layout));

	VkPipelineVertexInputStateCreateInfo vertex_input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

	VkPipelineInputAssemblyStateCreateInfo input_assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	raster.cullMode  = VK_CULL_MODE_BACK_BIT;
	raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
	raster.lineWidth = 1.0f;

	VkPipelineColorBlendAttachmentState blend_attachment{};
	blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
	blend.attachmentCount = 1;
	blend.pAttachments    = &blend_attachment;

	VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	viewport.viewportCount = 1;
	viewport.scissorCount  = 1;

	VkPipelineDepthStencilStateCreateInfo depth_stencil{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};

	VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	std::array<VkDynamicState, 2> dynamics{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
	dynamic.pDynamicStates    = dynamics.data();
	dynamic.dynamicStateCount = vkb::to_u32(dynamics.size());

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};

	shader_stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
	shader_stages[0].module = load_shader_module("triangle.vert");
	shader_stages[0].pName  = "main";

	shader_stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_stages[1].module = load_shader_module("triangle.frag");
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

	pipe.renderPass = context.render_pass;
	pipe.layout     = context.pipeline_layout;

	VK_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipe, nullptr, &context.pipeline));

	vkDestroyShaderModule(context.device, shader_stages[0].module, nullptr);
	vkDestroyShaderModule(context.device, shader_stages[1].module, nullptr);
}

VkResult FullScreenExclusive::acquire_next_image(uint32_t *image)
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

	VkResult result = vkAcquireNextImageKHR(context.device, context.swapchain, UINT64_MAX, acquire_semaphore, VK_NULL_HANDLE, image);

	if (result != VK_SUCCESS)
	{
		context.recycled_semaphores.push_back(acquire_semaphore);
		return result;
	}

	if (context.per_frame[*image].queue_submit_fence != VK_NULL_HANDLE)
	{
		vkWaitForFences(context.device, 1, &context.per_frame[*image].queue_submit_fence, true, UINT64_MAX);
		vkResetFences(context.device, 1, &context.per_frame[*image].queue_submit_fence);
	}

	if (context.per_frame[*image].primary_command_pool != VK_NULL_HANDLE)
	{
		vkResetCommandPool(context.device, context.per_frame[*image].primary_command_pool, 0);
	}

	VkSemaphore old_semaphore = context.per_frame[*image].swapchain_acquire_semaphore;

	if (old_semaphore != VK_NULL_HANDLE)
	{
		context.recycled_semaphores.push_back(old_semaphore);
	}

	context.per_frame[*image].swapchain_acquire_semaphore = acquire_semaphore;

	return VK_SUCCESS;
}

void FullScreenExclusive::render_triangle(uint32_t swapchain_index)
{
	VkFramebuffer framebuffer = context.swapchain_framebuffers[swapchain_index];

	VkCommandBuffer cmd = context.per_frame[swapchain_index].primary_command_buffer;

	VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &begin_info);

	VkClearValue clear_value;
	clear_value.color = {{0.01f, 0.01f, 0.033f, 1.0f}};

	VkRenderPassBeginInfo rp_begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	rp_begin.renderPass        = context.render_pass;
	rp_begin.framebuffer       = framebuffer;
	rp_begin.renderArea.extent = context.swapchain_dimensions;
	rp_begin.clearValueCount   = 1;
	rp_begin.pClearValues      = &clear_value;

	vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, context.pipeline);

	VkViewport vp{};
	vp.width    = static_cast<float>(context.swapchain_dimensions.width);
	vp.height   = static_cast<float>(context.swapchain_dimensions.height);
	vp.minDepth = 0.0f;
	vp.maxDepth = 1.0f;
	vkCmdSetViewport(cmd, 0, 1, &vp);

	VkRect2D scissor{};
	scissor.extent = context.swapchain_dimensions;
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	vkCmdDraw(cmd, 3, 1, 0, 0);

	vkCmdEndRenderPass(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

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

	VK_CHECK(vkQueueSubmit(context.queue, 1, &info, context.per_frame[swapchain_index].queue_submit_fence));
}

VkResult FullScreenExclusive::present_image(uint32_t index)
{
	VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
	present.swapchainCount     = 1;
	present.pSwapchains        = &context.swapchain;
	present.pImageIndices      = &index;
	present.waitSemaphoreCount = 1;
	present.pWaitSemaphores    = &context.per_frame[index].swapchain_release_semaphore;
	return vkQueuePresentKHR(context.queue, &present);
}

void FullScreenExclusive::init_framebuffers()
{
	VkDevice device = context.device;

	for (auto &image_view : context.swapchain_image_views)
	{
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

void FullScreenExclusive::teardown_framebuffers()
{
	vkQueueWaitIdle(context.queue);

	for (auto &framebuffer : context.swapchain_framebuffers)
	{
		vkDestroyFramebuffer(context.device, framebuffer, nullptr);
	}

	context.swapchain_framebuffers.clear();
}

void FullScreenExclusive::teardown()
{
	vkDeviceWaitIdle(context.device);

	teardown_framebuffers();

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
		vkDestroyDebugUtilsMessengerEXT(context.instance, context.debug_callback, nullptr);
		context.debug_callback = VK_NULL_HANDLE;
	}

	vk_instance.reset();
}

FullScreenExclusive::~FullScreenExclusive()
{
	teardown();
}

void FullScreenExclusive::initialize_windows()
{
	HWND_application_window = GetActiveWindow();
	// The following variable has to be attached to the pNext of surface_full_screen_exclusive_info_EXT:
	surface_full_screen_exclusive_Win32_info_EXT.sType    = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT;
	surface_full_screen_exclusive_Win32_info_EXT.pNext    = nullptr;
	surface_full_screen_exclusive_Win32_info_EXT.hmonitor = MonitorFromWindow(HWND_application_window, MONITOR_DEFAULTTONEAREST);

	surface_full_screen_exclusive_info_EXT.sType               = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
	surface_full_screen_exclusive_info_EXT.pNext               = &surface_full_screen_exclusive_Win32_info_EXT;
	surface_full_screen_exclusive_info_EXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_DEFAULT_EXT;        // Set the fullScreenExclusive stage to default when initializing

	// Windows placement should only be registered ONCE:
	GetWindowPlacement(HWND_application_window, &wpc);
}

bool FullScreenExclusive::prepare(const vkb::ApplicationOptions &options)
{
	if (!vkb::Application::prepare(options))
	{
		return false;
	}

	init_instance({VK_KHR_SURFACE_EXTENSION_NAME}, {});

	vk_instance = std::make_unique<vkb::Instance>(context.instance);

	context.surface = window->create_surface(*vk_instance);
	if (!context.surface)
		throw std::runtime_error("Failed to create window surface.");
	auto &extent                        = window->get_extent();
	context.swapchain_dimensions.width  = extent.width;
	context.swapchain_dimensions.height = extent.height;

	// As application is running on a Windows platform, then the following TWO device extension are also needed:
	init_device({VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME});

	init_swapchain();

	init_render_pass();
	init_pipeline();
	init_framebuffers();

	return true;
}

void FullScreenExclusive::update(float delta_time)
{
	uint32_t index;

	auto res = acquire_next_image(&index);

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

	if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
	{
		resize(context.swapchain_dimensions.width, context.swapchain_dimensions.height);
	}
	else if (res != VK_SUCCESS)
	{
		LOGE("Failed to present swapchain image.")
	}
}

bool FullScreenExclusive::resize(uint32_t width, uint32_t height)
{
	if (context.device == VK_NULL_HANDLE)
	{
		return false;
	}

	VkSurfaceCapabilitiesKHR surface_properties;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.gpu, context.surface, &surface_properties));

	if (surface_properties.currentExtent.width == context.swapchain_dimensions.width &&
	    surface_properties.currentExtent.height == context.swapchain_dimensions.height)
	{
		return false;
	}

	vkDeviceWaitIdle(context.device);
	teardown_framebuffers();

	init_swapchain();
	init_framebuffers();
	return true;
}

VkExtent2D FullScreenExclusive::get_current_max_image_extent() const
{
	VkExtent2D returnMe{};

	if (context.device != VK_NULL_HANDLE)
	{
		VkSurfaceCapabilitiesKHR surface_properties{};
		VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.gpu, context.surface, &surface_properties));

		returnMe = surface_properties.maxImageExtent;
	}

	return returnMe;
}

void FullScreenExclusive::input_event(const vkb::InputEvent &input_event)
{
	if (input_event.get_source() == vkb::EventSource::Keyboard)
	{
		const auto &key_button = reinterpret_cast<const vkb::KeyInputEvent &>(input_event);

		if (key_button.get_action() == vkb::KeyAction::Down)
		{
			bool needs_recreate = false;

			switch (key_button.get_code())
			{
				case vkb::KeyCode::F1:        // FullscreenExclusiveEXT = Disallowed
					if (full_screen_status != SwapchainMode::Windowed)
					{
						full_screen_status                                         = SwapchainMode::Windowed;
						application_window_status                                  = ApplicationWindowMode::Windowed;
						surface_full_screen_exclusive_info_EXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_DISALLOWED_EXT;
						needs_recreate                                             = true;

						// If it is switched from the exclusive full screen mode, then release full screen first
						if (is_full_screen_exclusive)
						{
							VkResult result = vkReleaseFullScreenExclusiveModeEXT(context.device, context.swapchain);
							if (result != VK_SUCCESS)
							{
								LOGI("vkReleaseFullScreenExclusiveModeEXT: " + vkb::to_string(result))
							}
							is_full_screen_exclusive = false;
						}

						LOGI("Windowed Mode Detected!")
					}
					break;
				case vkb::KeyCode::F2:        // FullscreenExclusiveEXT = Allowed
					if (full_screen_status != SwapchainMode::BorderlessFullscreen)
					{
						full_screen_status                                         = SwapchainMode::BorderlessFullscreen;
						application_window_status                                  = ApplicationWindowMode::Fullscreen;
						surface_full_screen_exclusive_info_EXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT;
						needs_recreate                                             = true;

						// If it is switched from the exclusive full screen mode, then release full screen first
						if (is_full_screen_exclusive)
						{
							VkResult result = vkReleaseFullScreenExclusiveModeEXT(context.device, context.swapchain);
							if (result != VK_SUCCESS)
							{
								LOGI("vkReleaseFullScreenExclusiveModeEXT: " + vkb::to_string(result))
							}
							is_full_screen_exclusive = false;
						}

						LOGI("Borderless Fullscreen Mode Detected!")
					}
					break;
				case vkb::KeyCode::F3:        // FullscreenExclusiveEXT = Application Controlled
					if (full_screen_status != SwapchainMode::ExclusiveFullscreen)
					{
						full_screen_status                                         = SwapchainMode::ExclusiveFullscreen;
						application_window_status                                  = ApplicationWindowMode::Fullscreen;
						surface_full_screen_exclusive_info_EXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT;
						needs_recreate                                             = true;
						is_full_screen_exclusive                                   = true;
						LOGI("Exclusive Fullscreen Mode Detected!")
					}
					break;
				default:        // FullscreenExclusiveEXT = Default
					needs_recreate = false;
					break;
			}

			// now if to recreate the swapchain and everything related:
			if (needs_recreate)
			{
				FullScreenExclusive::recreate();
			}
		}
	}
}

void FullScreenExclusive::update_application_window()
{
	if (application_window_status == ApplicationWindowMode::Fullscreen && is_windowed)        // check if it is already in fullscreen, if is, then do nothing
	{
		is_windowed = false;

		if (HWND_style == 0)
			HWND_style = GetWindowLong(HWND_application_window, GWL_STYLE);
		if (HWND_extended_style == 0)
			HWND_extended_style = GetWindowLong(HWND_application_window, GWL_EXSTYLE);

		long HWND_newStyle          = HWND_style & (~WS_BORDER) & (~WS_DLGFRAME) & (~WS_THICKFRAME);
		long HWND_extended_newStyle = HWND_extended_style & (~WS_EX_WINDOWEDGE);

		SetWindowLong(HWND_application_window, GWL_STYLE, HWND_newStyle | static_cast<long>(WS_POPUP));
		SetWindowLong(HWND_application_window, GWL_EXSTYLE, HWND_extended_newStyle | WS_EX_TOPMOST);

		ShowWindow(HWND_application_window, SW_SHOWMAXIMIZED);
	}
	else if (application_window_status == ApplicationWindowMode::Windowed && !is_windowed)        // check if it is already "windowed", if is, then do nothing
	{
		is_windowed = true;

		SetWindowLong(HWND_application_window, GWL_STYLE, HWND_style);
		SetWindowLong(HWND_application_window, GWL_EXSTYLE, HWND_extended_style);
		ShowWindow(HWND_application_window, SW_SHOWNORMAL);
		SetWindowPlacement(HWND_application_window, &wpc);
	}
}

void FullScreenExclusive::recreate()
{
	// Check if there IS a device, if not don't do anything
	if (context.device != VK_NULL_HANDLE)
	{
		// Step: 0) Idle the device, destroy/teardown the current swapchain and frame buffers.
		vkDeviceWaitIdle(context.device);        // pause the renderer
		teardown_framebuffers();                 // basically destroy everything swapchain related

		// Step: 1) recreate the swapchain with its properly selected FullscreenExclusive enum value
		init_swapchain();

		// Step: 2) recreate the frame buffers using the newly created swapchain
		init_framebuffers();

		// Step: 3-1) update the window mode, corresponding to the FullscreenExclusive enum value
		update_application_window();

		// Step: 3-2) remember: ALWAYS change the application window mode BEFORE acquire the full screen exclusive mode!
		if (is_full_screen_exclusive)
		{
			VkResult result = vkAcquireFullScreenExclusiveModeEXT(context.device, context.swapchain);
			if (result != VK_SUCCESS)
				LOGI("vkAcquireFullScreenExclusiveModeEXT: " + vkb::to_string(result))        // it would be necessary to learn the result on an unsuccessful attempt
		}
	}
}

std::unique_ptr<vkb::Application> create_full_screen_exclusive()
{
	return std::make_unique<FullScreenExclusive>();
}
