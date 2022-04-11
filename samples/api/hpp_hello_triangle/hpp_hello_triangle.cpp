/* Copyright (c) 2021-2022, NVIDIA CORPORATION. All rights reserved.
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

#include <components/vfs/filesystem.hpp>

#include <common/logging.h>
#include <glsl_compiler.h>

// Note: the default dispatcher is instantiated in hpp_api_vulkan_sample.cpp.
//			 Even though, that file is not part of this sample, it's part of the sample-project!

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
 * @param available A vk::ExtensionProperties object containing available extensions.
 * @return true if all required extensions are available
 * @return false otherwise
 */
bool HPPHelloTriangle::validate_extensions(const std::vector<const char *> &           required,
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

/**
 * @brief Find the vulkan shader stage for a given a string.
 *
 * @param ext A string containing the shader stage name.
 * @return vk::ShaderStageFlagBits The shader stage mapping from the given string, vk::ShaderStageFlagBits::eVertex otherwise.
 */
VkShaderStageFlagBits HPPHelloTriangle::find_shader_stage(const std::string &ext)
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

bool validate_layers(const std::vector<const char *> &       required,
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

/**
 * @brief Initializes the Vulkan instance.
 *
 * @param context A newly created Vulkan context.
 * @param required_instance_extensions The required Vulkan instance extensions.
 * @param required_validation_layers The required Vulkan validation layers
 */
void HPPHelloTriangle::init_instance(Context &                        context,
                                     const std::vector<const char *> &required_instance_extensions,
                                     const std::vector<const char *> &required_validation_layers)
{
	LOGI("Initializing vulkan instance.");

	static vk::DynamicLoader  dl;
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
	    dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	std::vector<vk::ExtensionProperties> instance_extensions = vk::enumerateInstanceExtensionProperties();

	std::vector<const char *> active_instance_extensions(required_instance_extensions);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	active_instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
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

	vk::ApplicationInfo app("HPP Hello Triangle", {}, "Vulkan Samples", VK_MAKE_VERSION(1, 0, 0));

	vk::InstanceCreateInfo instance_info({}, &app, requested_validation_layers, active_instance_extensions);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	vk::DebugReportCallbackCreateInfoEXT debug_report_create_info(vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning, debug_callback);

	instance_info.pNext = &debug_report_create_info;
#endif

	// Create the Vulkan instance
	context.instance = vk::createInstance(instance_info);

	// initialize function pointers for instance
	VULKAN_HPP_DEFAULT_DISPATCHER.init(context.instance);

#if defined(VK_USE_PLATFORM_DISPLAY_KHR)
	// we need some additional initializing for this platform!
	if (volkInitialize())
	{
		throw std::runtime_error("Failed to initialize volk.");
	}
	volkLoadInstance(context.instance);
#endif

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	context.debug_callback = context.instance.createDebugReportCallbackEXT(debug_report_create_info);
#endif
}

/**
 * @brief Select a physical device.
 *
 * @param platform The platform the application is being run on
 */
void HPPHelloTriangle::select_physical_device_and_surface(vkb::platform::HPPPlatform &platform)
{
	std::vector<vk::PhysicalDevice> gpus = context.instance.enumeratePhysicalDevices();

	for (size_t i = 0; i < gpus.size() && (context.graphics_queue_index < 0); i++)
	{
		context.gpu = gpus[i];

		std::vector<vk::QueueFamilyProperties> queue_family_properties = context.gpu.getQueueFamilyProperties();

		if (queue_family_properties.empty())
		{
			throw std::runtime_error("No queue family found.");
		}

		if (context.surface)
		{
			context.instance.destroySurfaceKHR(context.surface);
		}
		context.surface = platform.get_window().create_surface(context.instance, context.gpu);

		for (uint32_t j = 0; j < vkb::to_u32(queue_family_properties.size()); j++)
		{
			vk::Bool32 supports_present = context.gpu.getSurfaceSupportKHR(j, context.surface);

			// Find a queue family which supports graphics and presentation.
			if ((queue_family_properties[j].queueFlags & vk::QueueFlagBits::eGraphics) && supports_present)
			{
				context.graphics_queue_index = j;
				break;
			}
		}
	}

	if (context.graphics_queue_index < 0)
	{
		LOGE("Did not find suitable queue which supports graphics and presentation.");
	}
}

/**
 * @brief Initializes the logical device.
 *
 * @param context A Vulkan context with an instance already set up.
 * @param required_device_extensions The required Vulkan device extensions.
 */
void HPPHelloTriangle::init_device(Context &                        context,
                                   const std::vector<const char *> &required_device_extensions)
{
	LOGI("Initializing vulkan device.");

	std::vector<vk::ExtensionProperties> device_extensions = context.gpu.enumerateDeviceExtensionProperties();

	if (!validate_extensions(required_device_extensions, device_extensions))
	{
		throw std::runtime_error("Required device extensions are missing, will try without.");
	}

	float queue_priority = 1.0f;

	// Create one queue
	vk::DeviceQueueCreateInfo queue_info({}, context.graphics_queue_index, 1, &queue_priority);

	vk::DeviceCreateInfo device_info({}, queue_info, {}, required_device_extensions);

	context.device = context.gpu.createDevice(device_info);
	// initialize function pointers for device
	VULKAN_HPP_DEFAULT_DISPATCHER.init(context.device);

	context.queue = context.device.getQueue(context.graphics_queue_index, 0);
}

/**
 * @brief Initializes per frame data.
 * @param context A newly created Vulkan context.
 * @param per_frame The data of a frame.
 */
void HPPHelloTriangle::init_per_frame(Context &context, PerFrame &per_frame)
{
	per_frame.queue_submit_fence = context.device.createFence({vk::FenceCreateFlagBits::eSignaled});

	vk::CommandPoolCreateInfo cmd_pool_info(vk::CommandPoolCreateFlagBits::eTransient, context.graphics_queue_index);
	per_frame.primary_command_pool = context.device.createCommandPool(cmd_pool_info);

	vk::CommandBufferAllocateInfo cmd_buf_info(per_frame.primary_command_pool, vk::CommandBufferLevel::ePrimary, 1);
	per_frame.primary_command_buffer = context.device.allocateCommandBuffers(cmd_buf_info).front();

	per_frame.device      = context.device;
	per_frame.queue_index = context.graphics_queue_index;
}

/**
 * @brief Tears down the frame data.
 * @param context The Vulkan context.
 * @param per_frame The data of a frame.
 */
void HPPHelloTriangle::teardown_per_frame(Context &context, PerFrame &per_frame)
{
	if (per_frame.queue_submit_fence)
	{
		context.device.destroyFence(per_frame.queue_submit_fence);

		per_frame.queue_submit_fence = nullptr;
	}

	if (per_frame.primary_command_buffer)
	{
		context.device.freeCommandBuffers(per_frame.primary_command_pool, per_frame.primary_command_buffer);

		per_frame.primary_command_buffer = nullptr;
	}

	if (per_frame.primary_command_pool)
	{
		context.device.destroyCommandPool(per_frame.primary_command_pool);

		per_frame.primary_command_pool = nullptr;
	}

	if (per_frame.swapchain_acquire_semaphore)
	{
		context.device.destroySemaphore(per_frame.swapchain_acquire_semaphore);

		per_frame.swapchain_acquire_semaphore = nullptr;
	}

	if (per_frame.swapchain_release_semaphore)
	{
		context.device.destroySemaphore(per_frame.swapchain_release_semaphore);

		per_frame.swapchain_release_semaphore = nullptr;
	}

	per_frame.device      = nullptr;
	per_frame.queue_index = -1;
}

/**
 * @brief Initializes the Vulkan swapchain.
 * @param context A Vulkan context with a physical device already set up.
 */
void HPPHelloTriangle::init_swapchain(Context &context)
{
	vk::SurfaceCapabilitiesKHR surface_properties = context.gpu.getSurfaceCapabilitiesKHR(context.surface);

	std::vector<vk::SurfaceFormatKHR> formats = context.gpu.getSurfaceFormatsKHR(context.surface);

	vk::SurfaceFormatKHR format;
	if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined)
	{
		// There is no preferred format, so pick a default one
		format        = formats[0];
		format.format = vk::Format::eB8G8R8A8Unorm;
	}
	else
	{
		if (formats.empty())
		{
			throw std::runtime_error("Surface has no formats.");
		}

		format.format = vk::Format::eUndefined;
		for (auto &candidate : formats)
		{
			switch (candidate.format)
			{
				case vk::Format::eR8G8B8A8Unorm:
				case vk::Format::eB8G8R8A8Unorm:
				case vk::Format::eA8B8G8R8UnormPack32:
					format = candidate;
					break;

				default:
					break;
			}

			if (format.format != vk::Format::eUndefined)
			{
				break;
			}
		}

		if (format.format == vk::Format::eUndefined)
		{
			format = formats[0];
		}
	}

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

	vk::SwapchainKHR old_swapchain = context.swapchain;

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

	vk::SwapchainCreateInfoKHR info;
	info.surface            = context.surface;
	info.minImageCount      = desired_swapchain_images;
	info.imageFormat        = format.format;
	info.imageColorSpace    = format.colorSpace;
	info.imageExtent.width  = swapchain_size.width;
	info.imageExtent.height = swapchain_size.height;
	info.imageArrayLayers   = 1;
	info.imageUsage         = vk::ImageUsageFlagBits::eColorAttachment;
	info.imageSharingMode   = vk::SharingMode::eExclusive;
	info.preTransform       = pre_transform;
	info.compositeAlpha     = composite;
	info.presentMode        = swapchain_present_mode;
	info.clipped            = true;
	info.oldSwapchain       = old_swapchain;

	context.swapchain = context.device.createSwapchainKHR(info);

	if (old_swapchain)
	{
		for (vk::ImageView image_view : context.swapchain_image_views)
		{
			context.device.destroyImageView(image_view);
		}

		size_t image_count = context.device.getSwapchainImagesKHR(old_swapchain).size();

		for (size_t i = 0; i < image_count; i++)
		{
			teardown_per_frame(context, context.per_frame[i]);
		}

		context.swapchain_image_views.clear();

		context.device.destroySwapchainKHR(old_swapchain);
	}

	context.swapchain_dimensions = {swapchain_size.width, swapchain_size.height, format.format};

	/// The swapchain images.
	std::vector<vk::Image> swapchain_images = context.device.getSwapchainImagesKHR(context.swapchain);
	size_t                 image_count      = swapchain_images.size();

	// Initialize per-frame resources.
	// Every swapchain image has its own command pool and fence manager.
	// This makes it very easy to keep track of when we can reset command buffers and such.
	context.per_frame.clear();
	context.per_frame.resize(image_count);

	for (size_t i = 0; i < image_count; i++)
	{
		init_per_frame(context, context.per_frame[i]);
	}

	vk::ImageViewCreateInfo view_info;
	view_info.viewType                    = vk::ImageViewType::e2D;
	view_info.format                      = context.swapchain_dimensions.format;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.layerCount = 1;
	view_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	view_info.components.r                = vk::ComponentSwizzle::eR;
	view_info.components.g                = vk::ComponentSwizzle::eG;
	view_info.components.b                = vk::ComponentSwizzle::eB;
	view_info.components.a                = vk::ComponentSwizzle::eA;
	for (size_t i = 0; i < image_count; i++)
	{
		// Create an image view which we can render into.
		view_info.image = swapchain_images[i];

		context.swapchain_image_views.push_back(context.device.createImageView(view_info));
	}
}

/**
 * @brief Initializes the Vulkan render pass.
 * @param context A Vulkan context with a device already set up.
 */
void HPPHelloTriangle::init_render_pass(Context &context)
{
	vk::AttachmentDescription attachment;
	// Backbuffer format.
	attachment.format = context.swapchain_dimensions.format;
	// Not multisampled.
	attachment.samples = vk::SampleCountFlagBits::e1;
	// When starting the frame, we want tiles to be cleared.
	attachment.loadOp = vk::AttachmentLoadOp::eClear;
	// When ending the frame, we want tiles to be written out.
	attachment.storeOp = vk::AttachmentStoreOp::eStore;
	// Don't care about stencil since we're not using it.
	attachment.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
	attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

	// The image layout will be undefined when the render pass begins.
	attachment.initialLayout = vk::ImageLayout::eUndefined;
	// After the render pass is complete, we will transition to ePresentSrcKHR layout.
	attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

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
	vk::SubpassDependency dependency;
	dependency.srcSubpass   = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass   = 0;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	// Since we changed the image layout, we need to make the memory visible to
	// color attachment to modify.
	dependency.srcAccessMask = {};
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

	// Finally, create the renderpass.
	vk::RenderPassCreateInfo rp_info({}, attachment, subpass, dependency);

	context.render_pass = context.device.createRenderPass(rp_info);
}

/**
 * @brief Helper function to load a shader module.
 * @param context A Vulkan context with a device.
 * @param path The path for the shader (relative to the assets directory).
 * @returns A vk::ShaderModule handle. Aborts execution if shader creation fails.
 */
vk::ShaderModule HPPHelloTriangle::load_shader_module(Context &context, const char *path)
{
	vkb::GLSLCompiler glsl_compiler;

	auto &fs = vfs::instance();

	std::shared_ptr<vfs::Blob> blob;

	if (fs.read_file(std::string{"/shaders/"} + path, &blob) != vfs::status::Success)
	{
		throw std::runtime_error{std::string{"failed to load /shaders/"} + path};
	}

	std::string file_ext = path;

	// Extract extension name from the glsl shader file
	file_ext = file_ext.substr(file_ext.find_last_of(".") + 1);

	std::vector<uint32_t> spirv;
	std::string           info_log;

	// Compile the GLSL source
	if (!glsl_compiler.compile_to_spirv(find_shader_stage(file_ext), blob->binary(), "main", {}, spirv, info_log))
	{
		LOGE("Failed to compile shader, Error: {}", info_log.c_str());
		return nullptr;
	}

	vk::ShaderModuleCreateInfo module_info({}, spirv);

	return context.device.createShaderModule(module_info);
}

/**
 * @brief Initializes the Vulkan pipeline.
 * @param context A Vulkan context with a device and a render pass already set up.
 */
void HPPHelloTriangle::init_pipeline(Context &context)
{
	// Create a blank pipeline layout.
	// We are not binding any resources to the pipeline in this first sample.
	context.pipeline_layout = context.device.createPipelineLayout({});

	vk::PipelineVertexInputStateCreateInfo vertex_input;

	// Specify we will use triangle lists to draw geometry.
	vk::PipelineInputAssemblyStateCreateInfo input_assembly({}, vk::PrimitiveTopology::eTriangleList);

	// Specify rasterization state.
	vk::PipelineRasterizationStateCreateInfo raster;
	raster.cullMode  = vk::CullModeFlagBits::eBack;
	raster.frontFace = vk::FrontFace::eClockwise;
	raster.lineWidth = 1.0f;

	// Our attachment will write to all color channels, but no blending is enabled.
	vk::PipelineColorBlendAttachmentState blend_attachment;
	blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendStateCreateInfo blend({}, {}, {}, blend_attachment);

	// We will have one viewport and scissor box.
	vk::PipelineViewportStateCreateInfo viewport;
	viewport.viewportCount = 1;
	viewport.scissorCount  = 1;

	// Disable all depth testing.
	vk::PipelineDepthStencilStateCreateInfo depth_stencil;

	// No multisampling.
	vk::PipelineMultisampleStateCreateInfo multisample({}, vk::SampleCountFlagBits::e1);

	// Specify that these states will be dynamic, i.e. not part of pipeline state object.
	std::array<vk::DynamicState, 2> dynamics{vk::DynamicState::eViewport, vk::DynamicState::eScissor};

	vk::PipelineDynamicStateCreateInfo dynamic({}, dynamics);

	// Load our SPIR-V shaders.
	std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages{
	    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, load_shader_module(context, "triangle.vert"), "main"),
	    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, load_shader_module(context, "triangle.frag"), "main")};

	vk::GraphicsPipelineCreateInfo pipe({}, shader_stages);
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

	context.pipeline = context.device.createGraphicsPipeline(nullptr, pipe).value;

	// Pipeline is baked, we can delete the shader modules now.
	context.device.destroyShaderModule(shader_stages[0].module);
	context.device.destroyShaderModule(shader_stages[1].module);
}

/**
 * @brief Acquires an image from the swapchain.
 * @param context A Vulkan context with a swapchain already set up.
 * @param[out] image The swapchain index for the acquired image.
 * @returns Vulkan result code
 */
vk::Result HPPHelloTriangle::acquire_next_image(Context &context, uint32_t *image)
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

	vk::Result res;
	std::tie(res, *image) = context.device.acquireNextImageKHR(context.swapchain, UINT64_MAX, acquire_semaphore);

	if (res != vk::Result::eSuccess)
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
	if (context.per_frame[*image].queue_submit_fence)
	{
		context.device.waitForFences(context.per_frame[*image].queue_submit_fence, true, UINT64_MAX);
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

	return vk::Result::eSuccess;
}

/**
 * @brief Renders a triangle to the specified swapchain image.
 * @param context A Vulkan context set up for rendering.
 * @param swapchain_index The swapchain index for the image being rendered.
 */
void HPPHelloTriangle::render_triangle(Context &context, uint32_t swapchain_index)
{
	// Render to this framebuffer.
	vk::Framebuffer framebuffer = context.swapchain_framebuffers[swapchain_index];

	// Allocate or re-use a primary command buffer.
	vk::CommandBuffer cmd = context.per_frame[swapchain_index].primary_command_buffer;

	// We will only submit this once before it's recycled.
	vk::CommandBufferBeginInfo begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	// Begin command recording
	cmd.begin(begin_info);

	// Set clear color values.
	vk::ClearValue clear_value;
	clear_value.color = vk::ClearColorValue(std::array<float, 4>({{0.1f, 0.1f, 0.2f, 1.0f}}));

	// Begin the render pass.
	vk::RenderPassBeginInfo rp_begin(context.render_pass, framebuffer, {{0, 0}, {context.swapchain_dimensions.width, context.swapchain_dimensions.height}},
	                                 clear_value);
	// We will add draw commands in the same command buffer.
	cmd.beginRenderPass(rp_begin, vk::SubpassContents::eInline);

	// Bind the graphics pipeline.
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, context.pipeline);

	vk::Viewport vp(0.0f, 0.0f, static_cast<float>(context.swapchain_dimensions.width), static_cast<float>(context.swapchain_dimensions.height), 0.0f, 1.0f);
	// Set viewport dynamically
	cmd.setViewport(0, vp);

	vk::Rect2D scissor({0, 0}, {context.swapchain_dimensions.width, context.swapchain_dimensions.height});
	// Set scissor dynamically
	cmd.setScissor(0, scissor);

	// Draw three vertices with one instance.
	cmd.draw(3, 1, 0, 0);

	// Complete render pass.
	cmd.endRenderPass();

	// Complete the command buffer.
	cmd.end();

	// Submit it to the queue with a release semaphore.
	if (!context.per_frame[swapchain_index].swapchain_release_semaphore)
	{
		context.per_frame[swapchain_index].swapchain_release_semaphore = context.device.createSemaphore({});
	}

	vk::PipelineStageFlags wait_stage{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	vk::SubmitInfo info(context.per_frame[swapchain_index].swapchain_acquire_semaphore, wait_stage, cmd,
	                    context.per_frame[swapchain_index].swapchain_release_semaphore);
	// Submit command buffer to graphics queue
	context.queue.submit(info, context.per_frame[swapchain_index].queue_submit_fence);
}

/**
 * @brief Presents an image to the swapchain.
 * @param context The Vulkan context, with a swapchain and per-frame resources already set up.
 * @param index The swapchain index previously obtained from @ref acquire_next_image.
 * @returns Vulkan result code
 */
vk::Result HPPHelloTriangle::present_image(Context &context, uint32_t index)
{
	vk::PresentInfoKHR present(context.per_frame[index].swapchain_release_semaphore, context.swapchain, index);
	// Present swapchain image
	return context.queue.presentKHR(present);
}

/**
 * @brief Initializes the Vulkan framebuffers.
 * @param context A Vulkan context with the render pass already set up.
 */
void HPPHelloTriangle::init_framebuffers(Context &context)
{
	vk::Device device = context.device;

	// Create framebuffer for each swapchain image view
	for (auto &image_view : context.swapchain_image_views)
	{
		// Build the framebuffer.
		vk::FramebufferCreateInfo fb_info({}, context.render_pass, image_view, context.swapchain_dimensions.width, context.swapchain_dimensions.height, 1);

		context.swapchain_framebuffers.push_back(device.createFramebuffer(fb_info));
	}
}

/**
 * @brief Tears down the framebuffers. If our swapchain changes, we will call this, and create a new swapchain.
 * @param context The Vulkan context.
 */
void HPPHelloTriangle::teardown_framebuffers(Context &context)
{
	// Wait until device is idle before teardown.
	context.queue.waitIdle();

	for (auto &framebuffer : context.swapchain_framebuffers)
	{
		context.device.destroyFramebuffer(framebuffer);
	}

	context.swapchain_framebuffers.clear();
}

/**
 * @brief Tears down the Vulkan context.
 * @param context The Vulkan context.
 */
void HPPHelloTriangle::teardown(Context &context)
{
	// Don't release anything until the GPU is completely idle.
	context.device.waitIdle();

	teardown_framebuffers(context);

	for (auto &per_frame : context.per_frame)
	{
		teardown_per_frame(context, per_frame);
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

	if (context.render_pass)
	{
		context.device.destroyRenderPass(context.render_pass);
	}

	for (auto image_view : context.swapchain_image_views)
	{
		context.device.destroyImageView(image_view);
	}

	if (context.swapchain)
	{
		context.device.destroySwapchainKHR(context.swapchain);
		context.swapchain = nullptr;
	}

	if (context.surface)
	{
		context.instance.destroySurfaceKHR(context.surface);
		context.surface = nullptr;
	}

	if (context.device)
	{
		context.device.destroy();
		context.device = nullptr;
	}

	if (context.debug_callback)
	{
		context.instance.destroyDebugReportCallbackEXT(context.debug_callback);
		context.debug_callback = nullptr;
	}

	context.instance.destroy();
}

HPPHelloTriangle::HPPHelloTriangle()
{
}

HPPHelloTriangle::~HPPHelloTriangle()
{
	teardown(context);
}

bool HPPHelloTriangle::prepare(vkb::platform::HPPPlatform &platform)
{
	init_instance(context, {VK_KHR_SURFACE_EXTENSION_NAME}, {});
	select_physical_device_and_surface(platform);

	const auto &extent                  = platform.get_window().get_extent();
	context.swapchain_dimensions.width  = extent.width;
	context.swapchain_dimensions.height = extent.height;

	init_device(context, {VK_KHR_SWAPCHAIN_EXTENSION_NAME});

	init_swapchain(context);

	// Create the necessary objects for rendering.
	init_render_pass(context);
	init_pipeline(context);
	init_framebuffers(context);

	return true;
}

void HPPHelloTriangle::update(float delta_time)
{
	uint32_t index;

	auto res = acquire_next_image(context, &index);

	// Handle outdated error in acquire.
	if (res == vk::Result::eSuboptimalKHR || res == vk::Result::eErrorOutOfDateKHR)
	{
		resize(context.swapchain_dimensions.width, context.swapchain_dimensions.height);
		res = acquire_next_image(context, &index);
	}

	if (res != vk::Result::eSuccess)
	{
		context.queue.waitIdle();
		return;
	}

	render_triangle(context, index);
	res = present_image(context, index);

	// Handle Outdated error in present.
	if (res == vk::Result::eSuboptimalKHR || res == vk::Result::eErrorOutOfDateKHR)
	{
		resize(context.swapchain_dimensions.width, context.swapchain_dimensions.height);
	}
	else if (res != vk::Result::eSuccess)
	{
		LOGE("Failed to present swapchain image.");
	}
}

bool HPPHelloTriangle::resize(const uint32_t, const uint32_t)
{
	if (!context.device)
	{
		return false;
	}

	vk::SurfaceCapabilitiesKHR surface_properties = context.gpu.getSurfaceCapabilitiesKHR(context.surface);

	// Only rebuild the swapchain if the dimensions have changed
	if (surface_properties.currentExtent.width == context.swapchain_dimensions.width &&
	    surface_properties.currentExtent.height == context.swapchain_dimensions.height)
	{
		return false;
	}

	context.device.waitIdle();
	teardown_framebuffers(context);

	init_swapchain(context);
	init_framebuffers(context);
	return true;
}

std::unique_ptr<vkb::Application> create_hpp_hello_triangle()
{
	return std::make_unique<HPPHelloTriangle>();
}
