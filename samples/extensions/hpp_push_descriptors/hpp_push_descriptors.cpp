/* Copyright (c) 2019-2025, Sascha Willems
 * Copyright (c) 2024-2025, NVIDIA CORPORATION. All rights reserved.
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

#include "hpp_push_descriptors.h"

HPPPushDescriptors::HPPPushDescriptors()
{
	title = "Push descriptors";

	// Enable extension required for push descriptors
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
}

HPPPushDescriptors::~HPPPushDescriptors()
{
	if (has_device() && get_device().get_handle())
	{
		vk::Device const &device = get_device().get_handle();

		device.destroyPipeline(pipeline);
		device.destroyPipelineLayout(pipeline_layout);
		device.destroyDescriptorSetLayout(descriptor_set_layout);
		for (auto &cube : cubes)
		{
			device.destroySampler(cube.texture.sampler);
		}
	}
}

bool HPPPushDescriptors::prepare(const vkb::ApplicationOptions &options)
{
	assert(!prepared);

	if (HPPApiVulkanSample::prepare(options))
	{
		/*
		    Extension specific functions
		*/

		// Get device push descriptor properties (to display them)
		auto property_chain  = get_device().get_gpu().get_handle().getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDevicePushDescriptorPropertiesKHR>();
		max_push_descriptors = property_chain.get<vk::PhysicalDevicePushDescriptorPropertiesKHR>().maxPushDescriptors;

		/*
		    End of extension specific functions
		*/

		initializeCamera();
		load_assets();
		create_uniform_buffers();
		create_descriptor_set_layout();
		create_pipeline_layout();
		create_pipeline();
		build_command_buffers();
		prepared = true;
	}
	return prepared;
}

void HPPPushDescriptors::request_gpu_features(vkb::core::PhysicalDeviceCpp &gpu)
{
	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}
}

void HPPPushDescriptors::build_command_buffers()
{
	vk::CommandBufferBeginInfo command_buffer_begin_info;

	std::array<vk::ClearValue, 2> clear_values;
	clear_values[0].color        = default_clear_color;
	clear_values[1].depthStencil = vk::ClearDepthStencilValue{0.0f, 0};

	vk::RenderPassBeginInfo render_pass_begin_info{.renderPass      = render_pass,
	                                               .renderArea      = {{0, 0}, extent},
	                                               .clearValueCount = static_cast<uint32_t>(clear_values.size()),
	                                               .pClearValues    = clear_values.data()};

	vk::Viewport viewport{0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f};
	vk::Rect2D   scissor{{0, 0}, extent};

	vk::Buffer vertex_buffer = models.cube->get_vertex_buffer("vertex_buffer").get_handle();
	vk::Buffer index_buffer  = models.cube->get_index_buffer().get_handle();

	vk::DeviceSize offset = 0;

	vk::DescriptorBufferInfo scene_buffer_descriptor{uniform_buffers.scene->get_handle(), 0, vk::WholeSize};

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		render_pass_begin_info.framebuffer = framebuffers[i];
		vk::CommandBuffer &command_buffer  = draw_cmd_buffers[i];

		command_buffer.begin(command_buffer_begin_info);
		command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
		command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
		command_buffer.setViewport(0, viewport);
		command_buffer.setScissor(0, scissor);

		command_buffer.bindVertexBuffers(0, vertex_buffer, offset);
		command_buffer.bindIndexBuffer(index_buffer, 0, models.cube->get_index_type());

		// Render two cubes using different descriptor sets using push descriptors
		for (auto &cube : cubes)
		{
			// Instead of preparing the descriptor sets up-front, using push descriptors we can set (push) them inside of a command buffer
			// This allows a more dynamic approach without the need to create descriptor sets for each model
			// Note: dstSet for each descriptor set write is left at nullptr as this is ignored when using push descriptors

			vk::DescriptorBufferInfo cube_buffer_descriptor{cube.uniform_buffer->get_handle(), 0, vk::WholeSize};
			vk::DescriptorImageInfo  cube_image_descriptor{cube.texture.sampler,
                                                          cube.texture.image->get_vk_image_view().get_handle(),
                                                          descriptor_type_to_image_layout(vk::DescriptorType::eCombinedImageSampler,
			                                                                               cube.texture.image->get_vk_image_view().get_format())};

			std::array<vk::WriteDescriptorSet, 3> write_descriptor_sets = {
			    {{.dstBinding = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBufferInfo = &scene_buffer_descriptor},
			     {.dstBinding = 1, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBufferInfo = &cube_buffer_descriptor},
			     {.dstBinding = 2, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &cube_image_descriptor}}};

			command_buffer.pushDescriptorSetKHR(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, write_descriptor_sets);

			draw_model(models.cube, command_buffer);
		}

		draw_ui(command_buffer);

		command_buffer.endRenderPass();

		command_buffer.end();
	}
}

void HPPPushDescriptors::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		drawer.checkbox("Animate", &animate);
	}
	if (drawer.header("Device properties"))
	{
		drawer.text("maxPushDescriptors: %d", max_push_descriptors);
	}
}

void HPPPushDescriptors::render(float delta_time)
{
	if (prepared)
	{
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
}

void HPPPushDescriptors::create_descriptor_set_layout()
{
	std::array<vk::DescriptorSetLayoutBinding, 3> set_layout_bindings = {{{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
	                                                                      {1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
	                                                                      {2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};

	vk::DescriptorSetLayoutCreateInfo descriptor_layout_create_info{.flags        = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR,
	                                                                .bindingCount = static_cast<uint32_t>(set_layout_bindings.size()),
	                                                                .pBindings    = set_layout_bindings.data()};
	descriptor_set_layout = get_device().get_handle().createDescriptorSetLayout(descriptor_layout_create_info);
}

void HPPPushDescriptors::create_pipeline()
{
	const std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = {load_shader("push_descriptors", "cube.vert.spv", vk::ShaderStageFlagBits::eVertex),
	                                                                      load_shader("push_descriptors", "cube.frag.spv", vk::ShaderStageFlagBits::eFragment)};

	// Vertex bindings and attributes
	vk::VertexInputBindingDescription                  vertex_input_binding{0, sizeof(HPPVertex), vk::VertexInputRate::eVertex};
	std::array<vk::VertexInputAttributeDescription, 3> vertex_input_attributes = {
	    {{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(HPPVertex, pos)},           // Location 0: Position
	     {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(HPPVertex, normal)},        // Location 1 : Normal
	     {2, 0, vk::Format::eR32G32Sfloat, offsetof(HPPVertex, uv)}}};             // Location 2: UV
	vk::PipelineVertexInputStateCreateInfo vertex_input_state{.vertexBindingDescriptionCount   = 1,
	                                                          .pVertexBindingDescriptions      = &vertex_input_binding,
	                                                          .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attributes.size()),
	                                                          .pVertexAttributeDescriptions    = vertex_input_attributes.data()};

	vk::PipelineColorBlendAttachmentState blend_attachment_state{.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
	                                                                               vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

	// Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state{
	    .depthTestEnable = true, .depthWriteEnable = true, .depthCompareOp = vk::CompareOp::eGreater, .back = {.compareOp = vk::CompareOp::eAlways}};

	pipeline = vkb::common::create_graphics_pipeline(get_device().get_handle(),
	                                                 pipeline_cache,
	                                                 shader_stages,
	                                                 vertex_input_state,
	                                                 vk::PrimitiveTopology::eTriangleList,
	                                                 0,
	                                                 vk::PolygonMode::eFill,
	                                                 vk::CullModeFlagBits::eBack,
	                                                 vk::FrontFace::eClockwise,
	                                                 {blend_attachment_state},
	                                                 depth_stencil_state,
	                                                 pipeline_layout,
	                                                 render_pass);
}

void HPPPushDescriptors::create_uniform_buffers()
{
	// Vertex shader scene uniform buffer block
	uniform_buffers.scene = std::make_unique<vkb::core::BufferCpp>(get_device(),
	                                                               sizeof(UboScene),
	                                                               vk::BufferUsageFlagBits::eUniformBuffer,
	                                                               VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Vertex shader cube model uniform buffer blocks
	for (auto &cube : cubes)
	{
		cube.uniform_buffer = std::make_unique<vkb::core::BufferCpp>(get_device(),
		                                                             sizeof(glm::mat4),
		                                                             vk::BufferUsageFlagBits::eUniformBuffer,
		                                                             VMA_MEMORY_USAGE_CPU_TO_GPU);
	}

	update_uniform_buffers();
	update_cube_uniform_buffers(0.0f);
}

void HPPPushDescriptors::create_pipeline_layout()
{
	vk::PipelineLayoutCreateInfo pipeline_layout_create_info{.setLayoutCount = 1, .pSetLayouts = &descriptor_set_layout};
	pipeline_layout = get_device().get_handle().createPipelineLayout(pipeline_layout_create_info);
}

void HPPPushDescriptors::draw()
{
	HPPApiVulkanSample::prepare_frame();

	// Submit to queue
	submit_info.setCommandBuffers(draw_cmd_buffers[current_buffer]);
	queue.submit(submit_info);

	HPPApiVulkanSample::submit_frame();
}

void HPPPushDescriptors::initializeCamera()
{
	// Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.type = vkb::CameraType::LookAt;
	camera.set_perspective(60.0f, static_cast<float>(extent.width) / extent.height, 512.0f, 0.1f);
	camera.set_rotation(glm::vec3(0.0f, 0.0f, 0.0f));
	camera.set_translation(glm::vec3(0.0f, 0.0f, -5.0f));
}

void HPPPushDescriptors::load_assets()
{
	models.cube      = load_model("scenes/textured_unit_cube.gltf");
	cubes[0].texture = load_texture("textures/crate01_color_height_rgba.ktx", vkb::scene_graph::components::HPPImage::Color);
	cubes[1].texture = load_texture("textures/crate02_color_height_rgba.ktx", vkb::scene_graph::components::HPPImage::Color);
}

void HPPPushDescriptors::update_cube_uniform_buffers(float delta_time)
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

void HPPPushDescriptors::update_uniform_buffers()
{
	ubo_scene.projection = camera.matrices.perspective;
	ubo_scene.view       = camera.matrices.view;
	uniform_buffers.scene->convert_and_update(ubo_scene);
}

std::unique_ptr<vkb::VulkanSampleCpp> create_hpp_push_descriptors()
{
	return std::make_unique<HPPPushDescriptors>();
}
