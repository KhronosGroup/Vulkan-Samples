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

/*
 * Texture loading (and display) example (including mip maps), using vulkan.hpp
 */

#include <hpp_texture_loading.h>

HPPTextureLoading::HPPTextureLoading()
{
	zoom     = -2.5f;
	rotation = {0.0f, 15.0f, 0.0f};
	title    = "HPP Texture loading";
}

HPPTextureLoading::~HPPTextureLoading()
{
	if (get_device().get_handle())
	{
		// Clean up used Vulkan resources
		// Note : Inherited destructor cleans up resources stored in base class

		get_device().get_handle().destroyPipeline(pipeline);

		get_device().get_handle().destroyPipelineLayout(pipeline_layout);
		get_device().get_handle().destroyDescriptorSetLayout(descriptor_set_layout);
	}

	destroy_texture(texture);

	vertex_buffer.reset();
	index_buffer.reset();
	uniform_buffer_vs.reset();
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
	std::string filename = vkb::fs::path::get(vkb::fs::path::Assets, "textures/metalplate01_rgba.ktx");
	// Texture data contains 4 channels (RGBA) with unnormalized 8-bit values, this is the most commonly supported format
	vk::Format format = vk::Format::eR8G8B8A8Unorm;

	ktxTexture *   ktx_texture;
	KTX_error_code result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx_texture);
	if ((result != KTX_SUCCESS) || (ktx_texture == nullptr))
	{
		throw std::runtime_error("Couldn't load texture");
	}

	texture.extent.width  = ktx_texture->baseWidth;
	texture.extent.height = ktx_texture->baseHeight;
	texture.mip_levels    = ktx_texture->numLevels;

	// We prefer using staging to copy the texture data to a device local optimal image
	vk::Bool32 use_staging = true;

	// Only use linear tiling if forced
	bool force_linear_tiling = false;
	if (force_linear_tiling)
	{
		// Don't use linear if format is not supported for (linear) shader sampling
		// Get device properites for the requested texture format
		vk::FormatProperties format_properties = get_device().get_gpu().get_handle().getFormatProperties(format);
		use_staging                            = !(format_properties.linearTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage);
	}

	if (use_staging)
	{
		// Copy data to an optimal tiled image
		// This loads the texture data into a host local buffer that is copied to the optimal tiled image on the device

		// Create a host-visible staging buffer that contains the raw image data
		// This buffer will be the data source for copying texture data to the optimal tiled image on the device
		// This buffer is used as a transfer source for the buffer copy
		vk::BufferCreateInfo buffer_create_info({}, ktx_texture->dataSize, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive, {});
		vk::Buffer           staging_buffer = get_device().get_handle().createBuffer(buffer_create_info);

		// Get memory requirements for the staging buffer (alignment, memory type bits)
		vk::MemoryRequirements memory_requirements = get_device().get_handle().getBufferMemoryRequirements(staging_buffer);
		vk::MemoryAllocateInfo memory_allocate_info(
		    memory_requirements.size,
		    // Get memory type index for a host visible buffer
		    get_device().get_memory_type(memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
		vk::DeviceMemory staging_memory = get_device().get_handle().allocateMemory(memory_allocate_info);
		get_device().get_handle().bindBufferMemory(staging_buffer, staging_memory, 0);

		// Copy texture data into host local staging buffer

		uint8_t *data = reinterpret_cast<uint8_t *>(get_device().get_handle().mapMemory(staging_memory, 0, memory_requirements.size));
		memcpy(data, ktx_texture->pData, ktx_texture->dataSize);
		get_device().get_handle().unmapMemory(staging_memory);

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
		image_create_info.imageType   = vk::ImageType::e2D;
		image_create_info.format      = format;
		image_create_info.mipLevels   = texture.mip_levels;
		image_create_info.arrayLayers = 1;
		image_create_info.samples     = vk::SampleCountFlagBits::e1;
		image_create_info.tiling      = vk::ImageTiling::eOptimal;
		image_create_info.sharingMode = vk::SharingMode::eExclusive;
		// Set initial layout of the image to undefined
		image_create_info.initialLayout = vk::ImageLayout::eUndefined;
		image_create_info.extent        = vk::Extent3D(texture.extent, 1);
		image_create_info.usage         = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
		texture.image                   = get_device().get_handle().createImage(image_create_info);

		memory_requirements   = get_device().get_handle().getImageMemoryRequirements(texture.image);
		memory_allocate_info  = {memory_requirements.size,
                                get_device().get_memory_type(memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)};
		texture.device_memory = get_device().get_handle().allocateMemory(memory_allocate_info);
		VK_CHECK(vkBindImageMemory(get_device().get_handle(), texture.image, texture.device_memory, 0));

		vk::CommandBuffer copy_command = get_device().create_command_buffer(vk::CommandBufferLevel::ePrimary, true);

		// Image memory barriers for the texture image

		// The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
		vk::ImageSubresourceRange subresource_range;
		// Image only contains color data
		subresource_range.aspectMask = vk::ImageAspectFlagBits::eColor;
		// Start at first mip level
		subresource_range.baseMipLevel = 0;
		// We will transition on all mip levels
		subresource_range.levelCount = texture.mip_levels;
		// The 2D texture only has one layer
		subresource_range.layerCount = 1;

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
		// Source pipeline stage is host write/read exection (VK_PIPELINE_STAGE_HOST_BIT)
		// Destination pipeline stage is copy command exection (VK_PIPELINE_STAGE_TRANSFER_BIT)
		copy_command.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer, {}, nullptr, nullptr, image_memory_barrier);

		// Copy mip levels from staging buffer
		copy_command.copyBufferToImage(staging_buffer, texture.image, vk::ImageLayout::eTransferDstOptimal, buffer_copy_regions);

		// Once the data has been uploaded we transfer the texture image to the shader read layout, so it can be sampled from
		image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		image_memory_barrier.oldLayout     = vk::ImageLayout::eTransferDstOptimal;
		image_memory_barrier.newLayout     = vk::ImageLayout::eShaderReadOnlyOptimal;

		// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
		// Source pipeline stage stage is copy command exection (VK_PIPELINE_STAGE_TRANSFER_BIT)
		// Destination pipeline stage fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
		copy_command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr, image_memory_barrier);

		// Store current layout for later reuse
		texture.image_layout = vk::ImageLayout::eShaderReadOnlyOptimal;

		get_device().flush_command_buffer(copy_command, queue, true);

		// Clean up staging resources
		get_device().get_handle().freeMemory(staging_memory);
		get_device().get_handle().destroyBuffer(staging_buffer);
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
		vk::Image mappable_image        = get_device().get_handle().createImage(image_create_info);

		// Get memory requirements for this image like size and alignment
		vk::MemoryRequirements memory_requirements = get_device().get_handle().getImageMemoryRequirements(mappable_image);
		// Set memory allocation size to required memory size
		vk::MemoryAllocateInfo memory_allocate_info(
		    memory_requirements.size,
		    // Get memory type that can be mapped to host memory
		    get_device().get_memory_type(memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
		vk::DeviceMemory mappable_memory = get_device().get_handle().allocateMemory(memory_allocate_info);
		get_device().get_handle().bindImageMemory(mappable_image, mappable_memory, 0);

		// Map image memory
		void *data = get_device().get_handle().mapMemory(mappable_memory, 0, memory_requirements.size);
		// Copy image data of the first mip level into memory
		memcpy(data, ktx_texture->pData, ktxTexture_GetImageSize(ktx_texture, 0));
		get_device().get_handle().unmapMemory(mappable_memory);

		// Linear tiled images don't need to be staged and can be directly used as textures
		texture.image         = mappable_image;
		texture.device_memory = mappable_memory;
		texture.image_layout  = vk::ImageLayout::eShaderReadOnlyOptimal;

		// Setup image memory barrier transfer image to shader read layout
		vk::CommandBuffer copy_command = get_device().create_command_buffer(vk::CommandBufferLevel::ePrimary, true);

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
		// Source pipeline stage is host write/read exection (VK_PIPELINE_STAGE_HOST_BIT)
		// Destination pipeline stage fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
		copy_command.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr, image_memory_barrier);

		get_device().flush_command_buffer(copy_command, queue, true);
	}

	// Create a texture sampler
	// In Vulkan textures are accessed by samplers
	// This separates all the sampling information from the texture data. This means you could have multiple sampler objects for the same texture with different settings
	// Note: Similar to the samplers available with OpenGL 3.3
	vk::SamplerCreateInfo sampler;
	sampler.magFilter    = vk::Filter::eLinear;
	sampler.minFilter    = vk::Filter::eLinear;
	sampler.mipmapMode   = vk::SamplerMipmapMode::eLinear;
	sampler.addressModeU = vk::SamplerAddressMode::eRepeat;
	sampler.addressModeV = vk::SamplerAddressMode::eRepeat;
	sampler.addressModeW = vk::SamplerAddressMode::eRepeat;
	sampler.mipLodBias   = 0.0f;
	sampler.compareOp    = vk::CompareOp::eNever;
	sampler.minLod       = 0.0f;
	// Set max level-of-detail to mip level count of the texture
	sampler.maxLod        = (use_staging) ? (float) texture.mip_levels : 0.0f;
	sampler.maxAnisotropy = 1.0f;
	// Enable anisotropic filtering
	// This feature is optional, so we must check if it's supported on the device
	if (get_device().get_gpu().get_features().samplerAnisotropy)
	{
		// Use max. level of anisotropy for this example
		sampler.maxAnisotropy    = get_device().get_gpu().get_properties().limits.maxSamplerAnisotropy;
		sampler.anisotropyEnable = true;
	}
	else
	{
		// The device does not support anisotropic filtering
		sampler.maxAnisotropy    = 1.0;
		sampler.anisotropyEnable = false;
	}
	sampler.borderColor = vk::BorderColor::eFloatOpaqueWhite;
	texture.sampler     = get_device().get_handle().createSampler(sampler);

	// Create image view
	// Textures are not directly accessed by the shaders and
	// are abstracted by image views containing additional
	// information and sub resource ranges
	vk::ImageViewCreateInfo view;
	view.viewType   = vk::ImageViewType::e2D;
	view.format     = format;
	view.components = {vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA};
	// The subresource range describes the set of mip levels (and array layers) that can be accessed through this image view
	// It's possible to create multiple image views for a single image referring to different (and/or overlapping) ranges of the image
	view.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
	view.subresourceRange.baseMipLevel   = 0;
	view.subresourceRange.baseArrayLayer = 0;
	view.subresourceRange.layerCount     = 1;
	// Linear tiling usually won't support mip maps
	// Only set mip map count if optimal tiling is used
	view.subresourceRange.levelCount = (use_staging) ? texture.mip_levels : 1;
	// The view will be based on the texture's image
	view.image   = texture.image;
	texture.view = get_device().get_handle().createImageView(view);
}

// Free all Vulkan resources used by a texture object
void HPPTextureLoading::destroy_texture(Texture texture)
{
	get_device().get_handle().destroyImageView(texture.view);
	get_device().get_handle().destroyImage(texture.image);
	get_device().get_handle().destroySampler(texture.sampler);
	get_device().get_handle().freeMemory(texture.device_memory);
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
		// Set target frame buffer
		render_pass_begin_info.framebuffer = framebuffers[i];

		draw_cmd_buffers[i].begin(command_buffer_begin_info);

		draw_cmd_buffers[i].beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

		vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f);
		draw_cmd_buffers[i].setViewport(0, viewport);

		vk::Rect2D scissor({0, 0}, extent);
		draw_cmd_buffers[i].setScissor(0, scissor);

		draw_cmd_buffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, descriptor_set, {});
		draw_cmd_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

		vk::DeviceSize offset = 0;
		draw_cmd_buffers[i].bindVertexBuffers(0, vertex_buffer->get_handle(), offset);
		draw_cmd_buffers[i].bindIndexBuffer(index_buffer->get_handle(), 0, vk::IndexType::eUint32);

		draw_cmd_buffers[i].drawIndexed(index_count, 1, 0, 0, 0);

		draw_ui(draw_cmd_buffers[i]);

		draw_cmd_buffers[i].endRenderPass();

		draw_cmd_buffers[i].end();
	}
}

void HPPTextureLoading::draw()
{
	HPPApiVulkanSample::prepare_frame();

	// Command buffer to be sumitted to the queue
	submit_info.setCommandBuffers(draw_cmd_buffers[current_buffer]);

	// Submit to queue
	queue.submit(submit_info);

	HPPApiVulkanSample::submit_frame();
}

void HPPTextureLoading::generate_quad()
{
	// Setup vertices for a single uv-mapped quad made from two triangles
	std::vector<HPPTextureLoadingVertexStructure> vertices =
	    {
	        {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
	        {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
	        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
	        {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};

	// Setup indices
	std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};
	index_count                   = static_cast<uint32_t>(indices.size());

	auto vertex_buffer_size = vkb::to_u32(vertices.size() * sizeof(HPPTextureLoadingVertexStructure));
	auto index_buffer_size  = vkb::to_u32(indices.size() * sizeof(uint32_t));

	// Create buffers
	// For the sake of simplicity we won't stage the vertex data to the gpu memory
	// Vertex buffer
	vertex_buffer = std::make_unique<vkb::core::HPPBuffer>(
	    get_device(), vertex_buffer_size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
	vertex_buffer->update(vertices.data(), vertex_buffer_size);

	// Index buffer
	index_buffer = std::make_unique<vkb::core::HPPBuffer>(
	    get_device(), index_buffer_size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
	index_buffer->update(indices.data(), index_buffer_size);
}

void HPPTextureLoading::setup_descriptor_pool()
{
	// Example uses one ubo and one image sampler
	std::array<vk::DescriptorPoolSize, 2> pool_sizes = {{{vk::DescriptorType::eUniformBuffer, 1}, {vk::DescriptorType::eCombinedImageSampler, 1}}};

	vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 2, pool_sizes);

	descriptor_pool = get_device().get_handle().createDescriptorPool(descriptor_pool_create_info);
}

void HPPTextureLoading::setup_descriptor_set_layout()
{
	std::array<vk::DescriptorSetLayoutBinding, 2> set_layout_bindings = {
	    {{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
	     {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};

	vk::DescriptorSetLayoutCreateInfo descriptor_layout({}, set_layout_bindings);

	descriptor_set_layout = get_device().get_handle().createDescriptorSetLayout(descriptor_layout);

#if defined(ANDROID)
	vk::PipelineLayoutCreateInfo pipeline_layout_create_info({}, 1, &descriptor_set_layout);
#else
	vk::PipelineLayoutCreateInfo pipeline_layout_create_info({}, descriptor_set_layout);
#endif

	pipeline_layout = get_device().get_handle().createPipelineLayout(pipeline_layout_create_info);
}

void HPPTextureLoading::setup_descriptor_set()
{
	vk::DescriptorSetAllocateInfo alloc_info(descriptor_pool, 1, &descriptor_set_layout);

	descriptor_set = get_device().get_handle().allocateDescriptorSets(alloc_info).front();

	vk::DescriptorBufferInfo buffer_descriptor(uniform_buffer_vs->get_handle(), 0, VK_WHOLE_SIZE);

	// Setup a descriptor image info for the current texture to be used as a combined image sampler
	vk::DescriptorImageInfo image_descriptor;
	image_descriptor.imageView   = texture.view;                // The image's view (images are never directly accessed by the shader, but rather through views defining subresources)
	image_descriptor.sampler     = texture.sampler;             // The sampler (Telling the pipeline how to sample the texture, including repeat, border, etc.)
	image_descriptor.imageLayout = texture.image_layout;        // The current layout of the image (Note: Should always fit the actual use, e.g. shader read)

	std::array<vk::WriteDescriptorSet, 2> write_descriptor_sets = {
	    {// Binding 0 : Vertex shader uniform buffer
	     {descriptor_set, 0, {}, vk::DescriptorType::eUniformBuffer, {}, buffer_descriptor},
	     // Binding 1 : Fragment shader texture sampler
	     //	Fragment shader: layout (binding = 1) uniform sampler2D samplerColor;
	     {descriptor_set, 1, {}, vk::DescriptorType::eCombinedImageSampler, image_descriptor}}};

	get_device().get_handle().updateDescriptorSets(write_descriptor_sets, {});
}

void HPPTextureLoading::prepare_pipeline()
{
	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state({}, vk::PrimitiveTopology::eTriangleList);

	vk::PipelineRasterizationStateCreateInfo rasterization_state;
	rasterization_state.polygonMode = vk::PolygonMode::eFill;
	rasterization_state.cullMode    = vk::CullModeFlagBits::eNone;
	rasterization_state.frontFace   = vk::FrontFace::eCounterClockwise;
	rasterization_state.lineWidth   = 1.0f;

	vk::PipelineColorBlendAttachmentState blend_attachment_state;
	blend_attachment_state.colorWriteMask =
	    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendStateCreateInfo color_blend_state({}, {}, {}, blend_attachment_state);

	// Note: Using Reversed depth-buffer for increased precision, so Greater depth values are kept
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;
	depth_stencil_state.depthTestEnable  = true;
	depth_stencil_state.depthWriteEnable = true;
	depth_stencil_state.depthCompareOp   = vk::CompareOp::eGreater;
	depth_stencil_state.back.compareOp   = vk::CompareOp::eGreater;

	vk::PipelineViewportStateCreateInfo viewport_state({}, 1, nullptr, 1, nullptr);

	vk::PipelineMultisampleStateCreateInfo multisample_state({}, vk::SampleCountFlagBits::e1);

	std::array<vk::DynamicState, 2>    dynamic_state_enables = {{vk::DynamicState::eViewport, vk::DynamicState::eScissor}};
	vk::PipelineDynamicStateCreateInfo dynamic_state({}, dynamic_state_enables);

	// Load shaders
	std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages = {{load_shader("texture_loading/texture.vert", vk::ShaderStageFlagBits::eVertex),
	                                                                   load_shader("texture_loading/texture.frag", vk::ShaderStageFlagBits::eFragment)}};

	// Vertex bindings and attributes
	vk::VertexInputBindingDescription                  vertex_input_binding(0, sizeof(HPPTextureLoadingVertexStructure), vk::VertexInputRate::eVertex);
	std::array<vk::VertexInputAttributeDescription, 3> vertex_input_attributes = {
	    {{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(HPPTextureLoadingVertexStructure, pos)},             // Location 0 : Position
	     {1, 0, vk::Format::eR32G32Sfloat, offsetof(HPPTextureLoadingVertexStructure, uv)},                 // Location 1: Texture Coordinates
	     {2, 0, vk::Format::eR32G32B32Sfloat, offsetof(HPPTextureLoadingVertexStructure, normal)}}};        // Location 2 : Normal
	vk::PipelineVertexInputStateCreateInfo vertex_input_state({}, vertex_input_binding, vertex_input_attributes);

	vk::GraphicsPipelineCreateInfo pipeline_create_info({},
	                                                    shader_stages,
	                                                    &vertex_input_state,
	                                                    &input_assembly_state,
	                                                    {},
	                                                    &viewport_state,
	                                                    &rasterization_state,
	                                                    &multisample_state,
	                                                    &depth_stencil_state,
	                                                    &color_blend_state,
	                                                    &dynamic_state,
	                                                    pipeline_layout,
	                                                    render_pass,
	                                                    {},
	                                                    {},
	                                                    -1);

	pipeline = get_device().get_handle().createGraphicsPipeline(pipeline_cache, pipeline_create_info).value;
}

// Prepare and initialize uniform buffer containing shader uniforms
void HPPTextureLoading::prepare_uniform_buffers()
{
	// Vertex shader uniform buffer block
	uniform_buffer_vs = std::make_unique<vkb::core::HPPBuffer>(get_device(), sizeof(ubo_vs), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void HPPTextureLoading::update_uniform_buffers()
{
	// Vertex shader
	ubo_vs.projection     = glm::perspective(glm::radians(60.0f), (float) extent.width / (float) extent.height, 0.001f, 256.0f);
	glm::mat4 view_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zoom));

	ubo_vs.model = view_matrix * glm::translate(glm::mat4(1.0f), camera_pos);
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	ubo_vs.view_pos = glm::vec4(0.0f, 0.0f, -zoom, 0.0f);

	uniform_buffer_vs->convert_and_update(ubo_vs);
}

bool HPPTextureLoading::prepare(vkb::platform::HPPPlatform &platform)
{
	if (!HPPApiVulkanSample::prepare(platform))
	{
		return false;
	}
	load_texture();
	generate_quad();
	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipeline();
	setup_descriptor_pool();
	setup_descriptor_set();
	build_command_buffers();
	prepared = true;
	return true;
}

void HPPTextureLoading::render(float delta_time)
{
	if (!prepared)
		return;
	draw();
}

void HPPTextureLoading::view_changed()
{
	update_uniform_buffers();
}

void HPPTextureLoading::on_update_ui_overlay(vkb::HPPDrawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.slider_float("LOD bias", &ubo_vs.lod_bias, 0.0f, (float) texture.mip_levels))
		{
			update_uniform_buffers();
		}
	}
}

std::unique_ptr<vkb::Application> create_hpp_texture_loading()
{
	return std::make_unique<HPPTextureLoading>();
}
