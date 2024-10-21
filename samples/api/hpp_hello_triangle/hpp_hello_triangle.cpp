/* Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "hpp_hello_triangle.h"

#include <common/hpp_error.h>
#include <common/hpp_vk_common.h>
#include <core/util/logging.hpp>
#include <filesystem/legacy.h>
#include <hpp_glsl_compiler.h>
#include <platform/window.h>

// Note: the default dispatcher is instantiated in hpp_api_vulkan_sample.cpp.
//			 Even though, that file is not part of this sample, it's part of the sample-project!

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
/// @brief A debug callback called from Vulkan validation layers.
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
#endif

/**
 * @brief Validates a list of required extensions, comparing it with the available ones.
 *
 * @param required A vector containing required extension names.
 * @param available A vk::ExtensionProperties object containing available extensions.
 * @return true if all required extensions are available
 * @return false otherwise
 */
bool validate_extensions(const std::vector<const char *>            &required,
                         const std::vector<vk::ExtensionProperties> &available)
{
	// inner find_if gives true if the extension was not found
	// outer find_if gives true if none of the extensions were not found, that is if all extensions were found
	return std::find_if(required.begin(),
	                    required.end(),
	                    [&available](auto extension) {
		                    return std::find_if(available.begin(),
		                                        available.end(),
		                                        [&extension](auto const &ep) {
			                                        return strcmp(ep.extensionName, extension) == 0;
		                                        }) == available.end();
	                    }) == required.end();
}

bool validate_layers(const std::vector<const char *>        &required,
                     const std::vector<vk::LayerProperties> &available)
{
	// inner find_if returns true if the layer was not found
	// outer find_if returns iterator to the not found layer, if any
	auto requiredButNotFoundIt = std::find_if(required.begin(),
	                                          required.end(),
	                                          [&available](auto layer) {
		                                          return std::find_if(available.begin(),
		                                                              available.end(),
		                                                              [&layer](auto const &lp) {
			                                                              return strcmp(lp.layerName, layer) == 0;
		                                                              }) == available.end();
	                                          });
	if (requiredButNotFoundIt != required.end())
	{
		LOGE("Validation Layer {} not found", *requiredButNotFoundIt);
	}
	return (requiredButNotFoundIt == required.end());
}

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

HPPHelloTriangle::HPPHelloTriangle()
{
}

HPPHelloTriangle::~HPPHelloTriangle()
{
	// Don't release anything until the GPU is completely idle.
	device.waitIdle();

	teardown_framebuffers();

	for (auto &pfd : per_frame_data)
	{
		teardown_per_frame(pfd);
	}

	per_frame_data.clear();

	for (auto semaphore : recycled_semaphores)
	{
		device.destroySemaphore(semaphore);
	}

	if (pipeline)
	{
		device.destroyPipeline(pipeline);
	}

	if (pipeline_layout)
	{
		device.destroyPipelineLayout(pipeline_layout);
	}

	if (render_pass)
	{
		device.destroyRenderPass(render_pass);
	}

	for (auto image_view : swapchain_data.image_views)
	{
		device.destroyImageView(image_view);
	}

	if (swapchain_data.swapchain)
	{
		device.destroySwapchainKHR(swapchain_data.swapchain);
	}

	if (surface)
	{
		instance.destroySurfaceKHR(surface);
	}

	if (device)
	{
		device.destroy();
	}

	if (debug_utils_messenger)
	{
		instance.destroyDebugUtilsMessengerEXT(debug_utils_messenger);
	}

	instance.destroy();
}

bool HPPHelloTriangle::prepare(const vkb::ApplicationOptions &options)
{
	if (Application::prepare(options))
	{
		instance = create_instance({VK_KHR_SURFACE_EXTENSION_NAME}, {});
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
		debug_utils_messenger = instance.createDebugUtilsMessengerEXT(debug_utils_create_info);
#endif

		select_physical_device_and_surface();

		const vkb::Window::Extent &extent = options.window->get_extent();
		swapchain_data.extent.width       = extent.width;
		swapchain_data.extent.height      = extent.height;

		// create a device
		device = create_device({VK_KHR_SWAPCHAIN_EXTENSION_NAME});

		// get the (graphics) queue
		queue = device.getQueue(graphics_queue_index, 0);

		init_swapchain();

		// Create the necessary objects for rendering.
		render_pass = create_render_pass();

		// Create a blank pipeline layout.
		// We are not binding any resources to the pipeline in this first sample.
		pipeline_layout = device.createPipelineLayout({});

		pipeline = create_graphics_pipeline();

		init_framebuffers();
	}

	return true;
}

void HPPHelloTriangle::update(float delta_time)
{
	vk::Result res;
	uint32_t   index;
	std::tie(res, index) = acquire_next_image();

	// Handle outdated error in acquire.
	if (res == vk::Result::eSuboptimalKHR || res == vk::Result::eErrorOutOfDateKHR)
	{
		resize(swapchain_data.extent.width, swapchain_data.extent.height);
		std::tie(res, index) = acquire_next_image();
	}

	if (res != vk::Result::eSuccess)
	{
		queue.waitIdle();
		return;
	}

	render_triangle(index);

	// Present swapchain image
	vk::PresentInfoKHR present_info(per_frame_data[index].swapchain_release_semaphore, swapchain_data.swapchain, index);
	res = queue.presentKHR(present_info);

	// Handle Outdated error in present.
	if (res == vk::Result::eSuboptimalKHR || res == vk::Result::eErrorOutOfDateKHR)
	{
		resize(swapchain_data.extent.width, swapchain_data.extent.height);
	}
	else if (res != vk::Result::eSuccess)
	{
		LOGE("Failed to present swapchain image.");
	}
}

bool HPPHelloTriangle::resize(const uint32_t, const uint32_t)
{
	if (!device)
	{
		return false;
	}

	vk::SurfaceCapabilitiesKHR surface_properties = gpu.getSurfaceCapabilitiesKHR(surface);

	// Only rebuild the swapchain if the dimensions have changed
	if (surface_properties.currentExtent == swapchain_data.extent)
	{
		return false;
	}

	device.waitIdle();
	teardown_framebuffers();

	init_swapchain();
	init_framebuffers();
	return true;
}

/**
 * @brief Acquires an image from the swapchain.
 * @param[out] image The swapchain index for the acquired image.
 * @returns Vulkan result code
 */
std::pair<vk::Result, uint32_t> HPPHelloTriangle::acquire_next_image()
{
	vk::Semaphore acquire_semaphore;
	if (recycled_semaphores.empty())
	{
		acquire_semaphore = device.createSemaphore({});
	}
	else
	{
		acquire_semaphore = recycled_semaphores.back();
		recycled_semaphores.pop_back();
	}

	vk::Result res;
	uint32_t   image;
	std::tie(res, image) = device.acquireNextImageKHR(swapchain_data.swapchain, UINT64_MAX, acquire_semaphore);

	if (res != vk::Result::eSuccess)
	{
		recycled_semaphores.push_back(acquire_semaphore);
		return {res, image};
	}

	// If we have outstanding fences for this swapchain image, wait for them to complete first.
	// After begin frame returns, it is safe to reuse or delete resources which
	// were used previously.
	//
	// We wait for fences which completes N frames earlier, so we do not stall,
	// waiting for all GPU work to complete before this returns.
	// Normally, this doesn't really block at all,
	// since we're waiting for old frames to have been completed, but just in case.
	if (per_frame_data[image].queue_submit_fence)
	{
		(void) device.waitForFences(per_frame_data[image].queue_submit_fence, true, UINT64_MAX);
		device.resetFences(per_frame_data[image].queue_submit_fence);
	}

	if (per_frame_data[image].primary_command_pool)
	{
		device.resetCommandPool(per_frame_data[image].primary_command_pool);
	}

	// Recycle the old semaphore back into the semaphore manager.
	vk::Semaphore old_semaphore = per_frame_data[image].swapchain_acquire_semaphore;

	if (old_semaphore)
	{
		recycled_semaphores.push_back(old_semaphore);
	}

	per_frame_data[image].swapchain_acquire_semaphore = acquire_semaphore;

	return {vk::Result::eSuccess, image};
}

vk::Device HPPHelloTriangle::create_device(const std::vector<const char *> &required_device_extensions)
{
	std::vector<vk::ExtensionProperties> device_extensions = gpu.enumerateDeviceExtensionProperties();

	if (!validate_extensions(required_device_extensions, device_extensions))
	{
		throw std::runtime_error("Required device extensions are missing, will try without.");
	}

	// Create a device with one queue
	float                     queue_priority = 1.0f;
	vk::DeviceQueueCreateInfo queue_info({}, graphics_queue_index, 1, &queue_priority);
	vk::DeviceCreateInfo      device_info({}, queue_info, {}, required_device_extensions);
	vk::Device                device = gpu.createDevice(device_info);

	// initialize function pointers for device
	VULKAN_HPP_DEFAULT_DISPATCHER.init(device);

	return device;
}

vk::Pipeline HPPHelloTriangle::create_graphics_pipeline()
{
	// Load our SPIR-V shaders.
	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages{
	    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, create_shader_module("triangle.vert"), "main"),
	    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, create_shader_module("triangle.frag"), "main")};

	vk::PipelineVertexInputStateCreateInfo vertex_input;

	// Our attachment will write to all color channels, but no blending is enabled.
	vk::PipelineColorBlendAttachmentState blend_attachment;
	blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	// Disable all depth testing.
	vk::PipelineDepthStencilStateCreateInfo depth_stencil;

	vk::Pipeline pipeline = vkb::common::create_graphics_pipeline(device,
	                                                              nullptr,
	                                                              shader_stages,
	                                                              vertex_input,
	                                                              vk::PrimitiveTopology::eTriangleList,        // We will use triangle lists to draw geometry.
	                                                              0,
	                                                              vk::PolygonMode::eFill,
	                                                              vk::CullModeFlagBits::eBack,
	                                                              vk::FrontFace::eClockwise,
	                                                              {blend_attachment},
	                                                              depth_stencil,
	                                                              pipeline_layout,        // We need to specify the pipeline layout
	                                                              render_pass);           // and the render pass up front as well

	// Pipeline is baked, we can delete the shader modules now.
	device.destroyShaderModule(shader_stages[0].module);
	device.destroyShaderModule(shader_stages[1].module);

	return pipeline;
}

vk::ImageView HPPHelloTriangle::create_image_view(vk::Image image)
{
	vk::ImageViewCreateInfo image_view_create_info({},
	                                               image,
	                                               vk::ImageViewType::e2D,
	                                               swapchain_data.format,
	                                               {vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA},
	                                               {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
	return device.createImageView(image_view_create_info);
}

vk::Instance HPPHelloTriangle::create_instance(std::vector<const char *> const &required_instance_extensions, std::vector<const char *> const &required_validation_layers)
{
#if defined(_HPP_VULKAN_LIBRARY)
	static vk::DynamicLoader dl(_HPP_VULKAN_LIBRARY);
#else
	static vk::DynamicLoader dl;
#endif
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	std::vector<vk::ExtensionProperties> available_instance_extensions = vk::enumerateInstanceExtensionProperties();

	std::vector<const char *> active_instance_extensions(required_instance_extensions);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	active_instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

#if (defined(VKB_ENABLE_PORTABILITY))
	active_instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	bool portability_enumeration_available = false;
	if (std::any_of(available_instance_extensions.begin(),
	                available_instance_extensions.end(),
	                [](vk::ExtensionProperties const &extension) { return strcmp(extension.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0; }))
	{
		active_instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
		portability_enumeration_available = true;
	}
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

	if (!validate_extensions(active_instance_extensions, available_instance_extensions))
	{
		throw std::runtime_error("Required instance extensions are missing.");
	}

	std::vector<vk::LayerProperties> supported_validation_layers = vk::enumerateInstanceLayerProperties();

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

	vk::ApplicationInfo app("HPP Hello Triangle", {}, "Vulkan Samples", {}, VK_MAKE_VERSION(1, 0, 0));

	vk::InstanceCreateInfo instance_info({}, &app, requested_validation_layers, active_instance_extensions);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	debug_utils_create_info =
	    vk::DebugUtilsMessengerCreateInfoEXT({},
	                                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
	                                         vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
	                                         debug_utils_messenger_callback);

	instance_info.pNext = &debug_utils_create_info;
#endif

#if (defined(VKB_ENABLE_PORTABILITY))
	if (portability_enumeration_available)
	{
		instance_info.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
	}
#endif

	// Create the Vulkan instance
	vk::Instance instance = vk::createInstance(instance_info);

	// initialize function pointers for instance
	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

#if defined(VK_USE_PLATFORM_DISPLAY_KHR) || defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_USE_PLATFORM_METAL_EXT)
	// we need some additional initializing for this platform!
	if (volkInitialize())
	{
		throw std::runtime_error("Failed to initialize volk.");
	}
	volkLoadInstance(instance);
#endif

	return instance;
}

vk::RenderPass HPPHelloTriangle::create_render_pass()
{
	vk::AttachmentDescription attachment({},
	                                     swapchain_data.format,                  // Backbuffer format
	                                     vk::SampleCountFlagBits::e1,            // Not multisampled
	                                     vk::AttachmentLoadOp::eClear,           // When starting the frame, we want tiles to be cleared
	                                     vk::AttachmentStoreOp::eStore,          // When ending the frame, we want tiles to be written out
	                                     vk::AttachmentLoadOp::eDontCare,        // Don't care about stencil since we're not using it
	                                     vk::AttachmentStoreOp::eDontCare,
	                                     vk::ImageLayout::eUndefined,             // The image layout will be undefined when the render pass begins
	                                     vk::ImageLayout::ePresentSrcKHR);        // After the render pass is complete, we will transition to ePresentSrcKHR layout

	// We have one subpass. This subpass has one color attachment.
	// While executing this subpass, the attachment will be in attachment optimal layout.
	vk::AttachmentReference color_ref(0, vk::ImageLayout::eColorAttachmentOptimal);

	// We will end up with two transitions.
	// The first one happens right before we start subpass #0, where
	// eUndefined is transitioned into eColorAttachmentOptimal.
	// The final layout in the render pass attachment states ePresentSrcKHR, so we
	// will get a final transition from eColorAttachmentOptimal to ePresetSrcKHR.
	vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, color_ref);

	// Create a dependency to external events.
	// We need to wait for the WSI semaphore to signal.
	// Only pipeline stages which depend on eColorAttachmentOutput will
	// actually wait for the semaphore, so we must also wait for that pipeline stage.
	vk::SubpassDependency dependency(/*srcSubpass   */ VK_SUBPASS_EXTERNAL,
	                                 /*dstSubpass   */ 0,
	                                 /*srcStageMask */ vk::PipelineStageFlagBits::eColorAttachmentOutput,
	                                 /*dstStageMask */ vk::PipelineStageFlagBits::eColorAttachmentOutput,
	                                 // Since we changed the image layout, we need to make the memory visible to
	                                 // color attachment to modify.
	                                 /*srcAccessMask*/ {},
	                                 /*dstAccessMask*/ vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

	// Finally, create the renderpass.
	vk::RenderPassCreateInfo rp_info({}, attachment, subpass, dependency);
	return device.createRenderPass(rp_info);
}

/**
 * @brief Helper function to load a shader module.
 * @param path The path for the shader (relative to the assets directory).
 * @returns A vk::ShaderModule handle. Aborts execution if shader creation fails.
 */
vk::ShaderModule HPPHelloTriangle::create_shader_module(const char *path)
{
	static const std::map<std::string, vk::ShaderStageFlagBits> shader_stage_map = {{"comp", vk::ShaderStageFlagBits::eCompute},
	                                                                                {"frag", vk::ShaderStageFlagBits::eFragment},
	                                                                                {"geom", vk::ShaderStageFlagBits::eGeometry},
	                                                                                {"tesc", vk::ShaderStageFlagBits::eTessellationControl},
	                                                                                {"tese", vk::ShaderStageFlagBits::eTessellationEvaluation},
	                                                                                {"vert", vk::ShaderStageFlagBits::eVertex}};
	vkb::HPPGLSLCompiler                                        glsl_compiler;

	auto buffer = vkb::fs::read_shader_binary(path);

	std::string file_ext = path;

	// Extract extension name from the glsl shader file
	file_ext = file_ext.substr(file_ext.find_last_of(".") + 1);

	std::vector<uint32_t> spirvCode;
	std::string           info_log;

	// Compile the GLSL source
	auto stageIt = shader_stage_map.find(file_ext);
	if (stageIt == shader_stage_map.end())
	{
		throw std::runtime_error("File extension `" + file_ext + "` does not have a vulkan shader stage.");
	}
	if (!glsl_compiler.compile_to_spirv(stageIt->second, buffer, "main", {}, spirvCode, info_log))
	{
		LOGE("Failed to compile shader, Error: {}", info_log.c_str());
		return nullptr;
	}

	return device.createShaderModule({{}, spirvCode});
}

vk::SwapchainKHR
    HPPHelloTriangle::create_swapchain(vk::Extent2D const &swapchain_extent, vk::SurfaceFormatKHR surface_format, vk::SwapchainKHR old_swapchain)
{
	vk::SurfaceCapabilitiesKHR surface_properties = gpu.getSurfaceCapabilitiesKHR(surface);

	// Determine the number of vk::Image's to use in the swapchain.
	// Ideally, we desire to own 1 image at a time, the rest of the images can
	// either be rendered to and/or being queued up for display.
	uint32_t desired_swapchain_images = surface_properties.minImageCount + 1;
	if ((surface_properties.maxImageCount > 0) && (desired_swapchain_images > surface_properties.maxImageCount))
	{
		// Application must settle for fewer images than desired.
		desired_swapchain_images = surface_properties.maxImageCount;
	}

	// Figure out a suitable surface transform.
	vk::SurfaceTransformFlagBitsKHR pre_transform =
	    (surface_properties.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) ? vk::SurfaceTransformFlagBitsKHR::eIdentity : surface_properties.currentTransform;

	// Find a supported composite type.
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

	// FIFO must be supported by all implementations.
	vk::PresentModeKHR swapchain_present_mode = vk::PresentModeKHR::eFifo;

	vk::SwapchainCreateInfoKHR swapchain_create_info;
	swapchain_create_info.surface            = surface;
	swapchain_create_info.minImageCount      = desired_swapchain_images;
	swapchain_create_info.imageFormat        = surface_format.format;
	swapchain_create_info.imageColorSpace    = surface_format.colorSpace;
	swapchain_create_info.imageExtent.width  = swapchain_extent.width;
	swapchain_create_info.imageExtent.height = swapchain_extent.height;
	swapchain_create_info.imageArrayLayers   = 1;
	swapchain_create_info.imageUsage         = vk::ImageUsageFlagBits::eColorAttachment;
	swapchain_create_info.imageSharingMode   = vk::SharingMode::eExclusive;
	swapchain_create_info.preTransform       = pre_transform;
	swapchain_create_info.compositeAlpha     = composite;
	swapchain_create_info.presentMode        = swapchain_present_mode;
	swapchain_create_info.clipped            = true;
	swapchain_create_info.oldSwapchain       = old_swapchain;

	return device.createSwapchainKHR(swapchain_create_info);
}

/**
 * @brief Initializes the Vulkan framebuffers.
 */
void HPPHelloTriangle::init_framebuffers()
{
	assert(swapchain_data.framebuffers.empty());

	// Create framebuffer for each swapchain image view
	for (auto &image_view : swapchain_data.image_views)
	{
		// create the framebuffer.
		swapchain_data.framebuffers.push_back(vkb::common::create_framebuffer(device, render_pass, {image_view}, swapchain_data.extent));
	}
}

/**
 * @brief Initializes the Vulkan swapchain.
 */
void HPPHelloTriangle::init_swapchain()
{
	vk::SurfaceCapabilitiesKHR surface_properties = gpu.getSurfaceCapabilitiesKHR(surface);

	vk::Extent2D swapchain_extent = (surface_properties.currentExtent.width == 0xFFFFFFFF) ? swapchain_data.extent : surface_properties.currentExtent;

	vk::SurfaceFormatKHR surface_format = vkb::common::select_surface_format(gpu, surface);

	vk::SwapchainKHR old_swapchain = swapchain_data.swapchain;

	swapchain_data.swapchain = create_swapchain(swapchain_extent, surface_format, old_swapchain);

	if (old_swapchain)
	{
		for (vk::ImageView image_view : swapchain_data.image_views)
		{
			device.destroyImageView(image_view);
		}

		size_t image_count = device.getSwapchainImagesKHR(old_swapchain).size();

		for (size_t i = 0; i < image_count; i++)
		{
			teardown_per_frame(per_frame_data[i]);
		}

		swapchain_data.image_views.clear();

		device.destroySwapchainKHR(old_swapchain);
	}

	swapchain_data.extent = swapchain_extent;
	swapchain_data.format = surface_format.format;

	/// The swapchain images.
	std::vector<vk::Image> swapchain_images = device.getSwapchainImagesKHR(swapchain_data.swapchain);
	size_t                 image_count      = swapchain_images.size();

	// Initialize per-frame resources.
	// Every swapchain image has its own command pool and fence manager.
	// This makes it very easy to keep track of when we can reset command buffers and such.
	per_frame_data.clear();
	per_frame_data.resize(image_count);

	for (size_t frame = 0; frame < image_count; frame++)
	{
		auto &pfd                  = per_frame_data[frame];
		pfd.queue_submit_fence     = device.createFence({vk::FenceCreateFlagBits::eSignaled});
		pfd.primary_command_pool   = device.createCommandPool({vk::CommandPoolCreateFlagBits::eTransient, graphics_queue_index});
		pfd.primary_command_buffer = vkb::common::allocate_command_buffer(device, pfd.primary_command_pool);
	}

	for (size_t i = 0; i < image_count; i++)
	{
		// Create an image view which we can render into.
		swapchain_data.image_views.push_back(create_image_view(swapchain_images[i]));
	}
}

/**
 * @brief Renders a triangle to the specified swapchain image.
 * @param swapchain_index The swapchain index for the image being rendered.
 */
void HPPHelloTriangle::render_triangle(uint32_t swapchain_index)
{
	// Render to this framebuffer.
	vk::Framebuffer framebuffer = swapchain_data.framebuffers[swapchain_index];

	// Allocate or re-use a primary command buffer.
	vk::CommandBuffer cmd = per_frame_data[swapchain_index].primary_command_buffer;

	// We will only submit this once before it's recycled.
	vk::CommandBufferBeginInfo begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	// Begin command recording
	cmd.begin(begin_info);

	// Set clear color values.
	vk::ClearValue clear_value;
	clear_value.color = vk::ClearColorValue(std::array<float, 4>({{0.01f, 0.01f, 0.033f, 1.0f}}));

	// Begin the render pass.
	vk::RenderPassBeginInfo rp_begin(render_pass, framebuffer, {{0, 0}, {swapchain_data.extent.width, swapchain_data.extent.height}},
	                                 clear_value);
	// We will add draw commands in the same command buffer.
	cmd.beginRenderPass(rp_begin, vk::SubpassContents::eInline);

	// Bind the graphics pipeline.
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	vk::Viewport vp(0.0f, 0.0f, static_cast<float>(swapchain_data.extent.width), static_cast<float>(swapchain_data.extent.height), 0.0f, 1.0f);
	// Set viewport dynamically
	cmd.setViewport(0, vp);

	vk::Rect2D scissor({0, 0}, {swapchain_data.extent.width, swapchain_data.extent.height});
	// Set scissor dynamically
	cmd.setScissor(0, scissor);

	// Draw three vertices with one instance.
	cmd.draw(3, 1, 0, 0);

	// Complete render pass.
	cmd.endRenderPass();

	// Complete the command buffer.
	cmd.end();

	// Submit it to the queue with a release semaphore.
	if (!per_frame_data[swapchain_index].swapchain_release_semaphore)
	{
		per_frame_data[swapchain_index].swapchain_release_semaphore = device.createSemaphore({});
	}

	vk::PipelineStageFlags wait_stage{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	vk::SubmitInfo info(per_frame_data[swapchain_index].swapchain_acquire_semaphore, wait_stage, cmd,
	                    per_frame_data[swapchain_index].swapchain_release_semaphore);
	// Submit command buffer to graphics queue
	queue.submit(info, per_frame_data[swapchain_index].queue_submit_fence);
}

/**
 * @brief Select a physical device.
 */
void HPPHelloTriangle::select_physical_device_and_surface()
{
	std::vector<vk::PhysicalDevice> gpus = instance.enumeratePhysicalDevices();

	bool found_graphics_queue_index = false;
	for (size_t i = 0; i < gpus.size() && !found_graphics_queue_index; i++)
	{
		gpu = gpus[i];

		std::vector<vk::QueueFamilyProperties> queue_family_properties = gpu.getQueueFamilyProperties();

		if (queue_family_properties.empty())
		{
			throw std::runtime_error("No queue family found.");
		}

		if (surface)
		{
			instance.destroySurfaceKHR(surface);
		}

		surface = static_cast<vk::SurfaceKHR>(window->create_surface(static_cast<VkInstance>(instance), static_cast<VkPhysicalDevice>(gpu)));
		if (!surface)
		{
			throw std::runtime_error("Failed to create window surface.");
		}

		for (uint32_t j = 0; j < vkb::to_u32(queue_family_properties.size()); j++)
		{
			vk::Bool32 supports_present = gpu.getSurfaceSupportKHR(j, surface);

			// Find a queue family which supports graphics and presentation.
			if ((queue_family_properties[j].queueFlags & vk::QueueFlagBits::eGraphics) && supports_present)
			{
				graphics_queue_index       = j;
				found_graphics_queue_index = true;
				break;
			}
		}
	}

	if (!found_graphics_queue_index)
	{
		LOGE("Did not find suitable queue which supports graphics and presentation.");
	}
}

/**
 * @brief Tears down the framebuffers. If our swapchain changes, we will call this, and create a new swapchain.
 */
void HPPHelloTriangle::teardown_framebuffers()
{
	// Wait until device is idle before teardown.
	queue.waitIdle();

	for (auto &framebuffer : swapchain_data.framebuffers)
	{
		device.destroyFramebuffer(framebuffer);
	}

	swapchain_data.framebuffers.clear();
}

/**
 * @brief Tears down the frame data.
 * @param per_frame_data The data of a frame.
 */
void HPPHelloTriangle::teardown_per_frame(FrameData &per_frame_data)
{
	if (per_frame_data.queue_submit_fence)
	{
		device.destroyFence(per_frame_data.queue_submit_fence);
		per_frame_data.queue_submit_fence = nullptr;
	}

	if (per_frame_data.primary_command_buffer)
	{
		device.freeCommandBuffers(per_frame_data.primary_command_pool, per_frame_data.primary_command_buffer);
		per_frame_data.primary_command_buffer = nullptr;
	}

	if (per_frame_data.primary_command_pool)
	{
		device.destroyCommandPool(per_frame_data.primary_command_pool);
		per_frame_data.primary_command_pool = nullptr;
	}

	if (per_frame_data.swapchain_acquire_semaphore)
	{
		device.destroySemaphore(per_frame_data.swapchain_acquire_semaphore);
		per_frame_data.swapchain_acquire_semaphore = nullptr;
	}

	if (per_frame_data.swapchain_release_semaphore)
	{
		device.destroySemaphore(per_frame_data.swapchain_release_semaphore);
		per_frame_data.swapchain_release_semaphore = nullptr;
	}
}

std::unique_ptr<vkb::Application> create_hpp_hello_triangle()
{
	return std::make_unique<HPPHelloTriangle>();
}
