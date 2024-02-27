/* Copyright (c) 2024, Mobica Limited
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
 * Rendering using primitive clipping configured by dynamic pipeline state
 */

#include "dynamic_primitive_clipping.h"

DynamicPrimitiveClipping::DynamicPrimitiveClipping()
{
	title = "Dynamic primitive clipping";
	set_api_version(VK_API_VERSION_1_1);

	// Extensions required by vkCmdSetDepthClipEnableEXT().
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME);
	add_device_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
}

DynamicPrimitiveClipping::~DynamicPrimitiveClipping()
{
	if (device)
	{
		vkDestroyPipeline(get_device().get_handle(), sample_pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.models, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layouts.models, nullptr);
	}
}

bool DynamicPrimitiveClipping::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	// Setup camera position.
	camera.type = vkb::CameraType::LookAt;
	camera.set_position(glm::vec3(0.0f, 0.0f, -50.0f));
	camera.set_rotation(glm::vec3(0.0f, 180.0f, 0.0f));

	// Near plane is set far away from observer position in order to show depth clipping better.
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 30.f, 256.0f);

	// Load assets from file.
	load_assets();

	// Setup parameters used on CPU.
	visualization_names = {"World space X", "World space Y", "Half-space in world space coordinates", "Half-space in clip space coordinates", "Clip space X", "Clip space Y", "Euclidean distance to center", "Manhattan distance to center", "Chebyshev distance to center"};

	// Setup Vulkan objects required by GPU.
	prepare_uniform_buffers();
	setup_layouts();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_sets();
	build_command_buffers();

	prepared = true;
	return true;
}

void DynamicPrimitiveClipping::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// shaderClipDistance feature is required in order to gl_ClipDistance builtin shader variable to work.
	if (gpu.get_features().shaderClipDistance)
	{
		gpu.get_mutable_requested_features().shaderClipDistance = VK_TRUE;
	}
	else
	{
		throw vkb::VulkanException(VK_ERROR_FEATURE_NOT_PRESENT, "Selected GPU does not support gl_ClipDistance builtin shader variable");
	}

	// Features required by vkCmdSetDepthClipEnableEXT().
	{
		auto &features           = gpu.request_extension_features<VkPhysicalDeviceDepthClipEnableFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT);
		features.depthClipEnable = VK_TRUE;
	}
	{
		auto &features                                = gpu.request_extension_features<VkPhysicalDeviceExtendedDynamicState3FeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT);
		features.extendedDynamicState3DepthClipEnable = VK_TRUE;
	}
}

void DynamicPrimitiveClipping::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	// Clear color and depth values.
	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.1f, 0.1f, 0.1f, 0.0f}};
	clear_values[1].depthStencil = {1.0f, 0};

	// Begin the render pass.
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
		auto cmd = draw_cmd_buffers[i];

		// Begin command buffer.
		vkBeginCommandBuffer(cmd, &command_buffer_begin_info);

		// Set framebuffer for this command buffer.
		render_pass_begin_info.framebuffer = framebuffers[i];
		// We will add draw commands in the same command buffer.
		vkCmdBeginRenderPass(cmd, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		// Bind the graphics pipeline.
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sample_pipeline);

		// Set viewport dynamically.
		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		// Set scissor dynamically.
		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		// Enable depth clipping dynamically as defined in GUI.
		vkCmdSetDepthClipEnableEXT(cmd, params.useDepthClipping);

		// Draw object once using descriptor_positive.
		if (params.drawObject[0])
		{
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.models, 0, 1, &descriptor_sets.descriptor_positive, 0, NULL);
			draw_model(models.objects[models.object_index], cmd);
		}

		// Draw the same object for the second time, but this time using descriptor_negative.
		// Skip second rendering if primitive clipping is turned off by user in a GUI.
		if (params.drawObject[1] && params.usePrimitiveClipping)
		{
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.models, 0, 1, &descriptor_sets.descriptor_negative, 0, NULL);
			draw_model(models.objects[models.object_index], cmd);
		}

		// Draw user interface.
		draw_ui(draw_cmd_buffers[i]);

		// Complete render pass.
		vkCmdEndRenderPass(cmd);

		// Complete the command buffer.
		VK_CHECK(vkEndCommandBuffer(cmd));
	}
}

void DynamicPrimitiveClipping::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
	update_uniform_buffers();
}

void DynamicPrimitiveClipping::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.combo_box("Object type", &models.object_index, model_names))
		{
			update_uniform_buffers();
			rebuild_command_buffers();
		}
		if (drawer.checkbox("Use primitive clipping", &params.usePrimitiveClipping))
		{
			update_uniform_buffers();
			rebuild_command_buffers();
		}
		if (drawer.combo_box("Visualization", &params.visualization, visualization_names))
		{
		}
		if (drawer.checkbox("Draw object 1", &params.drawObject[0]))
		{
			rebuild_command_buffers();
		}
		if (drawer.checkbox("Draw object 2", &params.drawObject[1]))
		{
			rebuild_command_buffers();
		}
		if (drawer.checkbox("Use depth clipping", &params.useDepthClipping))
		{
			rebuild_command_buffers();
		}
	}
}

void DynamicPrimitiveClipping::load_assets()
{
	// Load three different models. User may pick them from GUI.
	std::vector<std::string> filenames = {"teapot.gltf", "torusknot.gltf", "geosphere.gltf"};
	model_names                        = {"Teapot", "Torusknot", "Sphere"};
	for (auto file : filenames)
	{
		auto object = load_model("scenes/" + file);
		models.objects.emplace_back(std::move(object));
	}

	// Setup model transformation matrices.
	auto teapot_matrix = glm::mat4(1.0f);
	teapot_matrix      = glm::scale(teapot_matrix, glm::vec3(10.0f, 10.0f, 10.0f));
	teapot_matrix      = glm::rotate(teapot_matrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	models.transforms.push_back(teapot_matrix);          // teapot matrix
	models.transforms.push_back(glm::mat4(1.0f));        // torusknot matrix
	models.transforms.push_back(glm::mat4(1.0f));        // sphere matrix
}

void DynamicPrimitiveClipping::setup_layouts()
{
	// Descriptor set layout contains information about single UBO.
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layouts.models));

	// Pipeline layout contains above defined descriptor set layout.
	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &descriptor_set_layouts.models,
	        1);

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layouts.models));
}

void DynamicPrimitiveClipping::prepare_pipelines()
{
	// Binding description.
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};

	// Attribute descriptions.
	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),                         // Position
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3u),        // Normal
	    vkb::initializers::vertex_input_attribute_description(0, 2, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 5u)            // UV
	};

	VkPipelineVertexInputStateCreateInfo vertex_input = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	// Specify we will use triangle lists to draw geometry.
	VkPipelineInputAssemblyStateCreateInfo input_assembly = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	// Specify rasterization state.
	VkPipelineRasterizationStateCreateInfo raster = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);

	// Our attachment will write to all color channels, but no blending is enabled.
	VkPipelineColorBlendAttachmentState blend_attachment = vkb::initializers::pipeline_color_blend_attachment_state(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);

	VkPipelineColorBlendStateCreateInfo blend = vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment);

	// We will have one viewport and scissor box.
	VkPipelineViewportStateCreateInfo viewport = vkb::initializers::pipeline_viewport_state_create_info(1, 1);

	// Enable depth testing.
	VkPipelineDepthStencilStateCreateInfo depth_stencil = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);

	// No multisampling.
	VkPipelineMultisampleStateCreateInfo multisample = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT);

	// Specify that these states will be dynamic, i.e. not part of pipeline state object.
	// VK_DYNAMIC_STATE_DEPTH_CLIP_ENABLE_EXT must be specified here in order to use vkCmdSetDepthClipEnableEXT()
	std::array<VkDynamicState, 3>    dynamics{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_CLIP_ENABLE_EXT};
	VkPipelineDynamicStateCreateInfo dynamic = vkb::initializers::pipeline_dynamic_state_create_info(dynamics.data(), vkb::to_u32(dynamics.size()));

	// Load shaders.
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};
	shader_stages[0] = load_shader("dynamic_primitive_clipping/primitive_clipping.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("dynamic_primitive_clipping/primitive_clipping.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// We need to specify the pipeline layout and the render pass description up front as well.
	VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(pipeline_layouts.models, render_pass);
	pipeline_create_info.stageCount                   = vkb::to_u32(shader_stages.size());
	pipeline_create_info.pStages                      = shader_stages.data();
	pipeline_create_info.pVertexInputState            = &vertex_input;
	pipeline_create_info.pInputAssemblyState          = &input_assembly;
	pipeline_create_info.pRasterizationState          = &raster;
	pipeline_create_info.pColorBlendState             = &blend;
	pipeline_create_info.pMultisampleState            = &multisample;
	pipeline_create_info.pViewportState               = &viewport;
	pipeline_create_info.pDepthStencilState           = &depth_stencil;
	pipeline_create_info.pDynamicState                = &dynamic;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &sample_pipeline));
}

// Prepare and initialize uniform buffer containing shader uniforms.
void DynamicPrimitiveClipping::prepare_uniform_buffers()
{
	// We will render the same object twice using two different sets of parameters called "positive" and "negative".
	uniform_buffers.buffer_positive = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                                      sizeof(UBOVS),
	                                                                      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                                      VMA_MEMORY_USAGE_CPU_TO_GPU);
	uniform_buffers.buffer_negative = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                                      sizeof(UBOVS),
	                                                                      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                                      VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void DynamicPrimitiveClipping::update_uniform_buffers()
{
	ubo_positive.projection           = camera.matrices.perspective;
	ubo_positive.view                 = camera.matrices.view;
	ubo_positive.model                = models.transforms[models.object_index];
	ubo_positive.colorTransformation  = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
	ubo_positive.sceneTransformation  = glm::ivec2(params.visualization, 1);
	ubo_positive.usePrimitiveClipping = params.usePrimitiveClipping ? 1.0f : -1.0f;
	uniform_buffers.buffer_positive->convert_and_update(ubo_positive);

	ubo_negative.projection           = camera.matrices.perspective;
	ubo_negative.view                 = camera.matrices.view;
	ubo_negative.model                = models.transforms[models.object_index];
	ubo_negative.colorTransformation  = glm::vec4(-1.0f, 1.0f, 0.0f, 0.0f);
	ubo_negative.sceneTransformation  = glm::ivec2(params.visualization, -1);
	ubo_negative.usePrimitiveClipping = params.usePrimitiveClipping ? 1.0f : -1.0f;
	uniform_buffers.buffer_negative->convert_and_update(ubo_negative);
}

void DynamicPrimitiveClipping::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * 4)};
	uint32_t                   num_descriptor_sets = 2 * 2 * 4;
	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), num_descriptor_sets);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void DynamicPrimitiveClipping::setup_descriptor_sets()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layouts.models,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.descriptor_positive));
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.descriptor_negative));

	std::vector<VkDescriptorBufferInfo> descriptor_buffer_infos = {
	    create_descriptor(*uniform_buffers.buffer_positive),
	    create_descriptor(*uniform_buffers.buffer_negative)};
	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(descriptor_sets.descriptor_positive, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &descriptor_buffer_infos[0]),
	    vkb::initializers::write_descriptor_set(descriptor_sets.descriptor_negative, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &descriptor_buffer_infos[1])};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);
}

std::unique_ptr<vkb::VulkanSample> create_dynamic_primitive_clipping()
{
	return std::make_unique<DynamicPrimitiveClipping>();
}
