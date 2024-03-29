/* Copyright (c) 2022-2024, NVIDIA CORPORATION. All rights reserved.
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
#include <common/hpp_vk_common.h>
#include <common/ktx_common.h>
#include <common/vk_initializers.h>
#include <core/hpp_command_pool.h>

HPPTextureMipMapGeneration::HPPTextureMipMapGeneration()
{
	title = "Texture MipMap generation";

	zoom     = -2.5f;
	rotation = {0.0f, 15.0f, 0.0f};
}

HPPTextureMipMapGeneration::~HPPTextureMipMapGeneration()
{
	if (has_device() && get_device().get_handle())
	{
		vk::Device device = get_device().get_handle();

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

bool HPPTextureMipMapGeneration::prepare(const vkb::ApplicationOptions &options)
{
	assert(!prepared);

	if (HPPApiVulkanSample::prepare(options))
	{
		prepare_camera();

		load_assets();
		prepare_uniform_buffers();
		descriptor_set_layout = create_descriptor_set_layout();
		pipeline_layout       = get_device().get_handle().createPipelineLayout({{}, descriptor_set_layout});
		pipeline              = create_pipeline();
		descriptor_pool       = create_descriptor_pool();
		descriptor_set        = vkb::common::allocate_descriptor_set(get_device().get_handle(), descriptor_pool, descriptor_set_layout);
		update_descriptor_set();
		build_command_buffers();

		prepared = true;
	}

	return prepared;
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
		auto command_buffer = draw_cmd_buffers[i];

		render_pass_begin_info.framebuffer = framebuffers[i];

		command_buffer.begin(command_buffer_begin_info);

		command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

		vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f);
		command_buffer.setViewport(0, viewport);

		vk::Rect2D scissor({0, 0}, extent);
		command_buffer.setScissor(0, scissor);

		command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, descriptor_set, {});
		command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

		draw_model(scene, command_buffer);

		draw_ui(command_buffer);

		command_buffer.endRenderPass();

		command_buffer.end();
	}
}

void HPPTextureMipMapGeneration::on_update_ui_overlay(vkb::Drawer &drawer)
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
	if (prepared)
	{
		draw();
		if (rotate_scene)
		{
			update_uniform_buffers(delta_time);
		}
	}
}

void HPPTextureMipMapGeneration::view_changed()
{
	update_uniform_buffers();
}

void HPPTextureMipMapGeneration::check_format_features(vk::Format format) const
{
	// Get device properties for the requested texture format
	vk::FormatProperties format_properties = get_device().get_gpu().get_handle().getFormatProperties(format);

	// Check if the selected format supports blit source and destination, which is required for generating the mip levels
	vk::FormatFeatureFlags format_feature_flags = vk::FormatFeatureFlagBits::eBlitSrc | vk::FormatFeatureFlagBits::eBlitDst;

	// If this is not supported you could implement a fallback via compute shader image writes and stores
	if ((format_properties.optimalTilingFeatures & format_feature_flags) != format_feature_flags)
	{
		throw std::runtime_error("Selected image format does not support blit source and destination");
	}
}

vk::DescriptorPool HPPTextureMipMapGeneration::create_descriptor_pool()
{
	// Example uses one ubo and one image sampler
	std::array<vk::DescriptorPoolSize, 3> pool_sizes = {
	    {{vk::DescriptorType::eUniformBuffer, 1}, {vk::DescriptorType::eSampledImage, 1}, {vk::DescriptorType::eSampler, 3}}};

	vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 2, pool_sizes);

	return get_device().get_handle().createDescriptorPool(descriptor_pool_create_info);
}

vk::DescriptorSetLayout HPPTextureMipMapGeneration::create_descriptor_set_layout()
{
	std::array<vk::DescriptorSetLayoutBinding, 3> set_layout_bindings = {
	    {{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},        // Binding 0 : Parameter uniform buffer
	     {1, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment},                                            // Binding 1 : Fragment shader image sampler
	     {2, vk::DescriptorType::eSampler, 3, vk::ShaderStageFlagBits::eFragment}}};                                               // Binding 2 : Sampler array (3 descriptors)

	vk::DescriptorSetLayoutCreateInfo descriptor_layout({}, set_layout_bindings);

	return get_device().get_handle().createDescriptorSetLayout(descriptor_layout);
}

vk::Pipeline HPPTextureMipMapGeneration::create_pipeline()
{
	// Load shaders
	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = {
	    load_shader("texture_mipmap_generation", "texture.vert", vk::ShaderStageFlagBits::eVertex),
	    load_shader("texture_mipmap_generation", "texture.frag", vk::ShaderStageFlagBits::eFragment)};

	// Vertex bindings and attributes
	vk::VertexInputBindingDescription                  vertex_input_binding(0, sizeof(HPPVertex), vk::VertexInputRate::eVertex);
	std::array<vk::VertexInputAttributeDescription, 2> vertex_input_attributes = {{
	    {0, 0, vk::Format::eR32G32B32Sfloat, 0},                     // Position
	    {1, 0, vk::Format::eR32G32Sfloat, sizeof(float) * 6},        // UV
	}};
	vk::PipelineVertexInputStateCreateInfo             vertex_input_state({}, vertex_input_binding, vertex_input_attributes);

	vk::PipelineColorBlendAttachmentState blend_attachment_state;
	blend_attachment_state.colorWriteMask =
	    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;
	depth_stencil_state.depthCompareOp   = vk::CompareOp::eLessOrEqual;
	depth_stencil_state.depthTestEnable  = true;
	depth_stencil_state.depthWriteEnable = true;
	depth_stencil_state.back.compareOp   = vk::CompareOp::eAlways;
	depth_stencil_state.front            = depth_stencil_state.back;

	return vkb::common::create_graphics_pipeline(get_device().get_handle(),
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
	scene = load_model("scenes/tunnel_cylinder.gltf");

	// Load the base texture containing only the first mip level and generate the whole mip-chain at runtime
	ktxTexture *ktx_texture = vkb::ktx::load_texture(vkb::fs::path::get(vkb::fs::path::Assets, "textures/checkerboard_rgba.ktx"));

	texture.extent = vk::Extent2D(ktx_texture->baseWidth, ktx_texture->baseHeight);

	// Calculate number of mip levels as per Vulkan specs:
	// numLevels = 1 + floor(log2(max(w, h, d)))
	texture.mip_levels = static_cast<uint32_t>(floor(log2(std::max(texture.extent.width, texture.extent.height))) + 1);

	// ktx1 doesn't know whether the content is sRGB or linear, but most tools save in sRGB, so assume that.
	constexpr vk::Format format = vk::Format::eR8G8B8A8Srgb;
	check_format_features(format);

	// Create a host-visible staging buffer that contains the raw image data
	vkb::core::HPPBuffer staging_buffer = vkb::core::HPPBuffer::create_staging_buffer(get_device(), ktx_texture->dataSize, ktx_texture->pData);

	// now, the ktx_texture can be destroyed
	ktxTexture_Destroy(ktx_texture);

	// Create optimal tiled target image on the device
	std::tie(texture.image, texture.device_memory) =
	    get_device().create_image(format,
	                              texture.extent,
	                              texture.mip_levels,
	                              vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled,
	                              vk::MemoryPropertyFlagBits::eDeviceLocal);

	vk::CommandBuffer copy_command = vkb::common::allocate_command_buffer(get_device().get_handle(), get_device().get_command_pool().get_handle());
	copy_command.begin(vk::CommandBufferBeginInfo());

	// Optimal image will be used as destination for the copy, so we must transfer from our initial undefined image layout to the transfer destination layout
	vkb::common::image_layout_transition(copy_command, texture.image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

	// Copy the first mip of the chain, remaining mips will be generated
	vk::BufferImageCopy buffer_copy_region({}, {}, {}, {vk::ImageAspectFlagBits::eColor, 0, 0, 1}, {}, vk::Extent3D(texture.extent, 1));
	copy_command.copyBufferToImage(staging_buffer.get_handle(), texture.image, vk::ImageLayout::eTransferDstOptimal, buffer_copy_region);

	// Transition first mip level to transfer source so we can blit(read) from it
	vkb::common::image_layout_transition(copy_command, texture.image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal);

	get_device().flush_command_buffer(copy_command, queue, true);

	// Generate the mip chain
	// ---------------------------------------------------------------
	// We copy down the whole mip chain doing a blit from mip-1 to mip
	// An alternative way would be to always blit from the first mip level and sample that one down
	vk::CommandBuffer blit_command = vkb::common::allocate_command_buffer(get_device().get_handle(), get_device().get_command_pool().get_handle());
	blit_command.begin(vk::CommandBufferBeginInfo());

	// Copy down mips from n-1 to n
	for (uint32_t i = 1; i < texture.mip_levels; i++)
	{
		vk::ImageBlit image_blit({vk::ImageAspectFlagBits::eColor, i - 1, 0, 1},
		                         {{{}, {static_cast<int32_t>(texture.extent.width >> (i - 1)), static_cast<int32_t>(texture.extent.height >> (i - 1)), static_cast<int32_t>(1)}}},
		                         {vk::ImageAspectFlagBits::eColor, i, 0, 1},
		                         {{{}, {static_cast<int32_t>(texture.extent.width >> i), static_cast<int32_t>(texture.extent.height >> i), static_cast<int32_t>(1)}}});

		// Prepare current mip level as image blit destination
		vk::ImageSubresourceRange image_subresource_range(vk::ImageAspectFlagBits::eColor, i, 1, 0, 1);
		vkb::common::image_layout_transition(
		    copy_command, texture.image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, image_subresource_range);

		// Blit from previous level
		blit_command.blitImage(texture.image, vk::ImageLayout::eTransferSrcOptimal, texture.image, vk::ImageLayout::eTransferDstOptimal, image_blit, vk::Filter::eLinear);

		// Prepare current mip level as image blit source for next level
		vkb::common::image_layout_transition(
		    copy_command, texture.image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, image_subresource_range);
	}

	// After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
	vkb::common::image_layout_transition(copy_command,
	                                     texture.image,
	                                     vk::ImageLayout::eTransferSrcOptimal,
	                                     vk::ImageLayout::eShaderReadOnlyOptimal,
	                                     {vk::ImageAspectFlagBits::eColor, 0, texture.mip_levels, 0, 1});

	get_device().flush_command_buffer(blit_command, queue, true);
	// ---------------------------------------------------------------

	// Create samplers for different mip map demonstration cases

	// Without mip mapping
	samplers[0] = vkb::common::create_sampler(get_device().get_gpu().get_handle(), get_device().get_handle(), format,
	                                          vk::Filter::eLinear, vk::SamplerAddressMode::eRepeat, 1.0f, 0.0f);

	// With mip mapping
	samplers[1] =
	    vkb::common::create_sampler(get_device().get_gpu().get_handle(), get_device().get_handle(), format,
	                                vk::Filter::eLinear, vk::SamplerAddressMode::eRepeat, 1.0f, static_cast<float>(texture.mip_levels));

	// With mip mapping and anisotropic filtering (when supported)
	samplers[2] = vkb::common::create_sampler(
	    get_device().get_gpu().get_handle(),
	    get_device().get_handle(),
	    format,
	    vk::Filter::eLinear,
	    vk::SamplerAddressMode::eRepeat,
	    get_device().get_gpu().get_features().samplerAnisotropy ? (get_device().get_gpu().get_properties().limits.maxSamplerAnisotropy) : 1.0f,
	    static_cast<float>(texture.mip_levels));

	// Create image view
	texture.view = vkb::common::create_image_view(
	    get_device().get_handle(), texture.image, vk::ImageViewType::e2D, format, vk::ImageAspectFlagBits::eColor, 0, texture.mip_levels);
}

void HPPTextureMipMapGeneration::prepare_camera()
{
	camera.type = vkb::CameraType::FirstPerson;
	camera.set_perspective(60.0f, static_cast<float>(extent.width) / static_cast<float>(extent.height), 0.1f, 1024.0f);
	camera.set_translation(glm::vec3(0.0f, 0.0f, -12.5f));
}

void HPPTextureMipMapGeneration::prepare_uniform_buffers()
{
	// Shared parameter uniform buffer block
	uniform_buffer = std::make_unique<vkb::core::HPPBuffer>(get_device(),
	                                                        sizeof(ubo),
	                                                        vk::BufferUsageFlagBits::eUniformBuffer,
	                                                        VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void HPPTextureMipMapGeneration::update_descriptor_set()
{
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

	get_device().get_handle().updateDescriptorSets(write_descriptor_sets, {});
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
