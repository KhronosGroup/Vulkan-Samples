/* Copyright (c) 2022, Sascha Willems
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
 * Using VK_EXT_conditional_rendering, which executes or discards draw commands based on values sourced from a buffer
 */

#include "conditional_rendering.h"

#include "gltf_loader.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/pbr_material.h"
#include "scene_graph/components/sub_mesh.h"

ConditionalRendering::ConditionalRendering()
{
	title = "Conditional rendering";
	add_device_extension(VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME);
}

ConditionalRendering::~ConditionalRendering()
{
	if (device)
	{
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
	}
}

void ConditionalRendering::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// We need to enable conditional rendering using a new feature struct
	auto &requested_extension_features                = gpu.request_extension_features<VkPhysicalDeviceConditionalRenderingFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT);
	requested_extension_features.conditionalRendering = VK_TRUE;
}

void ConditionalRendering::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = 2;
	render_pass_begin_info.pClearValues             = clear_values;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		render_pass_begin_info.framebuffer = framebuffers[i];

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport((float) width, (float) height, 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

		uint32_t node_index = 0;
		for (auto &node : linear_scene_nodes)
		{
			glm::mat4 node_transform = node.node->get_transform().get_world_matrix();

			VkDeviceSize offsets[1] = {0};

			const auto &vertex_buffer_pos    = node.sub_mesh->vertex_buffers.at("position");
			const auto &vertex_buffer_normal = node.sub_mesh->vertex_buffers.at("normal");
			auto &      index_buffer         = node.sub_mesh->index_buffer;

			auto mat = dynamic_cast<const vkb::sg::PBRMaterial *>(node.sub_mesh->get_material());

			// Start a conditional rendering block, commands in this block are only executed if the buffer at the current position is 1 at command buffer submission time
			VkConditionalRenderingBeginInfoEXT conditional_rendering_info{};
			conditional_rendering_info.sType  = VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT;
			conditional_rendering_info.buffer = conditional_visibility_buffer->get_handle();
			// We offset into the visibility buffer based on the index of the node to be drawn
			conditional_rendering_info.offset = sizeof(int32_t) * node_index;
			vkCmdBeginConditionalRenderingEXT(draw_cmd_buffers[i], &conditional_rendering_info);

			// Pass data for the current node via push commands
			push_const_block.model_matrix = node_transform;
			push_const_block.color        = glm::vec4(mat->base_color_factor.rgb, 1.0f);
			vkCmdPushConstants(draw_cmd_buffers[i], pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_const_block), &push_const_block);

			vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, vertex_buffer_pos.get(), offsets);
			vkCmdBindVertexBuffers(draw_cmd_buffers[i], 1, 1, vertex_buffer_normal.get(), offsets);
			vkCmdBindIndexBuffer(draw_cmd_buffers[i], index_buffer->get_handle(), 0, node.sub_mesh->index_type);

			vkCmdDrawIndexed(draw_cmd_buffers[i], node.sub_mesh->vertex_indices, 1, 0, 0, 0);

			// End the conditional rendering block
			vkCmdEndConditionalRenderingEXT(draw_cmd_buffers[i]);

			node_index++;
		}

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void ConditionalRendering::load_assets()
{
	vkb::GLTFLoader loader{get_device()};
	scene = loader.read_scene_from_file("scenes/Buggy/glTF-Embedded/Buggy.gltf");
	assert(scene);
	// Store all scene nodes in a linear vector for easier access
	for (auto &mesh : scene->get_components<vkb::sg::Mesh>())
	{
		for (auto &node : mesh->get_nodes())
		{
			for (auto &sub_mesh : mesh->get_submeshes())
			{
				linear_scene_nodes.push_back({mesh->get_name(), node, sub_mesh});
			}
		}
	}
	// By default, all nodes should be visible, so we initialize the list with ones for each element
	conditional_visibility_list.resize(linear_scene_nodes.size());
	std::fill(conditional_visibility_list.begin(), conditional_visibility_list.end(), 1);
}

void ConditionalRendering::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4)};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), 1);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void ConditionalRendering::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &descriptor_set_layout,
	        1);

	// Pass scene node information via push constants
	VkPushConstantRange push_constant_range            = vkb::initializers::push_constant_range(VK_SHADER_STAGE_VERTEX_BIT, sizeof(push_const_block), 0);
	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void ConditionalRendering::setup_descriptor_sets()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layout,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

	VkDescriptorBufferInfo            matrix_buffer_descriptor = create_descriptor(*uniform_buffer);
	std::vector<VkWriteDescriptorSet> write_descriptor_sets    = {
        vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matrix_buffer_descriptor)};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

void ConditionalRendering::prepare_pipelines()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(
	        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	        0,
	        VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(
	        VK_POLYGON_MODE_FILL,
	        VK_CULL_MODE_BACK_BIT,
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

	// Note: Using Reversed depth-buffer for increased precision, so Greater depth values are kept
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

	VkGraphicsPipelineCreateInfo pipeline_create_info =
	    vkb::initializers::pipeline_create_info(
	        pipeline_layout,
	        render_pass,
	        0);

	std::vector<VkPipelineColorBlendAttachmentState> blend_attachment_states = {
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE)};

	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;
	pipeline_create_info.layout              = pipeline_layout;

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;
	pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages    = shader_stages.data();

	// Vertex bindings an attributes for model rendering
	// Binding description, we use separate buffers for the vertex attributes
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX),
	    vkb::initializers::vertex_input_binding_description(1, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX),
	};

	// Attribute descriptions
	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),        // Position
	    vkb::initializers::vertex_input_attribute_description(1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0),        // Normal
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	pipeline_create_info.pVertexInputState = &vertex_input_state;

	shader_stages[0] = load_shader("conditional_rendering/model.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("conditional_rendering/model.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline));
}

// Prepare and initialize uniform buffer containing shader uniforms
void ConditionalRendering::prepare_uniform_buffers()
{
	// Matrices vertex shader uniform buffer
	uniform_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                     sizeof(uniform_data),
	                                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                     VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void ConditionalRendering::update_uniform_buffers()
{
	uniform_data.projection = camera.matrices.perspective;
	uniform_data.view       = camera.matrices.view * glm::mat4(1.0f);
	// Scale the view matrix as the model is pretty large, and also flip it upside down
	uniform_data.view = glm::scale(uniform_data.view, glm::vec3(0.1f, -0.1f, 0.1f));
	uniform_buffer->convert_and_update(uniform_data);
}

// Creates a dedicated buffer to store the visibility information sourced at draw time
void ConditionalRendering::preprare_visibility_buffer()
{
	// Conditional values are 32 bits wide and if it's zero the rendering commands are discarded
	// We therefore create a buffer that can hold int32 conditional values for all nodes in the glTF scene
	// The extension also introduces the new buffer usage flag VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT that we need to set
	conditional_visibility_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                                    sizeof(int32_t) * conditional_visibility_list.size(),
	                                                                    VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT,
	                                                                    VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_visibility_buffer();
}

// Updates the visibility buffer with the currently selected node visibility
void ConditionalRendering::update_visibility_buffer()
{
	conditional_visibility_buffer->update(conditional_visibility_list.data(), sizeof(int32_t) * conditional_visibility_list.size());
}

void ConditionalRendering::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

bool ConditionalRendering::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position(glm::vec3(1.9f, 2.05f, -18.0f));
	camera.set_rotation(glm::vec3(-11.25f, -38.0f, 0.0f));

	// Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.set_perspective(60.0f, (float) width / (float) height, 256.0f, 0.1f);
	load_assets();
	prepare_uniform_buffers();
	preprare_visibility_buffer();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_sets();
	build_command_buffers();
	prepared = true;
	return true;
}

void ConditionalRendering::render(float delta_time)
{
	if (!prepared)
		return;
	draw();
	if (camera.updated)
		update_uniform_buffers();
}

void ConditionalRendering::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Visibility"))
	{
		if (drawer.button("All"))
		{
			for (auto i = 0; i < conditional_visibility_list.size(); i++)
			{
				conditional_visibility_list[i] = 1;
			}
			update_visibility_buffer();
		}
		ImGui::SameLine();
		if (drawer.button("None"))
		{
			for (auto i = 0; i < conditional_visibility_list.size(); i++)
			{
				conditional_visibility_list[i] = 0;
			}
			update_visibility_buffer();
		}
		ImGui::NewLine();

		ImGui::BeginChild("InnerRegion", ImVec2(200.0f, 400.0f), false);
		uint32_t idx = 0;
		for (auto &node : linear_scene_nodes)
		{
			if (drawer.checkbox(("[" + std::to_string(idx) + "] " + node.name).c_str(), &conditional_visibility_list[idx]))
			{
				update_visibility_buffer();
			}
			idx++;
		}
		ImGui::EndChild();
	}
}

bool ConditionalRendering::resize(const uint32_t width, const uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	update_uniform_buffers();
	return true;
}

std::unique_ptr<vkb::Application> create_conditional_rendering()
{
	return std::make_unique<ConditionalRendering>();
}
