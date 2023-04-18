/* Copyright (c) 2023, Mobica Limited
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

#include "sparse_image.h"

SparseImage::SparseImage()
{
	title = "Sparse image";
}

SparseImage::~SparseImage()
{
	if (device)
	{
		object.reset();
		ubo.reset();

		vkDestroyImageView(get_device().get_handle(), texture.view, VK_NULL_HANDLE);
		vkDestroyImage(get_device().get_handle(), sparse_image.image, VK_NULL_HANDLE);
		vkDestroySampler(get_device().get_handle(), texture.sampler, VK_NULL_HANDLE);
		texture      = {};
		sparse_image = {};

		vkDestroyPipeline(get_device().get_handle(), pipeline, VK_NULL_HANDLE);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, VK_NULL_HANDLE);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, VK_NULL_HANDLE);
	}
}

/**
 * 	@fn bool SparseImage::prepare(vkb::Platform &platform)
 * 	@brief Configuring all sample specific settings, creating descriptor sets/pool,
 * 	pipelines, generating or loading models etc.
 */
bool SparseImage::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	set_camera();
	load_assets();
	create_sparse_image();
	create_sparse_texture();
	prepare_uniform_buffers();
	create_descriptor_pool();
	setup_descriptor_set_layout();
	create_descriptor_set();
	prepare_pipeline();
	build_command_buffers();

	prepared = true;
	return true;
}

/**
 * @fn void SparseImage::request_gpu_features(vkb::PhysicalDevice &gpu)
 * @brief Enabling gpu features
 */
void SparseImage::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}

	if (gpu.get_features().sparseBinding && gpu.get_features().sparseResidencyImage2D)
	{
		gpu.get_mutable_requested_features().sparseBinding           = VK_TRUE;
		gpu.get_mutable_requested_features().shaderResourceResidency = VK_TRUE;
		gpu.get_mutable_requested_features().sparseResidencyImage2D  = VK_TRUE;
	}
}

/**
 * @fn void SparseImage::create_sparse_image()
 * @brief Creating sparse image and allocating memory
 */
void SparseImage::create_sparse_image()
{
	auto texture_mipmaps = texture.image->get_mipmaps();
	assert(!texture_mipmaps.empty());

	// Calculate number of mip levels as per Vulkan specs:
	texture.mip_levels = vkb::to_u32(floor(log2(std::max(texture_mipmaps[0].extent.width, texture_mipmaps[0].extent.height))) + 1);

	VkImageCreateInfo sparseImageCreateInfo = vkb::initializers::image_create_info();
	sparseImageCreateInfo.imageType         = VK_IMAGE_TYPE_2D;
	sparseImageCreateInfo.extent.width      = texture_mipmaps[0].extent.width;
	sparseImageCreateInfo.extent.height     = texture_mipmaps[0].extent.height;
	sparseImageCreateInfo.mipLevels         = texture.mip_levels;
	sparseImageCreateInfo.extent.depth      = texture_mipmaps[0].extent.depth;
	sparseImageCreateInfo.arrayLayers       = 1;
	sparseImageCreateInfo.format            = VK_FORMAT_R8G8B8A8_SRGB;
	sparseImageCreateInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
	sparseImageCreateInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
	sparseImageCreateInfo.usage             = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	sparseImageCreateInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
	sparseImageCreateInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
	sparseImageCreateInfo.flags             = VK_IMAGE_CREATE_SPARSE_BINDING_BIT | VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT;

	VK_CHECK(vkCreateImage(get_device().get_handle(), &sparseImageCreateInfo, VK_NULL_HANDLE, &sparse_image.image));

	VkMemoryRequirements sparseImageMemoryRequirements{};
	vkGetImageMemoryRequirements(get_device().get_handle(), sparse_image.image, &sparseImageMemoryRequirements);

	VkPhysicalDeviceProperties gpu_properties{};
	vkGetPhysicalDeviceProperties(get_device().get_gpu().get_handle(), &gpu_properties);

	// Check requested image size against gpu sparse limit
	if (sparseImageMemoryRequirements.size > gpu_properties.limits.sparseAddressSpaceSize)
	{
		throw std::runtime_error("Sparse image memory size exceeds limits of sparse address space size");
	};

	uint32_t sparseMemoryRequirementCount = 0;
	vkGetImageSparseMemoryRequirements(get_device().get_handle(), sparse_image.image, &sparseMemoryRequirementCount, VK_NULL_HANDLE);
	if (sparseMemoryRequirementCount == 0)
	{
		throw std::runtime_error("No memory requirements for the sparse image");
	}

	std::vector<VkSparseImageMemoryRequirements> sparseMemoryRequirements(sparseMemoryRequirementCount);
	vkGetImageSparseMemoryRequirements(get_device().get_handle(), sparse_image.image, &sparseMemoryRequirementCount, sparseMemoryRequirements.data());
	texture.memory_type_index = get_device().get_memory_type(sparseImageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkMemoryAllocateInfo allocInfo = vkb::initializers::memory_allocate_info();
	allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize       = sparseImageMemoryRequirements.size;
	allocInfo.memoryTypeIndex      = texture.memory_type_index;

	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &allocInfo, VK_NULL_HANDLE, &sparse_image.memory));

	// Get sparse image requirements for the color aspect
	VkSparseImageMemoryRequirements sparseMemoryReq;
	bool                            colorAspectFound{false};
	for (auto &reqs : sparseMemoryRequirements)
	{
		if (reqs.formatProperties.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT)
		{
			sparseMemoryReq  = reqs;
			colorAspectFound = true;
			break;
		}
	}
	if (!colorAspectFound)
	{
		throw std::runtime_error("Could not find sparse image memory requirements for color aspect bit");
	}

	// Calculate number of required sparse memory bindings by alignment
	assert((sparseImageMemoryRequirements.size % sparseImageMemoryRequirements.alignment) == 0);

	// Get sparse bindings
	uint32_t                        sparseBindsCount = vkb::to_u32(sparseImageMemoryRequirements.size / sparseImageMemoryRequirements.alignment);
	std::vector<VkSparseMemoryBind> sparseMemoryBinds(sparseBindsCount);

	texture.sparse_image_memory_requirements = sparseMemoryReq;

	// The mip tail contains all mip levels > sparseMemoryReq.imageMipTailFirstLod
	// Check if the format has a single mip tail for all layers or one mip tail for each layer
	texture.single_mip_tail  = sparseMemoryReq.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT;
	texture.alinged_mip_size = sparseMemoryReq.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT;
}

/**
 * @fn void SparseImage::void SparseImage::create_sparse_texture()
 * @brief Creating texture
 */
void SparseImage::create_sparse_texture()
{
	texture.sub_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, texture.mip_levels, 0, 1};

	// Get device properties for the requested texture format
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(get_device().get_gpu().get_handle(), texture.format, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) || !(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT))
	{
		throw std::runtime_error("Selected image format does not support blit source and destination");
	}

	const VkImageType           imageType   = VK_IMAGE_TYPE_2D;
	const VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
	const VkImageUsageFlags     imageUsage  = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	const VkImageTiling         imageTiling = VK_IMAGE_TILING_OPTIMAL;

	// Get sparse image properties
	std::vector<VkSparseImageFormatProperties> sparseProperties;
	// Sparse properties count for the desired format
	uint32_t sparsePropertiesCount;
	vkGetPhysicalDeviceSparseImageFormatProperties(get_device().get_gpu().get_handle(), texture.format, imageType, sampleCount, imageUsage, imageTiling, &sparsePropertiesCount, nullptr);
	// Check if sparse is supported for this format
	if (sparsePropertiesCount == 0)
	{
		throw std::runtime_error("Requested format does not support sparse features");
	}

	// Get actual image format properties
	sparseProperties.resize(sparsePropertiesCount);
	vkGetPhysicalDeviceSparseImageFormatProperties(get_device().get_gpu().get_handle(), texture.format, imageType, sampleCount, imageUsage, imageTiling, &sparsePropertiesCount, sparseProperties.data());

	// FLAGS
	/*VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT specifies that the image uses a single mip tail region for all array layers.
	VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT specifies that the first mip level whose dimensions are not integer multiples of the corresponding dimensions of the sparse image block begins the mip tail region.
	VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT specifies that the image uses non-standard sparse image block dimensions, and the imageGranularity values do not match the standard sparse image block dimensions for the given format.*/

	VkCommandBuffer copy_cmd = device->create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	vkb::set_image_layout(copy_cmd, sparse_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture.sub_range);
	device->flush_command_buffer(copy_cmd, queue, true);
}

/**
 * @fn void SparseImage::on_update_ui_overlay(vkb::Drawer &drawer)
 * @brief Projecting GUI and transferring data between GUI and application
 */
void SparseImage::on_update_ui_overlay(vkb::Drawer &drawer)
{
}

/**
 * 	@fn void SparseImage::prepare_pipeline()
 * 	@brief Creating graphical pipeline
 */
void SparseImage::prepare_pipeline()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(
	        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	        0,
	        VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(
	        VK_POLYGON_MODE_FILL,
	        VK_CULL_MODE_BACK_BIT,
	        VK_FRONT_FACE_CLOCKWISE,
	        0);

	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(
	        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	        VK_FALSE);

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);

	/* Note: Using Reversed depth-buffer for increased precision, so Greater depth values are kept */
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(
	        VK_TRUE,
	        VK_TRUE,
	        VK_COMPARE_OP_GREATER);

	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR,
	};
	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        vkb::to_u32(dynamic_state_enables.size()),
	        0);

	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};

	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),        // Position
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = vkb::to_u32(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = vkb::to_u32(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};
	shader_stages[0] = load_shader("sparse_image/object.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("sparse_image/object.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	VkGraphicsPipelineCreateInfo graphics_create = vkb::initializers::pipeline_create_info(pipeline_layout, render_pass);
	graphics_create.pInputAssemblyState          = &input_assembly_state;
	graphics_create.pRasterizationState          = &rasterization_state;
	graphics_create.pColorBlendState             = &color_blend_state;
	graphics_create.pDepthStencilState           = &depth_stencil_state;
	graphics_create.pViewportState               = &viewport_state;
	graphics_create.pMultisampleState            = &multisample_state;
	graphics_create.pDynamicState                = &dynamic_state;
	graphics_create.pVertexInputState            = &vertex_input_state;
	graphics_create.stageCount                   = vkb::to_u32(shader_stages.size());
	graphics_create.pStages                      = shader_stages.data();

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &graphics_create, VK_NULL_HANDLE, &pipeline));
}

/**
 * 	@fn void SparseImage::build_command_buffers()
 * 	@brief Creating command buffers and drawing model on window
 */
void SparseImage::build_command_buffers()
{
	// Clear color and depth values
	std::array<VkClearValue, 2> clear_values{};
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0};

	auto command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = static_cast<uint32_t>(clear_values.size());
	render_pass_begin_info.pClearValues             = clear_values.data();

	for (size_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		auto cmd = draw_cmd_buffers[i];

		VK_CHECK(vkBeginCommandBuffer(cmd, &command_buffer_begin_info));

		// Set framebuffer for this command buffer
		render_pass_begin_info.framebuffer = framebuffers[i];

		vkCmdBeginRenderPass(cmd, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		// Set viewport dynamically
		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		// Set scissor dynamically
		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, VK_NULL_HANDLE);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		draw_model(object, cmd);

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(cmd);

		VK_CHECK(vkEndCommandBuffer(cmd));
	}
}

/**
 * 	@fn void SparseImage::render(float delta_time)
 * 	@brief Drawing frames and/or updating uniform buffers when camera position/rotation was changed
 */
void SparseImage::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
	if (camera.updated)
	{
		update_uniform_buffers();
	}
}

/**
 * 	@fn void SparseImage::draw()
 *  @brief Preparing frame and submitting it to the present queue
 */
void SparseImage::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

/**
 * 	@fn void SparseImage::set_camera()
 * 	@brief Set up camera properties
 */
void SparseImage::set_camera()
{
	camera.type = vkb::CameraType::LookAt;
	camera.set_position({2.0f, 0.0f, -10.0f});
	camera.set_rotation({0.0f, 15.0f, 0.0f});
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 256.0f, 0.1f);

	// Maybe different camera type will be better e.g. FirstPerson as in texture_mipmap_generation
	//	zoom = -2.5f;
	//	camera.set_rotation({0.0f, 15.0f, 0.0f});
	//	camera.type = vkb::CameraType::FirstPerson;
	//	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 1024.0f);
	//	camera.set_translation(glm::vec3(0.0f, 0.0f, -12.5f));
}

/**
 * 	@fn void SparseImage::load_assets()
 *	@brief Loading extra models, textures from assets
 */
void SparseImage::load_assets()
{
	// load model from gltf
	// object = load_model("scenes/textured_unit_cube.gltf");
	object = load_model("scenes/tunnel_cylinder.gltf");

	// load texture from file
	texture.image = vkb::sg::Image::load("textures/vulkan_logo_full.ktx", "textures/vulkan_logo_full.ktx", vkb::sg::Image::Color);
}

/**
 * 	@fn void SparseImage::prepare_uniform_buffers()
 * 	@brief Preparing uniform buffer and updating UB data
 */
void SparseImage::prepare_uniform_buffers()
{
	ubo = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(ubo_vs), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	update_uniform_buffers();
}

/**
 * 	@fn void SparseImage::update_uniform_buffers()
 * 	@brief Updating data from application to GPU uniform buffer
 */
void SparseImage::update_uniform_buffers()
{
	ubo_vs.projection = camera.matrices.perspective;
	ubo_vs.view       = camera.matrices.view;
	ubo->convert_and_update(ubo_vs);
}

/**
 * 	@fn void SparseImage::setup_descriptor_set_layout()
 * 	@brief Creating layout for descriptor sets
 */
void SparseImage::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        VK_SHADER_STAGE_VERTEX_BIT,
	        0),
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        VK_SHADER_STAGE_FRAGMENT_BIT,
	        1),
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, VK_NULL_HANDLE, &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &descriptor_set_layout,
	        1);

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, VK_NULL_HANDLE, &pipeline_layout));
}

/**
 * 	@fn void SparseImage::create_image_descriptor()
 * 	@brief Creating image descriptor info
 */
VkDescriptorImageInfo SparseImage::create_image_descriptor()
{
	VkDescriptorImageInfo descriptor{};

	// Create sampler
	VkSamplerCreateInfo sampler = vkb::initializers::sampler_create_info();
	sampler.magFilter           = VK_FILTER_LINEAR;
	sampler.minFilter           = VK_FILTER_LINEAR;
	sampler.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeW        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.mipLodBias          = 0.0f;
	sampler.compareOp           = VK_COMPARE_OP_NEVER;
	sampler.minLod              = 0.0f;
	sampler.maxLod              = static_cast<float>(texture.mip_levels);
	sampler.maxAnisotropy       = get_device().get_gpu().get_features().samplerAnisotropy ? get_device().get_gpu().get_properties().limits.maxSamplerAnisotropy : 1.0f;
	sampler.anisotropyEnable    = get_device().get_gpu().get_features().samplerAnisotropy;
	sampler.borderColor         = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler, VK_NULL_HANDLE, &texture.sampler));

	// Create image view
	VkImageViewCreateInfo view           = vkb::initializers::image_view_create_info();
	view.image                           = VK_NULL_HANDLE;
	view.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	view.format                          = texture.format;
	view.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	view.subresourceRange.baseMipLevel   = 0;
	view.subresourceRange.baseArrayLayer = 0;
	view.subresourceRange.layerCount     = 1;
	view.subresourceRange.levelCount     = texture.mip_levels;
	view.image                           = sparse_image.image;
	VK_CHECK(vkCreateImageView(get_device().get_handle(), &view, VK_NULL_HANDLE, &texture.view));

	descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;        // sparse_image.layout; - should use instead?
	descriptor.imageView   = texture.view;
	descriptor.sampler     = texture.sampler;

	return descriptor;
}

/**
 * 	@fn void SparseImage::create_descriptor_set()
 * 	@brief Creating both descriptor set:
 * 		   1. Uniform buffer
 * 		   2. Image sampler
 */
void SparseImage::create_descriptor_set()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layout,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

	VkDescriptorBufferInfo matrix_buffer_descriptor = create_descriptor(*ubo);
	VkDescriptorImageInfo  image_descriptor         = create_image_descriptor();

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(
	        descriptor_set,
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        0,
	        &matrix_buffer_descriptor),
	    vkb::initializers::write_descriptor_set(
	        descriptor_set,
	        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        1,
	        &image_descriptor),
	};

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()),
	                       write_descriptor_sets.data(), 0, VK_NULL_HANDLE);
}

/**
 * 	@fn void SparseImage::create_descriptor_pool()
 * 	@brief Creating descriptor pool with size adjusted to use uniform buffer and image sampler
 */
void SparseImage::create_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),
	};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(
	        static_cast<uint32_t>(pool_sizes.size()),
	        pool_sizes.data(),
	        2);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, VK_NULL_HANDLE, &descriptor_pool));
}

std::unique_ptr<vkb::VulkanSample> create_sparse_image()
{
	return std::make_unique<SparseImage>();
}
