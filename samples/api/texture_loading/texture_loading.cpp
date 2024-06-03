/* Copyright (c) 2019-2024, Sascha Willems
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
 * Texture loading (and display) example (including mip maps)
 */

#include "texture_loading.h"

TextureLoading::TextureLoading()
{
	zoom     = -2.5f;
	rotation = {0.0f, 15.0f, 0.0f};
	title    = "Texture loading";
}

TextureLoading::~TextureLoading()
{
	if (has_device())
	{
		// Clean up used Vulkan resources
		// Note : Inherited destructor cleans up resources stored in base class

		vkDestroyPipeline(get_device().get_handle(), pipelines.solid, nullptr);

		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
	}

	destroy_texture(texture);

	vertex_buffer.reset();
	index_buffer.reset();
	uniform_buffer_vs.reset();
}

// Enable physical device features required for this example
void TextureLoading::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}
}

/*
    Upload texture image data to the GPU

    Vulkan offers two types of image tiling (memory layout):

    Linear tiled images:
        These are stored as is and can be copied directly to. But due to the linear nature they're not a good match for GPUs and format and feature support is very limited.
        It's not advised to use linear tiled images for anything else than copying from host to GPU if buffer copies are not an option.
        Linear tiling is thus only implemented for learning purposes, one should always prefer optimal tiled image.

    Optimal tiled images:
        These are stored in an implementation specific layout matching the capability of the hardware. They usually support more formats and features and are much faster.
        Optimal tiled images are stored on the device and not accessible by the host. So they can't be written directly to (like liner tiled images) and always require
        some sort of data copy, either from a buffer or	a linear tiled image.

    In Short: Always use optimal tiled images for rendering.
*/
void TextureLoading::load_texture()
{
	// We use the Khronos texture format (https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/)
	std::string filename = vkb::fs::path::get(vkb::fs::path::Assets, "textures/metalplate01_rgba.ktx");
	// ktx1 doesn't know whether the content is sRGB or linear, but most tools save in sRGB, so assume that.
	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

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

	// We prefer using staging to copy the texture data to a device local optimal image
	VkBool32 use_staging = true;

	// Only use linear tiling if forced
	bool force_linear_tiling = false;
	if (force_linear_tiling)
	{
		// Don't use linear if format is not supported for (linear) shader sampling
		// Get device properties for the requested texture format
		VkFormatProperties format_properties;
		vkGetPhysicalDeviceFormatProperties(get_device().get_gpu().get_handle(), format, &format_properties);
		use_staging = !(format_properties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
	}

	VkMemoryAllocateInfo memory_allocate_info = vkb::initializers::memory_allocate_info();
	VkMemoryRequirements memory_requirements  = {};

	ktx_uint8_t *ktx_image_data   = ktx_texture->pData;
	ktx_size_t   ktx_texture_size = ktx_texture->dataSize;

	if (use_staging)
	{
		// Copy data to an optimal tiled image
		// This loads the texture data into a host local buffer that is copied to the optimal tiled image on the device

		// Create a host-visible staging buffer that contains the raw image data
		// This buffer will be the data source for copying texture data to the optimal tiled image on the device
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

		// Copy texture data into host local staging buffer

		uint8_t *data;
		VK_CHECK(vkMapMemory(get_device().get_handle(), staging_memory, 0, memory_requirements.size, 0, (void **) &data));
		memcpy(data, ktx_image_data, ktx_texture_size);
		vkUnmapMemory(get_device().get_handle(), staging_memory);

		// Setup buffer copy regions for each mip level
		std::vector<VkBufferImageCopy> buffer_copy_regions;
		for (uint32_t i = 0; i < texture.mip_levels; i++)
		{
			ktx_size_t        offset;
			KTX_error_code    result                           = ktxTexture_GetImageOffset(ktx_texture, i, 0, 0, &offset);
			VkBufferImageCopy buffer_copy_region               = {};
			buffer_copy_region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
			buffer_copy_region.imageSubresource.mipLevel       = i;
			buffer_copy_region.imageSubresource.baseArrayLayer = 0;
			buffer_copy_region.imageSubresource.layerCount     = 1;
			buffer_copy_region.imageExtent.width               = ktx_texture->baseWidth >> i;
			buffer_copy_region.imageExtent.height              = ktx_texture->baseHeight >> i;
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
		// Image only contains color data
		subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		// Start at first mip level
		subresource_range.baseMipLevel = 0;
		// We will transition on all mip levels
		subresource_range.levelCount = texture.mip_levels;
		// The 2D texture only has one layer
		subresource_range.layerCount = 1;

		// Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
		VkImageMemoryBarrier image_memory_barrier = vkb::initializers::image_memory_barrier();

		image_memory_barrier.image            = texture.image;
		image_memory_barrier.subresourceRange = subresource_range;
		image_memory_barrier.srcAccessMask    = 0;
		image_memory_barrier.dstAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT;
		image_memory_barrier.oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
		image_memory_barrier.newLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
		// Source pipeline stage is host write/read execution (VK_PIPELINE_STAGE_HOST_BIT)
		// Destination pipeline stage is copy command execution (VK_PIPELINE_STAGE_TRANSFER_BIT)
		vkCmdPipelineBarrier(
		    copy_command,
		    VK_PIPELINE_STAGE_HOST_BIT,
		    VK_PIPELINE_STAGE_TRANSFER_BIT,
		    0,
		    0, nullptr,
		    0, nullptr,
		    1, &image_memory_barrier);

		// Copy mip levels from staging buffer
		vkCmdCopyBufferToImage(
		    copy_command,
		    staging_buffer,
		    texture.image,
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    static_cast<uint32_t>(buffer_copy_regions.size()),
		    buffer_copy_regions.data());

		// Once the data has been uploaded we transfer to the texture image to the shader read layout, so it can be sampled from
		image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		image_memory_barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		image_memory_barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
		// Source pipeline stage stage is copy command execution (VK_PIPELINE_STAGE_TRANSFER_BIT)
		// Destination pipeline stage fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
		vkCmdPipelineBarrier(
		    copy_command,
		    VK_PIPELINE_STAGE_TRANSFER_BIT,
		    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		    0,
		    0, nullptr,
		    0, nullptr,
		    1, &image_memory_barrier);

		// Store current layout for later reuse
		texture.image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		get_device().flush_command_buffer(copy_command, queue, true);

		// Clean up staging resources
		vkDestroyBuffer(get_device().get_handle(), staging_buffer, nullptr);
		vkFreeMemory(get_device().get_handle(), staging_memory, nullptr);
	}
	else
	{
		// Copy data to a linear tiled image

		VkImage        mappable_image;
		VkDeviceMemory mappable_memory;

		// Load mip map level 0 to linear tiling image
		VkImageCreateInfo image_create_info = vkb::initializers::image_create_info();
		image_create_info.imageType         = VK_IMAGE_TYPE_2D;
		image_create_info.format            = format;
		image_create_info.mipLevels         = 1;
		image_create_info.arrayLayers       = 1;
		image_create_info.samples           = VK_SAMPLE_COUNT_1_BIT;
		image_create_info.tiling            = VK_IMAGE_TILING_LINEAR;
		image_create_info.usage             = VK_IMAGE_USAGE_SAMPLED_BIT;
		image_create_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
		image_create_info.initialLayout     = VK_IMAGE_LAYOUT_PREINITIALIZED;
		image_create_info.extent            = {texture.width, texture.height, 1};
		VK_CHECK(vkCreateImage(get_device().get_handle(), &image_create_info, nullptr, &mappable_image));

		// Get memory requirements for this image like size and alignment
		vkGetImageMemoryRequirements(get_device().get_handle(), mappable_image, &memory_requirements);
		// Set memory allocation size to required memory size
		memory_allocate_info.allocationSize = memory_requirements.size;
		// Get memory type that can be mapped to host memory
		memory_allocate_info.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocate_info, nullptr, &mappable_memory));
		VK_CHECK(vkBindImageMemory(get_device().get_handle(), mappable_image, mappable_memory, 0));

		// Map image memory
		void      *data;
		ktx_size_t ktx_image_size = ktxTexture_GetImageSize(ktx_texture, 0);
		VK_CHECK(vkMapMemory(get_device().get_handle(), mappable_memory, 0, memory_requirements.size, 0, &data));
		// Copy image data of the first mip level into memory
		memcpy(data, ktx_image_data, ktx_image_size);
		vkUnmapMemory(get_device().get_handle(), mappable_memory);

		// Linear tiled images don't need to be staged and can be directly used as textures
		texture.image         = mappable_image;
		texture.device_memory = mappable_memory;
		texture.image_layout  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// Setup image memory barrier transfer image to shader read layout
		VkCommandBuffer copy_command = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		// The sub resource range describes the regions of the image we will be transition
		VkImageSubresourceRange subresource_range = {};
		subresource_range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
		subresource_range.baseMipLevel            = 0;
		subresource_range.levelCount              = 1;
		subresource_range.layerCount              = 1;

		// Transition the texture image layout to shader read, so it can be sampled from
		VkImageMemoryBarrier image_memory_barrier = vkb::initializers::image_memory_barrier();
		;
		image_memory_barrier.image            = texture.image;
		image_memory_barrier.subresourceRange = subresource_range;
		image_memory_barrier.srcAccessMask    = VK_ACCESS_HOST_WRITE_BIT;
		image_memory_barrier.dstAccessMask    = VK_ACCESS_SHADER_READ_BIT;
		image_memory_barrier.oldLayout        = VK_IMAGE_LAYOUT_PREINITIALIZED;
		image_memory_barrier.newLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
		// Source pipeline stage is host write/read execution (VK_PIPELINE_STAGE_HOST_BIT)
		// Destination pipeline stage fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
		vkCmdPipelineBarrier(
		    copy_command,
		    VK_PIPELINE_STAGE_HOST_BIT,
		    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		    0,
		    0, nullptr,
		    0, nullptr,
		    1, &image_memory_barrier);

		get_device().flush_command_buffer(copy_command, queue, true);
	}

	// now, the ktx_texture can be destroyed
	ktxTexture_Destroy(ktx_texture);

	// Calculate valid filter and mipmap modes
	VkFilter            filter      = VK_FILTER_LINEAR;
	VkSamplerMipmapMode mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	vkb::make_filters_valid(get_device().get_gpu().get_handle(), format, &filter, &mipmap_mode);

	// Create a texture sampler
	// In Vulkan textures are accessed by samplers
	// This separates all the sampling information from the texture data. This means you could have multiple sampler objects for the same texture with different settings
	// Note: Similar to the samplers available with OpenGL 3.3
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
	// Set max level-of-detail to mip level count of the texture
	sampler.maxLod = (use_staging) ? static_cast<float>(texture.mip_levels) : 0.0f;
	// Enable anisotropic filtering
	// This feature is optional, so we must check if it's supported on the device
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
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler, nullptr, &texture.sampler));

	// Create image view
	// Textures are not directly accessed by the shaders and
	// are abstracted by image views containing additional
	// information and sub resource ranges
	VkImageViewCreateInfo view = vkb::initializers::image_view_create_info();
	view.viewType              = VK_IMAGE_VIEW_TYPE_2D;
	view.format                = format;
	view.components            = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
	// The subresource range describes the set of mip levels (and array layers) that can be accessed through this image view
	// It's possible to create multiple image views for a single image referring to different (and/or overlapping) ranges of the image
	view.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	view.subresourceRange.baseMipLevel   = 0;
	view.subresourceRange.baseArrayLayer = 0;
	view.subresourceRange.layerCount     = 1;
	// Linear tiling usually won't support mip maps
	// Only set mip map count if optimal tiling is used
	view.subresourceRange.levelCount = (use_staging) ? texture.mip_levels : 1;
	// The view will be based on the texture's image
	view.image = texture.image;
	VK_CHECK(vkCreateImageView(get_device().get_handle(), &view, nullptr, &texture.view));
}

// Free all Vulkan resources used by a texture object
void TextureLoading::destroy_texture(Texture texture)
{
	vkDestroyImageView(get_device().get_handle(), texture.view, nullptr);
	vkDestroyImage(get_device().get_handle(), texture.image, nullptr);
	vkDestroySampler(get_device().get_handle(), texture.sampler, nullptr);
	vkFreeMemory(get_device().get_handle(), texture.device_memory, nullptr);
}

void TextureLoading::build_command_buffers()
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

		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, NULL);
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.solid);

		VkDeviceSize offsets[1] = {0};
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, vertex_buffer->get(), offsets);
		vkCmdBindIndexBuffer(draw_cmd_buffers[i], index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(draw_cmd_buffers[i], index_count, 1, 0, 0, 0);

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void TextureLoading::draw()
{
	ApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();
}

void TextureLoading::generate_quad()
{
	// Setup vertices for a single uv-mapped quad made from two triangles
	std::vector<TextureLoadingVertexStructure> vertices =
	    {
	        {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
	        {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
	        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
	        {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};

	// Setup indices
	std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};
	index_count                   = static_cast<uint32_t>(indices.size());

	auto vertex_buffer_size = vkb::to_u32(vertices.size() * sizeof(TextureLoadingVertexStructure));
	auto index_buffer_size  = vkb::to_u32(indices.size() * sizeof(uint32_t));

	// Create buffers
	// For the sake of simplicity we won't stage the vertex data to the gpu memory
	// Vertex buffer
	vertex_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                    vertex_buffer_size,
	                                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                                                    VMA_MEMORY_USAGE_CPU_TO_GPU);
	vertex_buffer->update(vertices.data(), vertex_buffer_size);

	index_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                   index_buffer_size,
	                                                   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	                                                   VMA_MEMORY_USAGE_CPU_TO_GPU);

	index_buffer->update(indices.data(), index_buffer_size);
}

void TextureLoading::setup_descriptor_pool()
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

void TextureLoading::setup_descriptor_set_layout()
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

void TextureLoading::setup_descriptor_set()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layout,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

	VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*uniform_buffer_vs);

	// Setup a descriptor image info for the current texture to be used as a combined image sampler
	VkDescriptorImageInfo image_descriptor;
	image_descriptor.imageView   = texture.view;                // The image's view (images are never directly accessed by the shader, but rather through views defining subresources)
	image_descriptor.sampler     = texture.sampler;             // The sampler (Telling the pipeline how to sample the texture, including repeat, border, etc.)
	image_descriptor.imageLayout = texture.image_layout;        // The current layout of the image (Note: Should always fit the actual use, e.g. shader read)

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

void TextureLoading::prepare_pipelines()
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

	shader_stages[0] = load_shader("texture_loading", "texture.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("texture_loading", "texture.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Vertex bindings and attributes
	const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(TextureLoadingVertexStructure), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TextureLoadingVertexStructure, pos)),
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(TextureLoadingVertexStructure, uv)),
	    vkb::initializers::vertex_input_attribute_description(0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TextureLoadingVertexStructure, normal)),
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

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.solid));
}

// Prepare and initialize uniform buffer containing shader uniforms
void TextureLoading::prepare_uniform_buffers()
{
	// Vertex shader uniform buffer block
	uniform_buffer_vs = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                        sizeof(ubo_vs),
	                                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                        VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void TextureLoading::update_uniform_buffers()
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

bool TextureLoading::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}
	load_texture();
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

void TextureLoading::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
}

void TextureLoading::view_changed()
{
	update_uniform_buffers();
}

void TextureLoading::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.slider_float("LOD bias", &ubo_vs.lod_bias, 0.0f, static_cast<float>(texture.mip_levels)))
		{
			update_uniform_buffers();
		}
	}
}

std::unique_ptr<vkb::Application> create_texture_loading()
{
	return std::make_unique<TextureLoading>();
}
