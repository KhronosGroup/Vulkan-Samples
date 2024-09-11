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
 * Push descriptors
 *
 * Note: Requires a device that supports the VK_KHR_push_descriptor extension
 *
 * Push descriptors apply the push constants concept to descriptor sets. So instead of creating
 * per-model descriptor sets (along with a pool for each descriptor type) for rendering multiple objects,
 * this example uses push descriptors to pass descriptor sets for per-model textures and matrices
 * at command buffer creation time.
 */

#include "push_descriptors.h"

#include "core/buffer.h"
#include "scene_graph/components/sub_mesh.h"

PushDescriptors::PushDescriptors()
{
	title = "Push descriptors";

	// Enable extension required for push descriptors
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
}

PushDescriptors::~PushDescriptors()
{
	if (has_device())
	{
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
		for (auto &cube : cubes)
		{
			cube.uniform_buffer.reset();
			cube.texture.image.reset();
			vkDestroySampler(get_device().get_handle(), cube.texture.sampler, nullptr);
		}
		uniform_buffers.scene.reset();
	}
}

void PushDescriptors::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}
}

void PushDescriptors::build_command_buffers()
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

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		const auto &vertex_buffer = models.cube->vertex_buffers.at("vertex_buffer");
		auto       &index_buffer  = models.cube->index_buffer;

		VkDeviceSize offsets[1] = {0};
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, vertex_buffer.get(), offsets);
		vkCmdBindIndexBuffer(draw_cmd_buffers[i], index_buffer->get_handle(), 0, models.cube->index_type);

		// Render two cubes using different descriptor sets using push descriptors
		for (auto &cube : cubes)
		{
			// Instead of preparing the descriptor sets up-front, using push descriptors we can set (push) them inside of a command buffer
			// This allows a more dynamic approach without the need to create descriptor sets for each model
			// Note: dstSet for each descriptor set write is left at zero as this is ignored when using push descriptors

			std::array<VkWriteDescriptorSet, 3> write_descriptor_sets{};

			// Scene matrices
			VkDescriptorBufferInfo scene_buffer_descriptor = create_descriptor(*uniform_buffers.scene);
			write_descriptor_sets[0].sType                 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write_descriptor_sets[0].dstSet                = 0;
			write_descriptor_sets[0].dstBinding            = 0;
			write_descriptor_sets[0].descriptorCount       = 1;
			write_descriptor_sets[0].descriptorType        = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write_descriptor_sets[0].pBufferInfo           = &scene_buffer_descriptor;

			// Model matrices
			VkDescriptorBufferInfo cube_buffer_descriptor = create_descriptor(*cube.uniform_buffer);
			write_descriptor_sets[1].sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write_descriptor_sets[1].dstSet               = 0;
			write_descriptor_sets[1].dstBinding           = 1;
			write_descriptor_sets[1].descriptorCount      = 1;
			write_descriptor_sets[1].descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write_descriptor_sets[1].pBufferInfo          = &cube_buffer_descriptor;

			// Texture
			VkDescriptorImageInfo image_descriptor   = create_descriptor(cube.texture);
			write_descriptor_sets[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write_descriptor_sets[2].dstSet          = 0;
			write_descriptor_sets[2].dstBinding      = 2;
			write_descriptor_sets[2].descriptorCount = 1;
			write_descriptor_sets[2].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write_descriptor_sets[2].pImageInfo      = &image_descriptor;

			vkCmdPushDescriptorSetKHR(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 3, write_descriptor_sets.data());

			draw_model(models.cube, draw_cmd_buffers[i]);
		}

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void PushDescriptors::load_assets()
{
	models.cube      = load_model("scenes/textured_unit_cube.gltf");
	cubes[0].texture = load_texture("textures/crate01_color_height_rgba.ktx", vkb::sg::Image::Color);
	cubes[1].texture = load_texture("textures/crate02_color_height_rgba.ktx", vkb::sg::Image::Color);
}

void PushDescriptors::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info{};
	descriptor_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	// Setting this flag tells the descriptor set layouts that no actual descriptor sets are allocated but instead pushed at command buffer creation time
	descriptor_layout_create_info.flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
	descriptor_layout_create_info.bindingCount = static_cast<uint32_t>(set_layout_bindings.size());
	descriptor_layout_create_info.pBindings    = set_layout_bindings.data();
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layout, 1);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void PushDescriptors::prepare_pipelines()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 0);

	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);

	// Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER);

	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

	std::vector<VkDynamicState> dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(dynamic_state_enables);

	// Vertex bindings and attributes
	const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),                        // Location 0: Position
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3),        // Location 1: Normal
	    vkb::initializers::vertex_input_attribute_description(0, 2, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 6),           // Location 2: UV
	    vkb::initializers::vertex_input_attribute_description(0, 3, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 8),        // Location 3: Color
	};
	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(pipeline_layout, render_pass, 0);
	pipeline_create_info.pVertexInputState            = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState          = &input_assembly_state;
	pipeline_create_info.pRasterizationState          = &rasterization_state;
	pipeline_create_info.pColorBlendState             = &color_blend_state;
	pipeline_create_info.pMultisampleState            = &multisample_state;
	pipeline_create_info.pViewportState               = &viewport_state;
	pipeline_create_info.pDepthStencilState           = &depth_stencil_state;
	pipeline_create_info.pDynamicState                = &dynamic_state;

	const std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {
	    load_shader("push_descriptors", "cube.vert", VK_SHADER_STAGE_VERTEX_BIT),
	    load_shader("push_descriptors", "cube.frag", VK_SHADER_STAGE_FRAGMENT_BIT)};

	pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages    = shader_stages.data();

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline));
}

void PushDescriptors::prepare_uniform_buffers()
{
	// Vertex shader scene uniform buffer block
	uniform_buffers.scene = std::make_unique<vkb::core::BufferC>(get_device(),
	                                                             sizeof(UboScene),
	                                                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                             VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Vertex shader cube model uniform buffer blocks
	for (auto &cube : cubes)
	{
		cube.uniform_buffer = std::make_unique<vkb::core::BufferC>(get_device(),
		                                                           sizeof(glm::mat4),
		                                                           VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		                                                           VMA_MEMORY_USAGE_CPU_TO_GPU);
	}

	update_uniform_buffers();
	update_cube_uniform_buffers(0.0f);
}

void PushDescriptors::update_uniform_buffers()
{
	ubo_scene.projection = camera.matrices.perspective;
	ubo_scene.view       = camera.matrices.view;
	uniform_buffers.scene->convert_and_update(ubo_scene);
}

void PushDescriptors::update_cube_uniform_buffers(float delta_time)
{
	cubes[0].model_mat = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f));
	cubes[1].model_mat = glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, 0.5f, 0.0f));

	for (auto &cube : cubes)
	{
		cube.model_mat = glm::rotate(cube.model_mat, glm::radians(cube.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		cube.model_mat = glm::rotate(cube.model_mat, glm::radians(cube.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		cube.model_mat = glm::rotate(cube.model_mat, glm::radians(cube.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
		cube.uniform_buffer->convert_and_update(cube.model_mat);
	}

	if (animate)
	{
		cubes[0].rotation.x += 2.5f * delta_time;
		if (cubes[0].rotation.x > 360.0f)
		{
			cubes[0].rotation.x -= 360.0f;
		}
		cubes[1].rotation.y += 2.0f * delta_time;
		if (cubes[1].rotation.x > 360.0f)
		{
			cubes[1].rotation.x -= 360.0f;
		}
	}
}

void PushDescriptors::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

bool PushDescriptors::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	/*
	    Extension specific functions
	*/

	// The push descriptor update function is part of an extension so it has to be manually loaded
	vkCmdPushDescriptorSetKHR = reinterpret_cast<PFN_vkCmdPushDescriptorSetKHR>(vkGetDeviceProcAddr(get_device().get_handle(), "vkCmdPushDescriptorSetKHR"));
	if (!vkCmdPushDescriptorSetKHR)
	{
		throw std::runtime_error("Could not get a valid function pointer for vkCmdPushDescriptorSetKHR");
	}

	// Get device push descriptor properties (to display them)
	PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR =
	    reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(get_instance().get_handle(), "vkGetPhysicalDeviceProperties2KHR"));
	if (!vkGetPhysicalDeviceProperties2KHR)
	{
		throw std::runtime_error("Could not get a valid function pointer for vkGetPhysicalDeviceProperties2KHR");
	}
	VkPhysicalDeviceProperties2KHR device_properties{};
	push_descriptor_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR;
	device_properties.sType          = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
	device_properties.pNext          = &push_descriptor_properties;
	vkGetPhysicalDeviceProperties2KHR(get_device().get_gpu().get_handle(), &device_properties);

	/*
	    End of extension specific functions
	*/

	// Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.type = vkb::CameraType::LookAt;
	camera.set_perspective(60.0f, static_cast<float>(width) / height, 512.0f, 0.1f);
	camera.set_rotation(glm::vec3(0.0f, 0.0f, 0.0f));
	camera.set_translation(glm::vec3(0.0f, 0.0f, -5.0f));

	load_assets();
	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	build_command_buffers();
	prepared = true;
	return true;
}

void PushDescriptors::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
	if (animate)
	{
		update_cube_uniform_buffers(delta_time);
	}
	if (camera.updated)
	{
		update_uniform_buffers();
	}
}

void PushDescriptors::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		drawer.checkbox("Animate", &animate);
	}
	if (drawer.header("Device properties"))
	{
		drawer.text("maxPushDescriptors: %d", push_descriptor_properties.maxPushDescriptors);
	}
}

std::unique_ptr<vkb::VulkanSampleC> create_push_descriptors()
{
	return std::make_unique<PushDescriptors>();
}
