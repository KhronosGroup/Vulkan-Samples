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
 * Separate samplers and image to draw a single image with different sampling options, using vulkan.hpp
 */

#include "hpp_separate_image_sampler.h"

HPPSeparateImageSampler::HPPSeparateImageSampler()
{
	title = "HPP Separate sampler and image";

	zoom     = -0.5f;
	rotation = {45.0f, 0.0f, 0.0f};
}

HPPSeparateImageSampler::~HPPSeparateImageSampler()
{
	if (has_device() && get_device().get_handle())
	{
		vk::Device device = get_device().get_handle();

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

bool HPPSeparateImageSampler::prepare(const vkb::ApplicationOptions &options)
{
	assert(!prepared);

	if (HPPApiVulkanSample::prepare(options))
	{
		load_assets();
		generate_quad();
		prepare_uniform_buffers();

		// Create two samplers with different options, first one with linear filtering, the second one with nearest filtering
		samplers = {{create_sampler(vk::Filter::eLinear), create_sampler(vk::Filter::eNearest)}};

		descriptor_pool = create_descriptor_pool();

		// We separate the descriptor sets for the uniform buffer + image and samplers, so we don't need to duplicate the descriptors for the former
		// Descriptors set for the uniform buffer and the image
		base_descriptor_set_layout = create_base_descriptor_set_layout();
		base_descriptor_set        = vkb::common::allocate_descriptor_set(get_device().get_handle(), descriptor_pool, base_descriptor_set_layout);
		update_base_descriptor_set();

		// Sets for each of the sampler
		sampler_descriptor_set_layout = create_sampler_descriptor_set_layout();
		for (size_t i = 0; i < sampler_descriptor_sets.size(); i++)
		{
			sampler_descriptor_sets[i] = vkb::common::allocate_descriptor_set(get_device().get_handle(), descriptor_pool, sampler_descriptor_set_layout);
			update_sampler_descriptor_set(i);
		}

		// Pipeline layout
		// Set layout for the base descriptors in set 0 and set layout for the sampler descriptors in set 1
		pipeline_layout = create_pipeline_layout({base_descriptor_set_layout, sampler_descriptor_set_layout});
		pipeline        = create_graphics_pipeline();

		build_command_buffers();

		prepared = true;
	}

	return prepared;
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

		auto command_buffer = draw_cmd_buffers[i];
		command_buffer.begin(command_buffer_begin_info);

		command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

		vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f);
		command_buffer.setViewport(0, viewport);

		vk::Rect2D scissor({0, 0}, extent);
		command_buffer.setScissor(0, scissor);

		// Bind the uniform buffer and sampled image to set 0
		command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, base_descriptor_set, {});
		// Bind the selected sampler to set 1
		command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 1, sampler_descriptor_sets[selected_sampler], {});
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

void HPPSeparateImageSampler::on_update_ui_overlay(vkb::Drawer &drawer)
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
	if (prepared)
	{
		draw();
	}
}

void HPPSeparateImageSampler::view_changed()
{
	update_uniform_buffers();
}

vk::DescriptorSetLayout HPPSeparateImageSampler::create_base_descriptor_set_layout()
{
	// Set layout for the uniform buffer and the image
	std::array<vk::DescriptorSetLayoutBinding, 2> set_layout_bindings_buffer_and_image = {{
	    {0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},        // Binding 0 : Vertex shader uniform buffer
	    {1, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment}        // Binding 1 : Fragment shader sampled image
	}};

	vk::DescriptorSetLayoutCreateInfo descriptor_layout_create_info({}, set_layout_bindings_buffer_and_image);

	return get_device().get_handle().createDescriptorSetLayout(descriptor_layout_create_info);
}

vk::DescriptorPool HPPSeparateImageSampler::create_descriptor_pool()
{
	std::array<vk::DescriptorPoolSize, 3> pool_sizes = {
	    {{vk::DescriptorType::eUniformBuffer, 1}, {vk::DescriptorType::eSampledImage, 1}, {vk::DescriptorType::eSampler, 2}}};

	vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 3, pool_sizes);

	return get_device().get_handle().createDescriptorPool(descriptor_pool_create_info);
}

vk::Pipeline HPPSeparateImageSampler::create_graphics_pipeline()
{
	// Load shaders
	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = {
	    load_shader("separate_image_sampler/separate_image_sampler.vert", vk::ShaderStageFlagBits::eVertex),
	    load_shader("separate_image_sampler/separate_image_sampler.frag", vk::ShaderStageFlagBits::eFragment)};

	// Vertex bindings and attributes
	vk::VertexInputBindingDescription                  input_binding(0, sizeof(VertexStructure), vk::VertexInputRate::eVertex);
	std::array<vk::VertexInputAttributeDescription, 3> input_attributes = {
	    {{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexStructure, pos)},             // Location 0 : Position
	     {1, 0, vk::Format::eR32G32Sfloat, offsetof(VertexStructure, uv)},                 // Location 1: Texture Coordinates
	     {2, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexStructure, normal)}}};        // Location 2 : Normal
	vk::PipelineVertexInputStateCreateInfo input_state({}, input_binding, input_attributes);

	vk::PipelineColorBlendAttachmentState blend_attachment_state;
	blend_attachment_state.colorWriteMask =
	    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	// Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;
	depth_stencil_state.depthTestEnable  = true;
	depth_stencil_state.depthWriteEnable = true;
	depth_stencil_state.depthCompareOp   = vk::CompareOp::eGreater;
	depth_stencil_state.back.compareOp   = vk::CompareOp::eGreater;

	return vkb::common::create_graphics_pipeline(get_device().get_handle(),
	                                             pipeline_cache,
	                                             shader_stages,
	                                             input_state,
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

vk::PipelineLayout HPPSeparateImageSampler::create_pipeline_layout(std::vector<vk::DescriptorSetLayout> const &descriptor_set_layouts)
{
	vk::PipelineLayoutCreateInfo pipeline_layout_create_info({}, descriptor_set_layouts);
	return get_device().get_handle().createPipelineLayout(pipeline_layout_create_info);
}

vk::Sampler HPPSeparateImageSampler::create_sampler(vk::Filter filter)
{
	return vkb::common::create_sampler(
	    get_device().get_handle(),
	    filter,
	    vk::SamplerAddressMode::eRepeat,
	    get_device().get_gpu().get_features().samplerAnisotropy ? (get_device().get_gpu().get_properties().limits.maxSamplerAnisotropy) : 1.0f,
	    static_cast<float>(texture.image->get_mipmaps().size()));
}

vk::DescriptorSetLayout HPPSeparateImageSampler::create_sampler_descriptor_set_layout()
{
	// Set layout for the samplers
	vk::DescriptorSetLayoutBinding set_layout_binding_sampler(0, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment);

	vk::DescriptorSetLayoutCreateInfo descriptor_layout_create_info({}, set_layout_binding_sampler);

	return get_device().get_handle().createDescriptorSetLayout(descriptor_layout_create_info);
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
	vertex_buffer = std::make_unique<vkb::core::HPPBuffer>(get_device(),
	                                                       vertex_buffer_size,
	                                                       vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
	                                                       VMA_MEMORY_USAGE_CPU_TO_GPU);
	vertex_buffer->update(vertices.data(), vertex_buffer_size);

	index_buffer = std::make_unique<vkb::core::HPPBuffer>(get_device(),
	                                                      index_buffer_size,
	                                                      vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
	                                                      VMA_MEMORY_USAGE_CPU_TO_GPU);
	index_buffer->update(indices.data(), index_buffer_size);
}

void HPPSeparateImageSampler::load_assets()
{
	texture = load_texture("textures/metalplate01_rgba.ktx", vkb::scene_graph::components::HPPImage::Color);
}

// Prepare and initialize uniform buffer containing shader uniforms
void HPPSeparateImageSampler::prepare_uniform_buffers()
{
	// Vertex shader uniform buffer block
	uniform_buffer_vs =
	    std::make_unique<vkb::core::HPPBuffer>(get_device(), sizeof(ubo_vs), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void HPPSeparateImageSampler::update_base_descriptor_set()
{
	vk::DescriptorBufferInfo buffer_descriptor(uniform_buffer_vs->get_handle(), 0, VK_WHOLE_SIZE);

	// Image info only references the image
	vk::DescriptorImageInfo image_info({}, texture.image->get_vk_image_view().get_handle(), vk::ImageLayout::eShaderReadOnlyOptimal);

	// Sampled image descriptor
	vk::WriteDescriptorSet image_write_descriptor_set(base_descriptor_set, 1, 0, vk::DescriptorType::eSampledImage, image_info);

	std::array<vk::WriteDescriptorSet, 2> write_descriptor_sets = {{
	    {base_descriptor_set, 0, 0, vk::DescriptorType::eUniformBuffer, {}, buffer_descriptor},        // Binding 0 : Vertex shader uniform buffer
	    image_write_descriptor_set                                                                     // Binding 1 : Fragment shader sampled image
	}};
	get_device().get_handle().updateDescriptorSets(write_descriptor_sets, {});
}

void HPPSeparateImageSampler::update_sampler_descriptor_set(size_t index)
{
	assert((index < samplers.size()) && (index < sampler_descriptor_sets.size()));

	// Descriptor info only references the sampler
	vk::DescriptorImageInfo sampler_info(samplers[index]);

	vk::WriteDescriptorSet sampler_write_descriptor_set(sampler_descriptor_sets[index], 0, 0, vk::DescriptorType::eSampler, sampler_info);

	get_device().get_handle().updateDescriptorSets(sampler_write_descriptor_set, {});
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
