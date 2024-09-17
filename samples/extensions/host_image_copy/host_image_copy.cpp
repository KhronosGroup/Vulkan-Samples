/* Copyright (c) 2024, Sascha Willems
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

#include "host_image_copy.h"

HostImageCopy::HostImageCopy()
{
	title    = "Host image copy";
	zoom     = -4.0f;
	rotation = {-25.0f, 45.0f, 0.0f};

	// Enable required extensions
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_FORMAT_FEATURE_FLAGS_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME);
	add_device_extension(VK_EXT_HOST_IMAGE_COPY_EXTENSION_NAME);
}

HostImageCopy::~HostImageCopy()
{
	if (has_device())
	{
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
		destroy_texture(texture);
		uniform_buffer_vs.reset();
	}
}

// Enable physical device features required for this example
void HostImageCopy::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// Enable host image copy feature (required for this sample to work)
	auto &requested_host_image_copy_features         = gpu.request_extension_features<VkPhysicalDeviceHostImageCopyFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_IMAGE_COPY_FEATURES_EXT);
	requested_host_image_copy_features.hostImageCopy = VK_TRUE;

	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}
}

/*
    Upload texture image data to the GPU

    Unlike the texture(3d/array/etc) samples, this one uses the VK_EXT_host_image_copy to drasticly simplify the process
    of uploading an image from the host to the GPU. This new extension adds a way of directly uploading image data from
    host memory to an optimal tiled image on the device (GPU). This no longer requires a staging buffer in between, as we can
    now directly copy data stored in host memory to the image. The extension also adds new functionality to simplify image barriers
*/
void HostImageCopy::load_texture()
{
	// We use the Khronos texture format (https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/)
	std::string filename = vkb::fs::path::get(vkb::fs::path::Assets, "textures/metalplate01_rgba.ktx");
	// ktx1 doesn't know whether the content is sRGB or linear, but most tools save in sRGB, so assume that.
	VkFormat image_format = VK_FORMAT_R8G8B8A8_SRGB;

	ktxTexture    *ktx_texture;
	KTX_error_code result;

	result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx_texture);

	if (ktx_texture == nullptr)
	{
		throw std::runtime_error("Couldn't load texture");
	}

	// assert(!tex2D.empty());

	texture.width      = ktx_texture->baseWidth;
	texture.height     = ktx_texture->baseHeight;
	texture.mip_levels = ktx_texture->numLevels;

	ktx_uint8_t *ktx_image_data   = ktx_texture->pData;
	ktx_size_t   ktx_texture_size = ktx_texture->dataSize;

	// Check if the image format supports the host image copy flag
	// Note: All formats that support sampling are required to support this flag
	// So for the format used here (R8G8B8A8_UNORM) we could skip this check
	// The flag we need to check is an extension flag, so we need to go through VkFormatProperties3
	VkFormatProperties3 format_properties_3{};
	format_properties_3.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3_KHR;
	// Properties3 need to be chained into Properties2
	VkFormatProperties2 format_properties_2{};
	format_properties_2.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
	format_properties_2.pNext = &format_properties_3;
	vkGetPhysicalDeviceFormatProperties2KHR(get_device().get_gpu().get_handle(), image_format, &format_properties_2);

	if ((format_properties_3.optimalTilingFeatures & VK_FORMAT_FEATURE_2_HOST_IMAGE_TRANSFER_BIT_EXT) == 0)
	{
		LOGE("The selected image format does not support the required host transfer bit")
		throw std::runtime_error{"The selected image format does not support the required host transfer bit"};
	}

	// Create optimal tiled target image on the device
	VkImageCreateInfo image_create_info = vkb::initializers::image_create_info();
	image_create_info.imageType         = VK_IMAGE_TYPE_2D;
	image_create_info.format            = image_format;
	image_create_info.mipLevels         = texture.mip_levels;
	image_create_info.arrayLayers       = 1;
	image_create_info.samples           = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
	image_create_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
	image_create_info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
	image_create_info.extent            = {texture.width, texture.height, 1};
	// For images that use host image copy we need to specify the VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT usage flag
	image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT;
	VK_CHECK(vkCreateImage(get_device().get_handle(), &image_create_info, nullptr, &texture.image));

	// Setup memory for backing the image on the device
	VkMemoryAllocateInfo memory_allocate_info = vkb::initializers::memory_allocate_info();
	VkMemoryRequirements memory_requirements  = {};
	vkGetImageMemoryRequirements(get_device().get_handle(), texture.image, &memory_requirements);
	memory_allocate_info.allocationSize  = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocate_info, nullptr, &texture.device_memory));
	VK_CHECK(vkBindImageMemory(get_device().get_handle(), texture.image, texture.device_memory, 0));

	// With host image copy we can directly copy from the KTX image in host memory to the device
	// This is pretty straight forward, as the KTX image is already tightly packed, doesn't need any swizzle and as such matches
	// what the device expects

	// Set up copy information for all mip levels stored in the image
	std::vector<VkMemoryToImageCopyEXT> memory_to_image_copies{};
	for (uint32_t i = 0; i < texture.mip_levels; i++)
	{
		// Setup a buffer image copy structure for the current mip level
		VkMemoryToImageCopyEXT memory_to_image_copy          = {};
		memory_to_image_copy.sType                           = VK_STRUCTURE_TYPE_MEMORY_TO_IMAGE_COPY_EXT;
		memory_to_image_copy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		memory_to_image_copy.imageSubresource.mipLevel       = i;
		memory_to_image_copy.imageSubresource.baseArrayLayer = 0;
		memory_to_image_copy.imageSubresource.layerCount     = 1;
		memory_to_image_copy.imageExtent.width               = ktx_texture->baseWidth >> i;
		memory_to_image_copy.imageExtent.height              = ktx_texture->baseHeight >> i;
		memory_to_image_copy.imageExtent.depth               = 1;

		// This tells the implementation where to read the data from
		// As the KTX file is tightly packed, we can simply offset into that buffer for the current mip level
		ktx_size_t     offset;
		KTX_error_code ret = ktxTexture_GetImageOffset(ktx_texture, i, 0, 0, &offset);
		assert(ret == KTX_SUCCESS);
		memory_to_image_copy.pHostPointer = ktx_image_data + offset;

		memory_to_image_copies.push_back(memory_to_image_copy);
	}

	VkImageSubresourceRange subresource_range{};
	subresource_range.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource_range.baseMipLevel = 0;
	subresource_range.levelCount   = texture.mip_levels;
	subresource_range.layerCount   = 1;

	// VK_EXT_host_image_copy also introduces a simplified way of doing the required image transition on the host
	// This no longer requires a dedicated command buffer to submit the barrier
	// We also no longer need multiple transitions, and only have to do one for the final layout
	VkHostImageLayoutTransitionInfoEXT host_image_layout_transition_info{};
	host_image_layout_transition_info.sType            = VK_STRUCTURE_TYPE_HOST_IMAGE_LAYOUT_TRANSITION_INFO_EXT;
	host_image_layout_transition_info.image            = texture.image;
	host_image_layout_transition_info.oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
	host_image_layout_transition_info.newLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	host_image_layout_transition_info.subresourceRange = subresource_range;

	vkTransitionImageLayoutEXT(get_device().get_handle(), 1, &host_image_layout_transition_info);

	// With the image in the correct layout and copy information for all mip levels setup, we can now issue the copy to our taget image from the host
	// The implementation will then convert this to an implementation specific optimal tiling layout
	VkCopyMemoryToImageInfoEXT copy_memory_info{};
	copy_memory_info.sType          = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_IMAGE_INFO_EXT;
	copy_memory_info.dstImage       = texture.image;
	copy_memory_info.dstImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	copy_memory_info.regionCount    = static_cast<uint32_t>(memory_to_image_copies.size());
	copy_memory_info.pRegions       = memory_to_image_copies.data();

	vkCopyMemoryToImageEXT(get_device().get_handle(), &copy_memory_info);

	// Once uploaded, the ktx_texture can be safely destroyed
	ktxTexture_Destroy(ktx_texture);

	// Calculate valid filter and mipmap modes
	VkFilter            filter      = VK_FILTER_LINEAR;
	VkSamplerMipmapMode mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	vkb::make_filters_valid(get_device().get_gpu().get_handle(), image_format, &filter, &mipmap_mode);

	// Create a texture sampler
	VkSamplerCreateInfo sampler = vkb::initializers::sampler_create_info();
	sampler.magFilter           = filter;
	sampler.minFilter           = filter;
	sampler.mipmapMode          = mipmap_mode;
	sampler.addressModeU        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeV        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeW        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.mipLodBias          = 0.0f;
	sampler.compareOp           = VK_COMPARE_OP_NEVER;
	sampler.minLod              = 0.0f;
	sampler.maxLod              = static_cast<float>(texture.mip_levels);
	sampler.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	if (get_device().get_gpu().get_features().samplerAnisotropy)
	{
		sampler.maxAnisotropy    = get_device().get_gpu().get_properties().limits.maxSamplerAnisotropy;
		sampler.anisotropyEnable = VK_TRUE;
	}
	else
	{
		sampler.maxAnisotropy    = 1.0;
		sampler.anisotropyEnable = VK_FALSE;
	}
	VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler, nullptr, &texture.sampler));

	// Create image view
	VkImageViewCreateInfo view = vkb::initializers::image_view_create_info();
	view.viewType              = VK_IMAGE_VIEW_TYPE_2D;
	view.format                = image_format;
	view.components            = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
	view.subresourceRange      = subresource_range;
	view.image                 = texture.image;
	VK_CHECK(vkCreateImageView(get_device().get_handle(), &view, nullptr, &texture.view));
}

void HostImageCopy::load_assets()
{
	cube = load_model("scenes/textured_unit_cube.gltf");
}

// Free all Vulkan resources used by a texture object
void HostImageCopy::destroy_texture(Texture texture)
{
	vkDestroyImageView(get_device().get_handle(), texture.view, nullptr);
	vkDestroyImage(get_device().get_handle(), texture.image, nullptr);
	vkDestroySampler(get_device().get_handle(), texture.sampler, nullptr);
	vkFreeMemory(get_device().get_handle(), texture.device_memory, nullptr);
}

void HostImageCopy::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = default_clear_color;
	clear_values[1].depthStencil = {0.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.offset.x      = 0;
	render_pass_begin_info.renderArea.offset.y      = 0;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = 2;
	render_pass_begin_info.pClearValues             = clear_values;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		// Set target frame buffer
		render_pass_begin_info.framebuffer = framebuffers[i];

		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		draw_model(cube, draw_cmd_buffers[i]);

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void HostImageCopy::draw()
{
	ApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();
}

void HostImageCopy::setup_descriptor_pool()
{
	// Example uses one ubo and one image sampler
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), 2);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void HostImageCopy::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings =
	    {
	        // Binding 0 : Vertex shader uniform buffer
	        vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
	        // Binding 1 : Fragment shader image sampler
	        vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)};

	VkDescriptorSetLayoutCreateInfo descriptor_layout =
	    vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(&descriptor_set_layout, 1);

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void HostImageCopy::setup_descriptor_set()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

	VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*uniform_buffer_vs);

	VkDescriptorImageInfo image_descriptor;
	image_descriptor.imageView   = texture.view;
	image_descriptor.sampler     = texture.sampler;
	image_descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &buffer_descriptor),
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &image_descriptor)};

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

void HostImageCopy::prepare_pipelines()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 0);

	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);

	// Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER);

	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

	std::vector<VkDynamicState> dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(dynamic_state_enables.data(), static_cast<uint32_t>(dynamic_state_enables.size()), 0);

	const std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {
	    load_shader("texture_loading", "texture.vert", VK_SHADER_STAGE_VERTEX_BIT),
	    load_shader("texture_loading", "texture.frag", VK_SHADER_STAGE_FRAGMENT_BIT)};

	// Vertex bindings and attributes
	const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)),
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)),
	    vkb::initializers::vertex_input_attribute_description(0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)),
	};
	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	VkGraphicsPipelineCreateInfo pipeline_create_info =
	    vkb::initializers::pipeline_create_info(pipeline_layout, render_pass, 0);

	pipeline_create_info.pVertexInputState   = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;
	pipeline_create_info.stageCount          = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages             = shader_stages.data();

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline));
}

// Prepare and initialize uniform buffer containing shader uniforms
void HostImageCopy::prepare_uniform_buffers()
{
	// Vertex shader uniform buffer block
	uniform_buffer_vs = std::make_unique<vkb::core::BufferC>(get_device(),
	                                                         sizeof(ubo_vs),
	                                                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                         VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void HostImageCopy::update_uniform_buffers()
{
	// Vertex shader
	ubo_vs.projection     = glm::perspective(glm::radians(60.0f), static_cast<float>(width) / static_cast<float>(height), 0.001f, 256.0f);
	glm::mat4 view_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zoom));

	ubo_vs.model = view_matrix * glm::translate(glm::mat4(1.0f), camera_pos);
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	ubo_vs.view_pos = glm::vec4(0.0f, 0.0f, -zoom, 0.0f);

	uniform_buffer_vs->convert_and_update(ubo_vs);
}

bool HostImageCopy::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}
	load_assets();
	load_texture();
	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_set();
	build_command_buffers();
	prepared = true;
	return true;
}

void HostImageCopy::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
}

void HostImageCopy::view_changed()
{
	update_uniform_buffers();
}

void HostImageCopy::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.slider_float("LOD bias", &ubo_vs.lod_bias, 0.0f, static_cast<float>(texture.mip_levels)))
		{
			update_uniform_buffers();
		}
	}
}

std::unique_ptr<vkb::VulkanSampleC> create_host_image_copy()
{
	return std::make_unique<HostImageCopy>();
}
