/* Copyright (c) 2023-2024, Holochip Corporation
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
 * Demonstrate and showcase a sample application using mesh shader culling features.
 */

#include "mesh_shader_culling.h"

MeshShaderCulling::MeshShaderCulling()
{
	title = "Mesh shader culling";

	// Configure application version
	set_api_version(VK_API_VERSION_1_1);

	// Adding device extensions
	add_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
	add_device_extension(VK_EXT_MESH_SHADER_EXTENSION_NAME);
	add_device_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
	// Targeting SPIR-V version
	vkb::GLSLCompiler::set_target_environment(glslang::EShTargetSpv, glslang::EShTargetSpv_1_4);
}

MeshShaderCulling::~MeshShaderCulling()
{
	if (has_device())
	{
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);

		if (query_pool != VK_NULL_HANDLE)
		{
			vkDestroyQueryPool(get_device().get_handle(), query_pool, nullptr);
			vkDestroyBuffer(get_device().get_handle(), query_result.buffer, nullptr);
			vkFreeMemory(get_device().get_handle(), query_result.memory, nullptr);
		}
	}
}

void MeshShaderCulling::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// Check whether the device supports task and mesh shaders
	VkPhysicalDeviceMeshShaderFeaturesEXT mesh_shader_features;
	mesh_shader_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
	mesh_shader_features.pNext = VK_NULL_HANDLE;
	VkPhysicalDeviceFeatures2 features2;
	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features2.pNext = &mesh_shader_features;
	vkGetPhysicalDeviceFeatures2(gpu.get_handle(), &features2);

	if (!mesh_shader_features.taskShader || !mesh_shader_features.meshShader)
	{
		throw vkb::VulkanException(VK_ERROR_FEATURE_NOT_PRESENT, "Selected GPU does not support task and mesh shaders!");
	}
	if (!mesh_shader_features.meshShaderQueries)
	{
		throw vkb::VulkanException(VK_ERROR_FEATURE_NOT_PRESENT, "Selected GPU does not support mesh shader queries!");
	}

	// Enable extension features required by this sample
	// These are passed to device creation via a pNext structure chain
	auto &meshFeatures = gpu.request_extension_features<VkPhysicalDeviceMeshShaderFeaturesEXT>(
	    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT);

	meshFeatures.taskShader = VK_TRUE;
	meshFeatures.meshShader = VK_TRUE;

	// Pipeline statistics
	auto &requested_features = gpu.get_mutable_requested_features();
	if (gpu.get_features().pipelineStatisticsQuery)
	{
		requested_features.pipelineStatisticsQuery = VK_TRUE;
	}
}

void MeshShaderCulling::build_command_buffers()
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
		render_pass_begin_info.framebuffer = framebuffers[i];
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));
		if (get_device().get_gpu().get_features().pipelineStatisticsQuery)
		{
			vkCmdResetQueryPool(draw_cmd_buffers[i], query_pool, 0, 3);
		}

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(static_cast<int32_t>(width), static_cast<int32_t>(height), 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		// Mesh shaders need the vkCmdDrawMeshTasksExt
		uint32_t N = density_level == 0 ? 4 : (density_level == 1 ? 6 : (density_level == 2 ? 8 : 2));
		// dispatch N * N task shader workgroups
		uint32_t num_workgroups_x = N;
		uint32_t num_workgroups_y = N;
		uint32_t num_workgroups_z = 1;

		if (get_device().get_gpu().get_features().pipelineStatisticsQuery)
		{
			// Begin pipeline statistics query
			vkCmdBeginQuery(draw_cmd_buffers[i], query_pool, 0, 0);
		}

		vkCmdDrawMeshTasksEXT(draw_cmd_buffers[i], num_workgroups_x, num_workgroups_y, num_workgroups_z);

		if (get_device().get_gpu().get_features().pipelineStatisticsQuery)
		{
			// Begin pipeline statistics query
			vkCmdEndQuery(draw_cmd_buffers[i], query_pool, 0);
		}

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void MeshShaderCulling::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)};

	uint32_t number_of_descriptor_sets = 1;

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()),
	                                                   pool_sizes.data(),
	                                                   number_of_descriptor_sets);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void MeshShaderCulling::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	                                                     VK_SHADER_STAGE_TASK_BIT_EXT,
	                                                     0)};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(&descriptor_set_layout, 1);

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void MeshShaderCulling::setup_descriptor_sets()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1);

	// Task shader descriptor set
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

	VkDescriptorBufferInfo uniform_buffer_descriptor = create_descriptor(*uniform_buffer);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(descriptor_set,
	                                            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	                                            0,
	                                            &uniform_buffer_descriptor)};

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

void MeshShaderCulling::prepare_pipelines()
{
	// Pipeline creation information
	VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(pipeline_layout, render_pass, 0);

	// Rasterization state
	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL,
	                                                                VK_CULL_MODE_NONE,
	                                                                VK_FRONT_FACE_COUNTER_CLOCKWISE,
	                                                                0);

	// Color blend state
	VkPipelineColorBlendAttachmentState blend_attachment =
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment);

	// Multisample state
	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

	// Viewport state
	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	// Depth stencil state
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(VK_FALSE, VK_TRUE, VK_COMPARE_OP_GREATER);        // Depth test should be disabled;

	// Dynamic state
	std::vector<VkDynamicState> dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(dynamic_state_enables.data(),
	                                                          static_cast<uint32_t>(dynamic_state_enables.size()),
	                                                          0);

	// Shader state
	std::vector<VkPipelineShaderStageCreateInfo> shader_stages{};

	shader_stages.push_back(load_shader("mesh_shader_culling/mesh_shader_culling.task", VK_SHADER_STAGE_TASK_BIT_EXT));
	shader_stages.push_back(load_shader("mesh_shader_culling/mesh_shader_culling.mesh", VK_SHADER_STAGE_MESH_BIT_EXT));
	shader_stages.push_back(load_shader("mesh_shader_culling/mesh_shader_culling.frag", VK_SHADER_STAGE_FRAGMENT_BIT));

	pipeline_create_info.pVertexInputState   = nullptr;
	pipeline_create_info.pInputAssemblyState = nullptr;
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

void MeshShaderCulling::prepare_uniform_buffers()
{
	uniform_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                     sizeof(ubo_cull),
	                                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                     VMA_MEMORY_USAGE_CPU_TO_GPU);
	update_uniform_buffers();
}

void MeshShaderCulling::update_uniform_buffers()
{
	uniform_buffer->convert_and_update(ubo_cull);
}

void MeshShaderCulling::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	if (get_device().get_gpu().get_features().pipelineStatisticsQuery)
	{
		// Read query results for displaying in next frame
		get_query_results();
	}

	ApiVulkanSample::submit_frame();
}

bool MeshShaderCulling::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	camera.type = vkb::CameraType::FirstPerson;
	camera.set_position(glm::vec3(1.0f, 0.0f, 1.0f));
	camera.rotation_speed  = 0.0f;
	ubo_cull.cull_center_x = -camera.position.x;
	ubo_cull.cull_center_y = -camera.position.z;

	if (get_device().get_gpu().get_features().pipelineStatisticsQuery)
	{
		setup_query_result_buffer();
	}

	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_sets();
	build_command_buffers();

	prepared = true;
	return true;
}

void MeshShaderCulling::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}

	draw();

	if (camera.keys.left || camera.keys.right || camera.keys.up || camera.keys.down)
	{
		ubo_cull.cull_center_x = -camera.position.x;
		ubo_cull.cull_center_y = -camera.position.z;
		update_uniform_buffers();
	}
}

void MeshShaderCulling::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Use WASD to move the square\n Configurations:\n"))
	{
		if (drawer.slider_float("Cull Radius: ", &ubo_cull.cull_radius, 0.5f, 2.0f))
		{
			update_uniform_buffers();
		}
		if (drawer.combo_box("Meshlet Density Level: ", &density_level, {"4 x 4", "6 x 6", "8 x 8"}))
		{
			ubo_cull.meshlet_density = static_cast<float>(density_level);
			update_uniform_buffers();
		}

		if (get_device().get_gpu().get_features().pipelineStatisticsQuery)
		{
			if (drawer.header("Pipeline statistics"))
			{
				drawer.text("TS invocations: %d", pipeline_stats[1]);
				drawer.text("MS invocations: %d", pipeline_stats[2]);
				drawer.text("FS invocations: %d", pipeline_stats[0]);
			}
		}
	}
}

bool MeshShaderCulling::resize(uint32_t width, uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	update_uniform_buffers();
	return true;
}

std::unique_ptr<vkb::VulkanSampleC> create_mesh_shader_culling()
{
	return std::make_unique<MeshShaderCulling>();
}

// Setup pool and buffer for storing pipeline statistics results
void MeshShaderCulling::setup_query_result_buffer()
{
	uint32_t buffer_size = 2 * sizeof(uint64_t);

	VkMemoryRequirements memory_requirements;
	VkMemoryAllocateInfo memory_allocation = vkb::initializers::memory_allocate_info();
	VkBufferCreateInfo   buffer_create_info =
	    vkb::initializers::buffer_create_info(
	        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	        buffer_size);

	// Results are saved in a host visible buffer for easy access by the application
	VK_CHECK(vkCreateBuffer(get_device().get_handle(), &buffer_create_info, nullptr, &query_result.buffer));
	vkGetBufferMemoryRequirements(get_device().get_handle(), query_result.buffer, &memory_requirements);
	memory_allocation.allocationSize  = memory_requirements.size;
	memory_allocation.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocation, nullptr, &query_result.memory));
	VK_CHECK(vkBindBufferMemory(get_device().get_handle(), query_result.buffer, query_result.memory, 0));

	// Create query pool
	if (get_device().get_gpu().get_features().pipelineStatisticsQuery)
	{
		VkQueryPoolCreateInfo query_pool_info = {};
		query_pool_info.sType                 = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		query_pool_info.queryType             = VK_QUERY_TYPE_PIPELINE_STATISTICS;
		query_pool_info.pipelineStatistics =
		    VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
		    VK_QUERY_PIPELINE_STATISTIC_TASK_SHADER_INVOCATIONS_BIT_EXT |
		    VK_QUERY_PIPELINE_STATISTIC_MESH_SHADER_INVOCATIONS_BIT_EXT;
		query_pool_info.queryCount = 3;
		VK_CHECK(vkCreateQueryPool(get_device().get_handle(), &query_pool_info, nullptr, &query_pool));
	}
}

// Retrieves the results of the pipeline statistics query submitted to the command buffer
void MeshShaderCulling::get_query_results()
{
	// We use vkGetQueryResults to copy the results into a host visible buffer
	vkGetQueryPoolResults(
	    get_device().get_handle(),
	    query_pool,
	    0,
	    1,
	    sizeof(pipeline_stats),
	    pipeline_stats,
	    sizeof(uint64_t),
	    VK_QUERY_RESULT_64_BIT);
}
