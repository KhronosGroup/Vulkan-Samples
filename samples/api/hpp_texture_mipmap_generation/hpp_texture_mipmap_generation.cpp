/* Copyright (c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
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
 * Runtime mip map generation, using vulkan.hpp
 */

#include "hpp_texture_mipmap_generation.h"
#include <common/vk_initializers.h>

HPPTextureMipMapGeneration::HPPTextureMipMapGeneration()
{
	zoom     = -2.5f;
	rotation = {0.0f, 15.0f, 0.0f};
	title    = "Texture MipMap generation";
}

HPPTextureMipMapGeneration::~HPPTextureMipMapGeneration()
{
	if (get_device() && get_device()->get_handle())
	{
		vk::Device device = get_device()->get_handle();

		device.destroyPipeline(pipeline);
		device.destroyPipelineLayout(pipeline_layout);
		device.destroyDescriptorSetLayout(descriptor_set_layout);
		for (auto sampler : samplers)
		{
			device.destroySampler(sampler);
		}
		device.destroyImageView(texture.view);
		device.destroyImage(texture.image);
		device.freeMemory(texture.device_memory);
		uniform_buffer.reset();
	}
}

bool HPPTextureMipMapGeneration::prepare(vkb::platform::HPPPlatform &platform)
{
	if (!HPPApiVulkanSample::prepare(platform))
	{
		return false;
	}

	camera.type = vkb::CameraType::FirstPerson;
	camera.set_perspective(60.0f, static_cast<float>(extent.width) / static_cast<float>(extent.height), 0.1f, 1024.0f);
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

// Enable physical device features required for this example
void HPPTextureMipMapGeneration::request_gpu_features(vkb::core::HPPPhysicalDevice &gpu)
{
	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = true;
	}
}

void HPPTextureMipMapGeneration::build_command_buffers()
{
	vk::CommandBufferBeginInfo command_buffer_begin_info;

	std::array<vk::ClearValue, 2> clear_values = {{default_clear_color, vk::ClearDepthStencilValue(1.0f, 0)}};

	vk::RenderPassBeginInfo render_pass_begin_info(render_pass, {}, {{0, 0}, extent}, clear_values);

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		render_pass_begin_info.framebuffer = framebuffers[i];

		draw_cmd_buffers[i].begin(command_buffer_begin_info);

		draw_cmd_buffers[i].beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

		vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f);
		draw_cmd_buffers[i].setViewport(0, viewport);

		vk::Rect2D scissor({0, 0}, extent);
		draw_cmd_buffers[i].setScissor(0, scissor);

		draw_cmd_buffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, descriptor_set, {});
		draw_cmd_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

		draw_model(scene, draw_cmd_buffers[i]);

		draw_ui(draw_cmd_buffers[i]);

		draw_cmd_buffers[i].endRenderPass();

		draw_cmd_buffers[i].end();
	}
}

void HPPTextureMipMapGeneration::on_update_ui_overlay(vkb::HPPDrawer &drawer)
{
	if (drawer.header("Settings"))
	{
		drawer.checkbox("Rotate", &rotate_scene);
		if (drawer.slider_float("LOD bias", &ubo.lod_bias, 0.0f, static_cast<float>(texture.mip_levels)))
		{
			update_uniform_buffers();
		}
		if (drawer.combo_box("Sampler type", &ubo.sampler_index, sampler_names))
		{
			update_uniform_buffers();
		}
	}
}

void HPPTextureMipMapGeneration::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
	if (rotate_scene)
	{
		update_uniform_buffers(delta_time);
	}
}

void HPPTextureMipMapGeneration::view_changed()
{
	update_uniform_buffers();
}

void HPPTextureMipMapGeneration::draw()
{
	HPPApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.setCommandBuffers(draw_cmd_buffers[current_buffer]);

	// Submit to queue
	queue.submit(submit_info);

	HPPApiVulkanSample::submit_frame();
}

void HPPTextureMipMapGeneration::load_assets()
{
	load_texture_generate_mipmaps(vkb::fs::path::get(vkb::fs::path::Assets, "textures/checkerboard_rgba.ktx"));
	scene = load_model("scenes/tunnel_cylinder.gltf");
}

/*
    Load the base texture containing only the first mip level and generate the whole mip-chain at runtime
*/
void HPPTextureMipMapGeneration::load_texture_generate_mipmaps(std::string file_name)
{
	// ktx1 doesn't know whether the content is sRGB or linear, but most tools save in sRGB, so assume that.
	vk::Format format = vk::Format::eR8G8B8A8Srgb;

	ktxTexture    *ktx_texture;
	KTX_error_code result = ktxTexture_CreateFromNamedFile(file_name.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx_texture);
	// @todo: get format from libktx
	if (ktx_texture == nullptr)
	{
		throw std::runtime_error("Couldn't load texture");
	}

	texture.extent.width  = ktx_texture->baseWidth;
	texture.extent.height = ktx_texture->baseHeight;

	// Calculate number of mip levels as per Vulkan specs:
	// numLevels = 1 + floor(log2(max(w, h, d)))
	texture.mip_levels = static_cast<uint32_t>(floor(log2(std::max(texture.extent.width, texture.extent.height))) + 1);

	// Get device properties for the requested texture format
	// Check if the selected format supports blit source and destination, which is required for generating the mip levels
	// If this is not supported you could implement a fallback via compute shader image writes and stores
	vk::FormatProperties formatProperties = get_device()->get_gpu().get_handle().getFormatProperties(format);
	if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitSrc) || !(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst))
	{
		throw std::runtime_error("Selected image format does not support blit source and destination");
	}

	ktx_uint8_t *ktx_image_data   = ktx_texture->pData;
	ktx_size_t   ktx_texture_size = ktx_texture->dataSize;

	// Create a host-visible staging buffer that contains the raw image data
	// This buffer is used as a transfer source for the buffer copy
	vk::BufferCreateInfo buffer_create_info({}, ktx_texture_size, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive);
	vk::Buffer           staging_buffer = get_device()->get_handle().createBuffer(buffer_create_info);

	// Get memory requirements for the staging buffer (alignment, memory type bits)
	vk::MemoryRequirements memory_requirements = get_device()->get_handle().getBufferMemoryRequirements(staging_buffer);
	vk::MemoryAllocateInfo memory_allocate_info;
	memory_allocate_info.allocationSize  = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = get_device()->get_gpu().get_memory_type(
	    memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	vk::DeviceMemory staging_memory = get_device()->get_handle().allocateMemory(memory_allocate_info);
	get_device()->get_handle().bindBufferMemory(staging_buffer, staging_memory, 0);

	// Copy ktx image data into host local staging buffer
	uint8_t *data = reinterpret_cast<uint8_t *>(get_device()->get_handle().mapMemory(staging_memory, 0, memory_requirements.size));
	memcpy(data, ktx_image_data, ktx_texture_size);
	get_device()->get_handle().unmapMemory(staging_memory);

	// Create optimal tiled target image on the device
	vk::ImageCreateInfo image_create_info({},
	                                      vk::ImageType::e2D,
	                                      format,
	                                      vk::Extent3D(texture.extent, 1),
	                                      texture.mip_levels,
	                                      1,
	                                      vk::SampleCountFlagBits::e1,
	                                      vk::ImageTiling::eOptimal,
	                                      vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled,
	                                      vk::SharingMode::eExclusive,
	                                      {},
	                                      vk::ImageLayout::eUndefined);
	texture.image = get_device()->get_handle().createImage(image_create_info);

	memory_requirements                  = get_device()->get_handle().getImageMemoryRequirements(texture.image);
	memory_allocate_info.allocationSize  = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = get_device()->get_gpu().get_memory_type(memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
	texture.device_memory                = get_device()->get_handle().allocateMemory(memory_allocate_info);
	get_device()->get_handle().bindImageMemory(texture.image, texture.device_memory, 0);

	vk::CommandBuffer copy_command = device->create_command_buffer(vk::CommandBufferLevel::ePrimary, true);

	// Optimal image will be used as destination for the copy, so we must transfer from our initial undefined image layout to the transfer destination layout
	vk::ImageMemoryBarrier image_memory_barrier({},
	                                            vk::AccessFlagBits::eTransferWrite,
	                                            vk::ImageLayout::eUndefined,
	                                            vk::ImageLayout::eTransferDstOptimal,
	                                            VK_QUEUE_FAMILY_IGNORED,
	                                            VK_QUEUE_FAMILY_IGNORED,
	                                            texture.image,
	                                            {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
	copy_command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, image_memory_barrier);

	// Copy the first mip of the chain, remaining mips will be generated
	vk::BufferImageCopy buffer_copy_region({}, {}, {}, {vk::ImageAspectFlagBits::eColor, 0, 0, 1}, {}, vk::Extent3D(texture.extent, 1));
	copy_command.copyBufferToImage(staging_buffer, texture.image, vk::ImageLayout::eTransferDstOptimal, buffer_copy_region);

	// Transition first mip level to transfer source so we can blit(read) from it
	image_memory_barrier = vk::ImageMemoryBarrier(vk::AccessFlagBits::eTransferWrite,
	                                              vk::AccessFlagBits::eTransferRead,
	                                              vk::ImageLayout::eTransferDstOptimal,
	                                              vk::ImageLayout::eTransferSrcOptimal,
	                                              VK_QUEUE_FAMILY_IGNORED,
	                                              VK_QUEUE_FAMILY_IGNORED,
	                                              texture.image,
	                                              {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
	copy_command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, image_memory_barrier);

	device->flush_command_buffer(copy_command, queue, true);

	// Clean up staging resources
	get_device()->get_handle().destroyBuffer(staging_buffer);
	get_device()->get_handle().freeMemory(staging_memory);

	// Generate the mip chain
	// ---------------------------------------------------------------
	// We copy down the whole mip chain doing a blit from mip-1 to mip
	// An alternative way would be to always blit from the first mip level and sample that one down
	vk::CommandBuffer blit_command = device->create_command_buffer(vk::CommandBufferLevel::ePrimary, true);

	// Copy down mips from n-1 to n
	for (uint32_t i = 1; i < texture.mip_levels; i++)
	{
		vk::ImageBlit image_blit({vk::ImageAspectFlagBits::eColor, i - 1, 0, 1},
		                         {{{}, {static_cast<int32_t>(texture.extent.width >> (i - 1)), static_cast<int32_t>(texture.extent.height >> (i - 1)), static_cast<int32_t>(1)}}},
		                         {vk::ImageAspectFlagBits::eColor, i, 0, 1},
		                         {{{}, {static_cast<int32_t>(texture.extent.width >> i), static_cast<int32_t>(texture.extent.height >> i), static_cast<int32_t>(1)}}});

		// Prepare current mip level as image blit destination
		image_memory_barrier = vk::ImageMemoryBarrier({},
		                                              vk::AccessFlagBits::eTransferWrite,
		                                              vk::ImageLayout::eUndefined,
		                                              vk::ImageLayout::eTransferDstOptimal,
		                                              VK_QUEUE_FAMILY_IGNORED,
		                                              VK_QUEUE_FAMILY_IGNORED,
		                                              texture.image,
		                                              {vk::ImageAspectFlagBits::eColor, i, 1, 0, 1});
		copy_command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, image_memory_barrier);

		// Blit from previous level
		blit_command.blitImage(texture.image, vk::ImageLayout::eTransferSrcOptimal, texture.image, vk::ImageLayout::eTransferDstOptimal, image_blit, vk::Filter::eLinear);

		// Prepare current mip level as image blit source for next level
		image_memory_barrier = vk::ImageMemoryBarrier(vk::AccessFlagBits::eTransferWrite,
		                                              vk::AccessFlagBits::eTransferRead,
		                                              vk::ImageLayout::eTransferDstOptimal,
		                                              vk::ImageLayout::eTransferSrcOptimal,
		                                              VK_QUEUE_FAMILY_IGNORED,
		                                              VK_QUEUE_FAMILY_IGNORED,
		                                              texture.image,
		                                              {vk::ImageAspectFlagBits::eColor, i, 1, 0, 1});
		copy_command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, image_memory_barrier);
	}

	// After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
	image_memory_barrier = vk::ImageMemoryBarrier(vk::AccessFlagBits::eTransferRead,
	                                              vk::AccessFlagBits::eShaderRead,
	                                              vk::ImageLayout::eTransferSrcOptimal,
	                                              vk::ImageLayout::eShaderReadOnlyOptimal,
	                                              VK_QUEUE_FAMILY_IGNORED,
	                                              VK_QUEUE_FAMILY_IGNORED,
	                                              texture.image,
	                                              {vk::ImageAspectFlagBits::eColor, 0, texture.mip_levels, 0, 1});
	copy_command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, image_memory_barrier);

	device->flush_command_buffer(blit_command, queue, true);
	// ---------------------------------------------------------------

	// Create samplers for different mip map demonstration cases
	vk::SamplerCreateInfo sampler_create_info({},
	                                          vk::Filter::eLinear,
	                                          vk::Filter::eLinear,
	                                          vk::SamplerMipmapMode::eLinear,
	                                          vk::SamplerAddressMode::eRepeat,
	                                          vk::SamplerAddressMode::eRepeat,
	                                          vk::SamplerAddressMode::eRepeat,
	                                          0.0f,
	                                          false,
	                                          1.0f,
	                                          false,
	                                          vk::CompareOp::eNever,
	                                          0.0f,
	                                          0.0f,
	                                          vk::BorderColor::eFloatOpaqueWhite);

	// Without mip mapping
	samplers[0] = get_device()->get_handle().createSampler(sampler_create_info);

	// With mip mapping
	sampler_create_info.maxLod = static_cast<float>(texture.mip_levels);
	samplers[1]                = get_device()->get_handle().createSampler(sampler_create_info);

	// With mip mapping and anisotropic filtering (when supported)
	if (get_device()->get_gpu().get_features().samplerAnisotropy)
	{
		sampler_create_info.maxAnisotropy    = get_device()->get_gpu().get_properties().limits.maxSamplerAnisotropy;
		sampler_create_info.anisotropyEnable = true;
	}
	samplers[2] = get_device()->get_handle().createSampler(sampler_create_info);

	// Create image view
	vk::ImageViewCreateInfo image_view_create_info({},
	                                               texture.image,
	                                               vk::ImageViewType::e2D,
	                                               format,
	                                               {vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA},
	                                               {vk::ImageAspectFlagBits::eColor, 0, texture.mip_levels, 0, 1});
	texture.view = get_device()->get_handle().createImageView(image_view_create_info);
}

void HPPTextureMipMapGeneration::prepare_pipelines()
{
	// Load shaders
	std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages;
	shader_stages[0] = load_shader("texture_mipmap_generation/texture.vert", vk::ShaderStageFlagBits::eVertex);
	shader_stages[1] = load_shader("texture_mipmap_generation/texture.frag", vk::ShaderStageFlagBits::eFragment);

	std::array<vk::VertexInputAttributeDescription, 2> vertex_input_attributes = {{
	    {0, 0, vk::Format::eR32G32B32Sfloat, 0},                     // Position
	    {1, 0, vk::Format::eR32G32Sfloat, sizeof(float) * 6},        // UV
	}};

	// Vertex bindings and attributes
	vk::VertexInputBindingDescription vertex_input_binding(0, sizeof(HPPVertex), vk::VertexInputRate::eVertex);

	vk::PipelineVertexInputStateCreateInfo vertex_input_state({}, vertex_input_binding, vertex_input_attributes);

	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state({}, vk::PrimitiveTopology::eTriangleList, false);

	vk::PipelineViewportStateCreateInfo viewport_state({}, 1, nullptr, 1, nullptr);

	vk::PipelineRasterizationStateCreateInfo rasterization_state;
	rasterization_state.polygonMode = vk::PolygonMode::eFill;
	rasterization_state.cullMode    = vk::CullModeFlagBits::eNone;
	rasterization_state.frontFace   = vk::FrontFace::eCounterClockwise;
	rasterization_state.lineWidth   = 1.0f;

	vk::PipelineMultisampleStateCreateInfo multisample_state({}, vk::SampleCountFlagBits::e1);

	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;
	depth_stencil_state.depthCompareOp   = vk::CompareOp::eLessOrEqual;
	depth_stencil_state.depthTestEnable  = true;
	depth_stencil_state.depthWriteEnable = true;
	depth_stencil_state.back.compareOp   = vk::CompareOp::eAlways;
	depth_stencil_state.front            = depth_stencil_state.back;

	vk::PipelineColorBlendAttachmentState blend_attachment_state;
	blend_attachment_state.colorWriteMask =
	    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendStateCreateInfo color_blend_state({}, false, {}, blend_attachment_state);

	std::array<vk::DynamicState, 2> dynamic_state_enables = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

	vk::PipelineDynamicStateCreateInfo dynamic_state({}, dynamic_state_enables);

	vk::GraphicsPipelineCreateInfo pipeline_create_info({},
	                                                    shader_stages,
	                                                    &vertex_input_state,
	                                                    &input_assembly_state,
	                                                    nullptr,
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

	vk::Result result;
	std::tie(result, pipeline) = get_device()->get_handle().createGraphicsPipeline(pipeline_cache, pipeline_create_info);
	assert(result == vk::Result::eSuccess);
}

void HPPTextureMipMapGeneration::prepare_uniform_buffers()
{
	// Shared parameter uniform buffer block
	uniform_buffer = std::make_unique<vkb::core::HPPBuffer>(*get_device(),
	                                                        sizeof(ubo),
	                                                        vk::BufferUsageFlagBits::eUniformBuffer,
	                                                        VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void HPPTextureMipMapGeneration::setup_descriptor_pool()
{
	// Example uses one ubo and one image sampler
	std::array<vk::DescriptorPoolSize, 3> pool_sizes = {
	    {{vk::DescriptorType::eUniformBuffer, 1}, {vk::DescriptorType::eSampledImage, 1}, {vk::DescriptorType::eSampler, 3}}};

	vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 2, pool_sizes);

	descriptor_pool = get_device()->get_handle().createDescriptorPool(descriptor_pool_create_info);
}

void HPPTextureMipMapGeneration::setup_descriptor_set()
{
#if defined(ANDROID)
	vk::DescriptorSetAllocateInfo alloc_info(descriptor_pool, 1, &descriptor_set_layout);
#else
	vk::DescriptorSetAllocateInfo alloc_info(descriptor_pool, descriptor_set_layout);
#endif
	descriptor_set = get_device()->get_handle().allocateDescriptorSets(alloc_info).front();

	vk::DescriptorBufferInfo buffer_descriptor(uniform_buffer->get_handle(), 0, VK_WHOLE_SIZE);

	vk::DescriptorImageInfo image_descriptor(nullptr, texture.view, vk::ImageLayout::eShaderReadOnlyOptimal);

	std::array<vk::DescriptorImageInfo, 3> sampler_descriptors = {{{samplers[0], nullptr, vk::ImageLayout::eShaderReadOnlyOptimal},
	                                                               {samplers[1], nullptr, vk::ImageLayout::eShaderReadOnlyOptimal},
	                                                               {samplers[2], nullptr, vk::ImageLayout::eShaderReadOnlyOptimal}}};
	assert(samplers.size() == sampler_descriptors.size());

	std::array<vk::WriteDescriptorSet, 3> write_descriptor_sets = {
	    {{descriptor_set, 0, {}, vk::DescriptorType::eUniformBuffer, {}, buffer_descriptor},        // Binding 0 : Vertex shader uniform buffer
	     {descriptor_set, 1, {}, vk::DescriptorType::eSampledImage, image_descriptor},              // Binding 1 : Fragment shader texture sampler
	     {descriptor_set, 2, {}, vk::DescriptorType::eSampler, sampler_descriptors}}};              // Binding 2: Sampler array

	get_device()->get_handle().updateDescriptorSets(write_descriptor_sets, {});
}

void HPPTextureMipMapGeneration::setup_descriptor_set_layout()
{
	std::array<vk::DescriptorSetLayoutBinding, 3> set_layout_bindings = {
	    {{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},        // Binding 0 : Parameter uniform buffer
	     {1, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment},                                            // Binding 1 : Fragment shader image sampler
	     {2, vk::DescriptorType::eSampler, 3, vk::ShaderStageFlagBits::eFragment}}};                                               // Binding 2 : Sampler array (3 descriptors)

	vk::DescriptorSetLayoutCreateInfo descriptor_layout({}, set_layout_bindings);

	descriptor_set_layout = get_device()->get_handle().createDescriptorSetLayout(descriptor_layout);

#if defined(ANDROID)
	vk::PipelineLayoutCreateInfo pipeline_layout_create_info({}, 1, &descriptor_set_layout);
#else
	vk::PipelineLayoutCreateInfo  pipeline_layout_create_info({}, descriptor_set_layout);
#endif
	pipeline_layout = get_device()->get_handle().createPipelineLayout(pipeline_layout_create_info);
}

void HPPTextureMipMapGeneration::update_uniform_buffers(float delta_time)
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

std::unique_ptr<vkb::Application> create_hpp_texture_mipmap_generation()
{
	return std::make_unique<HPPTextureMipMapGeneration>();
}
