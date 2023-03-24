/* Copyright (c) 2023, Sascha Willems
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
 * Using descriptor buffers replacing descriptor sets with VK_EXT_descriptor_buffer
 * This render multiple cubes passing uniform buffers and combined image samples to the GPU using descriptor buffers instead of descriptor sets
 * This allows for a more bindless design
 */

#include "descriptor_buffer_basic.h"

#include "core/buffer.h"
#include "scene_graph/components/sub_mesh.h"

DescriptorBufferBasic::DescriptorBufferBasic()
{
	title = "Descriptor buffers";

	set_api_version(VK_API_VERSION_1_1);

	// Enable extension required for descriptor buffers
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	add_device_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	add_device_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
	add_device_extension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_MAINTENANCE3_EXTENSION_NAME);

	add_device_extension(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME);
}

DescriptorBufferBasic::~DescriptorBufferBasic()
{
	if (device)
	{
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout_buffer, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout_image, nullptr);
		for (auto &cube : cubes)
		{
			cube.uniform_buffer.reset();
			cube.texture.image.reset();
			vkDestroySampler(get_device().get_handle(), cube.texture.sampler, nullptr);
		}
		uniform_buffers.scene.reset();
		resource_descriptor_buffer.reset();
		image_descriptor_buffer.reset();
	}
}

void DescriptorBufferBasic::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}

	// Enable features required for this example

	// We need device addresses for buffers in certain places
	auto &requested_buffer_device_address_features               = gpu.request_extension_features<VkPhysicalDeviceBufferDeviceAddressFeatures>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES);
	requested_buffer_device_address_features.bufferDeviceAddress = VK_TRUE;

	// We need to enable the descriptor buffer feature of the VK_EXT_descriptor_buffer extension
	auto &requested_buffer_descriptor_buffer_features            = gpu.request_extension_features<VkPhysicalDeviceDescriptorBufferFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT);
	requested_buffer_descriptor_buffer_features.descriptorBuffer = VK_TRUE;
}

void DescriptorBufferBasic::build_command_buffers()
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

		// Descriptor buffer bindings
		// Binding 0 = uniform buffer
		VkDescriptorBufferBindingInfoEXT descriptor_buffer_binding_info[2]{};
		descriptor_buffer_binding_info[0].sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
		descriptor_buffer_binding_info[0].address = resource_descriptor_buffer->get_device_address();
		descriptor_buffer_binding_info[0].usage   = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
		// Binding 1 = Image
		descriptor_buffer_binding_info[1].sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
		descriptor_buffer_binding_info[1].address = image_descriptor_buffer->get_device_address();
		descriptor_buffer_binding_info[1].usage   = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
		vkCmdBindDescriptorBuffersEXT(draw_cmd_buffers[i], 2, descriptor_buffer_binding_info);

		uint32_t     buffer_index_ubo   = 0;
		uint32_t     buffer_index_image = 1;
		VkDeviceSize alignment          = descriptor_buffer_properties.descriptorBufferOffsetAlignment;
		VkDeviceSize buffer_offset      = 0;

		// Global Matrices (set 0)
		vkCmdSetDescriptorBufferOffsetsEXT(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &buffer_index_ubo, &buffer_offset);

		// Set and offset into descriptor for each model
		for (size_t j = 0; j < cubes.size(); j++)
		{
			// Uniform buffer (set 1)
			// Model ubos start at offset * 1 (slot 0 is global matrices)
			buffer_offset = (j + 1) * alignment;
			vkCmdSetDescriptorBufferOffsetsEXT(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 1, 1, &buffer_index_ubo, &buffer_offset);
			// Image (set 2)
			buffer_offset = j * alignment;
			vkCmdSetDescriptorBufferOffsetsEXT(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 2, 1, &buffer_index_image, &buffer_offset);
			draw_model(models.cube, draw_cmd_buffers[i]);
		}

		// @todo: can't mix descriptor buffers and descriptors, so we disable the UI for now
		// draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void DescriptorBufferBasic::load_assets()
{
	models.cube      = load_model("scenes/textured_unit_cube.gltf");
	cubes[0].texture = load_texture("textures/crate01_color_height_rgba.ktx", vkb::sg::Image::Color);
	cubes[1].texture = load_texture("textures/crate02_color_height_rgba.ktx", vkb::sg::Image::Color);
}

void DescriptorBufferBasic::setup_descriptor_set_layout()
{
	// Using descriptor buffers still requires use to create descriptor set layouts

	VkDescriptorSetLayoutBinding set_layout_binding{};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info{};
	descriptor_layout_create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_layout_create_info.bindingCount = 1;
	descriptor_layout_create_info.flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
	descriptor_layout_create_info.pBindings    = &set_layout_binding;

	// Create a layout for uniform buffers
	set_layout_binding = vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layout_buffer));

	// Create a layout for combined image samplers
	set_layout_binding = vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layout_image));

	// Create a pipeline layout using set 0 = Camera UBO, set 1 = Model UBO and set 2 = Model combined image
	const std::array<VkDescriptorSetLayout, 3> decriptor_set_layouts = {descriptor_set_layout_buffer, descriptor_set_layout_buffer, descriptor_set_layout_image};

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(decriptor_set_layouts.data(), static_cast<uint32_t>(decriptor_set_layouts.size()));
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void DescriptorBufferBasic::prepare_pipelines()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 0);

	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);

	// Note: Using Reversed depth-buffer for increased precision, so Greater depth values are kept
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

	// We need to set this flag to let the implemenation know that this pipeline uses descriptor buffers
	pipeline_create_info.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

	const std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {
	    load_shader("descriptor_buffer_basic/cube.vert", VK_SHADER_STAGE_VERTEX_BIT),
	    load_shader("descriptor_buffer_basic/cube.frag", VK_SHADER_STAGE_FRAGMENT_BIT)};

	pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages    = shader_stages.data();

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline));
}

// This function creates the desctiptor bffers and puts the descriptors into those buffers, so they can be used during command buffer creation
void DescriptorBufferBasic::prepare_descriptor_buffer()
{
	// For sizing the descriptor buffers, we need to know the size for the descriptor set layouts the pipeline is using
	std::array<VkDeviceSize, 2> descriptorLayoutSizes{};
	vkGetDescriptorSetLayoutSizeEXT(get_device().get_handle(), descriptor_set_layout_buffer, &descriptorLayoutSizes[0]);
	vkGetDescriptorSetLayoutSizeEXT(get_device().get_handle(), descriptor_set_layout_image, &descriptorLayoutSizes[1]);

	// This buffer will contain resource descriptors for all the uniform buffers (one per cube and one with global matrices)
	resource_descriptor_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                                 (static_cast<uint32_t>(cubes.size()) + 1) * descriptorLayoutSizes[0],
	                                                                 VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	                                                                 VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Samplers and combined images need to be stored in a separate buffer due to different flags (VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT) (one image per cube)
	image_descriptor_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                              static_cast<uint32_t>(cubes.size()) * descriptorLayoutSizes[1],
	                                                              VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	                                                              VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Put the descriptors into the above buffers
	// This is done with vkGetDescriptorEXT
	// We use pointers to offset and align the data we put into the descriptor buffers
	VkDescriptorGetInfoEXT desc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
	const uint32_t         alignment = descriptor_buffer_properties.descriptorBufferOffsetAlignment;

	// For combined images we need to put descriptors into the descriptor buffers
	char *buf_ptr  = (char *) image_descriptor_buffer->get_data();
	desc_info.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	for (size_t i = 0; i < cubes.size(); i++)
	{
		VkDescriptorImageInfo image_descriptor = create_descriptor(cubes[i].texture);
		desc_info.data.pCombinedImageSampler   = &image_descriptor;
		vkGetDescriptorEXT(get_device().get_handle(), &desc_info, descriptor_buffer_properties.combinedImageSamplerDescriptorSize, buf_ptr + i * alignment);
	}

	// For uniform buffers we only need to put their buffer device addresses into the descriptor buffers

	// Global uniform buffer
	buf_ptr = (char *) resource_descriptor_buffer->get_data();

	VkDescriptorAddressInfoEXT addr_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT};
	addr_info.address                    = uniform_buffers.scene->get_device_address();
	addr_info.range                      = uniform_buffers.scene->get_size();
	addr_info.format                     = VK_FORMAT_UNDEFINED;

	desc_info.type                       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	desc_info.data.pCombinedImageSampler = nullptr;
	desc_info.data.pUniformBuffer        = &addr_info;
	vkGetDescriptorEXT(get_device().get_handle(), &desc_info, descriptor_buffer_properties.uniformBufferDescriptorSize, buf_ptr);

	// Per-cube uniform buffers
	buf_ptr += alignment;
	for (size_t i = 0; i < cubes.size(); i++)
	{
		VkDescriptorAddressInfoEXT addr_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT};
		addr_info.address                    = cubes[i].uniform_buffer->get_device_address();
		addr_info.range                      = cubes[i].uniform_buffer->get_size();
		addr_info.format                     = VK_FORMAT_UNDEFINED;

		desc_info.type                       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		desc_info.data.pCombinedImageSampler = nullptr;
		desc_info.data.pUniformBuffer        = &addr_info;
		vkGetDescriptorEXT(get_device().get_handle(), &desc_info, descriptor_buffer_properties.combinedImageSamplerDescriptorSize, buf_ptr);
		buf_ptr += alignment;
	}
}

void DescriptorBufferBasic::prepare_uniform_buffers()
{
	// Vertex shader scene uniform buffer block
	uniform_buffers.scene = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                            sizeof(UboScene),
	                                                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	                                                            VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Vertex shader cube model uniform buffer blocks
	for (auto &cube : cubes)
	{
		cube.uniform_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
		                                                          sizeof(glm::mat4),
		                                                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		                                                          VMA_MEMORY_USAGE_CPU_TO_GPU);
	}

	update_uniform_buffers();
	update_cube_uniform_buffers(0.0f);
}

void DescriptorBufferBasic::update_uniform_buffers()
{
	ubo_scene.projection = camera.matrices.perspective;
	ubo_scene.view       = camera.matrices.view;
	uniform_buffers.scene->convert_and_update(ubo_scene);
}

void DescriptorBufferBasic::update_cube_uniform_buffers(float delta_time)
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

void DescriptorBufferBasic::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

bool DescriptorBufferBasic::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	/*
	    Extension specific functions
	*/

	VkPhysicalDeviceProperties2KHR device_properties{};
	descriptor_buffer_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;
	device_properties.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
	device_properties.pNext            = &descriptor_buffer_properties;
	vkGetPhysicalDeviceProperties2KHR(get_device().get_gpu().get_handle(), &device_properties);

	// This sample makes use of combined image samplers in a single array, which is an optional feeature
	if (!descriptor_buffer_properties.combinedImageSamplerDescriptorSingleArray)
	{
		throw std::runtime_error("This sample requires the combinedImageSamplerDescriptorSingleArray feature, which is not supported on the selected device");
	}

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
	prepare_descriptor_buffer();
	prepare_pipelines();
	build_command_buffers();
	prepared = true;
	return true;
}

void DescriptorBufferBasic::render(float delta_time)
{
	if (!prepared)
		return;
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

void DescriptorBufferBasic::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		drawer.checkbox("Animate", &animate);
	}
}

std::unique_ptr<vkb::VulkanSample> create_descriptor_buffer_basic()
{
	return std::make_unique<DescriptorBufferBasic>();
}
