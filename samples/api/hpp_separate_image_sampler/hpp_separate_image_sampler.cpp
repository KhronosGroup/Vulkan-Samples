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
 * Separate samplers and image to draw a single image with different sampling options, using vulkan.hpp
 */

#include "hpp_separate_image_sampler.h"

HPPSeparateImageSampler::HPPSeparateImageSampler()
{
	zoom     = -0.5f;
	rotation = {45.0f, 0.0f, 0.0f};
	title    = "HPP Separate sampler and image";
}

HPPSeparateImageSampler::~HPPSeparateImageSampler()
{
	if (get_device() && get_device()->get_handle())
	{
		vk::Device device = get_device()->get_handle();

		// Clean up used Vulkan resources
		// Note : Inherited destructor cleans up resources stored in base class

		device.destroyPipeline(pipeline);
		device.destroyPipelineLayout(pipeline_layout);
		device.destroyDescriptorSetLayout(base_descriptor_set_layout);
		device.destroyDescriptorSetLayout(sampler_descriptor_set_layout);
		for (vk::Sampler sampler : samplers)
		{
			device.destroySampler(sampler);
		}
		// Delete the implicitly created sampler for the texture loaded via the framework
		device.destroySampler(texture.sampler);
	}
}

bool HPPSeparateImageSampler::prepare(vkb::platform::HPPPlatform &platform)
{
	if (!HPPApiVulkanSample::prepare(platform))
	{
		return false;
	}

	load_assets();
	generate_quad();
	prepare_uniform_buffers();
	setup_samplers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_set();
	build_command_buffers();
	prepared = true;
	return true;
}

// Enable physical device features required for this example
void HPPSeparateImageSampler::request_gpu_features(vkb::core::HPPPhysicalDevice &gpu)
{
	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = true;
	}
}

void HPPSeparateImageSampler::build_command_buffers()
{
	vk::CommandBufferBeginInfo    command_buffer_begin_info;
	std::array<vk::ClearValue, 2> clear_values = {{default_clear_color, vk::ClearDepthStencilValue(0.0f, 0)}};

	vk::RenderPassBeginInfo render_pass_begin_info(render_pass, {}, {{0, 0}, extent}, clear_values);

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

		// Bind the uniform buffer and sampled image to set 0
		draw_cmd_buffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, base_descriptor_set, {});
		// Bind the selected sampler to set 1
		draw_cmd_buffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 1, sampler_descriptor_sets[selected_sampler], {});
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

void HPPSeparateImageSampler::on_update_ui_overlay(vkb::HPPDrawer &drawer)
{
	if (drawer.header("Settings"))
	{
		const std::vector<std::string> sampler_names = {"Linear filtering", "Nearest filtering"};
		if (drawer.combo_box("Sampler", &selected_sampler, sampler_names))
		{
			update_uniform_buffers();
		}
	}
}

void HPPSeparateImageSampler::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
}

void HPPSeparateImageSampler::view_changed()
{
	update_uniform_buffers();
}

void HPPSeparateImageSampler::draw()
{
	HPPApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.setCommandBuffers(draw_cmd_buffers[current_buffer]);

	// Submit to queue
	queue.submit(submit_info);

	HPPApiVulkanSample::submit_frame();
}

void HPPSeparateImageSampler::generate_quad()
{
	// Setup vertices for a single uv-mapped quad made from two triangles
	std::vector<VertexStructure> vertices =
	    {
	        {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
	        {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
	        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
	        {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};

	// Setup indices
	std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};
	index_count                   = static_cast<uint32_t>(indices.size());

	auto vertex_buffer_size = vkb::to_u32(vertices.size() * sizeof(VertexStructure));
	auto index_buffer_size  = vkb::to_u32(indices.size() * sizeof(uint32_t));

	// Create buffers
	// For the sake of simplicity we won't stage the vertex data to the gpu memory
	// Vertex buffer
	vertex_buffer = std::make_unique<vkb::core::HPPBuffer>(*get_device(),
	                                                       vertex_buffer_size,
	                                                       vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
	                                                       VMA_MEMORY_USAGE_CPU_TO_GPU);
	vertex_buffer->update(vertices.data(), vertex_buffer_size);

	index_buffer = std::make_unique<vkb::core::HPPBuffer>(*get_device(),
	                                                      index_buffer_size,
	                                                      vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
	                                                      VMA_MEMORY_USAGE_CPU_TO_GPU);
	index_buffer->update(indices.data(), index_buffer_size);
}

void HPPSeparateImageSampler::load_assets()
{
	texture = load_texture("textures/metalplate01_rgba.ktx", vkb::sg::Image::Color);
}

void HPPSeparateImageSampler::prepare_pipelines()
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
	std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages = {{load_shader("separate_image_sampler/separate_image_sampler.vert", vk::ShaderStageFlagBits::eVertex),
	                                                                   load_shader("separate_image_sampler/separate_image_sampler.frag", vk::ShaderStageFlagBits::eFragment)}};

	// Vertex bindings and attributes
	vk::VertexInputBindingDescription                  vertex_input_binding(0, sizeof(VertexStructure), vk::VertexInputRate::eVertex);
	std::array<vk::VertexInputAttributeDescription, 3> vertex_input_attributes = {
	    {{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexStructure, pos)},             // Location 0 : Position
	     {1, 0, vk::Format::eR32G32Sfloat, offsetof(VertexStructure, uv)},                 // Location 1: Texture Coordinates
	     {2, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexStructure, normal)}}};        // Location 2 : Normal
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

	vk::Result result;
	std::tie(result, pipeline) = get_device()->get_handle().createGraphicsPipeline(pipeline_cache, pipeline_create_info);
	assert(result == vk::Result::eSuccess);
}

// Prepare and initialize uniform buffer containing shader uniforms
void HPPSeparateImageSampler::prepare_uniform_buffers()
{
	// Vertex shader uniform buffer block
	uniform_buffer_vs = std::make_unique<vkb::core::HPPBuffer>(*get_device(),
	                                                           sizeof(ubo_vs),
	                                                           vk::BufferUsageFlagBits::eUniformBuffer,
	                                                           VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void HPPSeparateImageSampler::setup_descriptor_pool()
{
	std::array<vk::DescriptorPoolSize, 3> pool_sizes = {{{vk::DescriptorType::eUniformBuffer, 1}, {vk::DescriptorType::eSampledImage, 1}, {vk::DescriptorType::eSampler, 2}}};

	vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 3, pool_sizes);

	descriptor_pool = get_device()->get_handle().createDescriptorPool(descriptor_pool_create_info);
}

void HPPSeparateImageSampler::setup_descriptor_set()
{
	// We separate the descriptor sets for the uniform buffer + image and samplers, so we don't need to duplicate the descriptors for the former

	// Descriptors set for the uniform buffer and the image
#if defined(ANDROID)
	vk::DescriptorSetAllocateInfo descriptor_set_alloc_info(descriptor_pool, 1, &base_descriptor_set_layout);
#else
	vk::DescriptorSetAllocateInfo descriptor_set_alloc_info(descriptor_pool, base_descriptor_set_layout);
#endif
	base_descriptor_set = get_device()->get_handle().allocateDescriptorSets(descriptor_set_alloc_info).front();

	vk::DescriptorBufferInfo buffer_descriptor(uniform_buffer_vs->get_handle(), 0, VK_WHOLE_SIZE);

	// Image info only references the image
	vk::DescriptorImageInfo image_info({}, texture.image->get_vk_image_view().get_handle(), vk::ImageLayout::eShaderReadOnlyOptimal);

	// Sampled image descriptor
	vk::WriteDescriptorSet image_write_descriptor_set(base_descriptor_set, 1, 0, vk::DescriptorType::eSampledImage, image_info);

	std::array<vk::WriteDescriptorSet, 2> write_descriptor_sets = {{
	    {base_descriptor_set, 0, 0, vk::DescriptorType::eUniformBuffer, {}, buffer_descriptor},        // Binding 0 : Vertex shader uniform buffer
	    image_write_descriptor_set                                                                     // Binding 1 : Fragment shader sampled image
	}};
	get_device()->get_handle().updateDescriptorSets(write_descriptor_sets, {});

	// Sets for each of the sampler
	descriptor_set_alloc_info.pSetLayouts = &sampler_descriptor_set_layout;
	for (size_t i = 0; i < sampler_descriptor_sets.size(); i++)
	{
		sampler_descriptor_sets[i] = get_device()->get_handle().allocateDescriptorSets(descriptor_set_alloc_info).front();

		// Descriptor info only references the sampler
		vk::DescriptorImageInfo sampler_info(samplers[i]);

		vk::WriteDescriptorSet sampler_write_descriptor_set(sampler_descriptor_sets[i], 0, 0, vk::DescriptorType::eSampler, sampler_info);

		get_device()->get_handle().updateDescriptorSets(sampler_write_descriptor_set, {});
	}
}

void HPPSeparateImageSampler::setup_descriptor_set_layout()
{
	// We separate the descriptor sets for the uniform buffer + image and samplers, so we don't need to duplicate the descriptors for the former
	// Set layout for the uniform buffer and the image
	std::array<vk::DescriptorSetLayoutBinding, 2> set_layout_bindings_buffer_and_image = {{
	    {0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},        // Binding 0 : Vertex shader uniform buffer
	    {1, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment}        // Binding 1 : Fragment shader sampled image
	}};
	vk::DescriptorSetLayoutCreateInfo             descriptor_layout_create_info({}, set_layout_bindings_buffer_and_image);
	base_descriptor_set_layout = get_device()->get_handle().createDescriptorSetLayout(descriptor_layout_create_info);

	// Set layout for the samplers
	vk::DescriptorSetLayoutBinding set_layout_binding_sampler(0, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment);
	descriptor_layout_create_info.setBindings(set_layout_binding_sampler);
	sampler_descriptor_set_layout = get_device()->get_handle().createDescriptorSetLayout(descriptor_layout_create_info);

	// Pipeline layout
	// Set layout for the base descriptors in set 0 and set layout for the sampler descriptors in set 1
	std::array<vk::DescriptorSetLayout, 2> set_layouts = {{base_descriptor_set_layout, sampler_descriptor_set_layout}};
	vk::PipelineLayoutCreateInfo           pipeline_layout_create_info({}, set_layouts);
	pipeline_layout = get_device()->get_handle().createPipelineLayout(pipeline_layout_create_info);
}

void HPPSeparateImageSampler::setup_samplers()
{
	// Create two samplers with different options
	vk::SamplerCreateInfo sampler_create_info;
	sampler_create_info.magFilter    = vk::Filter::eLinear;
	sampler_create_info.minFilter    = vk::Filter::eLinear;
	sampler_create_info.mipmapMode   = vk::SamplerMipmapMode::eLinear;
	sampler_create_info.addressModeU = vk::SamplerAddressMode::eRepeat;
	sampler_create_info.addressModeV = vk::SamplerAddressMode::eRepeat;
	sampler_create_info.addressModeW = vk::SamplerAddressMode::eRepeat;
	sampler_create_info.mipLodBias   = 0.0f;
	sampler_create_info.compareOp    = vk::CompareOp::eNever;
	sampler_create_info.minLod       = 0.0f;
	// Set max level-of-detail to mip level count of the texture
	sampler_create_info.maxLod = static_cast<float>(texture.image->get_mipmaps().size());
	// Enable anisotropic filtering
	// This feature is optional, so we must check if it's supported on the device
	if (get_device()->get_gpu().get_features().samplerAnisotropy)
	{
		// Use max. level of anisotropy for this example
		sampler_create_info.maxAnisotropy    = get_device()->get_gpu().get_properties().limits.maxSamplerAnisotropy;
		sampler_create_info.anisotropyEnable = true;
	}
	else
	{
		// The device does not support anisotropic filtering
		sampler_create_info.maxAnisotropy    = 1.0;
		sampler_create_info.anisotropyEnable = false;
	}
	sampler_create_info.borderColor = vk::BorderColor::eFloatOpaqueWhite;

	// First sampler with linear filtering
	samplers[0] = get_device()->get_handle().createSampler(sampler_create_info);

	// Second sampler with nearest filtering
	sampler_create_info.magFilter = vk::Filter::eNearest;
	sampler_create_info.minFilter = vk::Filter::eNearest;
	samplers[1]                   = get_device()->get_handle().createSampler(sampler_create_info);
}

void HPPSeparateImageSampler::update_uniform_buffers()
{
	// Vertex shader
	ubo_vs.projection     = glm::perspective(glm::radians(60.0f), static_cast<float>(extent.width) / static_cast<float>(extent.height), 0.001f, 256.0f);
	glm::mat4 view_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zoom));

	ubo_vs.model = view_matrix * glm::translate(glm::mat4(1.0f), camera_pos);
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	ubo_vs.view_pos = glm::vec4(0.0f, 0.0f, -zoom, 0.0f);

	uniform_buffer_vs->convert_and_update(ubo_vs);
}

std::unique_ptr<vkb::Application> create_hpp_separate_image_sampler()
{
	return std::make_unique<HPPSeparateImageSampler>();
}
