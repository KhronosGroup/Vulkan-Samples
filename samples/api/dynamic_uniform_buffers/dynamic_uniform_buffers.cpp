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
 * Demonstrates the use of dynamic uniform buffers.
 *
 * Instead of using one uniform buffer per-object, this example allocates one big uniform buffer
 * with respect to the alignment reported by the device via minUniformBufferOffsetAlignment that
 * contains all matrices for the objects in the scene.
 *
 * The used descriptor type VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC then allows to set a dynamic
 * offset used to pass data from the single uniform buffer to the connected shader binding point.
 */

#include "dynamic_uniform_buffers.h"

#include "benchmark_mode/benchmark_mode.h"

DynamicUniformBuffers::DynamicUniformBuffers()
{
	title = "Dynamic uniform buffers";
}

DynamicUniformBuffers ::~DynamicUniformBuffers()
{
	if (has_device())
	{
		if (ubo_data_dynamic.model)
		{
			aligned_free(ubo_data_dynamic.model);
		}

		// Clean up used Vulkan resources
		// Note : Inherited destructor cleans up resources stored in base class
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);

		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
	}
}

// Wrapper functions for aligned memory allocation
// There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this
void *DynamicUniformBuffers::aligned_alloc(size_t size, size_t alignment)
{
	void *data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
	data = _aligned_malloc(size, alignment);
#else
	int res = posix_memalign(&data, alignment, size);
	if (res != 0)
	{
		data = nullptr;
	}
#endif
	return data;
}

void DynamicUniformBuffers::aligned_free(void *data)
{
#if defined(_MSC_VER) || defined(__MINGW32__)
	_aligned_free(data);
#else
	free(data);
#endif
}

void DynamicUniformBuffers::build_command_buffers()
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

	for (int32_t i = 0; i < static_cast<int32_t>(draw_cmd_buffers.size()); ++i)
	{
		render_pass_begin_info.framebuffer = framebuffers[i];

		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		VkDeviceSize offsets[1] = {0};
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, vertex_buffer->get(), offsets);
		vkCmdBindIndexBuffer(draw_cmd_buffers[i], index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);

		// Render multiple objects using different model matrices by dynamically offsetting into one uniform buffer
		for (uint32_t j = 0; j < OBJECT_INSTANCES; j++)
		{
			// One dynamic offset per dynamic descriptor to offset into the ubo containing all model matrices
			uint32_t dynamic_offset = j * static_cast<uint32_t>(dynamic_alignment);
			// Bind the descriptor set for rendering a mesh using the dynamic offset
			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 1, &dynamic_offset);

			vkCmdDrawIndexed(draw_cmd_buffers[i], index_count, 1, 0, 0, 0);
		}

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void DynamicUniformBuffers::draw()
{
	ApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();
}

void DynamicUniformBuffers::generate_cube()
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
	vertex_buffer = std::make_unique<vkb::core::BufferC>(get_device(),
	                                                     vertex_buffer_size,
	                                                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                                                     VMA_MEMORY_USAGE_CPU_TO_GPU);
	vertex_buffer->update(vertices.data(), vertex_buffer_size);

	index_buffer = std::make_unique<vkb::core::BufferC>(get_device(),
	                                                    index_buffer_size,
	                                                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	                                                    VMA_MEMORY_USAGE_CPU_TO_GPU);
	index_buffer->update(indices.data(), index_buffer_size);
}

void DynamicUniformBuffers::setup_descriptor_pool()
{
	// Example uses one ubo and one image sampler
	std::vector<VkDescriptorPoolSize> pool_sizes =
	    {
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1),
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(
	        static_cast<uint32_t>(pool_sizes.size()),
	        pool_sizes.data(),
	        2);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void DynamicUniformBuffers::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings =
	    {
	        vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
	        vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT, 1),
	        vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2)};

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

void DynamicUniformBuffers::setup_descriptor_set()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layout,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

	VkDescriptorBufferInfo view_buffer_descriptor = create_descriptor(*uniform_buffers.view);

	// Pass the  actual dynamic alignment as the descriptor's size
	VkDescriptorBufferInfo dynamic_buffer_descriptor = create_descriptor(*uniform_buffers.dynamic, dynamic_alignment);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    // Binding 0 : Projection/View matrix uniform buffer
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &view_buffer_descriptor),
	    // Binding 1 : Instance matrix as dynamic uniform buffer
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, &dynamic_buffer_descriptor),
	};

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);
}

void DynamicUniformBuffers::prepare_pipelines()
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

	shader_stages[0] = load_shader("dynamic_uniform_buffers", "base.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("dynamic_uniform_buffers", "base.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Vertex bindings and attributes
	const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)),          // Location 0 : Position
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)),        // Location 1 : Color
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
void DynamicUniformBuffers::prepare_uniform_buffers()
{
	// Allocate data for the dynamic uniform buffer object
	// We allocate this manually as the alignment of the offset differs between GPUs

	// Calculate required alignment based on minimum device offset alignment
	size_t min_ubo_alignment = static_cast<size_t>(get_device().get_gpu().get_properties().limits.minUniformBufferOffsetAlignment);
	dynamic_alignment        = sizeof(glm::mat4);
	if (min_ubo_alignment > 0)
	{
		dynamic_alignment = (dynamic_alignment + min_ubo_alignment - 1) & ~(min_ubo_alignment - 1);
	}

	size_t buffer_size = OBJECT_INSTANCES * dynamic_alignment;

	ubo_data_dynamic.model = static_cast<glm::mat4 *>(aligned_alloc(buffer_size, dynamic_alignment));
	assert(ubo_data_dynamic.model);

	std::cout << "minUniformBufferOffsetAlignment = " << min_ubo_alignment << std::endl;
	std::cout << "dynamicAlignment = " << dynamic_alignment << std::endl;

	// Vertex shader uniform buffer block

	// Static shared uniform buffer object with projection and view matrix
	uniform_buffers.view = std::make_unique<vkb::core::BufferC>(get_device(),
	                                                            sizeof(ubo_vs),
	                                                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                            VMA_MEMORY_USAGE_CPU_TO_GPU);

	uniform_buffers.dynamic = std::make_unique<vkb::core::BufferC>(get_device(),
	                                                               buffer_size,
	                                                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                               VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Prepare per-object matrices with offsets and random rotations
	std::default_random_engine      rnd_engine(lock_simulation_speed ? 0 : static_cast<unsigned>(time(nullptr)));
	std::normal_distribution<float> rnd_dist(-1.0f, 1.0f);
	for (uint32_t i = 0; i < OBJECT_INSTANCES; i++)
	{
		rotations[i]       = glm::vec3(rnd_dist(rnd_engine), rnd_dist(rnd_engine), rnd_dist(rnd_engine)) * 2.0f * glm::pi<float>();
		rotation_speeds[i] = glm::vec3(rnd_dist(rnd_engine), rnd_dist(rnd_engine), rnd_dist(rnd_engine));
	}

	update_uniform_buffers();
	update_dynamic_uniform_buffer(0.0f, true);
}

void DynamicUniformBuffers::update_uniform_buffers()
{
	// Fixed ubo with projection and view matrices
	ubo_vs.projection = camera.matrices.perspective;
	ubo_vs.view       = camera.matrices.view;

	uniform_buffers.view->convert_and_update(ubo_vs);
}

void DynamicUniformBuffers::update_dynamic_uniform_buffer(float delta_time, bool force)
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

bool DynamicUniformBuffers::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position(glm::vec3(0.0f, 0.0f, -30.0f));
	camera.set_rotation(glm::vec3(0.0f));

	// Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 256.0f, 0.1f);

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

bool DynamicUniformBuffers::resize(const uint32_t width, const uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	update_uniform_buffers();
	return true;
}

void DynamicUniformBuffers::render(float delta_time)
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

std::unique_ptr<vkb::Application> create_dynamic_uniform_buffers()
{
	return std::make_unique<DynamicUniformBuffers>();
}
