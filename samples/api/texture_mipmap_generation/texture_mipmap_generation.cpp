/* Copyright (c) 2019-2020, Sascha Willems
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

/*
 * Runtime mip map generation
 */

#include "texture_mipmap_generation.h"

TextureMipMapGeneration::TextureMipMapGeneration()
{
	zoom     = -2.5f;
	rotation = {0.0f, 15.0f, 0.0f};
	title    = "Texture MipMap generation";
}

TextureMipMapGeneration::~TextureMipMapGeneration()
{
	if (device)
	{
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
		for (auto sampler : samplers)
		{
			vkDestroySampler(get_device().get_handle(), sampler, nullptr);
		}
	}
	destroy_texture(texture);
	uniform_buffer.reset();
}

// Enable physical device features required for this example
void TextureMipMapGeneration::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}
}

/*
	Load the base texture containing only the first mip level and generate the whole mip-chain at runtime
*/
void TextureMipMapGeneration::load_texture_generate_mipmaps(std::string file_name)
{
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

	ktxTexture *   ktx_texture;
	KTX_error_code result;

	result = ktxTexture_CreateFromNamedFile(file_name.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx_texture);
	// @todo: get format from libktx

	if (ktx_texture == nullptr)
	{
		throw std::runtime_error("Couldn't load texture");
	}
	texture.width  = ktx_texture->baseWidth;
	texture.height = ktx_texture->baseHeight;
	// Calculate number of mip levels as per Vulkan specs:
	// numLevels = 1 + floor(log2(max(w, h, d)))
	texture.mip_levels = static_cast<uint32_t>(floor(log2(std::max(texture.width, texture.height))) + 1);

	// Get device properites for the requested texture format
	// Check if the selected format supports blit source and destination, which is required for generating the mip levels
	// If this is not supported you could implement a fallback via compute shader image writes and stores
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(get_device().get_gpu().get_handle(), format, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) || !(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT))
	{
		throw std::runtime_error("Selected image format does not support blit source and destination");
	}

	VkMemoryAllocateInfo memory_allocate_info = vkb::initializers::memory_allocate_info();
	VkMemoryRequirements memory_requirements  = {};

	ktx_uint8_t *ktx_image_data   = ktx_texture->pData;
	ktx_size_t   ktx_texture_size = ktx_texture->dataSize;

	// Create a host-visible staging buffer that contains the raw image data
	VkBuffer       staging_buffer;
	VkDeviceMemory staging_memory;

	VkBufferCreateInfo buffer_create_info = vkb::initializers::buffer_create_info();
	buffer_create_info.size               = ktx_texture_size;
	// This buffer is used as a transfer source for the buffer copy
	buffer_create_info.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK(vkCreateBuffer(get_device().get_handle(), &buffer_create_info, nullptr, &staging_buffer));

	// Get memory requirements for the staging buffer (alignment, memory type bits)
	vkGetBufferMemoryRequirements(get_device().get_handle(), staging_buffer, &memory_requirements);
	memory_allocate_info.allocationSize = memory_requirements.size;
	// Get memory type index for a host visible buffer
	memory_allocate_info.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocate_info, nullptr, &staging_memory));
	VK_CHECK(vkBindBufferMemory(get_device().get_handle(), staging_buffer, staging_memory, 0));

	// Copy ktx image data into host local staging buffer
	uint8_t *data;
	VK_CHECK(vkMapMemory(get_device().get_handle(), staging_memory, 0, memory_requirements.size, 0, (void **) &data));
	memcpy(data, ktx_image_data, ktx_texture_size);
	vkUnmapMemory(get_device().get_handle(), staging_memory);

	// Create optimal tiled target image on the device
	VkImageCreateInfo image_create_info = vkb::initializers::image_create_info();
	image_create_info.imageType         = VK_IMAGE_TYPE_2D;
	image_create_info.format            = format;
	image_create_info.mipLevels         = texture.mip_levels;
	image_create_info.arrayLayers       = 1;
	image_create_info.samples           = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
	image_create_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
	image_create_info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
	image_create_info.extent            = {texture.width, texture.height, 1};
	image_create_info.usage             = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	VK_CHECK(vkCreateImage(get_device().get_handle(), &image_create_info, nullptr, &texture.image));

	vkGetImageMemoryRequirements(get_device().get_handle(), texture.image, &memory_requirements);
	memory_allocate_info.allocationSize  = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocate_info, nullptr, &texture.device_memory));
	VK_CHECK(vkBindImageMemory(get_device().get_handle(), texture.image, texture.device_memory, 0));

	VkCommandBuffer copy_command = device->create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	// Optimal image will be used as destination for the copy, so we must transfer from our initial undefined image layout to the transfer destination layout
	vkb::insert_image_memory_barrier(
	    copy_command,
	    texture.image,
	    0,
	    VK_ACCESS_TRANSFER_WRITE_BIT,
	    VK_IMAGE_LAYOUT_UNDEFINED,
	    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	    VK_PIPELINE_STAGE_TRANSFER_BIT,
	    VK_PIPELINE_STAGE_TRANSFER_BIT,
	    {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

	// Copy the first mip of the chain, remaining mips will be generated
	VkBufferImageCopy buffer_copy_region               = {};
	buffer_copy_region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	buffer_copy_region.imageSubresource.mipLevel       = 0;
	buffer_copy_region.imageSubresource.baseArrayLayer = 0;
	buffer_copy_region.imageSubresource.layerCount     = 1;
	buffer_copy_region.imageExtent.width               = texture.width;
	buffer_copy_region.imageExtent.height              = texture.height;
	buffer_copy_region.imageExtent.depth               = 1;
	vkCmdCopyBufferToImage(copy_command, staging_buffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_copy_region);

	// Transition first mip level to transfer source so we can blit(read) from it
	vkb::insert_image_memory_barrier(
	    copy_command,
	    texture.image,
	    VK_ACCESS_TRANSFER_WRITE_BIT,
	    VK_ACCESS_TRANSFER_READ_BIT,
	    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	    VK_PIPELINE_STAGE_TRANSFER_BIT,
	    VK_PIPELINE_STAGE_TRANSFER_BIT,
	    {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

	device->flush_command_buffer(copy_command, queue, true);

	// Clean up staging resources
	vkFreeMemory(device->get_handle(), staging_memory, nullptr);
	vkDestroyBuffer(device->get_handle(), staging_buffer, nullptr);

	// Generate the mip chain
	// ---------------------------------------------------------------
	// We copy down the whole mip chain doing a blit from mip-1 to mip
	// An alternative way would be to always blit from the first mip level and sample that one down
	VkCommandBuffer blit_command = device->create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	// Copy down mips from n-1 to n
	for (uint32_t i = 1; i < texture.mip_levels; i++)
	{
		VkImageBlit image_blit{};

		// Source
		image_blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_blit.srcSubresource.layerCount = 1;
		image_blit.srcSubresource.mipLevel   = i - 1;
		image_blit.srcOffsets[1].x           = int32_t(texture.width >> (i - 1));
		image_blit.srcOffsets[1].y           = int32_t(texture.height >> (i - 1));
		image_blit.srcOffsets[1].z           = 1;

		// Destination
		image_blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_blit.dstSubresource.layerCount = 1;
		image_blit.dstSubresource.mipLevel   = i;
		image_blit.dstOffsets[1].x           = int32_t(texture.width >> i);
		image_blit.dstOffsets[1].y           = int32_t(texture.height >> i);
		image_blit.dstOffsets[1].z           = 1;

		// Prepare current mip level as image blit destination
		vkb::insert_image_memory_barrier(
		    blit_command,
		    texture.image,
		    0,
		    VK_ACCESS_TRANSFER_WRITE_BIT,
		    VK_IMAGE_LAYOUT_UNDEFINED,
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    VK_PIPELINE_STAGE_TRANSFER_BIT,
		    VK_PIPELINE_STAGE_TRANSFER_BIT,
		    {VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, 1});

		// Blit from previous level
		vkCmdBlitImage(
		    blit_command,
		    texture.image,
		    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		    texture.image,
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    1,
		    &image_blit,
		    VK_FILTER_LINEAR);

		// Prepare current mip level as image blit source for next level
		vkb::insert_image_memory_barrier(
		    blit_command,
		    texture.image,
		    VK_ACCESS_TRANSFER_WRITE_BIT,
		    VK_ACCESS_TRANSFER_READ_BIT,
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		    VK_PIPELINE_STAGE_TRANSFER_BIT,
		    VK_PIPELINE_STAGE_TRANSFER_BIT,
		    {VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, 1});
	}

	// After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
	vkb::insert_image_memory_barrier(
	    blit_command,
	    texture.image,
	    VK_ACCESS_TRANSFER_READ_BIT,
	    VK_ACCESS_SHADER_READ_BIT,
	    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	    VK_PIPELINE_STAGE_TRANSFER_BIT,
	    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	    {VK_IMAGE_ASPECT_COLOR_BIT, 0, texture.mip_levels, 0, 1});

	device->flush_command_buffer(blit_command, queue, true);
	// ---------------------------------------------------------------

	// Create samplers for different mip map demonstration cases
	samplers.resize(3);
	VkSamplerCreateInfo sampler = vkb::initializers::sampler_create_info();
	sampler.magFilter           = VK_FILTER_LINEAR;
	sampler.minFilter           = VK_FILTER_LINEAR;
	sampler.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeV        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeW        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.mipLodBias          = 0.0f;
	sampler.compareOp           = VK_COMPARE_OP_NEVER;
	sampler.minLod              = 0.0f;
	sampler.maxLod              = 0.0f;
	sampler.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	sampler.maxAnisotropy       = 1.0;
	sampler.anisotropyEnable    = VK_FALSE;

	// Without mip mapping
	VK_CHECK(vkCreateSampler(device->get_handle(), &sampler, nullptr, &samplers[0]));

	// With mip mapping
	sampler.maxLod = static_cast<float>(texture.mip_levels);
	VK_CHECK(vkCreateSampler(device->get_handle(), &sampler, nullptr, &samplers[1]));

	// With mip mapping and anisotropic filtering (when supported)
	if (get_device().get_gpu().get_features().samplerAnisotropy)
	{
		sampler.maxAnisotropy    = get_device().get_gpu().get_properties().limits.maxSamplerAnisotropy;
		sampler.anisotropyEnable = VK_TRUE;
	}
	VK_CHECK(vkCreateSampler(device->get_handle(), &sampler, nullptr, &samplers[2]));

	// Create image view
	VkImageViewCreateInfo view           = vkb::initializers::image_view_create_info();
	view.image                           = texture.image;
	view.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	view.format                          = format;
	view.components                      = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
	view.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	view.subresourceRange.baseMipLevel   = 0;
	view.subresourceRange.baseArrayLayer = 0;
	view.subresourceRange.layerCount     = 1;
	view.subresourceRange.levelCount     = texture.mip_levels;
	VK_CHECK(vkCreateImageView(device->get_handle(), &view, nullptr, &texture.view));
}

// Free all Vulkan resources used by a texture object
void TextureMipMapGeneration::destroy_texture(Texture texture)
{
	vkDestroyImageView(get_device().get_handle(), texture.view, nullptr);
	vkDestroyImage(get_device().get_handle(), texture.image, nullptr);
	vkFreeMemory(get_device().get_handle(), texture.device_memory, nullptr);
}

void TextureMipMapGeneration::load_assets()
{
	load_texture_generate_mipmaps(vkb::fs::path::get(vkb::fs::path::Assets, "textures/checkerboard_rgba.ktx"));
	scene = load_model("scenes/tunnel_cylinder.gltf");
}

void TextureMipMapGeneration::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = default_clear_color;
	clear_values[1].depthStencil = {1.0f, 0};

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
		render_pass_begin_info.framebuffer = framebuffers[i];

		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport((float) width, (float) height, 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, NULL);
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		draw_model(scene, draw_cmd_buffers[i]);

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void TextureMipMapGeneration::draw()
{
	ApiVulkanSample::prepare_frame();

	// Command buffer to be sumitted to the queue
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();
}

void TextureMipMapGeneration::setup_descriptor_pool()
{
	// Example uses one ubo and one image sampler
	std::vector<VkDescriptorPoolSize> pool_sizes =
	    {
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1),
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_SAMPLER, 3),
	    };

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(
	        static_cast<uint32_t>(pool_sizes.size()),
	        pool_sizes.data(),
	        2);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void TextureMipMapGeneration::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings =
	    {
	        // Binding 0 : Parameter uniform buffer
	        vkb::initializers::descriptor_set_layout_binding(
	            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	            0),
	        // Binding 1 : Fragment shader image sampler
	        vkb::initializers::descriptor_set_layout_binding(
	            VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
	            VK_SHADER_STAGE_FRAGMENT_BIT,
	            1),
	        // Binding 2 : Sampler array (3 descriptors)
	        vkb::initializers::descriptor_set_layout_binding(
	            VK_DESCRIPTOR_TYPE_SAMPLER,
	            VK_SHADER_STAGE_FRAGMENT_BIT,
	            2,
	            3),
	    };

	VkDescriptorSetLayoutCreateInfo descriptor_layout =
	    vkb::initializers::descriptor_set_layout_create_info(
	        set_layout_bindings.data(),
	        static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &descriptor_set_layout,
	        1);

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void TextureMipMapGeneration::setup_descriptor_set()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layout,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

	VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*uniform_buffer);

	VkDescriptorImageInfo image_descriptor;
	image_descriptor.imageView   = texture.view;
	image_descriptor.sampler     = VK_NULL_HANDLE;
	image_descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	std::vector<VkWriteDescriptorSet> write_descriptor_sets =
	    {
	        // Binding 0 : Vertex shader uniform buffer
	        vkb::initializers::write_descriptor_set(
	            descriptor_set,
	            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	            0,
	            &buffer_descriptor),
	        // Binding 1 : Fragment shader texture sampler
	        vkb::initializers::write_descriptor_set(
	            descriptor_set,
	            VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
	            1,
	            &image_descriptor)};

	// Binding 2: Sampler array
	std::vector<VkDescriptorImageInfo> sampler_descriptors;
	for (auto i = 0; i < samplers.size(); i++)
	{
		sampler_descriptors.push_back({samplers[i], VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
	}
	VkWriteDescriptorSet write_descriptor_set{};
	write_descriptor_set.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set.dstSet          = descriptor_set;
	write_descriptor_set.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
	write_descriptor_set.descriptorCount = static_cast<uint32_t>(sampler_descriptors.size());
	write_descriptor_set.pImageInfo      = sampler_descriptors.data();
	write_descriptor_set.dstBinding      = 2;
	write_descriptor_set.dstArrayElement = 0;
	write_descriptor_sets.push_back(write_descriptor_set);
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

void TextureMipMapGeneration::prepare_pipelines()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(
	        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	        0,
	        VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(
	        VK_POLYGON_MODE_FILL,
	        VK_CULL_MODE_NONE,
	        VK_FRONT_FACE_COUNTER_CLOCKWISE,
	        0);

	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(
	        0xf,
	        VK_FALSE);

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(
	        1,
	        &blend_attachment_state);

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(
	        VK_TRUE,
	        VK_TRUE,
	        VK_COMPARE_OP_LESS_OR_EQUAL);

	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(
	        VK_SAMPLE_COUNT_1_BIT,
	        0);

	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0);

	// Load shaders
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;

	shader_stages[0] = load_shader("texture_mipmap_generation/texture.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("texture_mipmap_generation/texture.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Vertex bindings and attributes
	const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),                        // Location 0: Position
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 6),           // Location 1: UV
	    vkb::initializers::vertex_input_attribute_description(0, 2, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 8),        // Location 2: Color
	};
	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(pipeline_layout, render_pass, 0);
	pipeline_create_info.pVertexInputState            = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState          = &input_assembly_state;
	pipeline_create_info.pRasterizationState          = &rasterization_state;
	pipeline_create_info.pColorBlendState             = &color_blend_state;
	pipeline_create_info.pMultisampleState            = &multisample_state;
	pipeline_create_info.pViewportState               = &viewport_state;
	pipeline_create_info.pDepthStencilState           = &depth_stencil_state;
	pipeline_create_info.pDynamicState                = &dynamic_state;
	pipeline_create_info.stageCount                   = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages                      = shader_stages.data();

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline));
}

void TextureMipMapGeneration::prepare_uniform_buffers()
{
	// Shared parameter uniform buffer block
	uniform_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                     sizeof(ubo),
	                                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                     VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void TextureMipMapGeneration::update_uniform_buffers(float delta_time)
{
	ubo.projection = camera.matrices.perspective;
	ubo.model      = camera.matrices.view;
	ubo.model      = glm::rotate(ubo.model, glm::radians(90.0f + timer * 360.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.model      = glm::scale(ubo.model, glm::vec3(0.5f));
	timer += delta_time * 0.005f;
	if (timer > 1.0f)
	{
		timer -= 1.0f;
	}
	uniform_buffer->convert_and_update(ubo);
}

bool TextureMipMapGeneration::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	camera.type = vkb::CameraType::FirstPerson;
	camera.set_perspective(60.0f, (float) width / (float) height, 0.1f, 1024.0f);
	camera.set_translation(glm::vec3(0.0f, 0.0f, -12.5f));

	load_assets();
	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_set();
	build_command_buffers();

	prepared = true;
	return true;
}

void TextureMipMapGeneration::render(float delta_time)
{
	if (!prepared)
		return;
	draw();
	if (rotate_scene)
		update_uniform_buffers(delta_time);
}

void TextureMipMapGeneration::view_changed()
{
	update_uniform_buffers();
}

void TextureMipMapGeneration::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		drawer.checkbox("Rotate", &rotate_scene);
		if (drawer.slider_float("LOD bias", &ubo.lod_bias, 0.0f, (float) texture.mip_levels))
		{
			update_uniform_buffers();
		}
		if (drawer.combo_box("Sampler type", &ubo.sampler_index, sampler_names))
		{
			update_uniform_buffers();
		}
	}
}

std::unique_ptr<vkb::Application> create_texture_mipmap_generation()
{
	return std::make_unique<TextureMipMapGeneration>();
}
