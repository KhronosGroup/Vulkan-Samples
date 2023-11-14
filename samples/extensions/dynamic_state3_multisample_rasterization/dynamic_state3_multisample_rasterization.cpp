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

#include "dynamic_state3_multisample_rasterization.h"

#include "gltf_loader.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/pbr_material.h"
#include "scene_graph/components/sub_mesh.h"

DynamicState3MultisampleRasterization::DynamicState3MultisampleRasterization()
{
	title = "DynamicState3 Multisample Rasterization";
}

DynamicState3MultisampleRasterization::~DynamicState3MultisampleRasterization()
{
	if (device)
	{
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
	}
}

bool DynamicState3MultisampleRasterization::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position(glm::vec3(1.9f, 2.05f, -18.0f));
	camera.set_rotation(glm::vec3(-11.25f, -38.0f, 0.0f));

	// Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 256.0f, 0.1f);
	load_assets();
	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_sets();
	build_command_buffers();

	prepare_pipelines();
	prepared = true;
	return true;
}

void DynamicState3MultisampleRasterization::build_command_buffers()
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

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

		for (auto &node : scene_nodes)
		{
			const auto &vertex_buffer_pos    = node.sub_mesh->vertex_buffers.at("position");
			const auto &vertex_buffer_normal = node.sub_mesh->vertex_buffers.at("normal");
			const auto & vertex_buffer_uv = node.sub_mesh->vertex_buffers.at("texcoord_0");
			auto       &index_buffer         = node.sub_mesh->index_buffer;

			// Pass data for the current node via push commands
			auto node_material            = dynamic_cast<const vkb::sg::PBRMaterial *>(node.sub_mesh->get_material());
			push_const_block.model_matrix = node.node->get_transform().get_world_matrix();

			push_const_block.base_color_factor = node_material->base_color_factor;
			push_const_block.metallic_factor   = node_material->metallic_factor;
			push_const_block.roughness_factor  = node_material->roughness_factor;
			push_const_block.normal_texture_index = -1;
			push_const_block.pbr_texture_index    = -1;

			auto base_color_texture = node_material->textures.find("base_color_texture");

			if (base_color_texture != node_material->textures.end())
			{
				auto base_color_texture_name          = base_color_texture->second->get_name();
				push_const_block.base_texture_index   = name_to_texture_id.at(base_color_texture_name);
			}

			auto normal_texture = node_material->textures.find("normal_texture");

			if (normal_texture != node_material->textures.end())
			{
				auto normal_texture_name              = normal_texture->second->get_name();
				push_const_block.normal_texture_index = name_to_texture_id.at(normal_texture_name);
			}

			auto metallic_roughness_texture = node_material->textures.find("metallic_roughness_texture");

			if (metallic_roughness_texture != node_material->textures.end())
			{
				auto metallic_roughness_texture_name = metallic_roughness_texture->second->get_name();
				push_const_block.pbr_texture_index = name_to_texture_id.at(metallic_roughness_texture_name);
			}

			vkCmdPushConstants(draw_cmd_buffers[i], pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_const_block), &push_const_block);

			VkDeviceSize offsets[1] = {0};
			vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, vertex_buffer_pos.get(), offsets);
			vkCmdBindVertexBuffers(draw_cmd_buffers[i], 1, 1, vertex_buffer_normal.get(), offsets);
			vkCmdBindVertexBuffers(draw_cmd_buffers[i], 2, 1, vertex_buffer_uv.get(), offsets);
			vkCmdBindIndexBuffer(draw_cmd_buffers[i], index_buffer->get_handle(), 0, node.sub_mesh->index_type);

			vkCmdDraw(draw_cmd_buffers[i], node.sub_mesh->vertex_indices, 1, 0, 0);
		}

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void DynamicState3MultisampleRasterization::load_assets()
{
	vkb::GLTFLoader loader{get_device()};
	scene = loader.read_scene_from_file("scenes/space_module/SpaceModule.gltf");
	assert(scene);
	// Store all scene nodes in a linear vector for easier access
	for (auto &mesh : scene->get_components<vkb::sg::Mesh>())
	{
		for (auto &node : mesh->get_nodes())
		{
			for (auto &sub_mesh : mesh->get_submeshes())
			{
				scene_nodes.push_back({mesh->get_name(), node, sub_mesh});
			}
		}
	}

	auto textures = scene->get_components<vkb::sg::Texture>();

	for (auto texture : textures)
	{
		const auto &name = texture->get_name();
		auto                  image = texture->get_image();
		VkDescriptorImageInfo imageInfo;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView   = image->get_vk_image_view().get_handle();
		imageInfo.sampler     = texture->get_sampler()->vk_sampler.get_handle();

		image_infos.push_back(imageInfo);
		name_to_texture_id.emplace(name, image_infos.size() - 1);
	}
}

void DynamicState3MultisampleRasterization::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(image_infos.size()))
	};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), 2);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void DynamicState3MultisampleRasterization::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, static_cast<uint32_t>(image_infos.size())),
	};

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

void DynamicState3MultisampleRasterization::setup_descriptor_sets()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layout,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

	VkDescriptorBufferInfo            matrix_buffer_descriptor = create_descriptor(*uniform_buffer);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets    = {
        vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matrix_buffer_descriptor),
		vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, image_infos.data(), image_infos.size())
	};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

void DynamicState3MultisampleRasterization::prepare_pipelines()
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
	        VK_COLOR_COMPONENT_R_BIT |
	            VK_COLOR_COMPONENT_G_BIT |
	            VK_COLOR_COMPONENT_B_BIT |
	            VK_COLOR_COMPONENT_A_BIT,
	        VK_TRUE);

	blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment_state.alphaBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

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
	    vkb::initializers::vertex_input_binding_description(2, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX),
	};

	// Attribute descriptions
	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),        // Position
	    vkb::initializers::vertex_input_attribute_description(1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0),        // Normal
	    vkb::initializers::vertex_input_attribute_description(2, 2, VK_FORMAT_R32G32B32_SFLOAT, 0),        // TexCoord
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	pipeline_create_info.pVertexInputState = &vertex_input_state;

	shader_stages[0] = load_shader("dynamic_state3_multisample_rasterization/model.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("dynamic_state3_multisample_rasterization/model.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline));
}

// Prepare and initialize uniform buffer containing shader uniforms
void DynamicState3MultisampleRasterization::prepare_uniform_buffers()
{
	// Matrices vertex shader uniform buffer
	uniform_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                     sizeof(uniform_data),
	                                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                     VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void DynamicState3MultisampleRasterization::update_uniform_buffers()
{
	uniform_data.projection = camera.matrices.perspective;
	// Scale the view matrix as the model is pretty large, and also flip it upside down
	uniform_data.view = glm::scale(camera.matrices.view, glm::vec3(0.1f, -0.1f, 0.1f));
	uniform_buffer->convert_and_update(uniform_data);
}

void DynamicState3MultisampleRasterization::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

void DynamicState3MultisampleRasterization::render(float delta_time)
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

std::unique_ptr<vkb::VulkanSample> create_dynamic_state3_multisample_rasterization()
{
	return std::make_unique<DynamicState3MultisampleRasterization>();
}
