/* Copyright (c) 2021-2023, NVIDIA CORPORATION. All rights reserved.
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
 * Texture loading (and display) example (including mip maps), using vulkan.hpp
 */

#include "hpp_texture_loading.h"
#include <common/ktx_common.h>
#include <core/hpp_command_pool.h>

HPPTextureLoading::HPPTextureLoading()
{
	title = "HPP Texture loading";

	zoom     = -2.5f;
	rotation = {0.0f, 15.0f, 0.0f};
}

HPPTextureLoading::~HPPTextureLoading()
{
	if (get_device() && get_device()->get_handle())
	{
		vk::Device device = get_device()->get_handle();

		// Clean up used Vulkan resources
		// Note : Inherited destructor cleans up resources stored in base class
		device.destroyPipeline(pipeline);
		device.destroyPipelineLayout(pipeline_layout);
		device.destroyDescriptorSetLayout(descriptor_set_layout);
		texture.destroy(device);
	}

	vertex_buffer.reset();
	index_buffer.reset();
	vertex_shader_data_buffer.reset();
}

bool HPPTextureLoading::prepare(const vkb::ApplicationOptions &options)
{
	assert(!prepared);

	if (HPPApiVulkanSample::prepare(options))
	{
		load_texture();
		generate_quad();
		prepare_uniform_buffers();
		descriptor_set_layout = create_descriptor_set_layout();
		pipeline_layout       = get_device()->get_handle().createPipelineLayout({{}, descriptor_set_layout});
		pipeline              = create_pipeline();
		descriptor_pool       = create_descriptor_pool();
		descriptor_set        = vkb::common::allocate_descriptor_set(get_device()->get_handle(), descriptor_pool, {descriptor_set_layout});
		update_descriptor_set();
		build_command_buffers();

		prepared = true;
	}

	return prepared;
}

// Enable physical device features required for this example
void HPPTextureLoading::request_gpu_features(vkb::core::HPPPhysicalDevice &gpu)
{
	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = true;
	}
}

void HPPTextureLoading::build_command_buffers()
{
	vk::CommandBufferBeginInfo command_buffer_begin_info;

	vk::ClearValue clear_values[2];
	clear_values[0].color        = default_clear_color;
	clear_values[1].depthStencil = vk::ClearDepthStencilValue(0.0f, 0);

	vk::RenderPassBeginInfo render_pass_begin_info;
	render_pass_begin_info.renderPass          = render_pass;
	render_pass_begin_info.renderArea.offset.x = 0;
	render_pass_begin_info.renderArea.offset.y = 0;
	render_pass_begin_info.renderArea.extent   = extent;
	render_pass_begin_info.clearValueCount     = 2;
	render_pass_begin_info.pClearValues        = clear_values;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		auto command_buffer = draw_cmd_buffers[i];

		// Set target frame buffer
		render_pass_begin_info.framebuffer = framebuffers[i];

		command_buffer.begin(command_buffer_begin_info);

		command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

		vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f);
		command_buffer.setViewport(0, viewport);

		vk::Rect2D scissor({0, 0}, extent);
		command_buffer.setScissor(0, scissor);

		command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, descriptor_set, {});
		command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

		vk::DeviceSize offset = 0;
		command_buffer.bindVertexBuffers(0, vertex_buffer->get_handle(), offset);
		command_buffer.bindIndexBuffer(index_buffer->get_handle(), 0, vk::IndexType::eUint32);

		command_buffer.drawIndexed(index_count, 1, 0, 0, 0);

		draw_ui(command_buffer);

		command_buffer.endRenderPass();

		command_buffer.end();
	}
}

void HPPTextureLoading::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.slider_float("LOD bias", &vertex_shader_data.lod_bias, 0.0f, static_cast<float>(texture.mip_levels)))
		{
			update_uniform_buffers();
		}
	}
}

void HPPTextureLoading::render(float delta_time)
{
	if (prepared)
	{
		draw();
	}
}

void HPPTextureLoading::view_changed()
{
	update_uniform_buffers();
}

vk::DescriptorPool HPPTextureLoading::create_descriptor_pool()
{
	// Example uses one ubo and one image sampler
	std::array<vk::DescriptorPoolSize, 2> pool_sizes = {{{vk::DescriptorType::eUniformBuffer, 1}, {vk::DescriptorType::eCombinedImageSampler, 1}}};

	return get_device()->get_handle().createDescriptorPool({{}, 2, pool_sizes});
}

vk::DescriptorSetLayout HPPTextureLoading::create_descriptor_set_layout()
{
	std::array<vk::DescriptorSetLayoutBinding, 2> set_layout_bindings = {
	    {{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
	     {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};

	return get_device()->get_handle().createDescriptorSetLayout({{}, set_layout_bindings});
}

vk::Pipeline HPPTextureLoading::create_pipeline()
{
	// Load shaders
	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = {{load_shader("texture_loading/texture.vert", vk::ShaderStageFlagBits::eVertex),
	                                                                 load_shader("texture_loading/texture.frag", vk::ShaderStageFlagBits::eFragment)}};

	// Vertex bindings and attributes
	vk::VertexInputBindingDescription                  vertex_input_binding(0, sizeof(Vertex), vk::VertexInputRate::eVertex);
	std::array<vk::VertexInputAttributeDescription, 3> vertex_input_attributes = {
	    {{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)},             // Location 0 : Position
	     {1, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv)},                 // Location 1: Texture Coordinates
	     {2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)}}};        // Location 2 : Normal
	vk::PipelineVertexInputStateCreateInfo vertex_input_state({}, vertex_input_binding, vertex_input_attributes);

	vk::PipelineColorBlendAttachmentState blend_attachment_state;
	blend_attachment_state.colorWriteMask =
	    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	// Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;
	depth_stencil_state.depthTestEnable  = true;
	depth_stencil_state.depthWriteEnable = true;
	depth_stencil_state.depthCompareOp   = vk::CompareOp::eGreater;
	depth_stencil_state.back.compareOp   = vk::CompareOp::eGreater;

	return vkb::common::create_graphics_pipeline(get_device()->get_handle(),
	                                             pipeline_cache,
	                                             shader_stages,
	                                             vertex_input_state,
	                                             vk::PrimitiveTopology::eTriangleList,
	                                             0,
	                                             vk::PolygonMode::eFill,
	                                             vk::CullModeFlagBits::eNone,
	                                             vk::FrontFace::eCounterClockwise,
	                                             {blend_attachment_state},
	                                             depth_stencil_state,
	                                             pipeline_layout,
	                                             render_pass);
}

void HPPTextureLoading::draw()
{
	HPPApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.setCommandBuffers(draw_cmd_buffers[current_buffer]);

	// Submit to queue
	queue.submit(submit_info);

	HPPApiVulkanSample::submit_frame();
}

void HPPTextureLoading::generate_quad()
{
	// Setup vertices for a single uv-mapped quad made from two triangles
	std::vector<Vertex> vertices = {{{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
	                                {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
	                                {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
	                                {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};

	// Setup indices
	std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};
	index_count                   = static_cast<uint32_t>(indices.size());

	auto vertex_buffer_size = vkb::to_u32(vertices.size() * sizeof(Vertex));
	auto index_buffer_size  = vkb::to_u32(indices.size() * sizeof(uint32_t));

	// Create buffers
	// For the sake of simplicity we won't stage the vertex data to the gpu memory
	// Vertex buffer
	vertex_buffer = std::make_unique<vkb::core::HPPBuffer>(
	    *get_device(), vertex_buffer_size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
	vertex_buffer->update(vertices.data(), vertex_buffer_size);

	// Index buffer
	index_buffer = std::make_unique<vkb::core::HPPBuffer>(
	    *get_device(), index_buffer_size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
	index_buffer->update(indices.data(), index_buffer_size);
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
void HPPTextureLoading::load_texture()
{
	// We use the Khronos texture format (https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/)
	const std::string filename = vkb::fs::path::get(vkb::fs::path::Assets, "textures/metalplate01_rgba.ktx");
	// ktx1 doesn't know whether the content is sRGB or linear, but most tools save in sRGB, so assume that.
	constexpr vk::Format format = vk::Format::eR8G8B8A8Srgb;

	ktxTexture *ktx_texture = vkb::ktx::load_texture(filename);

	texture.extent     = vk::Extent2D(ktx_texture->baseWidth, ktx_texture->baseHeight);
	texture.mip_levels = ktx_texture->numLevels;

	// We prefer using staging to copy the texture data to a device local optimal image
	vk::Bool32 use_staging = true;

	// Only use linear tiling if forced
	bool force_linear_tiling = false;
	if (force_linear_tiling)
	{
		// Don't use linear if format is not supported for (linear) shader sampling
		// Get device properties for the requested texture format
		vk::FormatProperties format_properties = get_device()->get_gpu().get_handle().getFormatProperties(format);
		use_staging                            = !(format_properties.linearTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage);
	}

	vk::Device device = get_device()->get_handle();

	if (use_staging)
	{
		// Copy data to an optimal tiled image
		// This loads the texture data into a host local buffer that is copied to the optimal tiled image on the device

		// Create a host-visible staging buffer that contains the raw image data
		// This buffer will be the data source for copying texture data to the optimal tiled image on the device
		// This buffer is used as a transfer source for the buffer copy
		vk::BufferCreateInfo buffer_create_info({}, ktx_texture->dataSize, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive, {});
		vk::Buffer           staging_buffer = device.createBuffer(buffer_create_info);

		// Get memory requirements for the staging buffer (alignment, memory type bits)
		vk::MemoryRequirements memory_requirements = device.getBufferMemoryRequirements(staging_buffer);

		// Get memory type index for a host visible buffer
		uint32_t memory_type = get_device()->get_gpu().get_memory_type(memory_requirements.memoryTypeBits,
		                                                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		vk::MemoryAllocateInfo memory_allocate_info(memory_requirements.size, memory_type);
		vk::DeviceMemory       staging_memory = device.allocateMemory(memory_allocate_info);
		device.bindBufferMemory(staging_buffer, staging_memory, 0);

		// Copy texture data into host local staging buffer
		uint8_t *data = reinterpret_cast<uint8_t *>(device.mapMemory(staging_memory, 0, memory_requirements.size));
		memcpy(data, ktx_texture->pData, ktx_texture->dataSize);
		device.unmapMemory(staging_memory);

		// Setup buffer copy regions for each mip level
		std::vector<vk::BufferImageCopy> buffer_copy_regions(texture.mip_levels);
		for (uint32_t i = 0; i < texture.mip_levels; i++)
		{
			ktx_size_t     offset;
			KTX_error_code result = ktxTexture_GetImageOffset(ktx_texture, i, 0, 0, &offset);

			buffer_copy_regions[i].imageSubresource.aspectMask     = vk::ImageAspectFlagBits::eColor;
			buffer_copy_regions[i].imageSubresource.mipLevel       = i;
			buffer_copy_regions[i].imageSubresource.baseArrayLayer = 0;
			buffer_copy_regions[i].imageSubresource.layerCount     = 1;
			buffer_copy_regions[i].imageExtent.width               = ktx_texture->baseWidth >> i;
			buffer_copy_regions[i].imageExtent.height              = ktx_texture->baseHeight >> i;
			buffer_copy_regions[i].imageExtent.depth               = 1;
			buffer_copy_regions[i].bufferOffset                    = offset;
		}

		// Create optimal tiled target image on the device
		vk::ImageCreateInfo image_create_info;
		image_create_info.imageType     = vk::ImageType::e2D;
		image_create_info.format        = format;
		image_create_info.mipLevels     = texture.mip_levels;
		image_create_info.arrayLayers   = 1;
		image_create_info.samples       = vk::SampleCountFlagBits::e1;
		image_create_info.tiling        = vk::ImageTiling::eOptimal;
		image_create_info.sharingMode   = vk::SharingMode::eExclusive;
		image_create_info.initialLayout = vk::ImageLayout::eUndefined;        // Set initial layout of the image to undefined
		image_create_info.extent        = vk::Extent3D(texture.extent, 1);
		image_create_info.usage         = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
		texture.image                   = device.createImage(image_create_info);

		memory_requirements   = device.getImageMemoryRequirements(texture.image);
		memory_type           = get_device()->get_gpu().get_memory_type(memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
		memory_allocate_info  = {memory_requirements.size, memory_type};
		texture.device_memory = device.allocateMemory(memory_allocate_info);
		device.bindImageMemory(texture.image, texture.device_memory, 0);

		vk::CommandBuffer copy_command = vkb::common::allocate_command_buffer(device, get_device()->get_command_pool().get_handle());
		copy_command.begin(vk::CommandBufferBeginInfo());

		// Image memory barriers for the texture image

		// The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
		vk::ImageSubresourceRange subresource_range;
		subresource_range.aspectMask   = vk::ImageAspectFlagBits::eColor;        // Image contains only color data
		subresource_range.baseMipLevel = 0;                                      // Start at first mip level
		subresource_range.levelCount   = texture.mip_levels;                     // We will transition on all mip levels
		subresource_range.layerCount   = 1;                                      // The 2D texture only has one layer

		// Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
		vk::ImageMemoryBarrier image_memory_barrier;
		image_memory_barrier.image               = texture.image;
		image_memory_barrier.subresourceRange    = subresource_range;
		image_memory_barrier.srcAccessMask       = {};
		image_memory_barrier.dstAccessMask       = vk::AccessFlagBits::eTransferWrite;
		image_memory_barrier.oldLayout           = vk::ImageLayout::eUndefined;
		image_memory_barrier.newLayout           = vk::ImageLayout::eTransferDstOptimal;
		image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
		// Source pipeline stage is host write/read execution (VK_PIPELINE_STAGE_HOST_BIT)
		// Destination pipeline stage is copy command execution (VK_PIPELINE_STAGE_TRANSFER_BIT)
		copy_command.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer, {}, nullptr, nullptr, image_memory_barrier);

		// Copy mip levels from staging buffer
		copy_command.copyBufferToImage(staging_buffer, texture.image, vk::ImageLayout::eTransferDstOptimal, buffer_copy_regions);

		// Once the data has been uploaded we transfer the texture image to the shader read layout, so it can be sampled from
		image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		image_memory_barrier.oldLayout     = vk::ImageLayout::eTransferDstOptimal;
		image_memory_barrier.newLayout     = vk::ImageLayout::eShaderReadOnlyOptimal;

		// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
		// Source pipeline stage stage is copy command execution (VK_PIPELINE_STAGE_TRANSFER_BIT)
		// Destination pipeline stage fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
		copy_command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr, image_memory_barrier);

		// Store current layout for later reuse
		texture.image_layout = vk::ImageLayout::eShaderReadOnlyOptimal;

		get_device()->flush_command_buffer(copy_command, queue, true);

		// Clean up staging resources
		device.destroyBuffer(staging_buffer);
		device.freeMemory(staging_memory);
	}
	else
	{
		// Copy data to a linear tiled image

		// Load mip map level 0 to linear tiling image
		vk::ImageCreateInfo image_create_info;
		image_create_info.imageType     = vk::ImageType::e2D;
		image_create_info.format        = format;
		image_create_info.mipLevels     = 1;
		image_create_info.arrayLayers   = 1;
		image_create_info.samples       = vk::SampleCountFlagBits::e1;
		image_create_info.tiling        = vk::ImageTiling::eLinear;
		image_create_info.usage         = vk::ImageUsageFlagBits::eSampled;
		image_create_info.sharingMode   = vk::SharingMode::eExclusive;
		image_create_info.initialLayout = vk::ImageLayout::ePreinitialized;
		image_create_info.extent        = vk::Extent3D(texture.extent, 1);
		vk::Image mappable_image        = device.createImage(image_create_info);

		// Get memory requirements for this image like size and alignment
		vk::MemoryRequirements memory_requirements = device.getImageMemoryRequirements(mappable_image);

		// Get memory type that can be mapped to host memory
		uint32_t memory_type = get_device()->get_gpu().get_memory_type(memory_requirements.memoryTypeBits,
		                                                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		// Set memory allocation size to required memory size
		vk::MemoryAllocateInfo memory_allocate_info(memory_requirements.size, memory_type);
		vk::DeviceMemory       mappable_memory = device.allocateMemory(memory_allocate_info);
		device.bindImageMemory(mappable_image, mappable_memory, 0);

		// Map image memory
		void *data = device.mapMemory(mappable_memory, 0, memory_requirements.size);
		// Copy image data of the first mip level into memory
		memcpy(data, ktx_texture->pData, ktxTexture_GetImageSize(ktx_texture, 0));
		device.unmapMemory(mappable_memory);

		// Linear tiled images don't need to be staged and can be directly used as textures
		texture.image         = mappable_image;
		texture.device_memory = mappable_memory;
		texture.image_layout  = vk::ImageLayout::eShaderReadOnlyOptimal;

		// Setup image memory barrier transfer image to shader read layout
		vk::CommandBuffer copy_command = vkb::common::allocate_command_buffer(device, get_device()->get_command_pool().get_handle());
		copy_command.begin(vk::CommandBufferBeginInfo());

		// The sub resource range describes the regions of the image we will be transition
		vk::ImageSubresourceRange subresource_range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

		// Transition the texture image layout to shader read, so it can be sampled from
		vk::ImageMemoryBarrier image_memory_barrier;
		image_memory_barrier.image               = texture.image;
		image_memory_barrier.subresourceRange    = subresource_range;
		image_memory_barrier.srcAccessMask       = vk::AccessFlagBits::eHostWrite;
		image_memory_barrier.dstAccessMask       = vk::AccessFlagBits::eShaderRead;
		image_memory_barrier.oldLayout           = vk::ImageLayout::ePreinitialized;
		image_memory_barrier.newLayout           = vk::ImageLayout::eShaderReadOnlyOptimal;
		image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
		// Source pipeline stage is host write/read execution (VK_PIPELINE_STAGE_HOST_BIT)
		// Destination pipeline stage fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
		copy_command.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr, image_memory_barrier);

		get_device()->flush_command_buffer(copy_command, queue, true);
	}

	// now, the ktx_texture can be destroyed
	ktxTexture_Destroy(ktx_texture);

	// Create a texture sampler
	// In Vulkan textures are accessed by samplers
	// This separates all the sampling information from the texture data. This means you could have multiple sampler objects for the same texture with different settings
	// Note: Similar to the samplers available with OpenGL 3.3
	vk::SamplerCreateInfo sampler;
	sampler.magFilter     = vk::Filter::eLinear;
	sampler.minFilter     = vk::Filter::eLinear;
	sampler.mipmapMode    = vk::SamplerMipmapMode::eLinear;
	sampler.addressModeU  = vk::SamplerAddressMode::eRepeat;
	sampler.addressModeV  = vk::SamplerAddressMode::eRepeat;
	sampler.addressModeW  = vk::SamplerAddressMode::eRepeat;
	sampler.mipLodBias    = 0.0f;
	sampler.compareOp     = vk::CompareOp::eNever;
	sampler.minLod        = 0.0f;
	sampler.maxLod        = (use_staging) ? static_cast<float>(texture.mip_levels) : 0.0f;        // Set max level-of-detail to mip level count of the texture
	sampler.maxAnisotropy = 1.0f;

	// Enable anisotropic filtering
	// This feature is optional, so we must check if it's supported on the device
	if (get_device()->get_gpu().get_features().samplerAnisotropy)
	{
		// Use max. level of anisotropy for this example
		sampler.maxAnisotropy    = get_device()->get_gpu().get_properties().limits.maxSamplerAnisotropy;
		sampler.anisotropyEnable = true;
	}
	else
	{
		// The device does not support anisotropic filtering
		sampler.maxAnisotropy    = 1.0;
		sampler.anisotropyEnable = false;
	}
	sampler.borderColor = vk::BorderColor::eFloatOpaqueWhite;
	texture.sampler     = device.createSampler(sampler);

	// Create image view
	// Textures are not directly accessed by the shaders and
	// are abstracted by image views containing additional
	// information and sub resource ranges
	vk::ImageViewCreateInfo image_view_create_info;
	image_view_create_info.viewType   = vk::ImageViewType::e2D;
	image_view_create_info.format     = format;
	image_view_create_info.components = {vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA};
	// The subresource range describes the set of mip levels (and array layers) that can be accessed through this image image_view_create_info
	// It's possible to create multiple image views for a single image referring to different (and/or overlapping) ranges of the image
	image_view_create_info.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
	image_view_create_info.subresourceRange.baseMipLevel   = 0;
	image_view_create_info.subresourceRange.baseArrayLayer = 0;
	image_view_create_info.subresourceRange.layerCount     = 1;
	// Linear tiling usually won't support mip maps
	// Only set mip map count if optimal tiling is used
	image_view_create_info.subresourceRange.levelCount = (use_staging) ? texture.mip_levels : 1;
	// The image_view_create_info will be based on the texture's image
	image_view_create_info.image = texture.image;
	texture.image_view           = device.createImageView(image_view_create_info);
}

// Prepare and initialize uniform buffer containing shader uniforms
void HPPTextureLoading::prepare_uniform_buffers()
{
	// Vertex shader uniform buffer block
	vertex_shader_data_buffer =
	    std::make_unique<vkb::core::HPPBuffer>(*get_device(), sizeof(vertex_shader_data), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void HPPTextureLoading::update_descriptor_set()
{
	vk::DescriptorBufferInfo buffer_descriptor(vertex_shader_data_buffer->get_handle(), 0, VK_WHOLE_SIZE);

	// Setup a descriptor image info for the current texture to be used as a combined image sampler
	vk::DescriptorImageInfo image_descriptor;
	image_descriptor.imageView   = texture.image_view;          // The image's view (images are never directly accessed by the shader, but rather through views defining subresources)
	image_descriptor.sampler     = texture.sampler;             // The sampler (Telling the pipeline how to sample the texture, including repeat, border, etc.)
	image_descriptor.imageLayout = texture.image_layout;        // The current layout of the image (Note: Should always fit the actual use, e.g. shader read)

	std::array<vk::WriteDescriptorSet, 2> write_descriptor_sets = {
	    {// Binding 0 : Vertex shader uniform buffer
	     {descriptor_set, 0, {}, vk::DescriptorType::eUniformBuffer, {}, buffer_descriptor},
	     // Binding 1 : Fragment shader texture sampler
	     //	Fragment shader: layout (binding = 1) uniform sampler2D samplerColor;
	     {descriptor_set, 1, {}, vk::DescriptorType::eCombinedImageSampler, image_descriptor}}};

	get_device()->get_handle().updateDescriptorSets(write_descriptor_sets, {});
}

void HPPTextureLoading::update_uniform_buffers()
{
	// Vertex shader
	vertex_shader_data.projection = glm::perspective(glm::radians(60.0f), static_cast<float>(extent.width) / static_cast<float>(extent.height), 0.001f, 256.0f);
	glm::mat4 view_matrix         = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zoom));

	vertex_shader_data.model = view_matrix * glm::translate(glm::mat4(1.0f), camera_pos);
	vertex_shader_data.model = glm::rotate(vertex_shader_data.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	vertex_shader_data.model = glm::rotate(vertex_shader_data.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	vertex_shader_data.model = glm::rotate(vertex_shader_data.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	vertex_shader_data.view_pos = glm::vec4(0.0f, 0.0f, -zoom, 0.0f);

	vertex_shader_data_buffer->convert_and_update(vertex_shader_data);
}

std::unique_ptr<vkb::Application> create_hpp_texture_loading()
{
	return std::make_unique<HPPTextureLoading>();
}
