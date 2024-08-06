/* Copyright (c) 2023-2024, Mobica
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

#include "dynamic_blending.h"

#include "common/vk_common.h"
#include "gui.h"

DynamicBlending::DynamicBlending()
{
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
	add_device_extension(VK_EXT_BLEND_OPERATION_ADVANCED_EXTENSION_NAME);

	title = "Dynamic blending";
}

DynamicBlending::~DynamicBlending()
{
	if (has_device())
	{
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
		vkDestroyDescriptorPool(get_device().get_handle(), descriptor_pool, nullptr);
	}
}

bool DynamicBlending::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position({0.0f, 0.0f, -5.0f});
	camera.set_rotation({-15.0f, 15.0f, 0.0f});
	camera.set_perspective(45.0f, static_cast<float>(width) / static_cast<float>(height), 256.0f, 0.1f);

	initialize_operator_names();
	prepare_uniform_buffers();
	prepare_scene();
	setup_descriptor_pool();
	create_descriptor_set_layout();
	create_descriptor_set();
	create_pipelines();
	build_command_buffers();

	rnd_engine = std::default_random_engine(time(nullptr));
	rnd_dist   = std::uniform_real_distribution<float>(0.0f, 1.0f);

	prepared = true;

	return true;
}

void DynamicBlending::initialize_operator_names()
{
	for (uint32_t i = VK_BLEND_OP_ADD; i <= VK_BLEND_OP_MAX; ++i)
	{
		VkBlendOp op = static_cast<VkBlendOp>(i);
		blend_operator.values.push_back(op);
		blend_operator.names.push_back(vkb::to_string(op));
	}
	current_blend_color_operator_index = VK_BLEND_OP_ADD;
	current_blend_alpha_operator_index = VK_BLEND_OP_ADD;

	for (uint32_t i = VK_BLEND_OP_ZERO_EXT; i <= VK_BLEND_OP_BLUE_EXT; ++i)
	{
		VkBlendOp op = static_cast<VkBlendOp>(i);
		advanced_blend_operator.values.push_back(op);
		advanced_blend_operator.names.push_back(vkb::to_string(op));
	}
	current_advanced_blend_operator_index = VK_BLEND_OP_SRC_OVER_EXT - VK_BLEND_OP_ZERO_EXT;

	for (uint32_t i = VK_BLEND_FACTOR_ZERO; i <= VK_BLEND_FACTOR_SRC_ALPHA_SATURATE; ++i)
	{
		// The substr(16) call is used to drop the "VK_BLEND_FACTOR_" prefix
		blend_factor_names.push_back(vkb::to_string(static_cast<VkBlendFactor>(i)).substr(16));
	}
}

void DynamicBlending::prepare_scene()
{
	vertices = {
	    {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f}},
	    {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f}},
	    {{1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
	    {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

	    {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f}},
	    {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f}},
	    {{1.0f, 1.0f, -1.0f}, {1.0f, 1.0f}},
	    {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f}},
	};

	std::vector<uint32_t> indices = {
	    6, 5, 4, 4, 7, 6,
	    0, 1, 2, 2, 3, 0};

	index_count = static_cast<uint32_t>(indices.size());

	vertex_buffer_size     = static_cast<uint32_t>(vertices.size() * sizeof(Vertex));
	auto index_buffer_size = indices.size() * sizeof(uint32_t);

	vertex_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                    vertex_buffer_size,
	                                                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                                                    VMA_MEMORY_USAGE_GPU_TO_CPU);
	vertex_buffer->update(vertices.data(), vertex_buffer_size);

	index_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                   index_buffer_size,
	                                                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	                                                   VMA_MEMORY_USAGE_GPU_TO_CPU);
	index_buffer->update(indices.data(), index_buffer_size);

	face_preferences[0].index_offset      = 0;
	face_preferences[0].index_count       = index_count / 2;
	face_preferences[0].color_bit_enabled = {true, true, true, true};
	face_preferences[0].color             = {{{1.0f, 0.0f, 0.0f, 1.0f},
	                                          {0.0f, 1.0f, 0.0f, 1.0f},
	                                          {0.0f, 0.0f, 1.0f, 1.0f},
	                                          {0.0f, 0.0f, 0.0f, 1.0f}}};

	face_preferences[1].index_offset      = index_count / 2;
	face_preferences[1].index_count       = index_count / 2;
	face_preferences[1].color_bit_enabled = {true, true, true, true};
	face_preferences[1].color             = {{{0.0f, 1.0f, 1.0f, 0.5f},
	                                          {1.0f, 0.0f, 1.0f, 0.5f},
	                                          {1.0f, 1.0f, 0.0f, 0.5f},
	                                          {1.0f, 1.0f, 1.0f, 0.5f}}};
}

void DynamicBlending::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// Query the extended dynamic state support
	eds_feature_support       = {};
	eds_feature_support.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;

	VkPhysicalDeviceFeatures2KHR features2{};
	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
	features2.pNext = &eds_feature_support;
	vkGetPhysicalDeviceFeatures2KHR(gpu.get_handle(), &features2);

	{
		// Only request the features that we support
		auto &features                                   = gpu.request_extension_features<VkPhysicalDeviceExtendedDynamicState3FeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT);
		features.extendedDynamicState3ColorWriteMask     = eds_feature_support.extendedDynamicState3ColorWriteMask;
		features.extendedDynamicState3ColorBlendEnable   = VK_TRUE;        // We must have this or the sample isn't useful
		features.extendedDynamicState3ColorBlendAdvanced = eds_feature_support.extendedDynamicState3ColorBlendAdvanced;
		features.extendedDynamicState3ColorBlendEquation = eds_feature_support.extendedDynamicState3ColorBlendEquation;
	}
	{
		auto &features                           = gpu.request_extension_features<VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_FEATURES_EXT);
		features.advancedBlendCoherentOperations = VK_TRUE;
	}
}

void DynamicBlending::prepare_uniform_buffers()
{
	camera_ubo = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(CameraUbo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	color_ubo  = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(ColorUbo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
}

void DynamicBlending::update_uniform_buffers()
{
	CameraUbo cam;
	cam.model      = glm::mat4(1.0f);
	cam.model      = glm::translate(cam.model, glm::vec3(0.0f));
	cam.view       = camera.matrices.view;
	cam.projection = camera.matrices.perspective;

	camera_ubo->convert_and_update(cam);

	update_color();

	glm::mat4 invView = glm::inverse(camera.matrices.view);
	glm::vec4 plane0(vertices[0].pos[0], vertices[0].pos[1], vertices[0].pos[2], 1.0f);
	glm::vec4 plane1(vertices[4].pos[0], vertices[4].pos[1], vertices[4].pos[2], 1.0f);

	plane0 = invView * plane0;
	plane1 = invView * plane1;

	reverse = plane0.z < plane1.z;

	rebuild_command_buffers();
}

void DynamicBlending::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2u),
	};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(
	        static_cast<uint32_t>(pool_sizes.size()),
	        pool_sizes.data(),
	        static_cast<uint32_t>(pool_sizes.size()));
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void DynamicBlending::create_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1u)};

	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings);
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_set_layout_create_info, nullptr, &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layout);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void DynamicBlending::create_descriptor_set()
{
	VkDescriptorSetAllocateInfo alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1u);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

	VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*camera_ubo);
	VkDescriptorBufferInfo color_descriptor  = create_descriptor(*color_ubo);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, &buffer_descriptor),
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, &color_descriptor),
	};

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0u, nullptr);
}

void DynamicBlending::create_pipelines()
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
	        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	        VK_TRUE);

	VkPipelineColorBlendAdvancedStateCreateInfoEXT blendAdvancedEXT{};
	blendAdvancedEXT.sType        = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_ADVANCED_STATE_CREATE_INFO_EXT;
	blendAdvancedEXT.blendOverlap = VK_BLEND_OVERLAP_UNCORRELATED_EXT;

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(
	        1,
	        &blend_attachment_state);

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
	    VK_DYNAMIC_STATE_SCISSOR,
	};

	if (eds_feature_support.extendedDynamicState3ColorWriteMask)
	{
		dynamic_state_enables.push_back(VK_DYNAMIC_STATE_COLOR_WRITE_MASK_EXT);
	}

	if (eds_feature_support.extendedDynamicState3ColorBlendEnable)
	{
		dynamic_state_enables.push_back(VK_DYNAMIC_STATE_COLOR_BLEND_ENABLE_EXT);
	}

	switch (current_blend_option)
	{
		case 0:
			if (eds_feature_support.extendedDynamicState3ColorBlendEquation)
			{
				dynamic_state_enables.push_back(VK_DYNAMIC_STATE_COLOR_BLEND_EQUATION_EXT);
			}
			break;
		case 1:
			if (eds_feature_support.extendedDynamicState3ColorBlendAdvanced)
			{
				dynamic_state_enables.push_back(VK_DYNAMIC_STATE_COLOR_BLEND_ADVANCED_EXT);
			}
			break;
	}

	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0);

	const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)),
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)),
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};
	shader_stages[0] = load_shader("dynamic_blending", "blending.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("dynamic_blending", "blending.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

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
	graphics_create.pTessellationState  = VK_NULL_HANDLE;
	graphics_create.stageCount          = 2;
	graphics_create.pStages             = shader_stages.data();
	graphics_create.layout              = pipeline_layout;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(),
	                                   pipeline_cache,
	                                   1,
	                                   &graphics_create,
	                                   VK_NULL_HANDLE,
	                                   &pipeline));
}

void DynamicBlending::update_pipeline()
{
	vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
	create_pipelines();
}

void DynamicBlending::update_color()
{
	for (uint32_t face = 0; face < 2; ++face)
	{
		for (uint32_t vertex = 0; vertex < 4; ++vertex)
		{
			auto &input_color             = face_preferences[face].color[vertex];
			color.data[face * 4 + vertex] = glm::vec4(input_color[0], input_color[1], input_color[2], input_color[3]);
		}
	}
	color_ubo->convert_and_update(color);
}

void DynamicBlending::randomize_color(std::array<float, 4> &color, bool alpha)
{
	for (size_t i = 0; i < 3; ++i)
	{
		color[i] = rnd_dist(rnd_engine);
	}
	if (alpha)
	{
		color[3] = rnd_dist(rnd_engine);
	}
}

void DynamicBlending::update_color_uniform()
{
	update_color();
	rebuild_command_buffers();
}

void DynamicBlending::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	std::array<VkClearValue, 2> clear_values;
	clear_values[0].color        = {clear_color[0], clear_color[1], clear_color[2], clear_color[3]};
	clear_values[1].depthStencil = {0.0f, 0u};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = static_cast<uint32_t>(clear_values.size());
	render_pass_begin_info.pClearValues             = clear_values.data();

	for (uint32_t i = 0u; i < draw_cmd_buffers.size(); ++i)
	{
		render_pass_begin_info.framebuffer = framebuffers[i];
		auto &cmd_buff                     = draw_cmd_buffers[i];

		VK_CHECK(vkBeginCommandBuffer(cmd_buff, &command_buffer_begin_info));

		vkCmdBeginRenderPass(cmd_buff, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(cmd_buff, 0u, 1u, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmd_buff, 0u, 1u, &scissor);

		{
			vkCmdBindDescriptorSets(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0u, 1u, &descriptor_set, 0u, nullptr);
			vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			VkDeviceSize offsets[1] = {0};
			vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, vertex_buffer->get(), offsets);
			vkCmdBindIndexBuffer(draw_cmd_buffers[i], index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);

			if (eds_feature_support.extendedDynamicState3ColorBlendEnable)
			{
				const VkBool32 blend_enable = this->blend_enable;
				vkCmdSetColorBlendEnableEXT(cmd_buff, 0, 1, &blend_enable);
			}

			if (current_blend_option == 0)
			{
				if (eds_feature_support.extendedDynamicState3ColorBlendEquation)
				{
					VkColorBlendEquationEXT color_blend_equation;
					color_blend_equation.colorBlendOp        = blend_operator.values[current_blend_color_operator_index];
					color_blend_equation.srcColorBlendFactor = static_cast<VkBlendFactor>(current_src_color_blend_factor);
					color_blend_equation.dstColorBlendFactor = static_cast<VkBlendFactor>(current_dst_color_blend_factor);
					color_blend_equation.alphaBlendOp        = blend_operator.values[current_blend_alpha_operator_index];
					color_blend_equation.srcAlphaBlendFactor = static_cast<VkBlendFactor>(current_src_alpha_blend_factor);
					color_blend_equation.dstAlphaBlendFactor = static_cast<VkBlendFactor>(current_dst_alpha_blend_factor);
					vkCmdSetColorBlendEquationEXT(cmd_buff, 0, 1, &color_blend_equation);
				}
			}
			else
			{
				if (eds_feature_support.extendedDynamicState3ColorBlendAdvanced)
				{
					VkColorBlendAdvancedEXT color_blend_advanced;
					color_blend_advanced.advancedBlendOp  = advanced_blend_operator.values[current_advanced_blend_operator_index];
					color_blend_advanced.srcPremultiplied = src_premultiplied;
					color_blend_advanced.dstPremultiplied = dst_premultiplied;
					color_blend_advanced.blendOverlap     = VK_BLEND_OVERLAP_CONJOINT_EXT;
					color_blend_advanced.clampResults     = VK_TRUE;
					vkCmdSetColorBlendAdvancedEXT(cmd_buff, 0, 1, &color_blend_advanced);
				}
			}

			if (reverse)
			{
				build_command_buffer_for_plane(cmd_buff, face_preferences[1]);
				build_command_buffer_for_plane(cmd_buff, face_preferences[0]);
			}
			else
			{
				build_command_buffer_for_plane(cmd_buff, face_preferences[0]);
				build_command_buffer_for_plane(cmd_buff, face_preferences[1]);
			}
		}

		draw_ui(cmd_buff);

		vkCmdEndRenderPass(cmd_buff);

		VK_CHECK(vkEndCommandBuffer(cmd_buff));
	}
}

void DynamicBlending::build_command_buffer_for_plane(VkCommandBuffer &command_buffer, FacePreferences preferences)
{
	if (eds_feature_support.extendedDynamicState3ColorWriteMask)
	{
		std::array<VkColorComponentFlags, 1> color_bit = {
		    (preferences.color_bit_enabled[0] ? VK_COLOR_COMPONENT_R_BIT : 0u) |
		    (preferences.color_bit_enabled[1] ? VK_COLOR_COMPONENT_G_BIT : 0u) |
		    (preferences.color_bit_enabled[2] ? VK_COLOR_COMPONENT_B_BIT : 0u) |
		    (preferences.color_bit_enabled[3] ? VK_COLOR_COMPONENT_A_BIT : 0u)};
		vkCmdSetColorWriteMaskEXT(command_buffer, 0, 1, color_bit.data());
	}
	vkCmdDrawIndexed(command_buffer, preferences.index_count, 1, preferences.index_offset, 0, 0);
}

void DynamicBlending::on_update_ui_overlay(vkb::Drawer &drawer)
{
	uint32_t item_id = 0;

	auto add_color_edit = [&](const char *caption, std::array<float, 4> &color) {
		ImGuiColorEditFlags flags            = ImGuiColorEditFlags_None | ImGuiColorEditFlags_Float;
		float               color_edit_width = 200;
		ImGui::PushID(++item_id);
		if (drawer.color_op<vkb::Drawer::ColorOp::Edit>(caption, color, color_edit_width, flags))
		// if (drawer.color_edit(caption, color, color_edit_width, flags))
		{
			update_color_uniform();
		}
		ImGui::PopID();
	};

	auto add_color_mask_checkbox = [&](const char *caption, bool &enabled, bool same_line = true) {
		ImGui::PushID(++item_id);
		if (drawer.checkbox(caption, &enabled))
		{
			update_color_uniform();
		}
		ImGui::PopID();
		if (same_line)
		{
			ImGui::SameLine();
		}
	};

	auto add_next_button = [&](int32_t &index, int32_t first, int32_t last) {

	};

	auto add_combo_with_button = [&](const char *caption, int32_t &index, int32_t first, int32_t last, std::vector<std::string> names) {
		ImGui::PushID(++item_id);
		if (drawer.button("Next"))
		{
			index = (index + 1) % (last - first + 1);
			update_uniform_buffers();
		}
		ImGui::PopID();
		ImGui::SameLine();
		if (drawer.combo_box(caption, &index, names))
		{
			update_uniform_buffers();
		}
	};

	add_color_edit("Background", clear_color);

	for (int i = 0; i < 2; ++i)
	{
		FacePreferences &current_face = face_preferences[i];
		if (drawer.header(current_face_index == 0 ? "First face" : "Second face"))
		{
			add_color_edit("Top left", current_face.color[0]);
			add_color_edit("Top right", current_face.color[1]);
			add_color_edit("Bottom left", current_face.color[2]);
			add_color_edit("Bottom right", current_face.color[3]);

			ImGui::PushID(++item_id);
			if (drawer.button("Random"))
			{
				for (int i = 0; i < 4; ++i)
				{
					randomize_color(current_face.color[i]);
				}
				update_color();
			}
			ImGui::PopID();

			if (eds_feature_support.extendedDynamicState3ColorWriteMask)
			{
				drawer.text("Color write mask");
				add_color_mask_checkbox("Red", current_face.color_bit_enabled[0]);
				add_color_mask_checkbox("Green", current_face.color_bit_enabled[1]);
				add_color_mask_checkbox("Blue", current_face.color_bit_enabled[2]);
				add_color_mask_checkbox("Alpha", current_face.color_bit_enabled[3], false);
			}
		}
	}

	if (drawer.header("Blending"))
	{
		if (eds_feature_support.extendedDynamicState3ColorBlendEnable)
		{
			if (drawer.checkbox("Enabled", &blend_enable))
			{
				update_color_uniform();
			}
		}
		if (eds_feature_support.extendedDynamicState3ColorBlendAdvanced)
		{
			if (drawer.radio_button("BlendEquationEXT", &current_blend_option, 0))
			{
				update_pipeline();
				update_color_uniform();
			}
			if (drawer.radio_button("BlendAdvancedEXT", &current_blend_option, 1))
			{
				update_pipeline();
				update_color_uniform();
			}
		}
		switch (current_blend_option)
		{
			case 0:
				if (eds_feature_support.extendedDynamicState3ColorBlendEquation)
				{
					if (drawer.header("BlendEquationEXT"))
					{
						add_combo_with_button("Color operator", current_blend_color_operator_index, VK_BLEND_OP_ADD, VK_BLEND_OP_MAX, blend_operator.names);
						add_combo_with_button("SrcColorBlendFactor", current_src_color_blend_factor, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_SRC_ALPHA_SATURATE, blend_factor_names);
						add_combo_with_button("DstColorBlendFactor", current_dst_color_blend_factor, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_SRC_ALPHA_SATURATE, blend_factor_names);

						add_combo_with_button("Alpha operator", current_blend_alpha_operator_index, VK_BLEND_OP_ADD, VK_BLEND_OP_MAX, blend_operator.names);
						add_combo_with_button("SrcAlphaBlendFactor", current_src_alpha_blend_factor, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_SRC_ALPHA_SATURATE, blend_factor_names);
						add_combo_with_button("DstAlphaBlendFactor", current_dst_alpha_blend_factor, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_SRC_ALPHA_SATURATE, blend_factor_names);
					}
				}
				break;
			case 1:
				if (eds_feature_support.extendedDynamicState3ColorBlendAdvanced)
				{
					if (drawer.header("BlendAdvancedEXT"))
					{
						add_combo_with_button("Operator", current_advanced_blend_operator_index, VK_BLEND_OP_ZERO_EXT, VK_BLEND_OP_BLUE_EXT, advanced_blend_operator.names);
						if (drawer.checkbox("Src premultiplied", &src_premultiplied))
						{
							update_color_uniform();
						}
						if (drawer.checkbox("Dst premultiplied", &dst_premultiplied))
						{
							update_color_uniform();
						}
					}
				}
				break;
		}
	}
}

void DynamicBlending::render(float delta_time)
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

void DynamicBlending::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	VK_CHECK(vkQueueSubmit(queue, 1u, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();

	VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
}

bool DynamicBlending::resize(const uint32_t width, const uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	update_uniform_buffers();
	return true;
}

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::C>> create_dynamic_blending()
{
	return std::make_unique<DynamicBlending>();
}
