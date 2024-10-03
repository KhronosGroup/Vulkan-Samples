/* Copyright (c) 2021-2024, Sascha Willems
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
 * Loading a Basis Universal supercompressed texture and transcoding it to a supported GPU texture format
 */

#include "texture_compression_basisu.h"

TextureCompressionBasisu::TextureCompressionBasisu()
{
	zoom     = -1.75f;
	rotation = {0.0f, 0.0f, 0.0f};
	title    = "Basis Universal texture loading";
}

TextureCompressionBasisu::~TextureCompressionBasisu()
{
	if (has_device())
	{
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
		destroy_texture(texture);
		vertex_buffer.reset();
		index_buffer.reset();
		uniform_buffer_vs.reset();
	}
}

void TextureCompressionBasisu::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}
}

// Check if the device supports sampling and transfers for the selected image
bool TextureCompressionBasisu::format_supported(VkFormat format)
{
	VkFormatProperties format_properties;
	vkGetPhysicalDeviceFormatProperties(get_device().get_gpu().get_handle(), format, &format_properties);
	return ((format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_DST_BIT) && (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT));
}

// Get a list of possible transcoding target formats supported by the selected gpu
// Note that this is a simple mechanism for demonstration purposes
// A real world application would probably need a more sophisticated way to determine the target formats based on texture usage
void TextureCompressionBasisu::get_available_target_formats()
{
	available_target_formats.clear();

	VkPhysicalDeviceFeatures device_features = get_device().get_gpu().get_features();

	// Block compression
	if (device_features.textureCompressionBC)
	{
		// BC7 is the preferred block compression if available
		if (format_supported(VK_FORMAT_BC7_SRGB_BLOCK))
		{
			available_target_formats.push_back(KTX_TTF_BC7_RGBA);
			available_target_formats_names.push_back("KTX_TTF_BC7_RGBA");
		}

		if (format_supported(VK_FORMAT_BC3_SRGB_BLOCK))
		{
			available_target_formats.push_back(KTX_TTF_BC3_RGBA);
			available_target_formats_names.push_back("KTX_TTF_BC3_RGBA");
		}
	}

	// Adaptive scalable texture compression
	if (device_features.textureCompressionASTC_LDR)
	{
		if (format_supported(VK_FORMAT_ASTC_4x4_SRGB_BLOCK))
		{
			available_target_formats.push_back(KTX_TTF_ASTC_4x4_RGBA);
			available_target_formats_names.push_back("KTX_TTF_ASTC_4x4_RGBA");
		}
	}

	// Ericsson texture compression
	if (device_features.textureCompressionETC2)
	{
		if (format_supported(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK))
		{
			available_target_formats.push_back(KTX_TTF_ETC2_RGBA);
			available_target_formats_names.push_back("KTX_TTF_ETC2_RGBA");
		}
	}

	// PowerVR texture compression support needs to be checked via an extension
	if (get_device().is_extension_supported(VK_IMG_FORMAT_PVRTC_EXTENSION_NAME))
	{
		if (format_supported(VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG))
		{
			available_target_formats.push_back(KTX_TTF_PVRTC1_4_RGBA);
			available_target_formats_names.push_back("KTX_TTF_PVRTC1_4_RGBA");
		}
	}

	// Always add uncompressed RGBA as a valid target
	available_target_formats.push_back(KTX_TTF_RGBA32);
	available_target_formats_names.push_back("KTX_TTF_RGBA32");
}

// Loads and transcodes the input KTX texture file to the desired native GPU target format
void TextureCompressionBasisu::transcode_texture(const std::string &input_file, ktx_transcode_fmt_e target_format)
{
	// Clean up resources for an already created image
	if (texture.image != VK_NULL_HANDLE)
	{
		destroy_texture(texture);
	}

	std::string file_name = vkb::fs::path::get(vkb::fs::path::Assets, "textures/basisu/" + input_file);

	// We are working with KTX2.0 files, so we need to use the ktxTexture2 class
	ktxTexture2 *ktx_texture;
	// Load the KTX2.0 file into memory. This is agnostic to the KTX version, so we cast the ktxTexture2 down to ktxTexture
	KTX_error_code result = ktxTexture_CreateFromNamedFile(file_name.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, reinterpret_cast<ktxTexture **>(&ktx_texture));
	if (result != KTX_SUCCESS)
	{
		throw std::runtime_error("Could not load the requested image file.");
	}

	// Check if the texture needs transcoding. This is the case, if the format stored in the KTX file is a non-native compression format
	// This is the case for all textures used in this sample, as they are compressed using Basis Universal, which has to be transcoded to a native GPU format
	if (ktxTexture2_NeedsTranscoding(ktx_texture))
	{
		auto tStart         = std::chrono::high_resolution_clock::now();
		result              = ktxTexture2_TranscodeBasis(ktx_texture, target_format, 0);
		last_transcode_time = std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock::now() - tStart).count();
		if (result != KTX_SUCCESS)
		{
			throw std::runtime_error("Could not transcode the input texture to the selected target format.");
		}
	}

	texture.width      = ktx_texture->baseWidth;
	texture.height     = ktx_texture->baseHeight;
	texture.mip_levels = ktx_texture->numLevels;

	// Once transcoded, we can read the native Vulkan format from the ktx texture object and upload the transcoded GPU native data via staging
	VkFormat format = static_cast<VkFormat>(ktx_texture->vkFormat);

	VkBuffer       staging_buffer;
	VkDeviceMemory staging_memory;

	VkBufferCreateInfo buffer_create_info = vkb::initializers::buffer_create_info();
	buffer_create_info.size               = ktx_texture->dataSize;
	// This buffer is used as a transfer source for the buffer copy
	buffer_create_info.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK(vkCreateBuffer(get_device().get_handle(), &buffer_create_info, nullptr, &staging_buffer));

	VkMemoryAllocateInfo memory_allocate_info = vkb::initializers::memory_allocate_info();
	VkMemoryRequirements memory_requirements  = {};
	vkGetBufferMemoryRequirements(get_device().get_handle(), staging_buffer, &memory_requirements);
	memory_allocate_info.allocationSize  = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocate_info, nullptr, &staging_memory));
	VK_CHECK(vkBindBufferMemory(get_device().get_handle(), staging_buffer, staging_memory, 0));

	// Copy texture data into host local staging buffer
	uint8_t *data;
	VK_CHECK(vkMapMemory(get_device().get_handle(), staging_memory, 0, memory_requirements.size, 0, (void **) &data));
	memcpy(data, ktx_texture->pData, ktx_texture->dataSize);
	vkUnmapMemory(get_device().get_handle(), staging_memory);
	// Setup buffer copy regions for each mip level
	std::vector<VkBufferImageCopy> buffer_copy_regions;
	for (uint32_t mip_level = 0; mip_level < texture.mip_levels; mip_level++)
	{
		ktx_size_t        offset;
		KTX_error_code    result                           = ktxTexture_GetImageOffset((ktxTexture *) ktx_texture, mip_level, 0, 0, &offset);
		VkBufferImageCopy buffer_copy_region               = {};
		buffer_copy_region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		buffer_copy_region.imageSubresource.mipLevel       = mip_level;
		buffer_copy_region.imageSubresource.baseArrayLayer = 0;
		buffer_copy_region.imageSubresource.layerCount     = 1;
		buffer_copy_region.imageExtent.width               = ktx_texture->baseWidth >> mip_level;
		buffer_copy_region.imageExtent.height              = ktx_texture->baseHeight >> mip_level;
		buffer_copy_region.imageExtent.depth               = 1;
		buffer_copy_region.bufferOffset                    = offset;
		buffer_copy_regions.push_back(buffer_copy_region);
	}

	// Create optimal tiled target image on the device
	VkImageCreateInfo image_create_info = vkb::initializers::image_create_info();
	image_create_info.imageType         = VK_IMAGE_TYPE_2D;
	image_create_info.format            = format;
	image_create_info.mipLevels         = texture.mip_levels;
	image_create_info.arrayLayers       = 1;
	image_create_info.samples           = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
	image_create_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
	// Set initial layout of the image to undefined
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_create_info.extent        = {texture.width, texture.height, 1};
	image_create_info.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	VK_CHECK(vkCreateImage(get_device().get_handle(), &image_create_info, nullptr, &texture.image));

	vkGetImageMemoryRequirements(get_device().get_handle(), texture.image, &memory_requirements);
	memory_allocate_info.allocationSize  = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocate_info, nullptr, &texture.device_memory));
	VK_CHECK(vkBindImageMemory(get_device().get_handle(), texture.image, texture.device_memory, 0));

	VkCommandBuffer copy_command = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	// Image memory barriers for the texture image

	// The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
	VkImageSubresourceRange subresource_range = {};
	subresource_range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource_range.baseMipLevel            = 0;
	subresource_range.levelCount              = texture.mip_levels;
	subresource_range.layerCount              = 1;

	// Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
	// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
	vkb::image_layout_transition(copy_command, texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);

	// Copy mip levels from staging buffer
	vkCmdCopyBufferToImage(
	    copy_command,
	    staging_buffer,
	    texture.image,
	    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	    static_cast<uint32_t>(buffer_copy_regions.size()),
	    buffer_copy_regions.data());

	// Once the data has been uploaded we transfer to the texture image to the shader read layout, so it can be sampled from
	// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
	vkb::image_layout_transition(
	    copy_command, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresource_range);

	// Store current layout for later reuse
	texture.image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	get_device().flush_command_buffer(copy_command, queue, true);

	// Clean up staging resources
	vkFreeMemory(get_device().get_handle(), staging_memory, nullptr);
	vkDestroyBuffer(get_device().get_handle(), staging_buffer, nullptr);

	// Calculate valid filter and mipmap modes
	VkFilter            filter      = VK_FILTER_LINEAR;
	VkSamplerMipmapMode mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	vkb::make_filters_valid(get_device().get_gpu().get_handle(), format, &filter, &mipmap_mode);

	// Create a texture sampler
	VkSamplerCreateInfo sampler = vkb::initializers::sampler_create_info();
	sampler.magFilter           = filter;
	sampler.minFilter           = filter;
	sampler.mipmapMode          = mipmap_mode;
	sampler.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeW        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.mipLodBias          = 0.0f;
	sampler.compareOp           = VK_COMPARE_OP_NEVER;
	sampler.minLod              = 0.0f;
	sampler.maxLod              = static_cast<float>(texture.mip_levels);
	if (get_device().get_gpu().get_features().samplerAnisotropy)
	{
		// Use max. level of anisotropy for this example
		sampler.maxAnisotropy    = get_device().get_gpu().get_properties().limits.maxSamplerAnisotropy;
		sampler.anisotropyEnable = VK_TRUE;
	}
	else
	{
		// The device does not support anisotropic filtering
		sampler.maxAnisotropy    = 1.0;
		sampler.anisotropyEnable = VK_FALSE;
	}
	VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler, nullptr, &texture.sampler));

	// Create image view
	VkImageViewCreateInfo view           = vkb::initializers::image_view_create_info();
	view.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	view.format                          = format;
	view.components                      = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
	view.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	view.subresourceRange.baseMipLevel   = 0;
	view.subresourceRange.baseArrayLayer = 0;
	view.subresourceRange.layerCount     = 1;
	view.subresourceRange.levelCount     = texture.mip_levels;
	view.image                           = texture.image;
	VK_CHECK(vkCreateImageView(get_device().get_handle(), &view, nullptr, &texture.view));
}

// Free all Vulkan resources used by a texture object
void TextureCompressionBasisu::destroy_texture(Texture texture)
{
	vkDestroyImageView(get_device().get_handle(), texture.view, nullptr);
	vkDestroyImage(get_device().get_handle(), texture.image, nullptr);
	vkDestroySampler(get_device().get_handle(), texture.sampler, nullptr);
	vkFreeMemory(get_device().get_handle(), texture.device_memory, nullptr);
	texture.image = VK_NULL_HANDLE;
}

void TextureCompressionBasisu::update_image_descriptor()
{
	VkDescriptorImageInfo image_descriptor     = {texture.sampler, texture.view, texture.image_layout};
	VkWriteDescriptorSet  write_descriptor_set = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &image_descriptor);
	vkUpdateDescriptorSets(get_device().get_handle(), 1, &write_descriptor_set, 0, nullptr);
}

void TextureCompressionBasisu::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.05f, 0.05f, 0.05f, 1.0f}};
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
		render_pass_begin_info.framebuffer = framebuffers[i];
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));
		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);
		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, NULL);
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		VkDeviceSize offsets[1] = {0};
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, vertex_buffer->get(), offsets);
		vkCmdBindIndexBuffer(draw_cmd_buffers[i], index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(draw_cmd_buffers[i], index_count, 1, 0, 0, 0);
		draw_ui(draw_cmd_buffers[i]);
		vkCmdEndRenderPass(draw_cmd_buffers[i]);
		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void TextureCompressionBasisu::draw()
{
	ApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();
}

void TextureCompressionBasisu::generate_quad()
{
	// Setup vertices for a single uv-mapped quad made from two triangles
	std::vector<VertexStructure> vertices =
	    {
	        {{1.5f, 1.0f, 0.0f}, {1.0f, 1.0f}},
	        {{-1.5f, 1.0f, 0.0f}, {0.0f, 1.0f}},
	        {{-1.5f, -1.0f, 0.0f}, {0.0f, 0.0f}},
	        {{1.5f, -1.0f, 0.0f}, {1.0f, 0.0f}}};

	// Setup indices
	std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};
	index_count                   = static_cast<uint32_t>(indices.size());

	auto vertex_buffer_size = vkb::to_u32(vertices.size() * sizeof(VertexStructure));
	auto index_buffer_size  = vkb::to_u32(indices.size() * sizeof(uint32_t));

	// Create buffers
	// For the sake of simplicity we won't stage the vertex data to the gpu memory
	// Vertex buffer
	vertex_buffer = std::make_unique<vkb::core::BufferC>(get_device(),
	                                                     vertex_buffer_size,
	                                                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                                                     VMA_MEMORY_USAGE_CPU_TO_GPU);
	vertex_buffer->update(vertices.data(), vertex_buffer_size);

	index_buffer = std::make_unique<vkb::core::BufferC>(get_device(),
	                                                    index_buffer_size,
	                                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	                                                    VMA_MEMORY_USAGE_CPU_TO_GPU);

	index_buffer->update(indices.data(), index_buffer_size);
}

void TextureCompressionBasisu::setup_descriptor_pool()
{
	// Example uses one ubo and one image sampler
	std::vector<VkDescriptorPoolSize> pool_sizes =
	    {
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(
	        static_cast<uint32_t>(pool_sizes.size()),
	        pool_sizes.data(),
	        2);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void TextureCompressionBasisu::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings =
	    {
	        // Binding 0 : Vertex shader uniform buffer
	        vkb::initializers::descriptor_set_layout_binding(
	            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	            VK_SHADER_STAGE_VERTEX_BIT,
	            0),
	        // Binding 1 : Fragment shader image sampler
	        vkb::initializers::descriptor_set_layout_binding(
	            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	            VK_SHADER_STAGE_FRAGMENT_BIT,
	            1)};

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

void TextureCompressionBasisu::setup_descriptor_set()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layout,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

	VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*uniform_buffer_vs);

	// Setup a descriptor image info for the current texture to be used as a combined image sampler
	VkDescriptorImageInfo image_descriptor = {texture.sampler, texture.view, texture.image_layout};

	std::vector<VkWriteDescriptorSet> write_descriptor_sets =
	    {
	        // Binding 0 : Vertex shader uniform buffer
	        vkb::initializers::write_descriptor_set(
	            descriptor_set,
	            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	            0,
	            &buffer_descriptor),
	        // Binding 1 : Fragment shader texture sampler
	        //	Fragment shader: layout (binding = 1) uniform sampler2D samplerColor;
	        vkb::initializers::write_descriptor_set(
	            descriptor_set,
	            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,        // The descriptor set will use a combined image sampler (sampler and image could be split)
	            1,                                                // Shader binding point 1
	            &image_descriptor)                                // Pointer to the descriptor image for our texture
	    };

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);
}

void TextureCompressionBasisu::prepare_pipelines()
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

	// Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(
	        VK_TRUE,
	        VK_TRUE,
	        VK_COMPARE_OP_GREATER);

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

	shader_stages[0] = load_shader("texture_compression_basisu", "texture.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("texture_compression_basisu", "texture.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Vertex bindings and attributes
	const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(VertexStructure), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexStructure, pos)),
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexStructure, uv)),
	};
	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	VkGraphicsPipelineCreateInfo pipeline_create_info =
	    vkb::initializers::pipeline_create_info(
	        pipeline_layout,
	        render_pass,
	        0);

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
void TextureCompressionBasisu::prepare_uniform_buffers()
{
	// Vertex shader uniform buffer block
	uniform_buffer_vs = std::make_unique<vkb::core::BufferC>(get_device(),
	                                                         sizeof(ubo_vs),
	                                                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                         VMA_MEMORY_USAGE_CPU_TO_GPU);
	update_uniform_buffers();
}

void TextureCompressionBasisu::update_uniform_buffers()
{
	// Vertex shader
	ubo_vs.projection     = glm::perspective(glm::radians(60.0f), static_cast<float>(width) / static_cast<float>(height), 0.001f, 256.0f);
	glm::mat4 view_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zoom));

	ubo_vs.model = view_matrix * glm::translate(glm::mat4(1.0f), camera_pos);
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	uniform_buffer_vs->convert_and_update(ubo_vs);
}

bool TextureCompressionBasisu::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}
	get_available_target_formats();
	texture_file_names = {"kodim23_UASTC.ktx2",
	                      "kodim23_ETC1S.ktx2",
	                      "kodim20_UASTC.ktx2",
	                      "kodim20_ETC1S.ktx2",
	                      "kodim05_UASTC.ktx2",
	                      "kodim05_ETC1S.ktx2",
	                      "kodim03_UASTC.ktx2",
	                      "kodim03_ETC1S.ktx2"};
	transcode_texture(texture_file_names[selected_input_texture], available_target_formats[selected_transcode_target_format]);
	generate_quad();
	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_set();
	build_command_buffers();
	prepared = true;
	return true;
}

void TextureCompressionBasisu::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
}

void TextureCompressionBasisu::view_changed()
{
	update_uniform_buffers();
}

void TextureCompressionBasisu::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Input"))
	{
		drawer.text("Input image:");
		ImGui::PushItemWidth(180);
		drawer.combo_box("##img", &selected_input_texture, texture_file_names);
		ImGui::PopItemWidth();
		drawer.text("Transcode target:");
		ImGui::PushItemWidth(180);
		drawer.combo_box("##tt", &selected_transcode_target_format, available_target_formats_names);
		ImGui::PopItemWidth();
		if (drawer.button("Transcode"))
		{
			vkQueueWaitIdle(queue);
			transcode_texture(texture_file_names[selected_input_texture], available_target_formats[selected_transcode_target_format]);
			update_image_descriptor();
		}
		drawer.text("Transcoded in %.2f ms", last_transcode_time);
	}
}

std::unique_ptr<vkb::Application> create_texture_compression_basisu()
{
	return std::make_unique<TextureCompressionBasisu>();
}
