/* Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
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

#include "hpp_hello_triangle_1_3.h"

#include "common/hpp_vk_common.h"
#include "filesystem/legacy.h"
#include "platform/window.h"

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
/// @brief A debug callback called from Vulkan validation layers.
static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(vk::DebugUtilsMessageSeverityFlagBitsEXT      message_severity,
                                                       vk::DebugUtilsMessageTypeFlagsEXT             message_types,
                                                       vk::DebugUtilsMessengerCallbackDataEXT const *callback_data,
                                                       void                                         *user_data)
{
	if (message_severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
	{
		LOGE("{} Validation Layer: Error: {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	else if (message_severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
	{
		LOGW("{} Validation Layer: Warning: {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	else if (message_severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
	{
		LOGI("{} Validation Layer: Information: {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	else if (message_types & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
	{
		LOGI("{} Validation Layer: Performance warning: {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	else if (message_severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose)
	{
		LOGD("{} Validation Layer: Verbose: {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	return false;
}
#endif

HPPHelloTriangleV13::~HPPHelloTriangleV13()
{
	// Don't release anything until the GPU is completely idle.
	if (context.device)
	{
		context.device.waitIdle();
	}

	for (auto &per_frame : context.per_frame)
	{
		teardown_per_frame(per_frame);
	}

	context.per_frame.clear();

	for (auto semaphore : context.recycled_semaphores)
	{
		context.device.destroySemaphore(semaphore);
	}

	if (context.pipeline)
	{
		context.device.destroyPipeline(context.pipeline);
	}

	if (context.pipeline_layout)
	{
		context.device.destroyPipelineLayout(context.pipeline_layout);
	}

	for (vk::ImageView image_view : context.swapchain_image_views)
	{
		context.device.destroyImageView(image_view);
	}

	if (context.swapchain)
	{
		context.device.destroySwapchainKHR(context.swapchain);
	}

	if (context.surface)
	{
		context.instance.destroySurfaceKHR(context.surface);
	}

	if (context.vertex_buffer)
	{
		context.device.destroyBuffer(context.vertex_buffer);
	}

	if (context.vertex_buffer_memory)
	{
		context.device.freeMemory(context.vertex_buffer_memory);
	}

	if (context.device)
	{
		context.device.destroy();
	}

	if (context.debug_callback)
	{
		context.instance.destroyDebugUtilsMessengerEXT(context.debug_callback);
	}
}

bool HPPHelloTriangleV13::prepare(const vkb::ApplicationOptions &options)
{
	// Headless is not supported to keep this sample as simple as possible
	assert(options.window != nullptr);
	assert(options.window->get_window_mode() != vkb::Window::Mode::Headless);

	bool prepared = Application::prepare(options);
	if (prepared)
	{
		init_instance();

		select_physical_device_and_surface(options.window);

		auto &extent                        = options.window->get_extent();
		context.swapchain_dimensions.width  = extent.width;
		context.swapchain_dimensions.height = extent.height;

		init_device();

		init_vertex_buffer();

		init_swapchain();

		// Create the necessary objects for rendering.
		init_pipeline();
	}

	return prepared;
}

bool HPPHelloTriangleV13::resize(const uint32_t, const uint32_t)
{
	if (context.device == VK_NULL_HANDLE)
	{
		return false;
	}

	vk::SurfaceCapabilitiesKHR surface_properties = context.gpu.getSurfaceCapabilitiesKHR(context.surface);

	// Only rebuild the swapchain if the dimensions have changed
	bool dimensions_changed = (surface_properties.currentExtent.width != context.swapchain_dimensions.width ||
	                           surface_properties.currentExtent.height != context.swapchain_dimensions.height);
	if (dimensions_changed)
	{
		context.device.waitIdle();

		init_swapchain();
	}
	return dimensions_changed;
}

/**
 * @brief Select a physical device.
 */
void HPPHelloTriangleV13::select_physical_device_and_surface(vkb::Window *window)
{
	std::vector<vk::PhysicalDevice> gpus = context.instance.enumeratePhysicalDevices();

	for (const auto &physical_device : gpus)
	{
		// Check if the device supports Vulkan 1.3
		vk::PhysicalDeviceProperties device_properties = physical_device.getProperties();
		if (device_properties.apiVersion < vk::ApiVersion13)
		{
			LOGW("Physical device '{}' does not support Vulkan 1.3, skipping.", device_properties.deviceName.data());
			continue;
		}

		if (context.surface)
		{
			context.instance.destroySurfaceKHR(context.surface);
		}

		context.surface = static_cast<vk::SurfaceKHR>(window->create_surface(static_cast<VkInstance>(context.instance), static_cast<VkPhysicalDevice>(physical_device)));
		if (!context.surface)
		{
			throw std::runtime_error("Failed to create window surface.");
		}

		// Find a queue family that supports graphics and presentation
		std::vector<vk::QueueFamilyProperties> queue_family_properties = physical_device.getQueueFamilyProperties();

		auto qfpIt = std::ranges::find_if(queue_family_properties,
		                                  [&physical_device, surface = context.surface](vk::QueueFamilyProperties const &qfp) {
			                                  static uint32_t index = 0;
			                                  return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) && physical_device.getSurfaceSupportKHR(index++, surface);
		                                  });
		if (qfpIt != queue_family_properties.end())
		{
			context.graphics_queue_index = std::distance(queue_family_properties.begin(), qfpIt);
			context.gpu                  = physical_device;
			break;
		}
	}

	if (context.graphics_queue_index < 0)
	{
		throw std::runtime_error("Failed to find a suitable GPU with Vulkan 1.3 support.");
	}
}

void HPPHelloTriangleV13::update(float delta_time)
{
	uint32_t index;

	auto res = acquire_next_swapchain_image(&index);

	// Handle outdated error in acquire.
	if (res == vk::Result::eSuboptimalKHR || res == vk::Result::eErrorOutOfDateKHR)
	{
		if (!resize(context.swapchain_dimensions.width, context.swapchain_dimensions.height))
		{
			LOGI("Resize failed");
		}
		res = acquire_next_swapchain_image(&index);
	}

	if (res != vk::Result::eSuccess)
	{
		context.queue.waitIdle();
		return;
	}

	render_triangle(index);
	res = present_image(index);

	// Handle Outdated error in present.
	if (res == vk::Result::eSuboptimalKHR || res == vk::Result::eErrorOutOfDateKHR)
	{
		if (!resize(context.swapchain_dimensions.width, context.swapchain_dimensions.height))
		{
			LOGI("Resize failed");
		}
	}
	else if (res != vk::Result::eSuccess)
	{
		LOGE("Failed to present swapchain image.");
	}
}

/**
 * @brief Acquires an image from the swapchain.
 * @param[out] image The swapchain index for the acquired image.
 * @returns Vulkan result code
 */
vk::Result HPPHelloTriangleV13::acquire_next_swapchain_image(uint32_t *image)
{
	vk::Semaphore acquire_semaphore;
	if (context.recycled_semaphores.empty())
	{
		acquire_semaphore = context.device.createSemaphore({});
	}
	else
	{
		acquire_semaphore = context.recycled_semaphores.back();
		context.recycled_semaphores.pop_back();
	}

	vk::Result result;
	try
	{
		std::tie(result, *image) = context.device.acquireNextImageKHR(context.swapchain, UINT64_MAX, acquire_semaphore);
	}
	catch (vk::OutOfDateKHRError &)
	{
		result = vk::Result::eErrorOutOfDateKHR;
	}

	if (result != vk::Result::eSuccess)
	{
		context.recycled_semaphores.push_back(acquire_semaphore);
	}
	else
	{
		assert(*image < context.per_frame.size());

		// If we have outstanding fences for this swapchain image, wait for them to complete first.
		// After begin frame returns, it is safe to reuse or delete resources which
		// were used previously.
		//
		// We wait for fences which completes N frames earlier, so we do not stall,
		// waiting for all GPU work to complete before this returns.
		// Normally, this doesn't really block at all,
		// since we're waiting for old frames to have been completed, but just in case.
		if (context.per_frame[*image].queue_submit_fence)
		{
			result = context.device.waitForFences(context.per_frame[*image].queue_submit_fence, true, UINT64_MAX);
			assert(result == vk::Result::eSuccess);
			context.device.resetFences(context.per_frame[*image].queue_submit_fence);
		}

		if (context.per_frame[*image].primary_command_pool)
		{
			context.device.resetCommandPool(context.per_frame[*image].primary_command_pool);
		}

		// Recycle the old semaphore back into the semaphore manager.
		vk::Semaphore old_semaphore = context.per_frame[*image].swapchain_acquire_semaphore;

		if (old_semaphore)
		{
			context.recycled_semaphores.push_back(old_semaphore);
		}

		context.per_frame[*image].swapchain_acquire_semaphore = acquire_semaphore;
	}
	return result;
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
uint32_t HPPHelloTriangleV13::find_memory_type(vk::PhysicalDevice physical_device, uint32_t type_filter, vk::MemoryPropertyFlags properties)
{
	// Structure to hold the physical device's memory properties
	vk::PhysicalDeviceMemoryProperties mem_properties = physical_device.getMemoryProperties();

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
 * @brief Initializes the Vulkan physical device and logical device.
 */
void HPPHelloTriangleV13::init_device()
{
	LOGI("Initializing Vulkan device.");

	std::vector<vk::ExtensionProperties> device_extensions = context.gpu.enumerateDeviceExtensionProperties();

	// Since this sample has visual output, the device needs to support the swapchain extension
	std::vector<const char *> required_device_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

	if (!validate_extensions(required_device_extensions, device_extensions))
	{
		throw std::runtime_error("Required device extensions are missing");
	}

#if (defined(VKB_ENABLE_PORTABILITY))
	// VK_KHR_portability_subset must be enabled if present in the implementation (e.g on macOS/iOS with beta extensions enabled)
	if (std::ranges::any_of(device_extensions,
	                        [](vk::ExtensionProperties const &extension) { return strcmp(extension.extensionName, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) == 0; }))
	{
		required_device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
	}
#endif

	// Query for Vulkan 1.3 features
	auto supported_features_chain =
	    context.gpu.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

	// Check if Physical device supports Vulkan 1.3 features
	if (!supported_features_chain.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering)
	{
		throw std::runtime_error("Dynamic Rendering feature is missing");
	}

	if (!supported_features_chain.get<vk::PhysicalDeviceVulkan13Features>().synchronization2)
	{
		throw std::runtime_error("Synchronization2 feature is missing");
	}

	if (!supported_features_chain.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState)
	{
		throw std::runtime_error("Extended Dynamic State feature is missing");
	}

	// Enable only specific Vulkan 1.3 features
	vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
	    enabled_features_chain = {{}, {.synchronization2 = true, .dynamicRendering = true}, {.extendedDynamicState = true}};

	// Create the logical device
	float queue_priority = 0.5f;

	// Create one queue
	vk::DeviceQueueCreateInfo queue_info{.queueFamilyIndex = static_cast<uint32_t>(context.graphics_queue_index),
	                                     .queueCount       = 1,
	                                     .pQueuePriorities = &queue_priority};

	vk::DeviceCreateInfo device_info{.pNext                   = &enabled_features_chain.get<vk::PhysicalDeviceFeatures2>(),
	                                 .queueCreateInfoCount    = 1,
	                                 .pQueueCreateInfos       = &queue_info,
	                                 .enabledExtensionCount   = static_cast<uint32_t>(required_device_extensions.size()),
	                                 .ppEnabledExtensionNames = required_device_extensions.data()};

	context.device = context.gpu.createDevice(device_info);

	// initialize function pointers for device
	VULKAN_HPP_DEFAULT_DISPATCHER.init(context.device);

	context.queue = context.device.getQueue(context.graphics_queue_index, 0);
}

/**
 * @brief Initializes the Vulkan instance.
 */
void HPPHelloTriangleV13::init_instance()
{
	LOGI("Initializing Vulkan instance.");

#if defined(_HPP_VULKAN_LIBRARY)
	static vk::detail::DynamicLoader dl(_HPP_VULKAN_LIBRARY);
#else
	static vk::detail::DynamicLoader dl;
#endif
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	std::vector<vk::ExtensionProperties> available_instance_extensions = vk::enumerateInstanceExtensionProperties();

	std::vector<const char *> required_instance_extensions{VK_KHR_SURFACE_EXTENSION_NAME};

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	bool has_debug_utils = std::ranges::any_of(
	    available_instance_extensions,
	    [](auto const &ep) { return strncmp(ep.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME, strlen(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) == 0; });
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
	bool portability_enumeration_available = std::ranges::any_of(
	    available_instance_extensions,
	    [](VkExtensionProperties const &extension) { return strcmp(extension.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0; });
	if (portability_enumeration_available)
	{
		required_instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
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

	std::vector<vk::LayerProperties> supported_instance_layers = vk::enumerateInstanceLayerProperties();

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

	vk::ApplicationInfo app{.pApplicationName = "Hello Triangle V1.3", .pEngineName = "Vulkan Samples", .apiVersion = VK_MAKE_VERSION(1, 3, 0)};

	vk::InstanceCreateInfo instance_info{.pApplicationInfo        = &app,
	                                     .enabledLayerCount       = static_cast<uint32_t>(requested_instance_layers.size()),
	                                     .ppEnabledLayerNames     = requested_instance_layers.data(),
	                                     .enabledExtensionCount   = static_cast<uint32_t>(required_instance_extensions.size()),
	                                     .ppEnabledExtensionNames = required_instance_extensions.data()};

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	vk::DebugUtilsMessengerCreateInfoEXT debug_messenger_create_info;
	if (has_debug_utils)
	{
		debug_messenger_create_info.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
		debug_messenger_create_info.messageType     = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
		debug_messenger_create_info.pfnUserCallback = debug_callback;

		instance_info.pNext = &debug_messenger_create_info;
	}
#endif

#if defined(VKB_ENABLE_PORTABILITY)
	if (portability_enumeration_available)
	{
		instance_info.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
	}
#endif

	// Create the Vulkan instance
	context.instance = vk::createInstance(instance_info);

	// initialize function pointers for instance
	VULKAN_HPP_DEFAULT_DISPATCHER.init(context.instance);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	if (has_debug_utils)
	{
		context.debug_callback = context.instance.createDebugUtilsMessengerEXT(debug_messenger_create_info);
	}
#endif

#if defined(VK_USE_PLATFORM_DISPLAY_KHR) || defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_USE_PLATFORM_METAL_EXT)
	// we need some additional initializing for this platform!
	if (volkInitialize())
	{
		throw std::runtime_error("Failed to initialize volk.");
	}
	volkLoadInstance(context.instance);
#endif
}

/**
 * @brief Initializes per frame data.
 * @param per_frame The data of a frame.
 */
void HPPHelloTriangleV13::init_per_frame(PerFrame &per_frame)
{
	per_frame.queue_submit_fence = context.device.createFence({.flags = vk::FenceCreateFlagBits::eSignaled});

	vk::CommandPoolCreateInfo cmd_pool_info{.flags            = vk::CommandPoolCreateFlagBits::eTransient,
	                                        .queueFamilyIndex = static_cast<uint32_t>(context.graphics_queue_index)};
	per_frame.primary_command_pool = context.device.createCommandPool(cmd_pool_info);

	vk::CommandBufferAllocateInfo cmd_buf_info{.commandPool        = per_frame.primary_command_pool,
	                                           .level              = vk::CommandBufferLevel::ePrimary,
	                                           .commandBufferCount = 1};
	per_frame.primary_command_buffer = context.device.allocateCommandBuffers(cmd_buf_info)[0];
}

/**
 * @brief Initializes the Vulkan pipeline.
 */
void HPPHelloTriangleV13::init_pipeline()
{
	// Create a blank pipeline layout.
	// We are not binding any resources to the pipeline in this first sample.
	context.pipeline_layout = context.device.createPipelineLayout({});

	// Define the vertex input binding description
	vk::VertexInputBindingDescription binding_description{.binding = 0, .stride = sizeof(Vertex), .inputRate = vk::VertexInputRate::eVertex};

	// Define the vertex input attribute descriptions
	std::array<vk::VertexInputAttributeDescription, 2> attribute_descriptions = {{
	    {.location = 0, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, position)},        // position
	    {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, color)}         // color
	}};

	// Create the vertex input state
	vk::PipelineVertexInputStateCreateInfo vertex_input{.vertexBindingDescriptionCount   = 1,
	                                                    .pVertexBindingDescriptions      = &binding_description,
	                                                    .vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size()),
	                                                    .pVertexAttributeDescriptions    = attribute_descriptions.data()};

	// Specify we will use triangle lists to draw geometry.
	vk::PipelineInputAssemblyStateCreateInfo input_assembly{.topology = vk::PrimitiveTopology::eTriangleList};

	// Specify rasterization state.
	vk::PipelineRasterizationStateCreateInfo raster{.polygonMode = vk::PolygonMode::eFill, .lineWidth = 1.0f};

	// Specify that these states will be dynamic, i.e. not part of pipeline state object.
	std::vector<vk::DynamicState> dynamic_states = {
	    vk::DynamicState::eViewport, vk::DynamicState::eScissor, vk::DynamicState::eCullMode, vk::DynamicState::eFrontFace, vk::DynamicState::ePrimitiveTopology};

	// Our attachment will write to all color channels, but no blending is enabled.
	vk::PipelineColorBlendAttachmentState blend_attachment{.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
	                                                                         vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

	vk::PipelineColorBlendStateCreateInfo blend{.attachmentCount = 1, .pAttachments = &blend_attachment};

	// We will have one viewport and scissor box.
	vk::PipelineViewportStateCreateInfo viewport{.viewportCount = 1, .scissorCount = 1};

	// Disable all depth testing.
	vk::PipelineDepthStencilStateCreateInfo depth_stencil{.depthCompareOp = vk::CompareOp::eAlways};

	// No multisampling.
	vk::PipelineMultisampleStateCreateInfo multisample{.rasterizationSamples = vk::SampleCountFlagBits::e1};

	vk::PipelineDynamicStateCreateInfo dynamic_state_info{.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
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

	std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages = {{
	    {.stage  = vk::ShaderStageFlagBits::eVertex,
	     .module = load_shader_module("hello_triangle_1_3/" + shader_folder + "/triangle.vert.spv", vk::ShaderStageFlagBits::eVertex),
	     .pName  = "main"},        // Vertex shader stage
	    {.stage  = vk::ShaderStageFlagBits::eFragment,
	     .module = load_shader_module("hello_triangle_1_3/" + shader_folder + "/triangle.frag.spv", vk::ShaderStageFlagBits::eFragment),
	     .pName  = "main"}        // Fragment shader stage
	}};

	// Pipeline rendering info (for dynamic rendering).
	vk::PipelineRenderingCreateInfo pipeline_rendering_info{.colorAttachmentCount = 1, .pColorAttachmentFormats = &context.swapchain_dimensions.format};

	// Create the graphics pipeline.
	vk::GraphicsPipelineCreateInfo pipeline_create_info{
	    .pNext               = &pipeline_rendering_info,
	    .stageCount          = static_cast<uint32_t>(shader_stages.size()),
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

	vk::Result result;
	std::tie(result, context.pipeline) = context.device.createGraphicsPipeline(nullptr, pipeline_create_info);
	assert(result == vk::Result::eSuccess);

	// Pipeline is baked, we can delete the shader modules now.
	for (auto &shader_stage : shader_stages)
	{
		context.device.destroyShaderModule(shader_stage.module);
	}
}

/**
 * @brief Initializes the Vulkan swapchain.
 */
void HPPHelloTriangleV13::init_swapchain()
{
	vk::SurfaceCapabilitiesKHR surface_properties = context.gpu.getSurfaceCapabilitiesKHR(context.surface);

	vk::SurfaceFormatKHR format = vkb::common::select_surface_format(context.gpu, context.surface);

	vk::Extent2D swapchain_size;
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
	vk::PresentModeKHR swapchain_present_mode = vk::PresentModeKHR::eFifo;

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
	vk::SurfaceTransformFlagBitsKHR pre_transform;
	if (surface_properties.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
	{
		pre_transform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
	}
	else
	{
		pre_transform = surface_properties.currentTransform;
	}

	vk::SwapchainKHR old_swapchain = context.swapchain;

	// one bitmask needs to be set according to the priority of presentation engine
	vk::CompositeAlphaFlagBitsKHR composite = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	if (surface_properties.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque)
	{
		composite = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	}
	else if (surface_properties.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit)
	{
		composite = vk::CompositeAlphaFlagBitsKHR::eInherit;
	}
	else if (surface_properties.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
	{
		composite = vk::CompositeAlphaFlagBitsKHR::ePreMultiplied;
	}
	else if (surface_properties.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied)
	{
		composite = vk::CompositeAlphaFlagBitsKHR::ePostMultiplied;
	}

	vk::SwapchainCreateInfoKHR info{
	    .surface          = context.surface,                                 // The surface onto which images will be presented
	    .minImageCount    = desired_swapchain_images,                        // Minimum number of images in the swapchain (number of buffers)
	    .imageFormat      = format.format,                                   // Format of the swapchain images (e.g., VK_FORMAT_B8G8R8A8_SRGB)
	    .imageColorSpace  = format.colorSpace,                               // Color space of the images (e.g., VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
	    .imageExtent      = swapchain_size,                                  // Resolution of the swapchain images (width and height)
	    .imageArrayLayers = 1,                                               // Number of layers in each image (usually 1 unless stereoscopic)
	    .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,        // How the images will be used (as color attachments)
	    .imageSharingMode = vk::SharingMode::eExclusive,                     // Access mode of the images (exclusive to one queue family)
	    .preTransform     = pre_transform,                                   // Transform to apply to images (e.g., rotation)
	    .compositeAlpha   = composite,                                       // Alpha blending to apply (e.g., opaque, pre-multiplied)
	    .presentMode      = swapchain_present_mode,                          // Presentation mode (e.g., vsync settings)
	    .clipped          = true,                                            // Whether to clip obscured pixels (improves performance)
	    .oldSwapchain     = old_swapchain                                    // Handle to the old swapchain, if replacing an existing one
	};

	context.swapchain = context.device.createSwapchainKHR(info);

	if (old_swapchain)
	{
		for (vk::ImageView image_view : context.swapchain_image_views)
		{
			context.device.destroyImageView(image_view);
		}
		context.swapchain_image_views.clear();

		for (auto &per_frame : context.per_frame)
		{
			teardown_per_frame(per_frame);
		}

		context.device.destroySwapchainKHR(old_swapchain);
	}

	context.swapchain_dimensions = {swapchain_size.width, swapchain_size.height, format.format};

	/// The swapchain images.
	context.swapchain_images = context.device.getSwapchainImagesKHR(context.swapchain);

	// Initialize per-frame resources.
	// Every swapchain image has its own command pool and fence manager.
	// This makes it very easy to keep track of when we can reset command buffers and such.
	context.per_frame.clear();
	context.per_frame.resize(context.swapchain_images.size());

	for (auto &per_frame : context.per_frame)
	{
		init_per_frame(per_frame);
	}

	for (auto const &swapchain_image : context.swapchain_images)
	{
		// Create an image view which we can render into.
		vk::ImageViewCreateInfo view_info{
		    .image            = swapchain_image,
		    .viewType         = vk::ImageViewType::e2D,
		    .format           = context.swapchain_dimensions.format,
		    .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};

		context.swapchain_image_views.push_back(context.device.createImageView(view_info));
	}
}

/**
 * @brief Initializes the vertex buffer by creating it, allocating memory, binding the memory, and uploading vertex data.
 * @note This function must be called after the Vulkan device has been initialized.
 * @throws std::runtime_error if any Vulkan operation fails.
 */
void HPPHelloTriangleV13::init_vertex_buffer()
{
	vk::DeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

	// Create the vertex buffer
	vk::BufferCreateInfo vertex_buffer_info{.size = buffer_size, .usage = vk::BufferUsageFlagBits::eVertexBuffer, .sharingMode = vk::SharingMode::eExclusive};

	context.vertex_buffer = context.device.createBuffer(vertex_buffer_info);

	// Get memory requirements
	vk::MemoryRequirements memory_requirements = context.device.getBufferMemoryRequirements(context.vertex_buffer);

	// Allocate memory for the buffer
	vk::MemoryAllocateInfo alloc_info{
	    .allocationSize = memory_requirements.size,
	    .memoryTypeIndex =
	        find_memory_type(context.gpu, memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)};

	context.vertex_buffer_memory = context.device.allocateMemory(alloc_info);

	// Bind the buffer with the allocated memory
	context.device.bindBufferMemory(context.vertex_buffer, context.vertex_buffer_memory, 0);

	// Map the memory and copy the vertex data
	void *data = context.device.mapMemory(context.vertex_buffer_memory, 0, buffer_size);
	memcpy(data, vertices.data(), static_cast<size_t>(buffer_size));
	context.device.unmapMemory(context.vertex_buffer_memory);
}

/**
 * @brief Helper function to load a shader module.
 * @param path The path for the shader (relative to the assets directory).
 * @param shader_stage The shader stage flag specifying the type of shader (e.g., vk::ShaderStageFlagBits::eVertex).
 * @returns A VkShaderModule handle. Aborts execution if shader creation fails.
 */
vk::ShaderModule HPPHelloTriangleV13::load_shader_module(const std::string &path, vk::ShaderStageFlagBits shader_stage)
{
	auto spirv = vkb::fs::read_shader_binary_u32(path);

	vk::ShaderModuleCreateInfo module_info{.codeSize = spirv.size() * sizeof(uint32_t), .pCode = spirv.data()};

	return context.device.createShaderModule(module_info);
}

/**
 * @brief Presents an image to the swapchain.
 * @param index The swapchain index previously obtained from @ref acquire_next_swapchain_image.
 * @returns Vulkan result code
 */
vk::Result HPPHelloTriangleV13::present_image(uint32_t index)
{
	vk::PresentInfoKHR present{
	    .waitSemaphoreCount = 1,
	    .pWaitSemaphores    = &context.per_frame[index].swapchain_release_semaphore,
	    .swapchainCount     = 1,
	    .pSwapchains        = &context.swapchain,
	    .pImageIndices      = &index,
	};

	// Present swapchain image
	vk::Result result;
	try
	{
		result = context.queue.presentKHR(present);
	}
	catch (vk::OutOfDateKHRError &)
	{
		result = vk::Result::eErrorOutOfDateKHR;
	}
	return result;
}

/**
 * @brief Renders a triangle to the specified swapchain image.
 * @param swapchain_index The swapchain index for the image being rendered.
 */
void HPPHelloTriangleV13::render_triangle(uint32_t swapchain_index)
{
	// Allocate or re-use a primary command buffer.
	vk::CommandBuffer cmd = context.per_frame[swapchain_index].primary_command_buffer;

	// We will only submit this once before it's recycled.
	vk::CommandBufferBeginInfo begin_info{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

	// Begin command recording
	cmd.begin(begin_info);

	// Before starting rendering, transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
	transition_image_layout(cmd,
	                        context.swapchain_images[swapchain_index],
	                        vk::ImageLayout::eUndefined,
	                        vk::ImageLayout::eColorAttachmentOptimal,
	                        {},                                                       // srcAccessMask (no need to wait for previous operations)
	                        vk::AccessFlagBits2::eColorAttachmentWrite,               // dstAccessMask
	                        vk::PipelineStageFlagBits2::eTopOfPipe,                   // srcStage
	                        vk::PipelineStageFlagBits2::eColorAttachmentOutput        // dstStage
	);

	// Set clear color values.
	vk::ClearValue clear_value;
	clear_value.color = std::array<float, 4>({{0.01f, 0.01f, 0.033f, 1.0f}});

	// Set up the rendering attachment info
	vk::RenderingAttachmentInfo color_attachment{.imageView   = context.swapchain_image_views[swapchain_index],
	                                             .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
	                                             .loadOp      = vk::AttachmentLoadOp::eClear,
	                                             .storeOp     = vk::AttachmentStoreOp::eStore,
	                                             .clearValue  = clear_value};

	// Begin rendering
	vk::RenderingInfo rendering_info{
	    .renderArea           = {                         // Initialize the nested `VkRect2D` structure
	                             .offset = {0, 0},        // Initialize the `VkOffset2D` inside `renderArea`
	                             .extent = {              // Initialize the `VkExtent2D` inside `renderArea`
	                                        .width  = context.swapchain_dimensions.width,
	                                        .height = context.swapchain_dimensions.height}},
	              .layerCount = 1,
	    .colorAttachmentCount = 1,
	    .pColorAttachments    = &color_attachment};

	cmd.beginRendering(rendering_info);

	// Bind the graphics pipeline.
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, context.pipeline);

	// Set dynamic states

	// Set viewport dynamically
	vk::Viewport vp{.width    = static_cast<float>(context.swapchain_dimensions.width),
	                .height   = static_cast<float>(context.swapchain_dimensions.height),
	                .minDepth = 0.0f,
	                .maxDepth = 1.0f};

	cmd.setViewport(0, vp);

	// Set scissor dynamically
	vk::Rect2D scissor{.extent = {.width = context.swapchain_dimensions.width, .height = context.swapchain_dimensions.height}};

	cmd.setScissor(0, scissor);

	// Since we declared VK_DYNAMIC_STATE_CULL_MODE as dynamic in the pipeline,
	// we need to set the cull mode here. VK_CULL_MODE_NONE disables face culling,
	// meaning both front and back faces will be rendered.
	cmd.setCullMode(vk::CullModeFlagBits::eNone);

	// Since we declared VK_DYNAMIC_STATE_FRONT_FACE as dynamic,
	// we need to specify the winding order considered as the front face.
	// VK_FRONT_FACE_CLOCKWISE indicates that vertices defined in clockwise order
	// are considered front-facing.
	cmd.setFrontFace(vk::FrontFace::eClockwise);

	// Since we declared VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY as dynamic,
	// we need to set the primitive topology here. VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
	// tells Vulkan that the input vertex data should be interpreted as a list of triangles.
	cmd.setPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);

	// Bind the vertex buffer
	cmd.bindVertexBuffers(0, context.vertex_buffer, {0});

	// Draw three vertices with one instance.
	cmd.draw(vertices.size(), 1, 0, 0);

	// Complete rendering.
	cmd.endRendering();

	// After rendering , transition the swapchain image to PRESENT_SRC
	transition_image_layout(cmd,
	                        context.swapchain_images[swapchain_index],
	                        vk::ImageLayout::eColorAttachmentOptimal,
	                        vk::ImageLayout::ePresentSrcKHR,
	                        vk::AccessFlagBits2::eColorAttachmentWrite,                // srcAccessMask
	                        {},                                                        // dstAccessMask
	                        vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
	                        vk::PipelineStageFlagBits2::eBottomOfPipe                  // dstStage
	);

	// Complete the command buffer.
	cmd.end();

	// Submit it to the queue with a release semaphore.
	if (!context.per_frame[swapchain_index].swapchain_release_semaphore)
	{
		context.per_frame[swapchain_index].swapchain_release_semaphore = context.device.createSemaphore({});
	}

	// Using TopOfPipe here to ensure that the command buffer does not begin executing any pipeline stages
	// (including the layout transition) until the swapchain image is actually acquired (signaled by the semaphore).
	// This prevents the GPU from starting operations too early and guarantees that the image is ready
	// before any rendering commands run.
	vk::PipelineStageFlags wait_stage = {vk::PipelineStageFlagBits::eTopOfPipe};

	vk::SubmitInfo info{.waitSemaphoreCount   = 1,
	                    .pWaitSemaphores      = &context.per_frame[swapchain_index].swapchain_acquire_semaphore,
	                    .pWaitDstStageMask    = &wait_stage,
	                    .commandBufferCount   = 1,
	                    .pCommandBuffers      = &cmd,
	                    .signalSemaphoreCount = 1,
	                    .pSignalSemaphores    = &context.per_frame[swapchain_index].swapchain_release_semaphore};

	// Submit command buffer to graphics queue
	context.queue.submit(info, context.per_frame[swapchain_index].queue_submit_fence);
}

/**
 * @brief Tears down the frame data.
 * @param per_frame The data of a frame.
 */
void HPPHelloTriangleV13::teardown_per_frame(PerFrame &per_frame)
{
	if (per_frame.queue_submit_fence)
	{
		context.device.destroyFence(per_frame.queue_submit_fence);
	}

	if (per_frame.primary_command_buffer)
	{
		context.device.freeCommandBuffers(per_frame.primary_command_pool, per_frame.primary_command_buffer);
	}

	if (per_frame.primary_command_pool)
	{
		context.device.destroyCommandPool(per_frame.primary_command_pool);
	}

	if (per_frame.swapchain_acquire_semaphore)
	{
		context.device.destroySemaphore(per_frame.swapchain_acquire_semaphore);
	}

	if (per_frame.swapchain_release_semaphore)
	{
		context.device.destroySemaphore(per_frame.swapchain_release_semaphore);
	}
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
void HPPHelloTriangleV13::transition_image_layout(vk::CommandBuffer       cmd,
                                                  vk::Image               image,
                                                  vk::ImageLayout         oldLayout,
                                                  vk::ImageLayout         newLayout,
                                                  vk::AccessFlags2        srcAccessMask,
                                                  vk::AccessFlags2        dstAccessMask,
                                                  vk::PipelineStageFlags2 srcStage,
                                                  vk::PipelineStageFlags2 dstStage)
{
	// Initialize the VkImageMemoryBarrier2 structure
	vk::ImageMemoryBarrier2 image_barrier{
	    // Specify the pipeline stages and access masks for the barrier
	    .srcStageMask  = srcStage,             // Source pipeline stage mask
	    .srcAccessMask = srcAccessMask,        // Source access mask
	    .dstStageMask  = dstStage,             // Destination pipeline stage mask
	    .dstAccessMask = dstAccessMask,        // Destination access mask

	    // Specify the old and new layouts of the image
	    .oldLayout = oldLayout,        // Current layout of the image
	    .newLayout = newLayout,        // Target layout of the image

	    // We are not changing the ownership between queues
	    .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
	    .dstQueueFamilyIndex = vk::QueueFamilyIgnored,

	    // Specify the image to be affected by this barrier
	    .image = image,

	    // Define the subresource range (which parts of the image are affected)
	    .subresourceRange = {
	        .aspectMask     = vk::ImageAspectFlagBits::eColor,        // Affects the color aspect of the image
	        .baseMipLevel   = 0,                                      // Start at mip level 0
	        .levelCount     = 1,                                      // Number of mip levels affected
	        .baseArrayLayer = 0,                                      // Start at array layer 0
	        .layerCount     = 1                                       // Number of array layers affected
	    }};

	// Initialize the VkDependencyInfo structure
	vk::DependencyInfo dependency_info{
	    .dependencyFlags         = {},                   // No special dependency flags
	    .imageMemoryBarrierCount = 1,                    // Number of image memory barriers
	    .pImageMemoryBarriers    = &image_barrier        // Pointer to the image memory barrier(s)
	};

	// Record the pipeline barrier into the command buffer
	cmd.pipelineBarrier2(dependency_info);
}

/**
 * @brief Validates a list of required extensions, comparing it with the available ones.
 *
 * @param required A vector containing required extension names.
 * @param available A VkExtensionProperties object containing available extensions.
 * @return true if all required extensions are available
 * @return false otherwise
 */
bool HPPHelloTriangleV13::validate_extensions(const std::vector<const char *>            &required,
                                              const std::vector<vk::ExtensionProperties> &available)
{
	return std::ranges::all_of(required,
	                           [&available](auto const &extension_name) {
		                           bool found = std::ranges::any_of(
		                               available, [&extension_name](auto const &ep) { return strcmp(ep.extensionName, extension_name) == 0; });
		                           if (!found)
		                           {
			                           // Output an error message for the missing extension
			                           LOGE("Error: Required extension not found: {}", extension_name);
		                           }
		                           return found;
	                           });
}

std::unique_ptr<vkb::Application> create_hpp_hello_triangle_1_3()
{
	return std::make_unique<HPPHelloTriangleV13>();
}
