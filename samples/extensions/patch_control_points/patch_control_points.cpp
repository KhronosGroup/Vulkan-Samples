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

#include "patch_control_points.h"

PatchControlPoints::PatchControlPoints()
{
	title = "Patch Control Points";

	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);
	add_device_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
}

PatchControlPoints::~PatchControlPoints()
{
	if (device)
	{
		uniform_buffers.common.reset();
		uniform_buffers.dynamically_tessellation.reset();
		uniform_buffers.statically_tessellation.reset();

		vkDestroyPipeline(get_device().get_handle(), pipeline.dynamically_tessellation, VK_NULL_HANDLE);
		vkDestroyPipeline(get_device().get_handle(), pipeline.statically_tessellation, VK_NULL_HANDLE);

		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.dynamically_tessellation, VK_NULL_HANDLE);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.statically_tessellation, VK_NULL_HANDLE);

		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layouts.dynamically_tessellation, VK_NULL_HANDLE);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layouts.statically_tessellation, VK_NULL_HANDLE);

		vkDestroyDescriptorPool(get_device().get_handle(), descriptor_pool, VK_NULL_HANDLE);
	}
}

/**
 * 	@fn bool PatchControlPoints::prepare(vkb::Platform &platform)
 * 	@brief Configuring all sample specific settings, creating descriptor sets/pool, pipelines, generating or loading models etc.
 */
bool PatchControlPoints::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	camera.type = vkb::CameraType::FirstPerson;
	camera.set_position({-1.25f, 0.0f, 0.0f});
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 256.0f, 0.1f);

	load_assets();
	prepare_uniform_buffers();
	create_descriptor_pool();
	setup_descriptor_set_layout();
	create_descriptor_sets();
	create_pipelines();
	build_command_buffers();
	prepared = true;

	return true;
}
/**
 * 	@fn void PatchControlPoints::load_assets()
 *	@brief Loading models from assets
 */
void PatchControlPoints::load_assets()
{
	model = load_model("scenes/terrain/terrain.gltf");
}

/**
 * 	@fn void PatchControlPoints::draw()
 *  @brief Preparing frame and submitting it to the present queue
 */
void PatchControlPoints::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

/**
 * 	@fn void PatchControlPoints::render(float delta_time)
 * 	@brief Drawing frames and/or updating uniform buffers when camera position/rotation was changed
 */
void PatchControlPoints::render(float delta_time)
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
 * 	@fn void PatchControlPoints::prepare_uniform_buffers()
 * 	@brief Preparing uniform buffer (setting bits) and updating UB data
 */
void PatchControlPoints::prepare_uniform_buffers()
{
	uniform_buffers.common                   = std::make_unique<vkb::core::Buffer>(get_device(),
                                                                 sizeof(ubo_common),
                                                                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                                 VMA_MEMORY_USAGE_CPU_TO_GPU);
	uniform_buffers.dynamically_tessellation = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                                               sizeof(ubo_tess),
	                                                                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                                               VMA_MEMORY_USAGE_CPU_TO_GPU);
	uniform_buffers.statically_tessellation  = std::make_unique<vkb::core::Buffer>(get_device(),
                                                                                  sizeof(ubo_tess),
                                                                                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                                                  VMA_MEMORY_USAGE_CPU_TO_GPU);
	update_uniform_buffers();
}

/**
 * 	@fn void PatchControlPoints::update_uniform_buffers()
 * 	@brief Updating data from application to GPU uniform buffer
 */
void PatchControlPoints::update_uniform_buffers()
{
	/* Common uniform buffer */
	ubo_common.projection = camera.matrices.perspective;
	ubo_common.view       = camera.matrices.view;
	uniform_buffers.common->convert_and_update(ubo_common);

	/* Tessellation uniform buffer */
	ubo_tess.tessellation_factor = gui_settings.tess_factor;
	if (!gui_settings.tessellation)
	{
		// Setting this to zero sets all tessellation factors to 1.0 in the shader
		ubo_tess.tessellation_factor = 0.0f;
	}

	/* Dynamically tessellation */
	uniform_buffers.dynamically_tessellation->convert_and_update(ubo_tess);

	/* Statically tessellation */
	uniform_buffers.statically_tessellation->convert_and_update(ubo_tess);
}

/**s
 * 	@fn void PatchControlPoints::create_pipelines()
 * 	@brief Creating graphical pipelines for tessellation.
 */
void PatchControlPoints::create_pipelines()
{
	/* Setup for pipelines */
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(
	        VK_PRIMITIVE_TOPOLOGY_PATCH_LIST, /* used in tessellation */
	        0,
	        VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(
	        VK_POLYGON_MODE_FILL,
	        VK_CULL_MODE_FRONT_BIT,
	        VK_FRONT_FACE_COUNTER_CLOCKWISE,
	        0);

	/* Wireframe mode */
	if (get_device().get_gpu().get_features().fillModeNonSolid)
	{
		rasterization_state.polygonMode = VK_POLYGON_MODE_LINE;
	}

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
	        VK_TRUE, /* depthTestEnable */
	        VK_TRUE, /* depthWriteEnable */
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

	/* Binding description */
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};

	/* Attribute descriptions */
	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),                               // Position
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)),        // Normal
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	std::array<VkPipelineShaderStageCreateInfo, 4> shader_stages{};
	shader_stages[0] = load_shader("patch_control_points/tess.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("patch_control_points/tess.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	shader_stages[2] = load_shader("patch_control_points/tess.tesc", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
	shader_stages[3] = load_shader("patch_control_points/tess.tese", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);

	/* Use the pNext to point to the rendering create struct */
	VkGraphicsPipelineCreateInfo graphics_create{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
	graphics_create.pNext               = VK_NULL_HANDLE;
	graphics_create.renderPass          = render_pass;
	graphics_create.pInputAssemblyState = &input_assembly_state;
	graphics_create.pRasterizationState = &rasterization_state;
	graphics_create.pColorBlendState    = &color_blend_state;
	graphics_create.pMultisampleState   = &multisample_state;
	graphics_create.pViewportState      = &viewport_state;
	graphics_create.pDepthStencilState  = &depth_stencil_state;
	graphics_create.pDynamicState       = &dynamic_state;
	graphics_create.pVertexInputState   = &vertex_input_state;
	graphics_create.stageCount          = static_cast<uint32_t>(shader_stages.size());
	graphics_create.pStages             = shader_stages.data();

	/* Specific setup of statically_tessellation pipeline */

	constexpr uint32_t                    patch_control_points = 3;
	VkPipelineTessellationStateCreateInfo tessellation_state =
	    vkb::initializers::pipeline_tessellation_state_create_info(patch_control_points); /* static patch control points */
	graphics_create.pTessellationState = &tessellation_state;
	graphics_create.layout             = pipeline_layouts.statically_tessellation;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &graphics_create, VK_NULL_HANDLE, &pipeline.statically_tessellation));

	/* Specific setup of dynamically_tessellation pipeline  */

	/*
	 * patchControlPoints might be set with any valid value i.e. 0 < patchControlPoints <= 32
	 * because it is set dynamically using vkCmdSetPatchControlPointsEXT
	 */
	tessellation_state.patchControlPoints = 1; /* set to 1 to demonstrate that value from vkCmdSetPatchControlPointsEXT is used */
	graphics_create.layout                = pipeline_layouts.dynamically_tessellation;

	/* Add patch control points dynamic state */
	dynamic_state_enables.push_back(VK_DYNAMIC_STATE_PATCH_CONTROL_POINTS_EXT);
	dynamic_state.pDynamicStates    = dynamic_state_enables.data();
	dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_state_enables.size());

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &graphics_create, VK_NULL_HANDLE, &pipeline.dynamically_tessellation));
}

/**
 * 	@fn void PatchControlPoints::build_command_buffers()
 * 	@brief Creating command buffers and drawing particular elements on window.
 * 	@details Drawing object list:
 * 			 - models from tessellation scene
 */
void PatchControlPoints::build_command_buffers()
{
	std::array<VkClearValue, 2> clear_values{};
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0};

	const std::array<glm::vec3, 2> directions = {glm::vec3(2.5f, -1.0f, 3.0f),  /* first model */
	                                             glm::vec3(0.0f, -1.0f, 3.0f)}; /* second model */

	constexpr uint32_t patch_control_points = 3;

	int i = -1; /* Required for accessing element in framebuffers vector */
	for (auto &draw_cmd_buffer : draw_cmd_buffers)
	{
		++i;
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

		vkCmdBindDescriptorSets(draw_cmd_buffer,
		                        VK_PIPELINE_BIND_POINT_GRAPHICS,
		                        pipeline_layouts.statically_tessellation,
		                        0,
		                        1,
		                        &descriptor_sets.statically_tessellation,
		                        0,
		                        nullptr);

		push_const_block.direction = directions[0];
		vkCmdPushConstants(draw_cmd_buffer,
		                   pipeline_layouts.statically_tessellation,
		                   VK_SHADER_STAGE_VERTEX_BIT,
		                   0,
		                   sizeof(push_const_block),
		                   &push_const_block);

		vkCmdBindPipeline(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.statically_tessellation);

		draw_model(model, draw_cmd_buffer);

		//	dynamically tessellation
		vkCmdBindDescriptorSets(draw_cmd_buffer,
		                        VK_PIPELINE_BIND_POINT_GRAPHICS,
		                        pipeline_layouts.dynamically_tessellation,
		                        0,
		                        1,
		                        &descriptor_sets.dynamically_tessellation,
		                        0,
		                        nullptr);

		push_const_block.direction = directions[1];
		vkCmdPushConstants(draw_cmd_buffer,
		                   pipeline_layouts.dynamically_tessellation,
		                   VK_SHADER_STAGE_VERTEX_BIT,
		                   0,
		                   sizeof(push_const_block),
		                   &push_const_block);

		vkCmdBindPipeline(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.dynamically_tessellation);

		vkCmdSetPatchControlPointsEXT(draw_cmd_buffer, patch_control_points);

		draw_model(model, draw_cmd_buffer);

		/* UI */
		draw_ui(draw_cmd_buffer);

		vkCmdEndRenderPass(draw_cmd_buffer);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffer));
	}
}

/**
 * 	@fn void PatchControlPoints::create_descriptor_pool()
 * 	@brief Creating descriptor pool with size adjusted to use uniform buffer and image sampler
 */
void PatchControlPoints::create_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4),
	};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(
	        static_cast<uint32_t>(pool_sizes.size()),
	        pool_sizes.data(),
	        2);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(),
	                                &descriptor_pool_create_info,
	                                nullptr,
	                                &descriptor_pool));
}

/**
 * 	@fn void PatchControlPoints::setup_descriptor_set_layout()
 * 	@brief Creating layout for descriptor sets
 */
void PatchControlPoints::setup_descriptor_set_layout()
{
	/* First descriptor set */
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_VERTEX_BIT,
	        0),
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
	        1),
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(),
	                                                         static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(),
	                                     &descriptor_layout_create_info,
	                                     nullptr,
	                                     &descriptor_set_layouts.statically_tessellation));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &descriptor_set_layouts.statically_tessellation,
	        1);

	/* Pass scene node information via push constants */
	VkPushConstantRange push_constant_range            = vkb::initializers::push_constant_range(VK_SHADER_STAGE_VERTEX_BIT,
	                                                                                            sizeof(push_const_block),
	                                                                                            0);
	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(),
	                                &pipeline_layout_create_info,
	                                nullptr,
	                                &pipeline_layouts.statically_tessellation));

	/* Second descriptor set */
	set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_VERTEX_BIT,
	        0),
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
	        1),
	};

	descriptor_layout_create_info.pBindings    = set_layout_bindings.data();
	descriptor_layout_create_info.bindingCount = static_cast<uint32_t>(set_layout_bindings.size());

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(),
	                                     &descriptor_layout_create_info,
	                                     nullptr,
	                                     &descriptor_set_layouts.dynamically_tessellation));

	pipeline_layout_create_info.pSetLayouts = &descriptor_set_layouts.dynamically_tessellation;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(),
	                                //&pipeline_layout_create_info_2,
	                                &pipeline_layout_create_info,
	                                nullptr,
	                                &pipeline_layouts.dynamically_tessellation));
}

/**
 * 	@fn void PatchControlPoints::create_descriptor_sets()
 * 	@brief Creating descriptor sets for 2 separate pipelines.
 */
void PatchControlPoints::create_descriptor_sets()
{
	/* First descriptor set */
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layouts.statically_tessellation,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(),
	                                  &alloc_info,
	                                  &descriptor_sets.statically_tessellation));

	VkDescriptorBufferInfo matrix_common_buffer_descriptor = create_descriptor(*uniform_buffers.common);
	VkDescriptorBufferInfo matrix_tess_buffer_descriptor   = create_descriptor(*uniform_buffers.statically_tessellation);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(
	        descriptor_sets.statically_tessellation,
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        0,
	        &matrix_common_buffer_descriptor),
	    vkb::initializers::write_descriptor_set(
	        descriptor_sets.statically_tessellation,
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        1,
	        &matrix_tess_buffer_descriptor)};

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()),
	                       write_descriptor_sets.data(), 0, VK_NULL_HANDLE);

	/* Second descriptor set */
	alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layouts.dynamically_tessellation,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.dynamically_tessellation));

	matrix_tess_buffer_descriptor = create_descriptor(*uniform_buffers.dynamically_tessellation);

	write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(
	        descriptor_sets.dynamically_tessellation,
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        0,
	        &matrix_common_buffer_descriptor),
	    vkb::initializers::write_descriptor_set(
	        descriptor_sets.dynamically_tessellation,
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        1,
	        &matrix_tess_buffer_descriptor)};

	vkUpdateDescriptorSets(get_device().get_handle(),
	                       static_cast<uint32_t>(write_descriptor_sets.size()),
	                       write_descriptor_sets.data(),
	                       0,
	                       VK_NULL_HANDLE);
}

/**
 * @fn void PatchControlPoints::request_gpu_features(vkb::PhysicalDevice &gpu)
 * @brief Enabling features related to Vulkan extensions
 */
void PatchControlPoints::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	/* Enable extension features required by this sample
	   These are passed to device creation via a pNext structure chain */
	auto &requested_extended_dynamic_state2_features =
	    gpu.request_extension_features<VkPhysicalDeviceExtendedDynamicState2FeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT);
	requested_extended_dynamic_state2_features.extendedDynamicState2                   = VK_TRUE;
	requested_extended_dynamic_state2_features.extendedDynamicState2PatchControlPoints = VK_TRUE;

	auto &requested_extended_dynamic_state_feature =
	    gpu.request_extension_features<VkPhysicalDeviceExtendedDynamicStateFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT);
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
 * @fn void PatchControlPoints::on_update_ui_overlay(vkb::Drawer &drawer)
 * @brief Projecting GUI and transferring data between GUI and app
 */
void PatchControlPoints::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.checkbox("Tessellation Enable", &gui_settings.tessellation))
		{
			update_uniform_buffers();
		}

		/* Maximum tessellation factor is set to 7.0 */
		if (drawer.slider_float("Tessellation Factor", &gui_settings.tess_factor, 3.0f, 7.0f))
		{
			update_uniform_buffers();
		}
	}
}

std::unique_ptr<vkb::VulkanSample> create_patch_control_points()
{
	return std::make_unique<PatchControlPoints>();
}
