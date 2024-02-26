/*
 * Copyright 2023 Nintendo
 * Copyright 2023, Sascha Willems
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "shader_object.h"
#include <glsl_compiler.h>
#include <heightmap.h>
#include <unordered_map>

ShaderObject::ShaderObject()
{
	title = "Shader Object";
	rng   = std::default_random_engine(12345);        // Use a fixed seed, makes random deterministic.

	// Show that shader object is usable with Vulkan 1.1 + Dynamic Rendering
	set_api_version(VK_API_VERSION_1_1);

	// Enable the Shader Object extension
	add_device_extension(VK_EXT_SHADER_OBJECT_EXTENSION_NAME);

	// Enable extensions for Dynamic Rendering
	add_device_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

	// Enable the Depth Stencil Resolve extension
	add_device_extension(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME);

	// Enable extensions for sample
	add_device_extension(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
}

ShaderObject::~ShaderObject()
{
	if (device)
	{
		auto vkdevice = get_device().get_handle();

		// Clean up samplers
		vkDestroySampler(vkdevice, envmap_texture.sampler, nullptr);
		vkDestroySampler(vkdevice, checkerboard_texture.sampler, nullptr);
		vkDestroySampler(vkdevice, terrain_array_textures.sampler, nullptr);
		vkDestroySampler(vkdevice, heightmap_texture.sampler, nullptr);
		vkDestroySampler(vkdevice, standard_sampler, nullptr);

		// Clean up objects
		skybox.reset();
		torus.reset();
		rock.reset();
		cube.reset();
		sphere.reset();
		teapot.reset();

		camera_mats_ubo_buffer.reset();

		// Destroy Post Processing Image
		vkDestroyImageView(vkdevice, post_process_image.image_view, nullptr);
		vkFreeMemory(vkdevice, post_process_image.memory, nullptr);
		vkDestroyImage(vkdevice, post_process_image.image, nullptr);

		// Destroy output images
		for (auto image : output_images)
		{
			vkDestroyImageView(vkdevice, image.image_view, nullptr);
			vkFreeMemory(vkdevice, image.memory, nullptr);
			vkDestroyImage(vkdevice, image.image, nullptr);
		}

		// Destroy depth output images
		for (auto image : depth_images)
		{
			vkDestroyImageView(vkdevice, image.image_view, nullptr);
			vkFreeMemory(vkdevice, image.memory, nullptr);
			vkDestroyImage(vkdevice, image.image, nullptr);
		}

		// Destroy shaders
		for (auto &shader : shader_handles)
		{
			shader->destroy(vkdevice);
			delete shader;
		}

		// Destroy descriptor sets and layouts. Descriptor sets are automatically cleared when the pool is destroyed.
		for (int i = 0; i < ShaderTypeCOUNT; ++i)
		{
			vkDestroyDescriptorSetLayout(vkdevice, descriptor_set_layouts[i], nullptr);
			vkDestroyPipelineLayout(vkdevice, pipeline_layout[i], nullptr);
		}

		vkDestroyDescriptorPool(vkdevice, descriptor_pool, nullptr);
	}
}

// Currently the sample calls through this function in order to get the list of any instance layers, not just validation layers.
// This is not suitable for a real application implementation using the layer, the layer will need to be shipped with the application.
const std::vector<const char *> ShaderObject::get_validation_layers()
{
	return {"VK_LAYER_KHRONOS_shader_object"};
}

bool ShaderObject::resize(const uint32_t _width, const uint32_t _height)
{
	if (!device)
	{
		return false;
	}

	ApiVulkanSample::resize(width, height);

	auto vkdevice = get_device().get_handle();
	device->wait_idle();

	// Destroy Post Processing Image
	vkDestroyImageView(vkdevice, post_process_image.image_view, nullptr);
	vkFreeMemory(vkdevice, post_process_image.memory, nullptr);
	vkDestroyImage(vkdevice, post_process_image.image, nullptr);

	// Destroy output images
	for (auto image : output_images)
	{
		vkDestroyImageView(vkdevice, image.image_view, nullptr);
		vkFreeMemory(vkdevice, image.memory, nullptr);
		vkDestroyImage(vkdevice, image.image, nullptr);
	}

	// Destroy depth output images
	for (auto image : depth_images)
	{
		vkDestroyImageView(vkdevice, image.image_view, nullptr);
		vkFreeMemory(vkdevice, image.memory, nullptr);
		vkDestroyImage(vkdevice, image.image, nullptr);
	}

	output_images.clear();
	depth_images.clear();

	// Create new output images
	create_images();

	initialize_descriptor_sets();

	update_uniform_buffers();

	// Update swapchain to allow transfer dst to blit to it
	update_swapchain_image_usage_flags({VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT});

	return true;
}

bool ShaderObject::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	// Setup camera as look at origin
	camera.type = vkb::CameraType::LookAt;
	camera.set_position({0.f, 0.f, -4.5f});
	camera.set_rotation({19.f, 312.f, 0.f});
	camera.set_perspective(60.f, static_cast<float>(width) / static_cast<float>(height), 1024.f, 0.1f);

	// Setup resources for sample
	create_default_sampler();
	load_assets();
	prepare_uniform_buffers();
	update_uniform_buffers();
	create_descriptor_pool();
	setup_descriptor_set_layout();
	create_descriptor_sets();
	create_shaders();
	create_images();
	initialize_descriptor_sets();

	// Update swapchain to allow transfer dst to blit to it
	update_swapchain_image_usage_flags({VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT});
	generate_terrain();
	build_command_buffers();

	// Set start CPU time
	start_time = std::chrono::steady_clock::now();
	prepared   = true;
	return true;
}

void ShaderObject::setup_framebuffer()
{
	// Delete existing frame buffers
	for (uint32_t i = 0; i < framebuffers.size(); i++)
	{
		if (framebuffers[i] != VK_NULL_HANDLE)
		{
			vkDestroyFramebuffer(device->get_handle(), framebuffers[i], nullptr);
		}
	}

	// Create frame buffer for every swap chain image
	framebuffers.resize(render_context->get_render_frames().size());
	for (uint32_t i = 0; i < framebuffers.size(); i++)
	{
		VkFramebufferCreateInfo framebuffer_create_info = {};
		framebuffer_create_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.pNext                   = NULL;
		framebuffer_create_info.renderPass              = render_pass;
		framebuffer_create_info.attachmentCount         = 1;
		framebuffer_create_info.pAttachments            = &swapchain_buffers[i].view;
		framebuffer_create_info.width                   = get_render_context().get_surface_extent().width;
		framebuffer_create_info.height                  = get_render_context().get_surface_extent().height;
		framebuffer_create_info.layers                  = 1;

		VK_CHECK(vkCreateFramebuffer(device->get_handle(), &framebuffer_create_info, nullptr, &framebuffers[i]));
	}
}

// Create render pass for UI drawing
void ShaderObject::setup_render_pass()
{
	VkAttachmentDescription color_attachment{};

	// Color attachment set to load color and ignore stencil
	color_attachment.format         = render_context->get_format();
	color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
	color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout  = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_reference = {};
	color_reference.attachment            = 0;
	color_reference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Setup subpass description binding the depth and color attachments
	VkSubpassDescription subpass_description    = {};
	subpass_description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_description.colorAttachmentCount    = 1;
	subpass_description.pColorAttachments       = &color_reference;
	subpass_description.pDepthStencilAttachment = nullptr;
	subpass_description.inputAttachmentCount    = 0;
	subpass_description.pInputAttachments       = nullptr;
	subpass_description.preserveAttachmentCount = 0;
	subpass_description.pPreserveAttachments    = nullptr;
	subpass_description.pResolveAttachments     = nullptr;

	// Subpass dependencies for layout transitions
	VkSubpassDependency dependency{};

	// Setup color destination stages for output, early, and late frag test so scene drawing finishes before drawing up
	dependency.srcSubpass      = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass      = 0;
	dependency.srcStageMask    = VK_PIPELINE_STAGE_TRANSFER_BIT;
	dependency.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask   = VK_ACCESS_TRANSFER_WRITE_BIT;
	dependency.dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Setup create info for the render pass for the UI
	VkRenderPassCreateInfo render_pass_create_info = {};
	render_pass_create_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount        = 1;
	render_pass_create_info.pAttachments           = &color_attachment;
	render_pass_create_info.subpassCount           = 1;
	render_pass_create_info.pSubpasses             = &subpass_description;
	render_pass_create_info.dependencyCount        = 1;
	render_pass_create_info.pDependencies          = &dependency;

	// Create the render pass
	VK_CHECK(vkCreateRenderPass(device->get_handle(), &render_pass_create_info, nullptr, &render_pass));
}

void ShaderObject::create_default_sampler()
{
	// Create a sampler
	VkSamplerCreateInfo sampler_create_info = {};
	sampler_create_info.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_create_info.magFilter           = VK_FILTER_LINEAR;
	sampler_create_info.minFilter           = VK_FILTER_LINEAR;
	sampler_create_info.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_create_info.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_create_info.addressModeV        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_create_info.addressModeW        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_create_info.compareOp           = VK_COMPARE_OP_NEVER;
	sampler_create_info.mipLodBias          = 0.0f;
	sampler_create_info.minLod              = 0.0f;
	sampler_create_info.maxLod              = 1.0f;

	// Only enable anisotropic filtering if enabled on the device
	// Note that for simplicity always use max. available anisotropy level for the current device
	// This may have an impact on performance, esp. on lower-specced devices
	// In a real-world scenario the level of anisotropy should be a user setting or e.g. lowered for mobile devices by default
	sampler_create_info.maxAnisotropy    = get_device().get_gpu().get_features().samplerAnisotropy ? (get_device().get_gpu().get_properties().limits.maxSamplerAnisotropy) : 1.0f;
	sampler_create_info.anisotropyEnable = get_device().get_gpu().get_features().samplerAnisotropy;
	sampler_create_info.borderColor      = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHECK(vkCreateSampler(device->get_handle(), &sampler_create_info, nullptr, &standard_sampler));
}

void ShaderObject::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// Enable Shader Object
	auto &requestedShaderObject        = gpu.request_extension_features<VkPhysicalDeviceShaderObjectFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT);
	requestedShaderObject.shaderObject = VK_TRUE;

	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}

	// Enable wireframe mode if supported
	if (gpu.get_features().fillModeNonSolid)
	{
		gpu.get_mutable_requested_features().fillModeNonSolid = VK_TRUE;
		wireframe_enabled                                     = true;
	}

	// Enable Dynamic Rendering
	auto &requested_dynamic_rendering            = gpu.request_extension_features<VkPhysicalDeviceDynamicRenderingFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR);
	requested_dynamic_rendering.dynamicRendering = VK_TRUE;

	// Enable Geometry Shaders
	auto &requested_geometry_shader          = gpu.get_mutable_requested_features();
	requested_geometry_shader.geometryShader = VK_TRUE;

	// Generate a list of supported output formats
	for (auto format : possible_depth_formats)
	{
		VkPhysicalDeviceImageFormatInfo2 image_format;
		image_format.sType  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
		image_format.pNext  = nullptr;
		image_format.format = format.format;
		image_format.type   = VK_IMAGE_TYPE_2D;
		image_format.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_format.usage  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		image_format.flags  = 0;

		VkImageFormatProperties2 image_properties;
		image_properties.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;
		image_properties.pNext = nullptr;
		VkResult format_result = vkGetPhysicalDeviceImageFormatProperties2(gpu.get_handle(), &image_format,
		                                                                   &image_properties);

		// Add supported depth formats
		if (format_result == VK_SUCCESS)
		{
			supported_depth_formats.push_back(format);
		}
	}

	// Generate a list of supported output formats
	for (auto format : possible_output_formats)
	{
		VkPhysicalDeviceImageFormatInfo2 image_format;
		image_format.sType  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
		image_format.pNext  = nullptr;
		image_format.format = format.format;
		image_format.type   = VK_IMAGE_TYPE_2D;
		image_format.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_format.usage  = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		image_format.flags  = 0;

		VkImageFormatProperties2 image_properties;
		image_properties.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;
		image_properties.pNext = nullptr;
		VkResult format_result = vkGetPhysicalDeviceImageFormatProperties2(gpu.get_handle(), &image_format,
		                                                                   &image_properties);

		// Add supported output formats
		if (format_result == VK_SUCCESS)
		{
			supported_output_formats.push_back(format);
		}
	}
}

void ShaderObject::load_assets()
{
	// Load models
	torus  = load_model("scenes/torusknot.gltf");
	rock   = load_model("scenes/rock.gltf");
	cube   = load_model("scenes/cube.gltf");
	skybox = load_model("scenes/geosphere.gltf");
	teapot = load_model("scenes/teapot.gltf");

	// Load textures
	envmap_texture       = load_texture("textures/skysphere_rgba.ktx", vkb::sg::Image::Color);
	checkerboard_texture = load_texture("textures/checkerboard_rgba.ktx", vkb::sg::Image::Color);

	// Terrain textures are stored in a texture array with layers corresponding to terrain height
	terrain_array_textures = load_texture_array("textures/terrain_texturearray_rgba.ktx", vkb::sg::Image::Color);

	// Height data is stored in a one-channel texture
	heightmap_texture = load_texture("textures/terrain_heightmap_r16.ktx", vkb::sg::Image::Other);

	VkSamplerCreateInfo sampler_create_info = vkb::initializers::sampler_create_info();

	// Setup a mirroring sampler for the height map
	vkDestroySampler(get_device().get_handle(), heightmap_texture.sampler, nullptr);
	sampler_create_info.magFilter    = VK_FILTER_LINEAR;
	sampler_create_info.minFilter    = VK_FILTER_LINEAR;
	sampler_create_info.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	sampler_create_info.addressModeV = sampler_create_info.addressModeU;
	sampler_create_info.addressModeW = sampler_create_info.addressModeU;
	sampler_create_info.compareOp    = VK_COMPARE_OP_NEVER;
	sampler_create_info.minLod       = 0.0f;
	sampler_create_info.maxLod       = static_cast<float>(heightmap_texture.image->get_mipmaps().size());
	sampler_create_info.borderColor  = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler_create_info, nullptr, &heightmap_texture.sampler));

	// Setup a repeating sampler for the terrain texture layers
	vkDestroySampler(get_device().get_handle(), terrain_array_textures.sampler, nullptr);
	sampler_create_info              = vkb::initializers::sampler_create_info();
	sampler_create_info.magFilter    = VK_FILTER_LINEAR;
	sampler_create_info.minFilter    = VK_FILTER_LINEAR;
	sampler_create_info.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_create_info.addressModeV = sampler_create_info.addressModeU;
	sampler_create_info.addressModeW = sampler_create_info.addressModeU;
	sampler_create_info.compareOp    = VK_COMPARE_OP_NEVER;
	sampler_create_info.minLod       = 0.0f;
	sampler_create_info.maxLod       = static_cast<float>(terrain_array_textures.image->get_mipmaps().size());
	sampler_create_info.borderColor  = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler_create_info, nullptr, &terrain_array_textures.sampler));
}

void ShaderObject::prepare_uniform_buffers()
{
	camera_mats_ubo_buffer = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(camera_mats_ubo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
}

void ShaderObject::update_uniform_buffers()
{
	camera_mats_ubo.projection = camera.matrices.perspective;
	camera_mats_ubo.view       = camera.matrices.view;
	camera_mats_ubo.proj_view  = camera.matrices.perspective * camera.matrices.view;

	camera_mats_ubo_buffer->convert_and_update(camera_mats_ubo);
}

void ShaderObject::create_descriptor_pool()
{
	// Create a pool of size 16
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 32),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 32),
	};
	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), ShaderTypeCOUNT);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void ShaderObject::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings[ShaderTypeCOUNT] =
	    {
	        {// ShaderTypeBasic
	         vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
	         vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
	         vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 2),
	         vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3)},
	        {
	            // ShaderTypeMaterial
	            vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
	            vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
	            vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
	        },
	        {// ShaderTypePostProcess
	         vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)}};

	// Set push constant for basic shader types to be in the vertex shader
	push_constant_ranges[ShaderTypeBasic] = vkb::initializers::push_constant_range(
	    VK_SHADER_STAGE_VERTEX_BIT,
	    sizeof(BasicPushConstant),
	    0);

	// Set push constant for material shader types to be in the vertex, geometry, and fragment shader
	push_constant_ranges[ShaderTypeMaterial] = vkb::initializers::push_constant_range(
	    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	    sizeof(MaterialPushConstant),
	    0);

	// Set push constant for post processing shader types to be in the vertex and fragment shader
	push_constant_ranges[ShaderTypePostProcess] = vkb::initializers::push_constant_range(
	    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	    sizeof(PostProcessPushConstant),
	    0);

	// Create the pipeline layout for each of the shader types
	for (int i = 0; i < ShaderTypeCOUNT; ++i)
	{
		VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info = {vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings[i].data(), static_cast<uint32_t>(set_layout_bindings[i].size()))};
		VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layouts[i]));
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(
		    &descriptor_set_layouts[i],
		    1);

		// Create pipeline layouts for each shader type
		pipeline_layout_create_info.pushConstantRangeCount = 1;
		pipeline_layout_create_info.pPushConstantRanges    = &push_constant_ranges[i];
		VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout[i]));
	}
}

void ShaderObject::create_descriptor_sets()
{
	for (int i = 0; i < ShaderTypeCOUNT; ++i)
	{
		// Allocate descriptor set for each shader type
		VkDescriptorSetAllocateInfo alloc_info =
		    vkb::initializers::descriptor_set_allocate_info(
		        descriptor_pool,
		        &descriptor_set_layouts[i],
		        1);

		VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets[i]));
	}
}

// Create vert and frag and geo shaders that may or may not be linked with each other
void ShaderObject::create_shaders()
{
	using json = nlohmann::json;

	// Initialize a GLSL compiler and load shaders from the shader json file
	vkb::GLSLCompiler glsl_compiler;
	std::string       shaders     = vkb::fs::read_shader("shader_object/shaders.json");
	json              shader_data = json::parse(shaders);
	VkDevice          device      = get_device().get_handle();

	// Pre calc string lengths
	const int unlinked_post_process_prefix_size = strlen("post_process_");
	const int unlinked_material_prefix_size     = strlen("material_");
	const int vert_suffix_size                  = strlen(".vert");
	const int geo_suffix_size                   = strlen(".geom");
	const int frag_suffix_size                  = strlen(".frag");

	// Load skybox shader
	{
		LOGI("Compiling skybox Shader");
		auto &shader = shader_data["skybox"];

		std::string          vert_shader_name = shader["vert"].get<std::string>();
		std::vector<uint8_t> vert_shader_data = vkb::fs::read_shader_binary("shader_object/" + vert_shader_name);

		std::string          frag_shader_name = shader["frag"].get<std::string>();
		std::vector<uint8_t> frag_shader_data = vkb::fs::read_shader_binary("shader_object/" + frag_shader_name);

		// Create shaders with current and next stage bits and set the shaders GLSL shader data, descriptor sets, and push constants
		skybox_vert_shader = new Shader(VK_SHADER_STAGE_VERTEX_BIT,
		                                VK_SHADER_STAGE_FRAGMENT_BIT,
		                                "skybox vert",
		                                vert_shader_data,
		                                &descriptor_set_layouts[ShaderTypeBasic],
		                                &push_constant_ranges[ShaderTypeBasic]);
		skybox_frag_shader = new Shader(VK_SHADER_STAGE_FRAGMENT_BIT,
		                                0,
		                                "skybox frag",
		                                frag_shader_data,
		                                &descriptor_set_layouts[ShaderTypeBasic],
		                                &push_constant_ranges[ShaderTypeBasic]);

		// Set the fragment shader as linked to build them linked and build the shader
		build_linked_shaders(device, skybox_vert_shader, skybox_frag_shader);

		// Save handles for resource management
		shader_handles.push_back(skybox_vert_shader);
		shader_handles.push_back(skybox_frag_shader);
	}

	// Load post processing vert shader
	{
		LOGI("Compiling FSQ Shader");
		auto &shader = shader_data["post_process"];

		std::string          vert_shader_name = shader["vert"].get<std::string>();
		std::vector<uint8_t> vert_shader_data = vkb::fs::read_shader_binary("shader_object/" + vert_shader_name);

		// Create shader with current and next stage bits and set the GLSL shader data, descriptor sets, and push constants
		post_process_vert_shader = new Shader(VK_SHADER_STAGE_VERTEX_BIT,
		                                      VK_SHADER_STAGE_FRAGMENT_BIT,
		                                      "FSQ",
		                                      vert_shader_data,
		                                      &descriptor_set_layouts[ShaderTypePostProcess],
		                                      &push_constant_ranges[ShaderTypePostProcess]);

		// Build shader
		build_shader(device, post_process_vert_shader);

		// Save handle for resource management
		shader_handles.push_back(post_process_vert_shader);
	}

	// Load terrain shaders
	{
		LOGI("Compiling Terrain Shader");
		auto &shader = shader_data["terrain"];

		std::string          vert_shader_name = shader["vert"].get<std::string>();
		std::vector<uint8_t> vert_shader_data = vkb::fs::read_shader_binary("shader_object/" + vert_shader_name);

		std::string          frag_shader_name = shader["frag"].get<std::string>();
		std::vector<uint8_t> frag_shader_data = vkb::fs::read_shader_binary("shader_object/" + frag_shader_name);

		// Create shaders with current and next stage bits and set the shaders GLSL shader data, descriptor sets, and push constants
		terrain_vert_shader = new Shader(VK_SHADER_STAGE_VERTEX_BIT,
		                                 VK_SHADER_STAGE_FRAGMENT_BIT,
		                                 "Terrain vert",
		                                 vert_shader_data,
		                                 &descriptor_set_layouts[ShaderTypeBasic],
		                                 &push_constant_ranges[ShaderTypeBasic]);
		terrain_frag_shader = new Shader(VK_SHADER_STAGE_FRAGMENT_BIT,
		                                 0,
		                                 "Terrain frag",
		                                 frag_shader_data,
		                                 &descriptor_set_layouts[ShaderTypeBasic],
		                                 &push_constant_ranges[ShaderTypeBasic]);

		// Set the fragment shader as linked to build them linked and build the shader
		build_linked_shaders(device, terrain_vert_shader, terrain_frag_shader);

		// Save handles for resource management
		shader_handles.push_back(terrain_vert_shader);
		shader_handles.push_back(terrain_frag_shader);
	}

	// Load linked basic shaders
	for (auto &shader : shader_data["basic"].items())
	{
		std::string shader_name = shader.key();

		std::string          vert_shader_name = shader.value()["vert"].get<std::string>();
		std::vector<uint8_t> vert_shader_data = vkb::fs::read_shader_binary("shader_object/" + vert_shader_name);

		std::string          frag_shader_name = shader.value()["frag"].get<std::string>();
		std::vector<uint8_t> frag_shader_data = vkb::fs::read_shader_binary("shader_object/" + frag_shader_name);

		LOGI("Compiling Shader Set {}", shader_name.c_str());

		// Create shader with current and next stage bits and set the GLSL shader data, descriptor sets, and push constants
		basic_vert_shaders.emplace_back(new Shader(VK_SHADER_STAGE_VERTEX_BIT,
		                                           VK_SHADER_STAGE_FRAGMENT_BIT,
		                                           shader_name,
		                                           vert_shader_data,
		                                           &descriptor_set_layouts[ShaderTypeBasic],
		                                           &push_constant_ranges[ShaderTypeBasic]));

		// Create shaders with current and next stage bits and set the GLSL shader data, descriptor sets, and push constants
		basic_frag_shaders.emplace_back(new Shader(VK_SHADER_STAGE_FRAGMENT_BIT,
		                                           0,
		                                           shader_name,
		                                           frag_shader_data,
		                                           &descriptor_set_layouts[ShaderTypeBasic],
		                                           &push_constant_ranges[ShaderTypeBasic]));

		// Set the fragment shader as linked to build them linked and build the shader
		build_linked_shaders(device, basic_vert_shaders.back(), basic_frag_shaders.back());

		// Save handles for resource management
		shader_handles.push_back(basic_vert_shaders.back());
		shader_handles.push_back(basic_frag_shaders.back());
	}

	// Load unlinked post_process frag shaders
	for (auto &shader : shader_data["post_process"]["frag"].items())
	{
		std::string          shader_name = shader.value().get<std::string>();
		std::vector<uint8_t> shader_data = vkb::fs::read_shader_binary("shader_object/" + shader_name);

		LOGI("Compiling Shader {}", shader_name.c_str());

		// Create shader with current and next stage bits and set the GLSL shader data, descriptor sets, and push constants
		post_process_frag_shaders.emplace_back(
		    new Shader(VK_SHADER_STAGE_FRAGMENT_BIT,
		               0,
		               shader_name.substr(unlinked_post_process_prefix_size, shader_name.length() - (unlinked_post_process_prefix_size + frag_suffix_size)),
		               shader_data,
		               &descriptor_set_layouts[ShaderTypePostProcess],
		               &push_constant_ranges[ShaderTypePostProcess]));

		// Build shader
		build_shader(device, post_process_frag_shaders.back());

		// Save handle for resource management
		shader_handles.push_back(post_process_frag_shaders.back());
	}

	// Load unlinked material vert shaders
	for (auto &shader : shader_data["material"]["vert"].items())
	{
		std::string          shader_name = shader.value().get<std::string>();
		std::vector<uint8_t> shader_data = vkb::fs::read_shader_binary("shader_object/" + shader_name);

		LOGI("Compiling Shader {}", shader_name.c_str());

		// Create shader with current and next stage bits set the GLSL shader data, descriptor sets, and push constants
		material_vert_shaders.emplace_back(
		    new Shader(VK_SHADER_STAGE_VERTEX_BIT,
		               VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		               shader_name.substr(unlinked_material_prefix_size, shader_name.length() - (unlinked_material_prefix_size + frag_suffix_size)),
		               shader_data,
		               &descriptor_set_layouts[ShaderTypeMaterial],
		               &push_constant_ranges[ShaderTypeMaterial]));

		// Build shader
		build_shader(device, material_vert_shaders.back());

		// Save handle for resource management
		shader_handles.push_back(material_vert_shaders.back());
	}

	// Load unlinked material geo shaders
	for (auto &shader : shader_data["material"]["geo"].items())
	{
		std::string          shader_name = shader.value().get<std::string>();
		std::vector<uint8_t> shader_data = vkb::fs::read_shader_binary("shader_object/" + shader_name);

		LOGI("Compiling Shader {}", shader_name.c_str());

		// Create shader with current and next stage bits and set the GLSL shader data, descriptor sets, and push constants
		material_geo_shaders.emplace_back(
		    new Shader(VK_SHADER_STAGE_GEOMETRY_BIT,
		               VK_SHADER_STAGE_FRAGMENT_BIT,
		               shader_name.substr(unlinked_material_prefix_size, shader_name.length() - (unlinked_material_prefix_size + geo_suffix_size)),
		               shader_data,
		               &descriptor_set_layouts[ShaderTypeMaterial],
		               &push_constant_ranges[ShaderTypeMaterial]));

		// Build shader
		build_shader(device, material_geo_shaders.back());

		// Save handle for resource management
		shader_handles.push_back(material_geo_shaders.back());
	}

	// Load unlinked material frag shaders
	for (auto &shader : shader_data["material"]["frag"].items())
	{
		std::string          shader_name = shader.value().get<std::string>();
		std::vector<uint8_t> shader_data = vkb::fs::read_shader_binary("shader_object/" + shader_name);

		LOGI("Compiling Shader {}", shader_name.c_str());

		// Create shader with current and next stage bits and set the GLSL shader data, descriptor sets, and push constants
		material_frag_shaders.emplace_back(
		    new Shader(VK_SHADER_STAGE_FRAGMENT_BIT,
		               0,
		               shader_name.substr(unlinked_material_prefix_size, shader_name.length() - (unlinked_material_prefix_size + frag_suffix_size)),
		               shader_data,
		               &descriptor_set_layouts[ShaderTypeMaterial],
		               &push_constant_ranges[ShaderTypeMaterial]));

		// Build shader
		build_shader(device, material_frag_shaders.back());

		// Save handle for resource management
		shader_handles.push_back(material_frag_shaders.back());
	}
}

void ShaderObject::create_images()
{
	// Set vector to size of output and depth images
	output_images.reserve(supported_output_formats.size());
	depth_images.reserve(supported_depth_formats.size());

	// Create image and set sampler for the post process image
	post_process_image                 = create_output_image(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	post_process_input_sampler.sampler = standard_sampler;

	// Create an output image for all supported formats
	for (auto format : supported_output_formats)
	{
		LOGI("Creating output image format of type {}", format.name);
		output_images.emplace_back(create_output_image(format.format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT));
	}

	// Create a depth output image for all supported formats
	for (auto format : supported_depth_formats)
	{
		LOGI("Creating output image format of type {}", format.name);
		depth_images.emplace_back(create_output_image(format.format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_DEPTH_BIT));
	}
}

void ShaderObject::initialize_descriptor_sets()
{
	// Set Initial descriptor sets
	post_process_input_sampler.image = output_images[current_output_format];

	VkDescriptorBufferInfo matrix_buffer_descriptor       = create_descriptor(*camera_mats_ubo_buffer);
	VkDescriptorImageInfo  post_process_image_descriptor  = create_image_descriptor(post_process_input_sampler);
	VkDescriptorImageInfo  environment_image_descriptor   = create_descriptor(envmap_texture);
	VkDescriptorImageInfo  checkerboard_image_descriptor  = create_descriptor(checkerboard_texture);
	VkDescriptorImageInfo  heightmap_image_descriptor     = create_descriptor(heightmap_texture);
	VkDescriptorImageInfo  texture_array_image_descriptor = create_descriptor(terrain_array_textures);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    // Buffer initial descriptor set data for ShaderTypeBasic
	    vkb::initializers::write_descriptor_set(descriptor_sets[ShaderTypeBasic], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matrix_buffer_descriptor),
	    vkb::initializers::write_descriptor_set(descriptor_sets[ShaderTypeBasic], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &environment_image_descriptor),
	    vkb::initializers::write_descriptor_set(descriptor_sets[ShaderTypeBasic], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &heightmap_image_descriptor),
	    vkb::initializers::write_descriptor_set(descriptor_sets[ShaderTypeBasic], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &texture_array_image_descriptor),

	    // Buffer initial descriptor set data for ShaderTypeMaterial
	    vkb::initializers::write_descriptor_set(descriptor_sets[ShaderTypeMaterial], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matrix_buffer_descriptor),
	    vkb::initializers::write_descriptor_set(descriptor_sets[ShaderTypeMaterial], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &checkerboard_image_descriptor),
	    vkb::initializers::write_descriptor_set(descriptor_sets[ShaderTypeMaterial], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &environment_image_descriptor),

	    // Buffer initial descriptor set data for ShaderTypePostProcess
	    vkb::initializers::write_descriptor_set(descriptor_sets[ShaderTypePostProcess], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &post_process_image_descriptor),
	};

	// Update descriptor sets
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

// Generate a terrain grid of triangles
void ShaderObject::generate_terrain()
{
	const uint32_t      terrain_resolution = 256;
	const uint32_t      terrain_size       = 1024;
	const float         uv_scale           = 1;
	const uint32_t      vertex_count       = terrain_resolution * terrain_resolution;
	std::vector<Vertex> vertices(vertex_count);

	// Calculate normals from height map using a sobel filter
	vkb::HeightMap heightmap("textures/terrain_heightmap_r16.ktx", terrain_resolution);

	// Indices
	const uint32_t        index_count = vertex_count * 6;
	std::vector<uint32_t> indices(index_count);

	// For each vertex generate pos, uv's, normals, and face indices
	for (auto x = 0; x < terrain_resolution; x++)
	{
		for (auto y = 0; y < terrain_resolution; y++)
		{
			uint32_t index          = (x + y * terrain_resolution);
			vertices[index].pos[0]  = x / static_cast<float>(terrain_resolution) * terrain_size - terrain_size / 2.0f;
			vertices[index].pos[1]  = 0;
			vertices[index].pos[2]  = y / static_cast<float>(terrain_resolution) * terrain_size - terrain_size / 2.0f;
			vertices[index].uv      = glm::vec2(static_cast<float>(x) / terrain_resolution, static_cast<float>(y) / terrain_resolution) * uv_scale;
			vertices[index].joint0  = glm::vec4(0);
			vertices[index].weight0 = glm::vec4(0);

			// Get height samples centered around current position
			float heights[3][3];
			for (auto hx = -1; hx <= 1; hx++)
			{
				for (auto hy = -1; hy <= 1; hy++)
				{
					heights[hx + 1][hy + 1] = heightmap.get_height(x + hx, y + hy);
				}
			}

			// Calculate the normal
			glm::vec3 normal;
			// Gx sobel filter
			normal.x = heights[0][0] - heights[2][0] + 2.0f * heights[0][1] - 2.0f * heights[2][1] + heights[0][2] - heights[2][2];
			// Gy sobel filter
			normal.z = heights[0][0] + 2.0f * heights[1][0] + heights[2][0] - heights[0][2] - 2.0f * heights[1][2] - heights[2][2];
			// Calculate missing up component of the normal using the filtered x and y axis
			// The first value controls the bump strength
			normal.y = 0.25f * sqrt(1.0f - normal.x * normal.x - normal.z * normal.z);

			vertices[index].normal = glm::normalize(normal * glm::vec3(2.0f, 1.0f, 2.0f));

			// Generate two triangles that form a quad using counter clockwise winding
			if (x < terrain_resolution - 1 && y < terrain_resolution - 1)
			{
				uint32_t indices_index = (x + y * terrain_resolution) * 6;
				// A,D,B
				indices[indices_index]     = (x + y * terrain_resolution);
				indices[indices_index + 1] = (x + (y + 1) * terrain_resolution);
				indices[indices_index + 2] = (x + 1 + y * terrain_resolution);
				// B,D,C
				indices[indices_index + 3] = (x + 1 + y * terrain_resolution);
				indices[indices_index + 4] = (x + (y + 1) * terrain_resolution);
				indices[indices_index + 5] = (x + 1 + (y + 1) * terrain_resolution);
			}
		}
	}

	terrain.index_count = index_count;

	uint32_t vertex_buffer_size = vertex_count * sizeof(Vertex);
	uint32_t index_buffer_size  = index_count * sizeof(uint32_t);

	// Create staging buffers
	vkb::core::Buffer vertex_staging(get_device(), vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	vertex_staging.update(vertices);

	vkb::core::Buffer index_staging(get_device(), index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	index_staging.update(indices);

	terrain.vertices = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                       vertex_buffer_size,
	                                                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                                                       VMA_MEMORY_USAGE_GPU_ONLY);

	terrain.indices = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                      index_buffer_size,
	                                                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                                                      VMA_MEMORY_USAGE_GPU_ONLY);

	// Copy from staging buffers
	VkCommandBuffer copy_command = device->create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkBufferCopy copy_region = {};

	copy_region.size = vertex_buffer_size;
	vkCmdCopyBuffer(
	    copy_command,
	    vertex_staging.get_handle(),
	    terrain.vertices->get_handle(),
	    1,
	    &copy_region);

	copy_region.size = index_buffer_size;
	vkCmdCopyBuffer(
	    copy_command,
	    index_staging.get_handle(),
	    terrain.indices->get_handle(),
	    1,
	    &copy_region);

	device->flush_command_buffer(copy_command, queue, true);
}

void ShaderObject::build_command_buffers()
{
	int i = 0;
	for (auto &draw_cmd_buffer : draw_cmd_buffers)
	{
		auto command_begin = vkb::initializers::command_buffer_begin_info();
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffer, &command_begin));

		// First set initial required state
		set_initial_state(draw_cmd_buffer);

		// Image subresources for the barriers
		VkImageSubresourceRange range{};
		range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel   = 0;
		range.levelCount     = 1;
		range.baseArrayLayer = 0;
		range.layerCount     = 1;

		VkImageSubresourceRange depth_range{};
		depth_range.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
		depth_range.baseMipLevel   = 0;
		depth_range.levelCount     = 1;
		depth_range.baseArrayLayer = 0;
		depth_range.layerCount     = 1;

		// Barriers for images that are rendered to
		vkb::image_layout_transition(draw_cmd_buffer,
		                             output_images[current_output_format].image,
		                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		                             0,
		                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		                             VK_IMAGE_LAYOUT_UNDEFINED,
		                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, range);

		vkb::image_layout_transition(draw_cmd_buffer,
		                             depth_images[current_depth_format].image,
		                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		                             VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		                             0,
		                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		                             VK_IMAGE_LAYOUT_UNDEFINED,
		                             VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, depth_range);

		// Setup dynamic rendering attachment info and begin rendering
		{
			// Because every pixel is drawn to via the skybox and objects there is no need to clear the color buffer.
			VkClearValue depth_clear_value{};
			depth_clear_value.depthStencil = {0.f, 0};

			// Standard color attachment information except load op is don't care because every pixel is written to
			VkRenderingAttachmentInfo color_attachment_info = vkb::initializers::rendering_attachment_info();
			color_attachment_info.imageView                 = output_images[current_output_format].image_view;
			color_attachment_info.imageLayout               = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			color_attachment_info.resolveMode               = VK_RESOLVE_MODE_NONE;
			color_attachment_info.loadOp                    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			color_attachment_info.storeOp                   = VK_ATTACHMENT_STORE_OP_STORE;
			// Do not need a clear value for color because every pixel is drawn to

			// Set depth attach info's clear value to 0,0, load op clear to clear the depth buffer
			VkRenderingAttachmentInfo depth_attachment_info = vkb::initializers::rendering_attachment_info();
			depth_attachment_info.imageView                 = depth_images[current_depth_format].image_view;
			depth_attachment_info.imageLayout               = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depth_attachment_info.resolveMode               = VK_RESOLVE_MODE_NONE;
			depth_attachment_info.loadOp                    = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depth_attachment_info.storeOp                   = VK_ATTACHMENT_STORE_OP_STORE;
			depth_attachment_info.clearValue                = depth_clear_value;

			// If wireframe mode is enabled the back buffer does need to be cleared
			if (wireframe_enabled && wireframe_mode)
			{
				VkClearValue color_clear_value{};
				color_clear_value.color = {0.f, 0.f, 0.f, 0.f};

				// Set load op to clear and set clear color
				color_attachment_info.loadOp     = VK_ATTACHMENT_LOAD_OP_CLEAR;
				color_attachment_info.clearValue = color_clear_value;
			}

			// Setup render area and render info for screen size with 1 color attachment and 1 depth attachment
			auto render_area             = VkRect2D{VkOffset2D{}, VkExtent2D{width, height}};
			auto render_info             = vkb::initializers::rendering_info(render_area, 1, &color_attachment_info);
			render_info.layerCount       = 1;
			render_info.pDepthAttachment = &depth_attachment_info;

			// This is how to enable stencil if a stencil buffer is used
			if (!vkb::is_depth_only_format(depth_format))
			{
				render_info.pStencilAttachment = &depth_attachment_info;
			}

			// Begin rendering with the rendering info created earlier
			vkCmdBeginRenderingKHR(draw_cmd_buffer, &render_info);
		}

		{
			// Disable depth write and use cull mode none to draw skybox
			vkCmdSetCullModeEXT(draw_cmd_buffer, VK_CULL_MODE_NONE);
			vkCmdSetDepthWriteEnableEXT(draw_cmd_buffer, VK_FALSE);

			// Bind descriptors and push constants for the skybox draw
			glm::mat4 model_matrix = glm::mat4(1.0f);
			vkCmdBindDescriptorSets(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout[ShaderTypeBasic], 0, 1, &descriptor_sets[ShaderTypeBasic], 0, nullptr);
			vkCmdPushConstants(draw_cmd_buffer, pipeline_layout[ShaderTypeBasic], VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BasicPushConstant), &model_matrix);

			// Bind shaders for the skybox
			bind_shader(draw_cmd_buffer, skybox_vert_shader);
			bind_shader(draw_cmd_buffer, skybox_frag_shader);

			// vkCmdBindShadersEXT() must be called at least once with each of their stages in pStages before drawing.
			// Vertex and fragment shaders are bound for this draw already. Specify no geometry shader for the geometry stage.
			VkShaderStageFlagBits geo_stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			vkCmdBindShadersEXT(draw_cmd_buffer, 1, &geo_stage, nullptr);

			// Draw the skybox model
			draw_model(skybox, draw_cmd_buffer);
		}

		// Material Shaders via big scene, uses cull mode back.
		{
			// Re-enable depth write and cull mode and bind patch list for terrain
			vkCmdSetCullModeEXT(draw_cmd_buffer, VK_CULL_MODE_BACK_BIT);
			vkCmdSetDepthWriteEnableEXT(draw_cmd_buffer, VK_TRUE);

			{
				// Bind vertex buffers for terrain
				VkDeviceSize offsets[1] = {0};
				vkCmdBindVertexBuffers(draw_cmd_buffer, 0, 1, terrain.vertices->get(), offsets);
			}

			// Use same descriptors as skybox and bind new push constants for the terrain draw and bind the index buffer
			glm::mat4 model_matrix = glm::translate(glm::vec3(0, -100, 0));
			vkCmdPushConstants(draw_cmd_buffer, pipeline_layout[ShaderTypeBasic], VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BasicPushConstant), &model_matrix);
			vkCmdBindIndexBuffer(draw_cmd_buffer, terrain.indices->get_handle(), 0, VK_INDEX_TYPE_UINT32);

			// Bind the terrain shader
			bind_shader(draw_cmd_buffer, terrain_vert_shader);
			bind_shader(draw_cmd_buffer, terrain_frag_shader);

			// Draw the terrain
			vkCmdDrawIndexed(draw_cmd_buffer, terrain.index_count, 1, 0, 0, 0);

			// Set cull mode for models
			vkCmdSetCullModeEXT(draw_cmd_buffer, VK_CULL_MODE_FRONT_BIT);

			// Bind descriptors for models
			vkCmdBindDescriptorSets(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout[ShaderTypeMaterial], 0, 1, &descriptor_sets[ShaderTypeMaterial], 0, nullptr);

			// Setup and initialize push constants for material shader types
			MaterialPushConstant material_push_constant;
			material_push_constant.elapsed_time = elapsed_time;
			material_push_constant.camera_pos   = camera.position;

			// Update and push constants for torus
			material_push_constant.model = glm::translate(glm::vec3(1.2f, 0, 0)) * glm::rotate((float) elapsed_time, glm::vec3(1, 0, 0)) * glm::scale(glm::vec3(0.015f));
			vkCmdPushConstants(draw_cmd_buffer, pipeline_layout[ShaderTypeMaterial],
			                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			                   0, sizeof(MaterialPushConstant), &material_push_constant);

			// Bind shaders for the torus
			bind_material_shader(draw_cmd_buffer, 0);

			// Draw torus
			draw_model(torus, draw_cmd_buffer);

			// Update and push constants for rock 1
			material_push_constant.model = glm::translate(glm::vec3(1.2f, 1.f, 0)) * glm::rotate((float) elapsed_time, glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(4.0f));
			vkCmdPushConstants(draw_cmd_buffer, pipeline_layout[ShaderTypeMaterial],
			                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			                   0, sizeof(MaterialPushConstant), &material_push_constant);

			// Bind shaders for rock 1
			bind_material_shader(draw_cmd_buffer, 1);

			// Draw rock 1
			draw_model(rock, draw_cmd_buffer);

			// Update and push constants for cube 1
			material_push_constant.model = glm::translate(glm::vec3(1.2f, -1.f, 0)) * glm::rotate((float) elapsed_time, glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.05f));
			vkCmdPushConstants(draw_cmd_buffer, pipeline_layout[ShaderTypeMaterial],
			                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			                   0, sizeof(MaterialPushConstant), &material_push_constant);

			// Bind shaders for cube 1
			bind_material_shader(draw_cmd_buffer, 2);

			// Draw cube 1
			draw_model(cube, draw_cmd_buffer);

			// Update and push constants for torus 2
			material_push_constant.model = glm::translate(glm::vec3(-1.2f, 1.0f, 0)) * glm::rotate((float) elapsed_time, glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.015f));
			vkCmdPushConstants(draw_cmd_buffer, pipeline_layout[ShaderTypeMaterial],
			                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			                   0, sizeof(MaterialPushConstant), &material_push_constant);

			// Bind shaders for torus 2
			bind_material_shader(draw_cmd_buffer, 3);

			// Draw torus 2
			draw_model(torus, draw_cmd_buffer);

			// Update and push constants for rock 2
			material_push_constant.model = glm::translate(glm::vec3(-1.2f, -1.f, 0)) * glm::rotate((float) elapsed_time, glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(4.0f));
			vkCmdPushConstants(draw_cmd_buffer, pipeline_layout[ShaderTypeMaterial],
			                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			                   0, sizeof(MaterialPushConstant), &material_push_constant);

			// Bind shaders for rock 2
			bind_material_shader(draw_cmd_buffer, 4);

			// Draw rock 2
			draw_model(rock, draw_cmd_buffer);

			// Update and push constants for cube 2
			material_push_constant.model = glm::translate(glm::vec3(-1.2f, 0, 0)) * glm::rotate((float) elapsed_time, glm::vec3(1, 0, 0)) * glm::scale(glm::vec3(0.05f));
			vkCmdPushConstants(draw_cmd_buffer, pipeline_layout[ShaderTypeMaterial],
			                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			                   0, sizeof(MaterialPushConstant), &material_push_constant);

			// Bind shaders for cube 2
			bind_material_shader(draw_cmd_buffer, 5);

			// Draw cube 2
			draw_model(cube, draw_cmd_buffer);

			// Unbind geometry shader by binding nullptr to the geometry stage
			VkShaderStageFlagBits geo_stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			vkCmdBindShadersEXT(draw_cmd_buffer, 1, &geo_stage, nullptr);
		}

		// Basic Shaders
		{
			// Bind basic shader descriptor set
			vkCmdBindDescriptorSets(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout[ShaderTypeBasic], 0, 1, &descriptor_sets[ShaderTypeBasic], 0, nullptr);

			// Update and push constants for rock
			glm::mat4 model_matrix = glm::translate(glm::vec3(0, 0, -1.2f)) * glm::rotate((float) elapsed_time, glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(4.0f));
			vkCmdPushConstants(draw_cmd_buffer, pipeline_layout[ShaderTypeBasic], VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BasicPushConstant), &model_matrix);

			// Bind shaders for rock
			bind_basic_shader(draw_cmd_buffer, 0);

			// Draw rock
			draw_model(rock, draw_cmd_buffer);

			// Update and push constants for teapot 1
			model_matrix = glm::translate(glm::vec3(0, 0, 0)) * glm::rotate((float) elapsed_time, glm::vec3(0, 1, 0)) * glm::rotate(glm::radians(180.0f), glm::vec3(1, 0, 0)) * glm::scale(glm::vec3(0.2f));
			vkCmdPushConstants(draw_cmd_buffer, pipeline_layout[ShaderTypeBasic], VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BasicPushConstant), &model_matrix);

			// Bind shaders for teapot 1
			bind_basic_shader(draw_cmd_buffer, 1);

			// Draw teapot 1
			draw_model(teapot, draw_cmd_buffer);

			// Update and push constants for teapot 2
			model_matrix = glm::translate(glm::vec3(0, -1.2f, 0)) * glm::rotate((float) elapsed_time, glm::vec3(1, 0, 0)) * glm::rotate(glm::radians(180.0f), glm::vec3(1, 0, 0)) * glm::scale(glm::vec3(0.2f));
			vkCmdPushConstants(draw_cmd_buffer, pipeline_layout[ShaderTypeBasic], VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BasicPushConstant), &model_matrix);

			// Bind shaders for teapot 2
			bind_basic_shader(draw_cmd_buffer, 2);

			// Draw teapot 2
			draw_model(teapot, draw_cmd_buffer);

			// Update and push constants for teapot 3
			model_matrix = glm::translate(glm::vec3(0, 1.2f, 0)) * glm::rotate((float) elapsed_time, glm::vec3(0, 0, 1)) * glm::rotate(glm::radians(180.0f), glm::vec3(1, 0, 0)) * glm::scale(glm::vec3(0.2f));
			vkCmdPushConstants(draw_cmd_buffer, pipeline_layout[ShaderTypeBasic], VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BasicPushConstant), &model_matrix);

			// Bind shaders for teapot 3
			bind_basic_shader(draw_cmd_buffer, 3);

			// Draw teapot 3
			draw_model(teapot, draw_cmd_buffer);

			// Update and push constants for cube
			model_matrix = glm::translate(glm::vec3(0, 0, 1.2f)) * glm::rotate((float) elapsed_time, glm::vec3(1, 1, 0)) * glm::scale(glm::vec3(0.05f));
			vkCmdPushConstants(draw_cmd_buffer, pipeline_layout[ShaderTypeBasic], VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BasicPushConstant), &model_matrix);

			// Bind shaders for cube
			bind_basic_shader(draw_cmd_buffer, 4);

			// Draw cube
			draw_model(cube, draw_cmd_buffer);
		}

		// End rendering of scene
		vkCmdEndRenderingKHR(draw_cmd_buffer);

		// Setup information for screen size blit, will be used either to blit to the post processing if enabled
		// or directly to the swapchain if post processing is not enabled
		VkImageBlit blit;
		blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount     = 1;
		blit.srcSubresource.mipLevel       = 0;
		blit.srcOffsets[0]                 = {0, 0, 0};
		blit.srcOffsets[1]                 = {(int) width, (int) height, 1};

		// Copy color from source to destination of screen size
		blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount     = 1;
		blit.dstSubresource.mipLevel       = 0;
		blit.dstOffsets[0]                 = {0, 0, 0};
		blit.dstOffsets[1]                 = {(int) width, (int) height, 1};

		// Add barrier for swapchain buffer image
		vkb::image_layout_transition(draw_cmd_buffer,
		                             swapchain_buffers[i].image,
		                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		                             VK_PIPELINE_STAGE_TRANSFER_BIT,
		                             0,
		                             VK_ACCESS_TRANSFER_WRITE_BIT,
		                             VK_IMAGE_LAYOUT_UNDEFINED,
		                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		                             range);

		if (post_processing == true)
		{
			// Reset polygon mode for post-processing draws
			vkCmdSetPolygonModeEXT(draw_cmd_buffer, VK_POLYGON_MODE_FILL);

			// Add barrier for the output image of the current output to be read from
			vkb::image_layout_transition(draw_cmd_buffer,
			                             output_images[current_output_format].image,
			                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			                             VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
			                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			                             VK_ACCESS_SHADER_READ_BIT,
			                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			                             range);

			// Add barrier for the post process image to be drawn to
			vkb::image_layout_transition(draw_cmd_buffer,
			                             post_process_image.image,
			                             VK_PIPELINE_STAGE_TRANSFER_BIT,
			                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			                             0,
			                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			                             VK_IMAGE_LAYOUT_UNDEFINED,
			                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, range);

			{
				// Setup rendering information for post processing pass
				VkRenderingAttachmentInfo post_process_color_attachment_info = vkb::initializers::rendering_attachment_info();
				post_process_color_attachment_info.imageView                 = post_process_image.image_view;
				post_process_color_attachment_info.imageLayout               = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				post_process_color_attachment_info.resolveMode               = VK_RESOLVE_MODE_NONE;
				post_process_color_attachment_info.loadOp                    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				post_process_color_attachment_info.storeOp                   = VK_ATTACHMENT_STORE_OP_STORE;
				// Do not need a clear value for color because every pixel is drawn to

				// Setup render information for screen size
				auto render_area                          = VkRect2D{VkOffset2D{}, VkExtent2D{width, height}};
				auto render_info_post_process             = vkb::initializers::rendering_info(render_area, 1, &post_process_color_attachment_info);
				render_info_post_process.layerCount       = 1;
				render_info_post_process.pDepthAttachment = nullptr;

				// Begin rendering to post processing image
				vkCmdBeginRenderingKHR(draw_cmd_buffer, &render_info_post_process);
			}

			// Setup post-process cull mode none and disable depth write state
			vkCmdSetCullModeEXT(draw_cmd_buffer, VK_CULL_MODE_NONE);
			vkCmdSetDepthWriteEnableEXT(draw_cmd_buffer, VK_FALSE);

			// Bind post-process descriptor and push constants
			vkCmdBindDescriptorSets(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout[ShaderTypePostProcess], 0, 1, &descriptor_sets[ShaderTypePostProcess], 0, nullptr);
			vkCmdPushConstants(draw_cmd_buffer, pipeline_layout[ShaderTypePostProcess], VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PostProcessPushConstant), &elapsed_time);

			// Bind shaders for post processing
			bind_shader(draw_cmd_buffer, post_process_vert_shader);
			bind_shader(draw_cmd_buffer, post_process_frag_shaders[current_post_process_shader]);

			// Render post-process
			vkCmdDraw(draw_cmd_buffer, 3, 1, 0, 0);
			vkCmdEndRenderingKHR(draw_cmd_buffer);

			// Add barrier on the post processing image so drawing finishes
			vkb::image_layout_transition(draw_cmd_buffer,
			                             post_process_image.image,
			                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			                             VK_PIPELINE_STAGE_TRANSFER_BIT,
			                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			                             VK_ACCESS_TRANSFER_READ_BIT,
			                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			                             range);

			// Copy the post processing image to the swapchain buffer
			vkCmdBlitImage(draw_cmd_buffer,
			               post_process_image.image,
			               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			               swapchain_buffers[i].image,
			               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			               1,
			               &blit,
			               VK_FILTER_LINEAR);
		}
		else
		{
			// Add barrier on the output image so drawing finishes
			vkb::image_layout_transition(draw_cmd_buffer,
			                             output_images[current_output_format].image,
			                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			                             VK_PIPELINE_STAGE_TRANSFER_BIT,
			                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			                             VK_ACCESS_TRANSFER_READ_BIT,
			                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			                             range);

			// Copy the output processing image to the swapchain buffer
			vkCmdBlitImage(draw_cmd_buffer,
			               output_images[current_output_format].image,
			               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			               swapchain_buffers[i].image,
			               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			               1,
			               &blit,
			               VK_FILTER_LINEAR);
		}

		// Showing interop between pipelined render passes and shader object with the UI system
		{
			// Setup render pass info using the UI's render pass and width and height.
			VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
			render_pass_begin_info.renderPass               = render_pass;
			render_pass_begin_info.renderArea.offset.x      = 0;
			render_pass_begin_info.renderArea.offset.y      = 0;
			render_pass_begin_info.renderArea.extent.width  = width;
			render_pass_begin_info.renderArea.extent.height = height;

			// Load op is load for color buffer and don't care for depth with clear values needed
			render_pass_begin_info.clearValueCount = 0;
			render_pass_begin_info.pClearValues    = nullptr;
			render_pass_begin_info.framebuffer     = framebuffers[i];

			// draw_ui is setup to draw to the swapchain_buffers[i].image
			vkCmdBeginRenderPass(draw_cmd_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		}

		// Draw UI using render passes and FBOs and end the render pass
		draw_ui(draw_cmd_buffer);
		vkCmdEndRenderPass(draw_cmd_buffer);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffer));
		++i;
	}
}

void ShaderObject::update_descriptor_sets()
{
	// Update post process image resource
	post_process_input_sampler.image = output_images[current_output_format];

	// Create matrix and post process descriptor update info
	VkDescriptorBufferInfo matrix_buffer_descriptor      = create_descriptor(*camera_mats_ubo_buffer);
	VkDescriptorImageInfo  post_process_image_descriptor = create_image_descriptor(post_process_input_sampler);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    // Buffer changing descriptor set data for ShaderTypeBasic
	    vkb::initializers::write_descriptor_set(descriptor_sets[ShaderTypeBasic], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matrix_buffer_descriptor),

	    // Buffer changing descriptor set data for ShaderTypeMaterial
	    vkb::initializers::write_descriptor_set(descriptor_sets[ShaderTypeMaterial], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matrix_buffer_descriptor),

	    // Buffer changing descriptor set data for ShaderTypePostProcess
	    vkb::initializers::write_descriptor_set(descriptor_sets[ShaderTypePostProcess], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &post_process_image_descriptor),
	};

	// Update descriptor sets
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

void ShaderObject::set_initial_state(VkCommandBuffer cmd)
{
	{
		// Set viewport and scissor to screen size
		const VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		const VkRect2D   scissor  = vkb::initializers::rect2D(width, height, 0, 0);

		vkCmdSetViewportWithCountEXT(cmd, 1, &viewport);
		vkCmdSetScissorWithCountEXT(cmd, 1, &scissor);
	}

	// Rasterization is always enabled
	vkCmdSetRasterizerDiscardEnableEXT(cmd, VK_FALSE);

	{
		// Setup vertex input with position, normals, and uv
		const VkVertexInputBindingDescription2EXT vertex_binding[] =
		    {
		        vkb::initializers::vertex_input_binding_description2ext(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX, 1)};

		const VkVertexInputAttributeDescription2EXT vertex_attribute_description_ext[] =
		    {
		        vkb::initializers::vertex_input_attribute_description2ext(
		            0,
		            0,
		            VK_FORMAT_R32G32B32_SFLOAT,
		            offsetof(Vertex, pos)),
		        vkb::initializers::vertex_input_attribute_description2ext(
		            0,
		            1,
		            VK_FORMAT_R32G32B32_SFLOAT,
		            offsetof(Vertex, normal)),
		        vkb::initializers::vertex_input_attribute_description2ext(
		            0,
		            2,
		            VK_FORMAT_R32G32_SFLOAT,
		            offsetof(Vertex, uv)),
		    };

		vkCmdSetVertexInputEXT(cmd, sizeof(vertex_binding) / sizeof(vertex_binding[0]), vertex_binding, sizeof(vertex_attribute_description_ext) / sizeof(vertex_attribute_description_ext[0]), vertex_attribute_description_ext);
	}

	// Set the topology to triangles, don't restart primitives, set samples to only 1 per pixel
	vkCmdSetPrimitiveTopologyEXT(cmd, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	vkCmdSetPrimitiveRestartEnableEXT(cmd, VK_FALSE);
	vkCmdSetRasterizationSamplesEXT(cmd, VK_SAMPLE_COUNT_1_BIT);

	{
		// Use 1 sample per pixel
		const VkSampleMask sample_mask = 0x1;
		vkCmdSetSampleMaskEXT(cmd, VK_SAMPLE_COUNT_1_BIT, &sample_mask);
	}

	// Do not use alpha to coverage or alpha to one because not using MSAA
	vkCmdSetAlphaToCoverageEnableEXT(cmd, VK_FALSE);

	// Enable wireframe only if supported and enabled
	vkCmdSetPolygonModeEXT(cmd, wireframe_mode && wireframe_enabled ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL);
	if (wireframe_mode && wireframe_enabled)
	{
		vkCmdSetLineWidth(cmd, 1.0f);
	}

	// Set front face, cull mode is set in build_command_buffers.
	vkCmdSetFrontFaceEXT(cmd, VK_FRONT_FACE_COUNTER_CLOCKWISE);

	// Set depth state, the depth write. Don't enable depth bounds, bias, or stencil test.
	vkCmdSetDepthTestEnableEXT(cmd, VK_TRUE);
	vkCmdSetDepthCompareOpEXT(cmd, VK_COMPARE_OP_GREATER);
	vkCmdSetDepthBoundsTestEnableEXT(cmd, VK_FALSE);
	vkCmdSetDepthBiasEnableEXT(cmd, VK_FALSE);
	vkCmdSetStencilTestEnableEXT(cmd, VK_FALSE);

	// Do not enable logic op
	vkCmdSetLogicOpEnableEXT(cmd, VK_FALSE);

	{
		// Disable color blending
		VkBool32 color_blend_enables[] = {VK_FALSE};
		vkCmdSetColorBlendEnableEXT(cmd, 0, 1, color_blend_enables);
	}

	{
		// Use RGBA color write mask
		VkColorComponentFlags color_component_flags[] = {VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_A_BIT};
		vkCmdSetColorWriteMaskEXT(cmd, 0, 1, color_component_flags);
	}
}

void ShaderObject::bind_material_shader(VkCommandBuffer cmd_buffer, int shader_index)
{
	CurrentShader &shader = current_material_shaders[shader_index];

	bind_shader(cmd_buffer, material_vert_shaders[shader.vert]);
	if (enable_geometry_pass)
		bind_shader(cmd_buffer, material_geo_shaders[shader.geom]);
	bind_shader(cmd_buffer, material_frag_shaders[shader.frag]);
}

void ShaderObject::bind_basic_shader(VkCommandBuffer cmd_buffer, int shader_index)
{
	bind_shader(cmd_buffer, basic_vert_shaders[current_basic_linked_shaders[shader_index]]);
	bind_shader(cmd_buffer, basic_frag_shaders[current_basic_linked_shaders[shader_index]]);
}

void ShaderObject::draw(float delta_time)
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

void ShaderObject::render(float delta_time)
{
	// Increment timers
	elapsed_time += delta_time;
	elapsed_iteration_time += delta_time;

	if (!prepared)
	{
		return;
	}

	if (elapsed_iteration_time > max_iteration_time && iterate_permutations)
	{
		elapsed_iteration_time = 0.0f;
		iterate_current();
	}

	update_descriptor_sets();

	if (camera.updated)
	{
		update_uniform_buffers();
	}

	rebuild_command_buffers();

	draw(delta_time);

	get_timestamp_results();
}

void ShaderObject::iterate_current()
{
	// Pick random numbers to decide what shader or output to change
	std::uniform_int_distribution<int> distribution{0, 100};

	constexpr int max_selectable_objects = std::max(num_basic_objects, num_material_objects);
	int           selected_shader        = distribution(rng) % max_selectable_objects;

	// If iteratable then push back lambda that iterates shader and pick random shader to iterate
	std::vector<std::function<void()>> funcs;

	if (iterate_basic)
	{
		funcs.emplace_back([selected_shader, this]() {
			selected_basic_object = selected_shader % num_basic_objects;
			current_basic_linked_shaders[selected_shader % num_basic_objects]++;
			current_basic_linked_shaders[selected_shader % num_basic_objects] %= basic_vert_shaders.size();
		});
	}

	if (iterate_material_vert)
	{
		funcs.emplace_back([selected_shader, this]() {
			selected_material_object = selected_shader % num_material_objects;
			current_material_shaders[selected_shader % num_material_objects].vert++;
			current_material_shaders[selected_shader % num_material_objects].vert %= material_vert_shaders.size();
		});
	}

	if (iterate_material_geo)
	{
		funcs.emplace_back([selected_shader, this]() {
			selected_material_object = selected_shader % num_material_objects;
			current_material_shaders[selected_shader % num_material_objects].geom++;
			current_material_shaders[selected_shader % num_material_objects].geom %= material_geo_shaders.size();
		});
	}

	if (iterate_material_frag)
	{
		funcs.emplace_back([selected_shader, this]() {
			selected_material_object = selected_shader % num_material_objects;
			current_material_shaders[selected_shader % num_material_objects].frag++;
			current_material_shaders[selected_shader % num_material_objects].frag %= material_frag_shaders.size();
		});
	};

	if (iterate_post_process)
	{
		funcs.emplace_back([this]() {
			current_post_process_shader++;
			current_post_process_shader %= post_process_frag_shaders.size();
		});
	};

	if (iterate_output)
	{
		funcs.emplace_back([this]() {
			current_output_format++;
			current_output_format %= output_images.size();
		});
	}

	if (iterate_depth)
	{
		funcs.emplace_back([this]() {
			current_depth_format++;
			current_depth_format %= depth_images.size();
		});
	}

	if (funcs.size() == 0)
		return;

	// Call a randomly chosen function
	funcs[distribution(rng) % funcs.size()]();
}

void ShaderObject::randomize_current()
{
	// For each shader and output select a new
	std::uniform_int_distribution<int> distribution{0, 100};

	if (iterate_basic)
	{
		for (int i = 0; i < num_basic_objects; ++i)
		{
			current_basic_linked_shaders[i] += distribution(rng);
			current_basic_linked_shaders[i] %= basic_vert_shaders.size();
		}
	}

	if (iterate_material_vert)
	{
		for (int i = 0; i < num_material_objects; ++i)
		{
			current_material_shaders[i].vert += distribution(rng);
			current_material_shaders[i].vert %= material_vert_shaders.size();
		}
	}

	if (iterate_material_geo)
	{
		for (int i = 0; i < num_material_objects; ++i)
		{
			current_material_shaders[i].geom += distribution(rng);
			current_material_shaders[i].geom %= material_geo_shaders.size();
		}
	}

	if (iterate_material_frag)
	{
		for (int i = 0; i < num_material_objects; ++i)
		{
			current_material_shaders[i].frag += distribution(rng);
			current_material_shaders[i].frag %= material_frag_shaders.size();
		}
	}

	if (iterate_post_process)
	{
		current_post_process_shader += distribution(rng);
		current_post_process_shader %= post_process_frag_shaders.size();
	}

	if (iterate_output)
	{
		current_output_format += distribution(rng);
		current_output_format %= output_images.size();
	}

	if (iterate_depth)
	{
		current_depth_format += distribution(rng);
		current_depth_format %= depth_images.size();
	}
}

// Helper function for imgui slider for togglable sliders
void imgui_slider(bool *enabled, std::string formatted_slider, std::string shader_name, int *slider_int, int num_shaders, const int alignment = 290, const int checkbox_alignment = 30)
{
	ImGui::Checkbox(fmt::format("##{}", formatted_slider.c_str()).c_str(), enabled);
	ImGui::SameLine(checkbox_alignment);
	if (*enabled)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.3f, 0.3f, 1.f));
	}
	ImGui::SliderInt(formatted_slider.c_str(), slider_int, 0, num_shaders);
	ImGui::PopStyleColor();
	ImGui::SameLine(alignment);
	ImGui::Text("%s", shader_name.c_str());
}

void ShaderObject::on_update_ui_overlay(vkb::Drawer &drawer)
{
	const float dpi_factor = window->get_dpi_factor();
	const float font_size  = ImGui::GetFontSize();

	if (ImGui::CollapsingHeader("Options"))
	{
		const int checkbox_option_spacing = std::min(std::max(width, 1300u), 2000u) * 0.12f * dpi_factor;
		const int slider_spacing          = std::min(std::max(width, 1300u), 2000u) * 0.24f * dpi_factor;
		const int checkbox_spacing        = std::min(std::max(width, 1300u), 2000u) * 0.025f * dpi_factor;

		// Only display wireframe setting if wireframe is enabled
		if (wireframe_enabled)
		{
			drawer.checkbox("Wireframe Mode", &wireframe_mode);
			ImGui::SameLine(checkbox_option_spacing);
			drawer.checkbox("Iterate Mode", &iterate_permutations);
			ImGui::SameLine(checkbox_option_spacing * 2);
		}
		else
		{
			drawer.checkbox("Iterate Mode", &iterate_permutations);
			ImGui::SameLine(checkbox_option_spacing);
		}

		drawer.checkbox("Post Processing Enabled", &post_processing);

		drawer.checkbox("Material Shader Geometry Pass Enabled", &enable_geometry_pass);

		drawer.text("Checkbox Enables Random Shader Iterate");

		ImGui::SliderInt("Selected Basic Object:", &selected_basic_object, 0, num_basic_objects - 1);

		imgui_slider(&iterate_basic, "Basic Linked Shader Set:",
		             basic_vert_shaders[current_basic_linked_shaders[selected_basic_object]]->get_name(),
		             &current_basic_linked_shaders[selected_basic_object], basic_vert_shaders.size() - 1,
		             slider_spacing, checkbox_spacing);

		ImGui::SliderInt("Selected Material Object:", &selected_material_object, 0, num_material_objects - 1);

		imgui_slider(&iterate_material_vert, "Material Vert Shader:",
		             material_vert_shaders[current_material_shaders[selected_material_object].vert]->get_name(),
		             &current_material_shaders[selected_material_object].vert, material_vert_shaders.size() - 1,
		             slider_spacing, checkbox_spacing);

		imgui_slider(&iterate_material_geo, "Material Geo Shader:",
		             material_geo_shaders[current_material_shaders[selected_material_object].geom]->get_name(),
		             &current_material_shaders[selected_material_object].geom, material_geo_shaders.size() - 1,
		             slider_spacing, checkbox_spacing);

		imgui_slider(&iterate_material_frag, "Material Frag Shader:",
		             material_frag_shaders[current_material_shaders[selected_material_object].frag]->get_name(),
		             &current_material_shaders[selected_material_object].frag, material_frag_shaders.size() - 1,
		             slider_spacing, checkbox_spacing);

		imgui_slider(&iterate_post_process, "Post Process Frag Shader:",
		             post_process_frag_shaders[current_post_process_shader]->get_name(),
		             &current_post_process_shader, post_process_frag_shaders.size() - 1,
		             slider_spacing, checkbox_spacing);

		imgui_slider(&iterate_output, "Output Format:",
		             supported_output_formats[current_output_format].name.c_str(),
		             &current_output_format, supported_output_formats.size() - 1,
		             slider_spacing, checkbox_spacing);

		imgui_slider(&iterate_depth, "Depth Format:",
		             supported_depth_formats[current_depth_format].name,
		             &current_depth_format, supported_depth_formats.size() - 1,
		             slider_spacing, checkbox_spacing);

		if (drawer.button("Randomize All"))
		{
			randomize_current();
		}
	}

	// Manually end and start new ImGui window for the CPU profiler at the bottom of the screen
	ImGui::End();

	const float graph_height  = std::min(height, 400u) * 0.25f * dpi_factor;
	const float window_height = graph_height + (font_size * 2.0f) * dpi_factor;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, window_height));
	ImGui::SetNextWindowPos(ImVec2(0, height - window_height), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(width, window_height));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);

	if (ImGui::Begin("Histograms of CPU Frame time in (ms) of last 2000 frames", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs))
	{
		float max_value = *std::max_element(timestamp_values.begin(), timestamp_values.end());

		ImGui::Text("16.667 ms");
		ImGui::SameLine(-font_size);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
		ImGui::PlotLines("##Frame Times", timestamp_values.data(), timestamp_values.size(), current_timestamp + 1, 0, 0.0f,
		                 16.667f, ImVec2(1.08f * width * dpi_factor, graph_height));

		ImGui::PopStyleColor();
		ImGui::Text("CPU Frame Time: %f ms (max %f ms)", timestamp_values[current_timestamp], max_value);
	}

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();

	current_timestamp = (current_timestamp + 1) % timestamp_values.size();

	// Add a push item width so the expected ImGui state matches and let framework call ImGui::End()
	ImGui::PushItemWidth(110.0f * dpi_factor);
}

void ShaderObject::get_timestamp_results()
{
	timestamp_values[current_timestamp] = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start_time).count();
	start_time                          = std::chrono::steady_clock::now();
}

VkDescriptorImageInfo ShaderObject::create_image_descriptor(Sampler &texture, VkDescriptorType descriptor_type)
{
	VkDescriptorImageInfo descriptor{};
	descriptor.sampler   = texture.sampler;
	descriptor.imageView = texture.image.image_view;

	// Add image layout info based on descriptor type
	switch (descriptor_type)
	{
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
			descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			break;
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			break;
		default:
			descriptor.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			break;
	}

	return descriptor;
}

ShaderObject::Image ShaderObject::create_output_image(VkFormat format, VkImageUsageFlags usageFlags, VkImageAspectFlags aspectMask)
{
	Image image;

	// Create image with 1 sample and optimal tiling for output image
	VkImageCreateInfo image_info = vkb::initializers::image_create_info();
	image_info.format            = format;
	image_info.extent            = {width, height, 1};
	image_info.mipLevels         = 1;
	image_info.arrayLayers       = 1;
	image_info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage             = usageFlags;
	image_info.imageType         = VK_IMAGE_TYPE_2D;
	image_info.samples           = VK_SAMPLE_COUNT_1_BIT;
	image_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
	image_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(vkCreateImage(get_device().get_handle(), &image_info, nullptr, &image.image));

	VkMemoryAllocateInfo memory_allocation_info = vkb::initializers::memory_allocate_info();
	VkMemoryRequirements memory_requirements;

	// Get and set memory allocation size then allocate and bind memory
	vkGetImageMemoryRequirements(get_device().get_handle(), image.image, &memory_requirements);
	memory_allocation_info.allocationSize  = memory_requirements.size;
	memory_allocation_info.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocation_info, nullptr, &image.memory));
	VK_CHECK(vkBindImageMemory(get_device().get_handle(), image.image, image.memory, 0));

	// Create image with specified format and aspect
	VkImageViewCreateInfo image_view           = vkb::initializers::image_view_create_info();
	image_view.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	image_view.format                          = format;
	image_view.subresourceRange.aspectMask     = aspectMask;
	image_view.subresourceRange.baseMipLevel   = 0;
	image_view.subresourceRange.levelCount     = 1;
	image_view.subresourceRange.baseArrayLayer = 0;
	image_view.subresourceRange.layerCount     = 1;
	image_view.image                           = image.image;

	// Create image view
	VK_CHECK(vkCreateImageView(get_device().get_handle(), &image_view, nullptr, &image.image_view));

	return image;
}

void ShaderObject::build_shader(VkDevice device, ShaderObject::Shader *shader)
{
	VkShaderEXT           shaderEXT;
	VkShaderCreateInfoEXT shaderCreateInfo = shader->get_create_info();

	VkResult result = vkCreateShadersEXT(device, 1, &shaderCreateInfo, nullptr, &shaderEXT);

	if (result != VK_SUCCESS)
	{
		LOGE("vkCreateShadersEXT failed\n");
	}

	shader->set_shader(shaderEXT);
}

void ShaderObject::build_linked_shaders(VkDevice device, ShaderObject::Shader *vert, ShaderObject::Shader *frag)
{
	VkShaderCreateInfoEXT shader_create_infos[2];

	if (vert == nullptr || frag == nullptr)
	{
		LOGE("build_linked_shaders failed with null vertex or fragment shader\n");
	}

	shader_create_infos[0] = vert->get_create_info();
	shader_create_infos[1] = frag->get_create_info();

	for (auto &shader_create : shader_create_infos)
	{
		shader_create.flags |= VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;
	}

	VkShaderEXT shaderEXTs[2];

	// Create the shader objects
	VkResult result = vkCreateShadersEXT(device,
	                                     2,
	                                     shader_create_infos,
	                                     nullptr,
	                                     shaderEXTs);

	if (result != VK_SUCCESS)
	{
		LOGE("vkCreateShadersEXT failed\n");
	}

	vert->set_shader(shaderEXTs[0]);
	frag->set_shader(shaderEXTs[1]);
}

void ShaderObject::bind_shader(VkCommandBuffer cmd_buffer, ShaderObject::Shader *shader)
{
	vkCmdBindShadersEXT(cmd_buffer, 1, shader->get_stage(), shader->get_shader());
}

ShaderObject::Shader::Shader(VkShaderStageFlagBits        stage_,
                             VkShaderStageFlags           next_stage_,
                             std::string                  shader_name_,
                             const std::vector<uint8_t>  &vert_glsl_source,
                             const VkDescriptorSetLayout *pSetLayouts,
                             const VkPushConstantRange   *pPushConstantRange)
{
	stage       = stage_;
	shader_name = shader_name_;
	next_stage  = next_stage_;

	vkb::GLSLCompiler glsl_compiler;
	std::string       info_log;

	// Compile the GLSL source
	if (!glsl_compiler.compile_to_spirv(stage, vert_glsl_source, "main", {}, spirv, info_log))
	{
		LOGE("Failed to compile shader, Error: {}", info_log.c_str());
	}

	// Fill out the shader create info struct
	vk_shader_create_info.sType                  = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
	vk_shader_create_info.pNext                  = nullptr;
	vk_shader_create_info.flags                  = 0;
	vk_shader_create_info.stage                  = stage;
	vk_shader_create_info.nextStage              = next_stage;
	vk_shader_create_info.codeType               = VK_SHADER_CODE_TYPE_SPIRV_EXT;
	vk_shader_create_info.codeSize               = spirv.size() * sizeof(spirv[0]);
	vk_shader_create_info.pCode                  = spirv.data();
	vk_shader_create_info.pName                  = "main";
	vk_shader_create_info.setLayoutCount         = 1;
	vk_shader_create_info.pSetLayouts            = pSetLayouts;
	vk_shader_create_info.pushConstantRangeCount = 1;
	vk_shader_create_info.pPushConstantRanges    = pPushConstantRange;
	vk_shader_create_info.pSpecializationInfo    = nullptr;
}

void ShaderObject::Shader::destroy(VkDevice device)
{
	// Cleanup shader if not null
	if (shader != VK_NULL_HANDLE)
	{
		vkDestroyShaderEXT(device, shader, nullptr);
		shader = VK_NULL_HANDLE;
	}
}

std::unique_ptr<vkb::VulkanSample> create_shader_object()
{
	return std::make_unique<ShaderObject>();
}
