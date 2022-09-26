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
 * Demonstrates the use of dynamic uniform buffers, using vulkan.hpp
 *
 * Instead of using one uniform buffer per-object, this example allocates one big uniform buffer
 * with respect to the alignment reported by the device via minUniformBufferOffsetAlignment that
 * contains all matrices for the objects in the scene.
 *
 * The used descriptor type vk::DescriptorType::eUniformBufferDynamic then allows to set a dynamic
 * offset used to pass data from the single uniform buffer to the connected shader binding point.
 */

#include <hpp_dynamic_uniform_buffers.h>

#include <benchmark_mode/benchmark_mode.h>
#include <iostream>
#include <random>

HPPDynamicUniformBuffers::HPPDynamicUniformBuffers()
{
	title = "HPP Dynamic uniform buffers";
}

HPPDynamicUniformBuffers ::~HPPDynamicUniformBuffers()
{
	if (get_device() && get_device()->get_handle())
	{
		if (ubo_data_dynamic.model)
		{
			aligned_free(ubo_data_dynamic.model);
		}

		vk::Device device = get_device()->get_handle();

		// Clean up used Vulkan resources
		// Note : Inherited destructor cleans up resources stored in base class
		device.destroyPipeline(pipeline);

		device.destroyPipelineLayout(pipeline_layout);
		device.destroyDescriptorSetLayout(descriptor_set_layout);
	}
}

// Wrapper functions for aligned memory allocation
// There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this
void *HPPDynamicUniformBuffers::aligned_alloc(size_t size, size_t alignment)
{
	void *data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
	data = _aligned_malloc(size, alignment);
#else
	int res = posix_memalign(&data, alignment, size);
	if (res != 0)
		data = nullptr;
#endif
	return data;
}

void HPPDynamicUniformBuffers::aligned_free(void *data)
{
#if defined(_MSC_VER) || defined(__MINGW32__)
	_aligned_free(data);
#else
	free(data);
#endif
}

void HPPDynamicUniformBuffers::build_command_buffers()
{
	vk::CommandBufferBeginInfo command_buffer_begin_info;
	vk::Viewport               viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f);
	vk::Rect2D                 scissor({0, 0}, extent);
	vk::DeviceSize             offset = 0;

	std::array<vk::ClearValue, 2> clear_values = {default_clear_color, vk::ClearDepthStencilValue(0.0f, 0)};

	vk::RenderPassBeginInfo render_pass_begin_info(render_pass, {}, {{0, 0}, extent}, clear_values);

	for (size_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		render_pass_begin_info.framebuffer = framebuffers[i];

		draw_cmd_buffers[i].begin(command_buffer_begin_info);

		draw_cmd_buffers[i].beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

		draw_cmd_buffers[i].setViewport(0, viewport);

		draw_cmd_buffers[i].setScissor(0, scissor);

		draw_cmd_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

		draw_cmd_buffers[i].bindVertexBuffers(0, static_cast<vk::Buffer>(vertex_buffer->get_handle()), offset);
		draw_cmd_buffers[i].bindIndexBuffer(index_buffer->get_handle(), 0, vk::IndexType::eUint32);

		// Render multiple objects using different model matrices by dynamically offsetting into one uniform buffer
		for (uint32_t j = 0; j < OBJECT_INSTANCES; j++)
		{
			// One dynamic offset per dynamic descriptor to offset into the ubo containing all model matrices
			uint32_t dynamic_offset = j * static_cast<uint32_t>(dynamic_alignment);
			// Bind the descriptor set for rendering a mesh using the dynamic offset
			draw_cmd_buffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, descriptor_set, dynamic_offset);

			draw_cmd_buffers[i].drawIndexed(index_count, 1, 0, 0, 0);
		}

		draw_ui(draw_cmd_buffers[i]);

		draw_cmd_buffers[i].endRenderPass();

		draw_cmd_buffers[i].end();
	}
}

void HPPDynamicUniformBuffers::draw()
{
	HPPApiVulkanSample::prepare_frame();

	// Submit to queue
	submit_info.setCommandBuffers(draw_cmd_buffers[current_buffer]);
	queue.submit(submit_info);

	HPPApiVulkanSample::submit_frame();
}

void HPPDynamicUniformBuffers::generate_cube()
{
	// Setup vertices indices for a colored cube
	std::vector<Vertex> vertices = {
	    {{-1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
	    {{1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
	    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
	    {{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}},
	    {{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},
	    {{1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
	    {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},
	    {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}},
	};

	std::vector<uint32_t> indices = {
	    0,
	    1,
	    2,
	    2,
	    3,
	    0,
	    1,
	    5,
	    6,
	    6,
	    2,
	    1,
	    7,
	    6,
	    5,
	    5,
	    4,
	    7,
	    4,
	    0,
	    3,
	    3,
	    7,
	    4,
	    4,
	    5,
	    1,
	    1,
	    0,
	    4,
	    3,
	    2,
	    6,
	    6,
	    7,
	    3,
	};

	index_count = static_cast<uint32_t>(indices.size());

	auto vertex_buffer_size = vertices.size() * sizeof(Vertex);
	auto index_buffer_size  = indices.size() * sizeof(uint32_t);

	// Create buffers
	// For the sake of simplicity we won't stage the vertex data to the gpu memory
	// Vertex buffer
	vertex_buffer =
	    std::make_unique<vkb::core::HPPBuffer>(*get_device(), vertex_buffer_size, vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_GPU_TO_CPU);
	vertex_buffer->update(vertices.data(), vertex_buffer_size);

	index_buffer = std::make_unique<vkb::core::HPPBuffer>(*get_device(), index_buffer_size, vk::BufferUsageFlagBits::eIndexBuffer, VMA_MEMORY_USAGE_GPU_TO_CPU);
	index_buffer->update(indices.data(), index_buffer_size);
}

void HPPDynamicUniformBuffers::setup_descriptor_pool()
{
	// Example uses one ubo and one image sampler
	std::array<vk::DescriptorPoolSize, 3> pool_sizes = {
	    {{vk::DescriptorType::eUniformBuffer, 1}, {vk::DescriptorType::eUniformBufferDynamic, 1}, {vk::DescriptorType::eCombinedImageSampler, 1}}};

	vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 2, pool_sizes);

	descriptor_pool = get_device()->get_handle().createDescriptorPool(descriptor_pool_create_info);
}

void HPPDynamicUniformBuffers::setup_descriptor_set_layout()
{
	std::array<vk::DescriptorSetLayoutBinding, 3> set_layout_bindings = {
	    {{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
	     {1, vk::DescriptorType::eUniformBufferDynamic, 1, vk::ShaderStageFlagBits::eVertex},
	     {2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};

	vk::DescriptorSetLayoutCreateInfo descriptor_layout({}, set_layout_bindings);

	descriptor_set_layout = get_device()->get_handle().createDescriptorSetLayout(descriptor_layout);

#if defined(ANDROID)
	vk::PipelineLayoutCreateInfo pipeline_layout_create_info({}, 1, &descriptor_set_layout);
#else
	vk::PipelineLayoutCreateInfo pipeline_layout_create_info({}, descriptor_set_layout);
#endif

	pipeline_layout = get_device()->get_handle().createPipelineLayout(pipeline_layout_create_info);
}

void HPPDynamicUniformBuffers::setup_descriptor_set()
{
	vk::DescriptorSetAllocateInfo alloc_info(descriptor_pool, 1, &descriptor_set_layout);

	descriptor_set = get_device()->get_handle().allocateDescriptorSets(alloc_info).front();

	vk::DescriptorBufferInfo view_buffer_descriptor(uniform_buffers.view->get_handle(), 0, VK_WHOLE_SIZE);
	vk::DescriptorBufferInfo dynamic_buffer_descriptor(uniform_buffers.dynamic->get_handle(), 0, dynamic_alignment);

	std::array<vk::WriteDescriptorSet, 2> write_descriptor_sets = {
	    {// Binding 0 : Projection/View matrix uniform buffer
	     {descriptor_set, 0, {}, vk::DescriptorType::eUniformBuffer, {}, view_buffer_descriptor},
	     // Binding 1 : Instance matrix as dynamic uniform buffer
	     {descriptor_set, 1, {}, vk::DescriptorType::eUniformBufferDynamic, {}, dynamic_buffer_descriptor}}};

	get_device()->get_handle().updateDescriptorSets(write_descriptor_sets, {});
}

void HPPDynamicUniformBuffers::prepare_pipelines()
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

	// there's one viewport and one scissor, but they're dynamically set!
	vk::PipelineViewportStateCreateInfo viewport_state({}, 1, nullptr, 1, nullptr);

	vk::PipelineMultisampleStateCreateInfo multisample_state({}, vk::SampleCountFlagBits::e1);

	std::array<vk::DynamicState, 2>    dynamic_state_enables = {{vk::DynamicState::eViewport, vk::DynamicState::eScissor}};
	vk::PipelineDynamicStateCreateInfo dynamic_state({}, dynamic_state_enables);

	// Load shaders
	std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages = {{load_shader("dynamic_uniform_buffers/base.vert", vk::ShaderStageFlagBits::eVertex),
	                                                                   load_shader("dynamic_uniform_buffers/base.frag",
	                                                                               vk::ShaderStageFlagBits::eFragment)}};

	// Vertex bindings and attributes
	vk::VertexInputBindingDescription                  vertex_input_binding(0, sizeof(Vertex), vk::VertexInputRate::eVertex);
	std::array<vk::VertexInputAttributeDescription, 2> vertex_input_attributes = {
	    {{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)},            // Location 0 : Position
	     {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)}}};        // Location 1 : Color
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
void HPPDynamicUniformBuffers::prepare_uniform_buffers()
{
	// Allocate data for the dynamic uniform buffer object
	// We allocate this manually as the alignment of the offset differs between GPUs

	// Calculate required alignment based on minimum device offset alignment
	vk::DeviceSize min_ubo_alignment = get_device()->get_gpu().get_handle().getProperties().limits.minUniformBufferOffsetAlignment;
	dynamic_alignment                = sizeof(glm::mat4);
	if (min_ubo_alignment > 0)
	{
		dynamic_alignment = (dynamic_alignment + min_ubo_alignment - 1) & ~(min_ubo_alignment - 1);
	}

	size_t buffer_size = OBJECT_INSTANCES * dynamic_alignment;

	ubo_data_dynamic.model = (glm::mat4 *) aligned_alloc(buffer_size, dynamic_alignment);
	assert(ubo_data_dynamic.model);

	std::cout << "minUniformBufferOffsetAlignment = " << min_ubo_alignment << std::endl;
	std::cout << "dynamicAlignment = " << dynamic_alignment << std::endl;

	// Vertex shader uniform buffer block

	// Static shared uniform buffer object with projection and view matrix
	uniform_buffers.view =
	    std::make_unique<vkb::core::HPPBuffer>(*get_device(), sizeof(ubo_vs), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

	uniform_buffers.dynamic =
	    std::make_unique<vkb::core::HPPBuffer>(*get_device(), buffer_size, vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Prepare per-object matrices with offsets and random rotations
	std::default_random_engine      rnd_engine(platform->using_plugin<::plugins::BenchmarkMode>() ? 0 : (unsigned) time(nullptr));
	std::normal_distribution<float> rnd_dist(-1.0f, 1.0f);
	for (uint32_t i = 0; i < OBJECT_INSTANCES; i++)
	{
		rotations[i]       = glm::vec3(rnd_dist(rnd_engine), rnd_dist(rnd_engine), rnd_dist(rnd_engine)) * 2.0f * glm::pi<float>();
		rotation_speeds[i] = glm::vec3(rnd_dist(rnd_engine), rnd_dist(rnd_engine), rnd_dist(rnd_engine));
	}

	update_uniform_buffers();
	update_dynamic_uniform_buffer(0.0f, true);
}

void HPPDynamicUniformBuffers::update_uniform_buffers()
{
	// Fixed ubo with projection and view matrices
	ubo_vs.projection = camera.matrices.perspective;
	ubo_vs.view       = camera.matrices.view;

	uniform_buffers.view->convert_and_update(ubo_vs);
}

void HPPDynamicUniformBuffers::update_dynamic_uniform_buffer(float delta_time, bool force)
{
	// Update at max. 60 fps
	animation_timer += delta_time;
	if ((animation_timer + 0.0025 < (1.0f / 60.0f)) && (!force))
	{
		return;
	}

	// Dynamic ubo with per-object model matrices indexed by offsets in the command buffer
	auto      dim  = static_cast<uint32_t>(pow(OBJECT_INSTANCES, (1.0f / 3.0f)));
	auto      fdim = static_cast<float>(dim);
	glm::vec3 offset(5.0f);

	for (uint32_t x = 0; x < dim; x++)
	{
		auto fx = static_cast<float>(x);
		for (uint32_t y = 0; y < dim; y++)
		{
			auto fy = static_cast<float>(y);
			for (uint32_t z = 0; z < dim; z++)
			{
				auto fz    = static_cast<float>(z);
				auto index = x * dim * dim + y * dim + z;

				// Aligned offset
				auto model_mat = (glm::mat4 *) (((uint64_t) ubo_data_dynamic.model + (index * dynamic_alignment)));

				// Update rotations
				rotations[index] += animation_timer * rotation_speeds[index];

				// Update matrices
				glm::vec3 pos(-((fdim * offset.x) / 2.0f) + offset.x / 2.0f + fx * offset.x,
				              -((fdim * offset.y) / 2.0f) + offset.y / 2.0f + fy * offset.y,
				              -((fdim * offset.z) / 2.0f) + offset.z / 2.0f + fz * offset.z);
				*model_mat = glm::translate(glm::mat4(1.0f), pos);
				*model_mat = glm::rotate(*model_mat, rotations[index].x, glm::vec3(1.0f, 1.0f, 0.0f));
				*model_mat = glm::rotate(*model_mat, rotations[index].y, glm::vec3(0.0f, 1.0f, 0.0f));
				*model_mat = glm::rotate(*model_mat, rotations[index].z, glm::vec3(0.0f, 0.0f, 1.0f));
			}
		}
	}

	animation_timer = 0.0f;

	uniform_buffers.dynamic->update(ubo_data_dynamic.model, static_cast<size_t>(uniform_buffers.dynamic->get_size()));

	// Flush to make changes visible to the device
	uniform_buffers.dynamic->flush();
}

bool HPPDynamicUniformBuffers::prepare(vkb::platform::HPPPlatform &platform)
{
	if (!HPPApiVulkanSample::prepare(platform))
	{
		return false;
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position(glm::vec3(0.0f, 0.0f, -30.0f));
	camera.set_rotation(glm::vec3(0.0f));

	// Note: Using Revsered depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.set_perspective(60.0f, (float) extent.width / (float) extent.height, 256.0f, 0.1f);

	generate_cube();
	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_set();
	build_command_buffers();
	prepared = true;
	return true;
}

bool HPPDynamicUniformBuffers::resize(const uint32_t width, const uint32_t height)
{
	HPPApiVulkanSample::resize(width, height);
	update_uniform_buffers();
	return true;
}

void HPPDynamicUniformBuffers::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
	if (!paused)
	{
		update_dynamic_uniform_buffer(delta_time);
	}
	if (camera.updated)
	{
		update_uniform_buffers();
	}
}

std::unique_ptr<vkb::Application> create_hpp_dynamic_uniform_buffers()
{
	return std::make_unique<HPPDynamicUniformBuffers>();
}
