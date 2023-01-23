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

#include "extended_dynamic_state2.h"

#include "gltf_loader.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/sub_mesh.h"

ExtendedDynamicState2::ExtendedDynamicState2()
{
	title = "Extended Dynamic State2";

	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);
	add_device_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
}

ExtendedDynamicState2::~ExtendedDynamicState2()
{
	if (device)
	{
		uniform_buffers.baseline.reset();
		uniform_buffers.tesselation.reset();
		uniform_buffers.background.reset();
		vkDestroySampler(get_device().get_handle(), textures.envmap.sampler, VK_NULL_HANDLE);
		textures = {};

		vkDestroyPipeline(get_device().get_handle(), pipeline.tesselation, VK_NULL_HANDLE);
		vkDestroyPipeline(get_device().get_handle(), pipeline.baseline, VK_NULL_HANDLE);
		vkDestroyPipeline(get_device().get_handle(), pipeline.background, VK_NULL_HANDLE);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.tesselation, VK_NULL_HANDLE);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.baseline, VK_NULL_HANDLE);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.background, VK_NULL_HANDLE);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layouts.tesselation, VK_NULL_HANDLE);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layouts.baseline, VK_NULL_HANDLE);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layouts.background, VK_NULL_HANDLE);
		vkDestroyDescriptorPool(get_device().get_handle(), descriptor_pool, VK_NULL_HANDLE);
	}
}

/**
 * 	@fn bool ExtendedDynamicState2::prepare(vkb::Platform &platform)
 * 	@brief Configuring all sample specific settings, creating descriptor sets/pool, pipelines, generating or loading models etc.
 */
bool ExtendedDynamicState2::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position({2.0f, -4.0f, -10.0f});
	camera.set_rotation({-15.0f, 190.0f, 0.0f});
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 256.0f, 0.1f);

	load_assets();
	model_data_creation();
	prepare_uniform_buffers();
	create_descriptor_pool();
	setup_descriptor_set_layout();
	create_descriptor_sets();
	create_pipeline();
	build_command_buffers();
	prepared = true;

	return true;
}
/**
 * 	@fn void ExtendedDynamicState2::load_assets()
 *	@brief Loading extra models, textures from assets
 */
void ExtendedDynamicState2::load_assets()
{
	load_scene("scenes/primitives/primitives.gltf");

	std::vector<SceneNode> scene_elements;
	// Store all scene nodes in a linear vector for easier access
	for (auto &mesh : scene->get_components<vkb::sg::Mesh>())
	{
		for (auto &node : mesh->get_nodes())
		{
			ModelDynamicParam object_param{};
			gui_settings.objects.push_back(object_param);
			for (auto &sub_mesh : mesh->get_submeshes())
			{
				scene_elements.push_back({mesh->get_name(), node, sub_mesh});
			}
		}
	}
	scene_nodes.push_back(scene_elements);

	/* Split scene */
	scene_pipeline_divide(&scene_nodes);

	background_model = load_model("scenes/cube.gltf");
	/* Load HDR cube map */
	textures.envmap = load_texture_cubemap("textures/uffizi_rgba16f_cube.ktx", vkb::sg::Image::Color);
}

/**
 * 	@fn void ExtendedDynamicState2::draw()
 *  @brief Preparing frame and submitting it to the present queue
 */
void ExtendedDynamicState2::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

/**
 * 	@fn void ExtendedDynamicState2::render(float delta_time)
 * 	@brief Drawing frames and/or updating uniform buffers when camera position/rotation was changed
 */
void ExtendedDynamicState2::render(float delta_time)
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
 * 	@fn void ExtendedDynamicState2::prepare_uniform_buffers()
 * 	@brief Preparing uniform buffer (setting bits) and updating UB data
 */
void ExtendedDynamicState2::prepare_uniform_buffers()
{
	uniform_buffers.baseline    = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(ubo_baseline), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	uniform_buffers.tesselation = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(ubo_tess), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	uniform_buffers.background  = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(ubo_background), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	update_uniform_buffers();
}

/**
 * 	@fn void ExtendedDynamicState2::update_uniform_buffers()
 * 	@brief Updating data from application to GPU uniform buffer
 */
void ExtendedDynamicState2::update_uniform_buffers()
{
	/* Baseline uniform buffer */
	ubo_baseline.projection = camera.matrices.perspective;
	ubo_baseline.view       = camera.matrices.view ;
	uniform_buffers.baseline->convert_and_update(ubo_baseline);

	/* Tessellation uniform buffer */
	ubo_tess.projection          = camera.matrices.perspective;
	ubo_tess.modelview           = camera.matrices.view ;
	ubo_tess.tessellation_factor = gui_settings.tess_factor;

	if (!gui_settings.tessellation)
	{
		// Setting this to zero sets all tessellation factors to 1.0 in the shader
		ubo_tess.tessellation_factor = 0.0f;
	}
	uniform_buffers.tesselation->convert_and_update(ubo_tess);

	/* Background uniform buffer */
	ubo_background.projection           = camera.matrices.perspective;
	ubo_background.background_modelview = camera.matrices.view;

	uniform_buffers.background->convert_and_update(ubo_background);
}

/**
 * 	@fn void ExtendedDynamicState2::create_pipeline()
 * 	@brief Creating graphical pipelines: baseline, background, tessellation.
 */
void ExtendedDynamicState2::create_pipeline()
{
	/* Setup for first pipeline */
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(
	        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	        0,
	        VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(
	        VK_POLYGON_MODE_FILL,
	        VK_CULL_MODE_BACK_BIT,
	        VK_FRONT_FACE_CLOCKWISE,
	        0);

	rasterization_state.depthBiasConstantFactor = 1.0f;
	rasterization_state.depthBiasSlopeFactor    = 1.0f;

	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(
	        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	        VK_TRUE);

	blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment_state.alphaBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(
	        1,
	        &blend_attachment_state);

	/* Note: Using Reversed depth-buffer for increased precision, so Greater depth values are kept */
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

	VkPipelineTessellationStateCreateInfo tessellation_state =
	    vkb::initializers::pipeline_tessellation_state_create_info(3);

	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR,
	    VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT,
	    VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE_EXT,
	    VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE_EXT,
	    VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE_EXT,
	};
	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0);

	/* Vertex bindings and attributes for model rendering */
	/* Binding description */
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX),
	    vkb::initializers::vertex_input_binding_description(1, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX),
	};

	/* Attribute descriptions */
	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),        // Position
	    vkb::initializers::vertex_input_attribute_description(1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0),        // Normal
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	std::array<VkPipelineShaderStageCreateInfo, 4> shader_stages{};
	shader_stages[0] = load_shader("extended_dynamic_state2/baseline.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("extended_dynamic_state2/baseline.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	/* Use the pNext to point to the rendering create struct */
	VkGraphicsPipelineCreateInfo graphics_create{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
	graphics_create.pNext               = VK_NULL_HANDLE;
	graphics_create.renderPass          = VK_NULL_HANDLE;
	graphics_create.pInputAssemblyState = &input_assembly_state;
	graphics_create.pRasterizationState = &rasterization_state;
	graphics_create.pColorBlendState    = &color_blend_state;
	graphics_create.pMultisampleState   = &multisample_state;
	graphics_create.pViewportState      = &viewport_state;
	graphics_create.pDepthStencilState  = &depth_stencil_state;
	graphics_create.pDynamicState       = &dynamic_state;
	graphics_create.pVertexInputState   = &vertex_input_state;
	graphics_create.pTessellationState  = VK_NULL_HANDLE;
	graphics_create.stageCount          = 2;
	graphics_create.pStages             = shader_stages.data();
	graphics_create.layout              = pipeline_layouts.baseline;

	graphics_create.pNext      = VK_NULL_HANDLE;
	graphics_create.renderPass = render_pass;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &graphics_create, VK_NULL_HANDLE, &pipeline.baseline));

	/* Setup for second pipeline */
	graphics_create.layout = pipeline_layouts.background;

	std::vector<VkDynamicState> dynamic_state_enables_background = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR,
	};
	dynamic_state.pDynamicStates    = dynamic_state_enables_background.data();
	dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_state_enables_background.size());

	/* Binding description */
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings_background = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};

	/* Attribute descriptions */
	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes_background = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),                               // Position
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)),        // Normal
	};

	vertex_input_state.vertexBindingDescriptionCount   = static_cast<uint32_t>(vertex_input_bindings_background.size());
	vertex_input_state.pVertexBindingDescriptions      = vertex_input_bindings_background.data();
	vertex_input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attributes_background.size());
	vertex_input_state.pVertexAttributeDescriptions    = vertex_input_attributes_background.data();

	rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	shader_stages[0] = load_shader("extended_dynamic_state2/background.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("extended_dynamic_state2/background.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &graphics_create, VK_NULL_HANDLE, &pipeline.background));

	/* Setup for third pipeline */
	graphics_create.pTessellationState = &tessellation_state;
	graphics_create.layout             = pipeline_layouts.tesselation;
	input_assembly_state.topology      = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

	dynamic_state_enables.push_back(VK_DYNAMIC_STATE_PATCH_CONTROL_POINTS_EXT);
	dynamic_state.pDynamicStates    = dynamic_state_enables.data();
	dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_state_enables.size());

	vertex_input_state.vertexBindingDescriptionCount   = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions      = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions    = vertex_input_attributes.data();

	/* Wireframe mode */
	if (get_device().get_gpu().get_features().fillModeNonSolid)
	{
		rasterization_state.polygonMode = VK_POLYGON_MODE_LINE;
	}

	shader_stages[0]           = load_shader("extended_dynamic_state2/tess.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1]           = load_shader("extended_dynamic_state2/tess.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	shader_stages[2]           = load_shader("extended_dynamic_state2/tess.tesc", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
	shader_stages[3]           = load_shader("extended_dynamic_state2/tess.tese", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
	graphics_create.stageCount = static_cast<uint32_t>(shader_stages.size());
	graphics_create.pStages    = shader_stages.data();
	/* Enable depth test and write */
	depth_stencil_state.depthWriteEnable = VK_TRUE;
	depth_stencil_state.depthTestEnable  = VK_TRUE;
	/* Flip cull mode */
	rasterization_state.cullMode = VK_CULL_MODE_FRONT_BIT;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &graphics_create, VK_NULL_HANDLE, &pipeline.tesselation));
}

/**
 * 	@fn void ExtendedDynamicState2::build_command_buffers()
 * 	@brief Creating command buffers and drawing particular elements on window.
 * 	@details Drawing object list:
 * 			 - models from baseline scene
 * 			 - model from tessellation scene
 * 			 - background model
 * 			 - primitive restart model
 */
void ExtendedDynamicState2::build_command_buffers()
{
	std::array<VkClearValue, 2> clear_values{};
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0};

	int i = -1; /* Required for accessing element in framebuffers vector */
	for (auto &draw_cmd_buffer : draw_cmd_buffers)
	{
		i++;
		auto command_begin = vkb::initializers::command_buffer_begin_info();
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffer, &command_begin));

		VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
		render_pass_begin_info.renderPass               = render_pass;
		render_pass_begin_info.framebuffer              = framebuffers[i];
		render_pass_begin_info.renderArea.extent.width  = width;
		render_pass_begin_info.renderArea.extent.height = height;
		render_pass_begin_info.clearValueCount          = static_cast<uint32_t>(clear_values.size());
		render_pass_begin_info.pClearValues             = clear_values.data();

		vkCmdBeginRenderPass(draw_cmd_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffer, 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(static_cast<int>(width), static_cast<int>(height), 0, 0);
		vkCmdSetScissor(draw_cmd_buffer, 0, 1, &scissor);

		/* Binding baseline pipeline and descriptor sets */
		vkCmdBindDescriptorSets(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.baseline, 0, 1, &descriptor_sets.baseline, 0, nullptr);
		vkCmdBindPipeline(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.baseline);

		/* Setting topology to triangle list and disabling primitive restart functionality */
		vkCmdSetPrimitiveTopologyEXT(draw_cmd_buffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		vkCmdSetPrimitiveRestartEnableEXT(draw_cmd_buffer, VK_FALSE);

		/* Drawing objects from baseline scene (with rasterizer discard and depth bias functionality) */
		draw_from_scene(draw_cmd_buffer, &scene_nodes, SCENE_BASELINE_OBJ_INDEX);

		/* Changing topology to triangle strip with using primitive restart feature */
		vkCmdSetPrimitiveTopologyEXT(draw_cmd_buffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
		vkCmdSetPrimitiveRestartEnableEXT(draw_cmd_buffer, VK_TRUE);

		/* Draw model with primitive restart functionality */
		draw_created_model(draw_cmd_buffer);

		/* Changing bindings to tessellation pipeline */
		vkCmdBindDescriptorSets(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.tesselation, 0, 1, &descriptor_sets.tesselation, 0, nullptr);
		vkCmdBindPipeline(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.tesselation);

		/* Change topology to patch list and setting patch control points value */
		vkCmdSetPrimitiveTopologyEXT(draw_cmd_buffer, VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
		vkCmdSetPatchControlPointsEXT(draw_cmd_buffer, gui_settings.patch_control_points);

		/* Drawing scene with objects using tessellation feature */
		draw_from_scene(draw_cmd_buffer, &scene_nodes, SCENE_TESSELLATION_OBJ_INDEX);

		/* Changing bindings to background pipeline */
		vkCmdBindDescriptorSets(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.background, 0, 1, &descriptor_sets.background, 0, nullptr);
		vkCmdBindPipeline(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.background);

		/* Drawing background */
		draw_model(background_model, draw_cmd_buffer);

		/* UI */
		draw_ui(draw_cmd_buffer);

		vkCmdEndRenderPass(draw_cmd_buffer);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffer));
	}
}

/**
 * 	@fn void ExtendedDynamicState2::create_descriptor_pool()
 * 	@brief Creating descriptor pool with size adjusted to use uniform buffer and image sampler
 */
void ExtendedDynamicState2::create_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),
	};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(
	        static_cast<uint32_t>(pool_sizes.size()),
	        pool_sizes.data(),
	        3);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

/**
 * 	@fn void ExtendedDynamicState2::setup_descriptor_set_layout()
 * 	@brief Creating layout for descriptor sets
 */
void ExtendedDynamicState2::setup_descriptor_set_layout()
{
	/* First descriptor set */
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	                                                     VK_SHADER_STAGE_VERTEX_BIT,
	                                                     0),
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layouts.baseline));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &descriptor_set_layouts.baseline,
	        1);

	/* Pass scene node information via push constants */
	VkPushConstantRange push_constant_range            = vkb::initializers::push_constant_range(VK_SHADER_STAGE_VERTEX_BIT, sizeof(push_const_block), 0);
	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layouts.baseline));

	/* Second descriptor set */
	set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_VERTEX_BIT,
	        0),
	};

	descriptor_layout_create_info.pBindings    = set_layout_bindings.data();
	descriptor_layout_create_info.bindingCount = static_cast<uint32_t>(set_layout_bindings.size());
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layouts.tesselation));

	pipeline_layout_create_info.pSetLayouts    = &descriptor_set_layouts.tesselation;
	pipeline_layout_create_info.setLayoutCount = 1;
	push_constant_range.stageFlags             = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layouts.tesselation));

	/* Third descriptor set */
	set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        VK_SHADER_STAGE_VERTEX_BIT,
	        0),
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        VK_SHADER_STAGE_FRAGMENT_BIT,
	        1),
	};

	descriptor_layout_create_info.pBindings    = set_layout_bindings.data();
	descriptor_layout_create_info.bindingCount = static_cast<uint32_t>(set_layout_bindings.size());
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layouts.background));

	pipeline_layout_create_info.pSetLayouts            = &descriptor_set_layouts.background;
	pipeline_layout_create_info.setLayoutCount         = 1;
	pipeline_layout_create_info.pushConstantRangeCount = 0;
	pipeline_layout_create_info.pPushConstantRanges    = VK_NULL_HANDLE;
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layouts.background));
}

/**
 * 	@fn void ExtendedDynamicState2::create_descriptor_sets()
 * 	@brief Creating descriptor sets for 3 separate pipelines.
 */
void ExtendedDynamicState2::create_descriptor_sets()
{
	/* First descriptor set */
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layouts.baseline,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.baseline));

	VkDescriptorBufferInfo matrix_baseline_buffer_descriptor = create_descriptor(*uniform_buffers.baseline);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(
	        descriptor_sets.baseline,
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        0,
	        &matrix_baseline_buffer_descriptor)};

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);

	/* Second descriptor set */
	alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layouts.tesselation,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.tesselation));

	VkDescriptorBufferInfo matrix_tess_buffer_descriptor = create_descriptor(*uniform_buffers.tesselation);

	write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(
	        descriptor_sets.tesselation,
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        0,
	        &matrix_tess_buffer_descriptor)};

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);

	/* Third descriptor set */
	alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layouts.background,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.background));

	VkDescriptorBufferInfo matrix_background_buffer_descriptor = create_descriptor(*uniform_buffers.background);
	VkDescriptorImageInfo  background_image_descriptor         = create_descriptor(textures.envmap);

	write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(
	        descriptor_sets.background,
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        0,
	        &matrix_background_buffer_descriptor),
	    vkb::initializers::write_descriptor_set(
	        descriptor_sets.background,
	        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        1,
	        &background_image_descriptor)};

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);
}

/**
 * @fn void ExtendedDynamicState2::request_gpu_features(vkb::PhysicalDevice &gpu)
 * @brief Enabling features related to Vulkan extensions
 */
void ExtendedDynamicState2::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	/* Enable extension features required by this sample
	   These are passed to device creation via a pNext structure chain */
	auto &requested_extended_dynamic_state2_features                                   = gpu.request_extension_features<VkPhysicalDeviceExtendedDynamicState2FeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT);
	requested_extended_dynamic_state2_features.extendedDynamicState2                   = VK_TRUE;
	requested_extended_dynamic_state2_features.extendedDynamicState2PatchControlPoints = VK_TRUE;

	auto &requested_extended_dynamic_state_feature                = gpu.request_extension_features<VkPhysicalDeviceExtendedDynamicStateFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT);
	requested_extended_dynamic_state_feature.extendedDynamicState = VK_TRUE;

	// Tessellation shader support is required for this example
	auto &requested_features = gpu.get_mutable_requested_features();
	if (gpu.get_features().tessellationShader)
	{
		requested_features.tessellationShader = VK_TRUE;
	}
	else
	{
		throw vkb::VulkanException(VK_ERROR_FEATURE_NOT_PRESENT, "Selected GPU does not support tessellation shaders!");
	}

	if (gpu.get_features().fillModeNonSolid)
	{
		requested_features.fillModeNonSolid = VK_TRUE;
	}

	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = true;
	}
}

/**
 * @fn void ExtendedDynamicState2::on_update_ui_overlay(vkb::Drawer &drawer)
 * @brief Projecting GUI and transferring data between GUI and app
 */
void ExtendedDynamicState2::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.checkbox("Tessellation Enable", &gui_settings.tessellation))
		{
			update_uniform_buffers();
		}
		if (drawer.input_float("Tessellation Factor", &gui_settings.tess_factor, 1.0f, 1))
		{
			update_uniform_buffers();
		}

		if (drawer.input_float("Patch Control Points", &gui_settings.patch_control_points_float, 1.0f, 1))
		{
			if (gui_settings.patch_control_points_float < 1)
			{
				gui_settings.patch_control_points_float = 1;
			}
			gui_settings.patch_control_points = static_cast<int>(roundf(gui_settings.patch_control_points_float));
		}
	}
	if (drawer.header("Models"))
	{
		const char *col_names = {"Name"};
		if (drawer.checkbox("Selection effect active", &gui_settings.selection_active))
		{
		}
		ImGui::Columns(2, "Name");
		ImGui::SetColumnWidth(0, 150);
		int                       obj_cnt = scene_nodes.at(SCENE_BASELINE_OBJ_INDEX).size();
		std::vector<const char *> obj_names;

		for (int i = 0; i < obj_cnt; i++)
		{
			obj_names.push_back((scene_nodes.at(SCENE_BASELINE_OBJ_INDEX).at(i).name).c_str());
		}
		ImGui::ListBox("", &gui_settings.selected_obj, obj_names.data(), obj_cnt);
		ImGui::NextColumn();
		if (drawer.checkbox("Depth Bias Enable", &gui_settings.objects[gui_settings.selected_obj].depth_bias))
		{
		}
		if (drawer.checkbox("Rasterizer Discard", &gui_settings.objects[gui_settings.selected_obj].rasterizer_discard))
		{
		}
	}
}

/**
 * @fn void ExtendedDynamicState2::update(float delta_time)
 * @brief Function which was called in every frame.
 * @details For presenting z-fighting, a small animation was implemented (cube_animation)
 */
void ExtendedDynamicState2::update(float delta_time)
{
	cube_animation(delta_time);
	ApiVulkanSample::update(delta_time);
}

/**
 * @fn int ExtendedDynamicState2::get_node_index(std::string name, std::vector<SceneNode> *scene_node)
 * @brief Extracting index value based on provided name (string)
 */
int ExtendedDynamicState2::get_node_index(std::string name, std::vector<SceneNode> *scene_node)
{
	int i = 0;
	for (i = 0; i < scene_node->size(); i++)
	{
		if (scene_node->at(i).node->get_name() == name)
		{
			break;
		}
	}
	return std::distance(scene_node->begin(),
	                     std::find_if(scene_node->begin(), scene_node->end(), [&name](SceneNode const &node) { return node.node->get_name() == name; }));
}

/**
 * @fn void ExtendedDynamicState2::selection_indicator(const vkb::sg::PBRMaterial *original_mat, vkb::sg::PBRMaterial *new_mat)
 * @brief Changing alpha value to create blinking effect on selected model
 */
void ExtendedDynamicState2::selection_indicator(const vkb::sg::PBRMaterial *original_mat, vkb::sg::PBRMaterial *new_mat)
{
	static bool                        rise              = false;
	static int                         previous_obj_id   = gui_settings.selected_obj;
	static const vkb::sg::PBRMaterial *previous_material = original_mat;
	static float                       accumulated_diff  = 0.0f;
	constexpr float                    alpha_step        = 0.075f;
	constexpr float                    alpha_max         = 0.98f;
	constexpr float                    alpha_min         = 0.3f;

	new_mat->base_color_factor = original_mat->base_color_factor;
	new_mat->alpha_mode        = vkb::sg::AlphaMode::Blend;

	/* Change alpha value */
	if (gui_settings.time_tick == true)
	{
		accumulated_diff += rise ? alpha_step : -alpha_step;
		gui_settings.time_tick = false;
	}
	new_mat->base_color_factor.w += accumulated_diff;

	/* Detecting change of selected object */
	if (previous_obj_id != gui_settings.selected_obj)
	{
		accumulated_diff = 0.0f;
		previous_obj_id  = gui_settings.selected_obj;
	}

	/* Determine if alpha need to increase or decrease */
	rise = new_mat->base_color_factor.w < alpha_min ? true : false;
}

/**
 * @fn void ExtendedDynamicState2::scene_pipeline_divide(std::vector<std::vector<SceneNode>> *scene_node)
 * @brief Spliting main scene into two separate.
 * @details This operation is required to use same "draw_from_scene" function to draw models that are using different
 * 			pipelines (baseline and tessellation)
 */
void ExtendedDynamicState2::scene_pipeline_divide(std::vector<std::vector<SceneNode>> *scene_node)
{
	int scene_nodes_cnt = scene_node->at(SCENE_ALL_OBJ_INDEX).size();

	std::vector<SceneNode> scene_elements_baseline;
	std::vector<SceneNode> scene_elements_tess;

	/* Divide main scene to two (baseline and tessellation) */
	for (int i = 0; i < scene_nodes_cnt; i++)
	{
		if (scene_node->at(SCENE_ALL_OBJ_INDEX).at(i).name == "Geosphere")
		{
			scene_elements_tess.push_back(scene_node->at(SCENE_ALL_OBJ_INDEX).at(i));
		}
		else
		{
			scene_elements_baseline.push_back(scene_node->at(SCENE_ALL_OBJ_INDEX).at(i));
		}
	}

	scene_node->push_back(scene_elements_baseline);
	scene_node->push_back(scene_elements_tess);
}

/**
 * @fn void ExtendedDynamicState2::draw_from_scene(VkCommandBuffer command_buffer, std::vector<std::vector<SceneNode>> *scene_node, sceneObjType_t scene_index)
 * @brief Drawing all objects included to scene that is passed as argument of this function
 */
void ExtendedDynamicState2::draw_from_scene(VkCommandBuffer command_buffer, std::vector<std::vector<SceneNode>> *scene_node, sceneObjType_t scene_index)
{
	auto &node               = scene_node->at(scene_index);
	int   scene_elements_cnt = scene_node->at(scene_index).size();

	for (int i = 0; i < scene_elements_cnt; i++)
	{
		const auto &vertex_buffer_pos    = node[i].sub_mesh->vertex_buffers.at("position");
		const auto &vertex_buffer_normal = node[i].sub_mesh->vertex_buffers.at("normal");
		auto       &index_buffer         = node[i].sub_mesh->index_buffer;

		if (scene_index == SCENE_BASELINE_OBJ_INDEX)
		{
			vkCmdSetDepthBiasEnableEXT(command_buffer, gui_settings.objects[i].depth_bias);
			vkCmdSetRasterizerDiscardEnableEXT(command_buffer, gui_settings.objects[i].rasterizer_discard);
		}

		/* Pass data for the current node via push commands */
		auto node_material            = dynamic_cast<const vkb::sg::PBRMaterial *>(node[i].sub_mesh->get_material());
		push_const_block.model_matrix = node[i].node->get_transform().get_world_matrix();
		if (i != gui_settings.selected_obj ||
		    gui_settings.selection_active == false)
		{
			push_const_block.color = node_material->base_color_factor;
		}
		else
		{
			vkb::sg::PBRMaterial temp_material{"Selected_Material"};
			selection_indicator(node_material, &temp_material);
			push_const_block.color = temp_material.base_color_factor;
		}
		vkCmdPushConstants(command_buffer, pipeline_layouts.baseline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_const_block), &push_const_block);

		VkDeviceSize offsets[1] = {0};
		vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffer_pos.get(), offsets);
		vkCmdBindVertexBuffers(command_buffer, 1, 1, vertex_buffer_normal.get(), offsets);
		vkCmdBindIndexBuffer(command_buffer, index_buffer->get_handle(), 0, node[i].sub_mesh->index_type);

		vkCmdDrawIndexed(command_buffer, node[i].sub_mesh->vertex_indices, 1, 0, 0, 0);
	}
	vkCmdSetDepthBiasEnableEXT(command_buffer, VK_FALSE);
	vkCmdSetRasterizerDiscardEnableEXT(command_buffer, VK_FALSE);
}

/**
 * @fn void ExtendedDynamicState2::draw_created_model(VkCommandBuffer commandBuffer)
 * @brief Drawing model created in function "model_data_creation"
 */
void ExtendedDynamicState2::draw_created_model(VkCommandBuffer commandBuffer)
{
	VkDeviceSize offsets[1] = {0};
	push_const_block.color  = glm::vec4{0.5f, 1.0f, 1.0f, 1.0f};
	vkCmdPushConstants(commandBuffer, pipeline_layouts.baseline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_const_block), &push_const_block);
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, cube.vertices_pos->get(), offsets);
	vkCmdBindVertexBuffers(commandBuffer, 1, 1, cube.vertices_norm->get(), offsets);
	vkCmdBindIndexBuffer(commandBuffer, cube.indices->get_handle(), 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(commandBuffer, cube.index_count, 1, 0, 0, 0);
}

/**
 * @fn void ExtendedDynamicState2::model_data_creation()
 * @brief Creating model (basic cube) vertex data
 */
void ExtendedDynamicState2::model_data_creation()
{
	constexpr uint32_t vertex_count = 8;

	std::array<glm::vec3, vertex_count> vertices_pos;
	std::array<glm::vec3, vertex_count> vertices_norm;

	vertices_pos[0] = {0.0f, 0.0f, 0.0f};
	vertices_pos[1] = {1.0f, 0.0f, 0.0f};
	vertices_pos[2] = {1.0f, 1.0f, 0.0f};
	vertices_pos[3] = {0.0f, 1.0f, 0.0f};
	vertices_pos[4] = {0.0f, 0.0f, 1.0f};
	vertices_pos[5] = {1.0f, 0.0f, 1.0f};
	vertices_pos[6] = {1.0f, 1.0f, 1.0f};
	vertices_pos[7] = {0.0f, 1.0f, 1.0f};

	/* Normalized normal vectors for each face of cube */
	glm::vec3 Xp = {1.0, 0.0, 0.0};
	glm::vec3 Xm = {-1.0, 0.0, 0.0};
	glm::vec3 Yp = {0.0, 1.0, 0.0};
	glm::vec3 Ym = {0.0, -1.0, 0.0};
	glm::vec3 Zp = {0.0, 0.0, 1.0};
	glm::vec3 Zm = {0.0, 0.0, -1.0};

	/* Normalized normal vectors for each vertex (created by sum of corresponding faces) */
	vertices_norm[0] = glm::normalize(Xm + Ym + Zm);
	vertices_norm[1] = glm::normalize(Xp + Ym + Zm);
	vertices_norm[2] = glm::normalize(Xp + Yp + Zm);
	vertices_norm[3] = glm::normalize(Xm + Yp + Zm);
	vertices_norm[4] = glm::normalize(Xm + Ym + Zp);
	vertices_norm[5] = glm::normalize(Xp + Ym + Zp);
	vertices_norm[6] = glm::normalize(Xp + Yp + Zp);
	vertices_norm[7] = glm::normalize(Xm + Yp + Zp);

	/* Scaling and position transform */
	for (uint8_t i = 0; i < vertex_count; i++)
	{
		vertices_pos[i] *= glm::vec3(4.0f, 4.0f, 4.0f);
		vertices_pos[i] += glm::vec3(15.0f, 2.0f, 0.0f);
	}

	constexpr uint32_t index_count        = 29;
	uint32_t           vertex_buffer_size = vertex_count * sizeof(glm::vec3);
	uint32_t           index_buffer_size  = index_count * sizeof(uint32_t);
	cube.index_count                      = index_count;

	/* Array with vertices indexes for corresponding triangles */
	std::array<uint32_t, index_count> indices{0, 4, 3, 7,
	                                          UINT32_MAX,
	                                          1, 0, 2, 3,
	                                          UINT32_MAX,
	                                          2, 6, 1, 5,
	                                          UINT32_MAX,
	                                          1, 5, 0, 4,
	                                          UINT32_MAX,
	                                          4, 5, 7, 6,
	                                          UINT32_MAX,
	                                          2, 3, 6, 7};

	struct
	{
		VkBuffer       buffer;
		VkDeviceMemory memory;
	} vertex_pos_staging, vertex_norm_staging, index_staging;

	vertex_pos_staging.buffer = get_device().create_buffer(
	    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	    vertex_buffer_size,
	    &vertex_pos_staging.memory,
	    vertices_pos.data());

	vertex_norm_staging.buffer = get_device().create_buffer(
	    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	    vertex_buffer_size,
	    &vertex_norm_staging.memory,
	    vertices_norm.data());

	index_staging.buffer = get_device().create_buffer(
	    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	    index_buffer_size,
	    &index_staging.memory,
	    indices.data());

	cube.vertices_pos = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                        vertex_buffer_size,
	                                                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                                                        VMA_MEMORY_USAGE_GPU_ONLY);

	cube.vertices_norm = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                         vertex_buffer_size,
	                                                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                                                         VMA_MEMORY_USAGE_GPU_ONLY);

	cube.indices = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                   index_buffer_size,
	                                                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                                                   VMA_MEMORY_USAGE_GPU_ONLY);

	/* Copy from staging buffers */
	VkCommandBuffer copy_command = device->create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkBufferCopy copy_region = {};

	copy_region.size = vertex_buffer_size;
	vkCmdCopyBuffer(
	    copy_command,
	    vertex_pos_staging.buffer,
	    cube.vertices_pos->get_handle(),
	    1,
	    &copy_region);

	vkCmdCopyBuffer(
	    copy_command,
	    vertex_norm_staging.buffer,
	    cube.vertices_norm->get_handle(),
	    1,
	    &copy_region);

	copy_region.size = index_buffer_size;
	vkCmdCopyBuffer(
	    copy_command,
	    index_staging.buffer,
	    cube.indices->get_handle(),
	    1,
	    &copy_region);

	device->flush_command_buffer(copy_command, queue, true);

	vkDestroyBuffer(get_device().get_handle(), vertex_pos_staging.buffer, nullptr);
	vkFreeMemory(get_device().get_handle(), vertex_pos_staging.memory, nullptr);
	vkDestroyBuffer(get_device().get_handle(), vertex_norm_staging.buffer, nullptr);
	vkFreeMemory(get_device().get_handle(), vertex_norm_staging.memory, nullptr);
	vkDestroyBuffer(get_device().get_handle(), index_staging.buffer, nullptr);
	vkFreeMemory(get_device().get_handle(), index_staging.memory, nullptr);
}

/**
 * @fn void ExtendedDynamicState2::cube_animation(float delta_time)
 * @brief Changing position of one z-fighting cube (visualize negative phenomenon z-fighting)
 */
void ExtendedDynamicState2::cube_animation(float delta_time)
{
	constexpr float tick_limit = 0.05;
	constexpr float delta      = 0.05;
	constexpr float move_step  = 0.0005;
	static float    time_pass  = 0;
	time_pass += delta_time;
	static glm::vec3 translation = scene_nodes[SCENE_BASELINE_OBJ_INDEX].at(get_node_index("Cube_1", &scene_nodes[SCENE_BASELINE_OBJ_INDEX])).node->get_transform().get_translation();
	static float     difference  = 0;
	static bool      rising      = true;

	/* Checking if tick time passed away */
	if (time_pass > tick_limit)
	{
		/* Determine direction of x axis */
		if (difference < -delta)
		{
			rising = true;
		}
		else if (difference > delta)
		{
			rising = false;
		}

		/* Move object by step value */
		if (rising == true)
		{
			translation.x += move_step;
			difference += move_step;
		}
		else
		{
			translation.x -= move_step;
			difference -= move_step;
		}
		time_pass = 0;

		/* Write new position to object */
		for (uint32_t i = 0; i < scene_nodes[SCENE_BASELINE_OBJ_INDEX].size(); i++)
		{
			if (scene_nodes[SCENE_BASELINE_OBJ_INDEX].at(i).node->get_name() == "Cube_1")
			{
				scene_nodes[SCENE_BASELINE_OBJ_INDEX].at(i).node->get_transform().set_translation(translation);
				break;
			}
		}
		gui_settings.time_tick = true;
		build_command_buffers();
	}
}

std::unique_ptr<vkb::VulkanSample> create_extended_dynamic_state2()
{
	return std::make_unique<ExtendedDynamicState2>();
}
