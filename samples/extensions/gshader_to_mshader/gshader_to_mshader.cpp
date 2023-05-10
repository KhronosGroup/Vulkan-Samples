/* Copyright (c) 2023, Mobica Limited
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

#include "gshader_to_mshader.h"

GshaderToMshader::GshaderToMshader()
{
	title = "task_mesh_migration";

	set_api_version(VK_API_VERSION_1_1);
	add_device_extension(VK_EXT_MESH_SHADER_EXTENSION_NAME);
	add_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
	add_device_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
	vkb::GLSLCompiler::set_target_environment(glslang::EShTargetSpv, glslang::EShTargetSpv_1_4);
}

GshaderToMshader::~GshaderToMshader()
{
	if (device)
	{
		uniform_buffer_vs.reset();
		uniform_buffer_gs.reset();
		uniform_buffer_ms.reset();
		object.reset();
		storage_buffer_object.reset();

		vkDestroyPipeline(get_device().get_handle(), model_pipeline, nullptr);
		vkDestroyPipeline(get_device().get_handle(), geometry_pipeline, nullptr);
		vkDestroyPipeline(get_device().get_handle(), mesh_pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
	}
}

bool GshaderToMshader::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position({0.0f, 0.0f, -10.0f});
	camera.set_rotation({0.0f, 0.0f, 0.0f});
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 256.f, 0.1f);
	camera.translation_speed = 20.0f;

	load_assets();
	prepare_uniform_buffers();

	setup_descriptor_set_layout();
	prepare_pipelines();

	setup_descriptor_pool();
	setup_descriptor_sets();
	build_command_buffers();
	prepared = true;
	return true;
}

/**
 * 	@fn void VertexDynamicState::load_assets()
 *	@brief Loading extra models, textures from assets
 */
void GshaderToMshader::load_assets()
{
	/* Models */

	object = load_model("scenes/teapot.gltf");

	storage_buffer_object = load_model("scenes/teapot.gltf", 0, true);
}

/**
 * 	@fn void VertexDynamicState::draw()
 *  @brief Preparing frame and submitting it to the present queue
 */
void GshaderToMshader::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

/**
 * 	@fn void VertexDynamicState::render(float delta_time)
 * 	@brief Drawing frames and/or updating uniform buffers when camera position/rotation was changed
 */
void GshaderToMshader::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
	if (camera.updated)
	{
		update_uniform_buffers();
	}
}

/**
 * 	@fn void VertexDynamicState::prepare_uniform_buffers()
 * 	@brief Preparing uniform buffer (setting bits) and updating UB data
 */
void GshaderToMshader::prepare_uniform_buffers()
{
	uniform_buffer_vs = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(ubos[0]), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	uniform_buffer_gs = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(ubos[1]), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	uniform_buffer_ms = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(ubos[2]), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	update_uniform_buffers();
}

/**
 * 	@fn void VertexDynamicState::update_uniform_buffers()
 * 	@brief Updating data from application to GPU uniform buffer
 */
void GshaderToMshader::update_uniform_buffers()
{
	for (auto &ubo : ubos)
	{
		ubo.proj  = camera.matrices.perspective;
		ubo.view  = camera.matrices.view;
		ubo.model = glm::mat4(1.f);
		ubo.model = glm::rotate(ubo.model, glm::pi<float>(), glm::vec3(0, 0, 1));
	}
	uniform_buffer_vs->convert_and_update(ubos[0]);
	uniform_buffer_gs->convert_and_update(ubos[1]);
	uniform_buffer_ms->convert_and_update(ubos[2]);
}

void GshaderToMshader::prepare_pipelines()
{
	std::array<VkPipelineShaderStageCreateInfo, 2> model_stages;
	model_stages[0] = load_shader("gshader_to_mshader/gshader_to_mshader.vert", VK_SHADER_STAGE_VERTEX_BIT);
	model_stages[1] = load_shader("gshader_to_mshader/gshader_to_mshader.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	std::array<VkPipelineShaderStageCreateInfo, 3> geometry_stages;
	geometry_stages[0] = load_shader("gshader_to_mshader/gshader_to_mshader_base.vert", VK_SHADER_STAGE_VERTEX_BIT);
	geometry_stages[1] = load_shader("gshader_to_mshader/gshader_to_mshader_base.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	geometry_stages[2] = load_shader("gshader_to_mshader/gshader_to_mshader.geom", VK_SHADER_STAGE_GEOMETRY_BIT);

	// task shader is not used, the amount of spawned mesh shaders is determined by amount of meshlets stored in the storage_buffer_object->vertex_indices
	std::array<VkPipelineShaderStageCreateInfo, 2> mesh_stages;
	mesh_stages[0] = load_shader("gshader_to_mshader/gshader_to_mshader.mesh", VK_SHADER_STAGE_MESH_BIT_EXT);
	mesh_stages[1] = load_shader("gshader_to_mshader/gshader_to_mshader_mesh.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Dynamic State
	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR,
	    VK_DYNAMIC_STATE_LINE_WIDTH};

	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0);

	// Vertex bindings an attributes for model rendering

	// Binding description
	// Vertex structure is used here for binding description, AlignedVertex is used only in the context of a mesh shader operations
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)};

	// Attribute descriptions
	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)),
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal))};

	// Vertex Input
	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(
	        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	        0,
	        VK_FALSE);

	// Viewwport and scissors
	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(
	        VK_POLYGON_MODE_FILL,
	        VK_CULL_MODE_NONE,
	        VK_FRONT_FACE_CLOCKWISE,
	        0);

	rasterization_state.depthBiasConstantFactor = 1.0f;
	rasterization_state.depthBiasSlopeFactor    = 1.0f;

	// Multisampling
	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(
	        VK_SAMPLE_COUNT_1_BIT,
	        0);
	multisample_state.minSampleShading = 1.0f;

	// Color Blending
	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(
	        0xf,
	        VK_FALSE);

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(
	        1,
	        &blend_attachment_state);

	// Depth Stencil
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(
	        VK_TRUE,
	        VK_TRUE,
	        VK_COMPARE_OP_GREATER);

	// Pipeline layout made in descriptors layout

	// Graphics Pipeline
	VkGraphicsPipelineCreateInfo pipeline_create_info =
	    vkb::initializers::pipeline_create_info(
	        pipeline_layout,
	        render_pass,
	        0);
	pipeline_create_info.pStages             = model_stages.data();
	pipeline_create_info.stageCount          = static_cast<uint32_t>(model_stages.size());
	pipeline_create_info.pVertexInputState   = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &model_pipeline));

	pipeline_create_info.pStages    = geometry_stages.data();
	pipeline_create_info.stageCount = static_cast<uint32_t>(geometry_stages.size());
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &geometry_pipeline));

	pipeline_create_info.pStages    = mesh_stages.data();
	pipeline_create_info.stageCount = static_cast<uint32_t>(mesh_stages.size());
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &mesh_pipeline));
}

void GshaderToMshader::build_command_buffers()
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

		VkRect2D scissor = vkb::initializers::rect2D(static_cast<int>(width), static_cast<int>(height), 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		vkCmdSetLineWidth(draw_cmd_buffers[i], 1.0f);

		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, model_pipeline);

		draw_model(object, draw_cmd_buffers[i]);

		if (showNormalsGeo)
		{
			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, geometry_pipeline);
			draw_model(object, draw_cmd_buffers[i]);
		}

		if (showNormalsMesh)
		{
			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_pipeline);
			uint32_t num_workgroups_x = storage_buffer_object->vertex_indices;        // meshlets count
			uint32_t num_workgroups_y = 1;
			uint32_t num_workgroups_z = 1;
			vkCmdDrawMeshTasksEXT(draw_cmd_buffers[i], num_workgroups_x, num_workgroups_y, num_workgroups_z);
		}

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

/**
 * 	@fn void VertexDynamicState::create_descriptor_pool()
 * 	@brief Creating descriptor pool with size adjusted to use uniform buffer and image sampler
 */
void GshaderToMshader::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2)};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), 1);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

/**
 * 	@fn void VertexDynamicState::setup_descriptor_set_layout()
 * 	@brief Creating layout for descriptor sets
 */
void GshaderToMshader::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_GEOMETRY_BIT, 1),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT, 2),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT, 3),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT, 4)};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &descriptor_set_layout,
	        1);

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

/**
 * 	@fn void VertexDynamicState::create_descriptor_sets()
 */
void GshaderToMshader::setup_descriptor_sets()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layout,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

	VkDescriptorBufferInfo vs_ubo_descriptor   = create_descriptor(*uniform_buffer_vs);
	VkDescriptorBufferInfo gs_ubo_descriptor   = create_descriptor(*uniform_buffer_gs);
	VkDescriptorBufferInfo ms_ubo_descriptor   = create_descriptor(*uniform_buffer_ms);
	VkDescriptorBufferInfo meshlet_descriptor  = create_descriptor(*storage_buffer_object->index_buffer);
	VkDescriptorBufferInfo vertices_descriptor = create_descriptor(storage_buffer_object->vertex_buffers.at("vertex_buffer"));

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &vs_ubo_descriptor),
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &gs_ubo_descriptor),
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &ms_ubo_descriptor),
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, &meshlet_descriptor),
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, &vertices_descriptor)};

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

bool GshaderToMshader::resize(const uint32_t width, const uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	update_uniform_buffers();
	return true;
}

void GshaderToMshader::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.checkbox("Display normals - gshader", &showNormalsGeo))
		{
			showNormalsMesh = false;
			build_command_buffers();
		}
		if (drawer.checkbox("Display normals - mshader", &showNormalsMesh))
		{
			showNormalsGeo = false;
			build_command_buffers();
		}
	}
}

void GshaderToMshader::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	auto &requested_vertex_input_features      = gpu.request_extension_features<VkPhysicalDeviceMeshShaderFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT);
	requested_vertex_input_features.meshShader = VK_TRUE;

	if (gpu.get_features().geometryShader)
	{
		gpu.get_mutable_requested_features().geometryShader = VK_TRUE;
	}
}

std::unique_ptr<vkb::Application> create_gshader_to_mshader()
{
	return std::make_unique<GshaderToMshader>();
}
