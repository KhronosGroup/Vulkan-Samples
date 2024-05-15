/* Copyright (c) 2023-2024, Mobica Limited
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

#include <memory>
#include <string>
#include <vector>

#include "logic_op_dynamic_state.h"

#include "gltf_loader.h"

/* Initialize the static variable containing a vector of logic operations' labels for GUI */
std::vector<std::string> LogicOpDynamicState::GUI_settings::logic_op_names =
    LogicOpDynamicState::GUI_settings::init_logic_op_names();

LogicOpDynamicState::LogicOpDynamicState()
{
	title = "Logic Operations Dynamic State";

	/* Extensions required for dynamic logic operations */
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);
	add_device_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
}

LogicOpDynamicState::~LogicOpDynamicState()
{
	if (has_device())
	{
		uniform_buffers.common.reset();
		uniform_buffers.baseline.reset();

		vkDestroySampler(get_device().get_handle(), textures.envmap.sampler, VK_NULL_HANDLE);
		textures = {};

		vkDestroyPipeline(get_device().get_handle(), pipeline.baseline, VK_NULL_HANDLE);
		vkDestroyPipeline(get_device().get_handle(), pipeline.background, VK_NULL_HANDLE);

		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.baseline, VK_NULL_HANDLE);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.background, VK_NULL_HANDLE);

		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layouts.baseline, VK_NULL_HANDLE);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layouts.background, VK_NULL_HANDLE);
	}
}

/**
 * 	@fn bool LogicOpDynamicState::prepare(const vkb::ApplicationOptions &options)
 * 	@brief Configuring all sample specific settings, creating descriptor sets/pool, pipelines, generating or loading models etc.
 */
bool LogicOpDynamicState::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	/* Set up camera properties */
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
 * 	@brief Setting custom surface format priority list to required VK_FORMAT_B8G8R8A8_UNORM format
 */
void LogicOpDynamicState::create_render_context()
{
	/* UNORM surface is required for logic operations */
	auto surface_priority_list = std::vector<VkSurfaceFormatKHR>{
	    {VK_FORMAT_B8G8R8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR},
	};
	VulkanSample::create_render_context(surface_priority_list);
}

/**
 * 	@fn void LogicOpDynamicState::render(float delta_time)
 * 	@brief Drawing frames and/or updating uniform buffers when camera position/rotation was changed
 */
void LogicOpDynamicState::render(float delta_time)
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
 * 	@fn void LogicOpDynamicState::build_command_buffers()
 * 	@brief Creating command buffers and drawing background and model on window
 */
void LogicOpDynamicState::build_command_buffers()
{
	std::array<VkClearValue, 2> clear_values{};
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0};

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

		/* Binding background pipeline and descriptor sets  */
		vkCmdBindDescriptorSets(draw_cmd_buffer,
		                        VK_PIPELINE_BIND_POINT_GRAPHICS,
		                        pipeline_layouts.background,
		                        0,
		                        1,
		                        &descriptor_sets.background,
		                        0,
		                        VK_NULL_HANDLE);
		vkCmdBindPipeline(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.background);

		/* Drawing background */
		draw_model(background_model, draw_cmd_buffer);

		/* Binding baseline pipeline and descriptor sets */
		vkCmdBindDescriptorSets(draw_cmd_buffer,
		                        VK_PIPELINE_BIND_POINT_GRAPHICS,
		                        pipeline_layouts.baseline,
		                        0,
		                        1,
		                        &descriptor_sets.baseline,
		                        0,
		                        VK_NULL_HANDLE);
		vkCmdBindPipeline(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.baseline);

		/* Changing topology to triangle strip with using primitive restart feature */
		vkCmdSetPrimitiveTopologyEXT(draw_cmd_buffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
		vkCmdSetPrimitiveRestartEnableEXT(draw_cmd_buffer, VK_TRUE);

		/* Set logic operation chosen in GUI for the cube model */
		vkCmdSetLogicOpEXT(draw_cmd_buffer, static_cast<VkLogicOp>(gui_settings.selected_operation));

		/* Draw model */
		draw_created_model(draw_cmd_buffer);

		/* UI */
		draw_ui(draw_cmd_buffer);

		vkCmdEndRenderPass(draw_cmd_buffer);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffer));
	}
}

/**
 * @fn void LogicOpDynamicState::request_gpu_features(vkb::PhysicalDevice &gpu)
 * @brief Enabling features related to Vulkan extensions
 */
void LogicOpDynamicState::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	/* Enable extension features required by this sample
	   These are passed to device creation via a pNext structure chain */
	auto &requested_extended_dynamic_state2_features =
	    gpu.request_extension_features<VkPhysicalDeviceExtendedDynamicState2FeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT);
	requested_extended_dynamic_state2_features.extendedDynamicState2        = VK_TRUE;
	requested_extended_dynamic_state2_features.extendedDynamicState2LogicOp = VK_TRUE;

	auto &requested_extended_dynamic_state_feature =
	    gpu.request_extension_features<VkPhysicalDeviceExtendedDynamicStateFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT);
	requested_extended_dynamic_state_feature.extendedDynamicState = VK_TRUE;

	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}

	gpu.get_mutable_requested_features().logicOp = VK_TRUE;
}

void LogicOpDynamicState::prepare_uniform_buffers()
{
	uniform_buffers.common =
	    std::make_unique<vkb::core::Buffer>(get_device(), sizeof(ubo_common), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	uniform_buffers.baseline =
	    std::make_unique<vkb::core::Buffer>(get_device(), sizeof(ubo_baseline), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void LogicOpDynamicState::update_uniform_buffers()
{
	/* Common uniform buffer */
	ubo_common.projection = camera.matrices.perspective;
	ubo_common.view       = camera.matrices.view;
	uniform_buffers.common->convert_and_update(ubo_common);

	/* Baseline uniform buffer */
	uniform_buffers.baseline->convert_and_update(ubo_baseline);
}

void LogicOpDynamicState::create_pipeline()
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
	    vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);

	/* Enable logic operations */
	color_blend_state.logicOpEnable = VK_TRUE;

	/* Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept */
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(
	        VK_TRUE,
	        VK_TRUE,
	        VK_COMPARE_OP_GREATER);

	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR,
	    VK_DYNAMIC_STATE_LOGIC_OP_EXT,
	    VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT,
	    VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE_EXT,
	};
	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0);

	/* Binding description */
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX),
	    vkb::initializers::vertex_input_binding_description(1, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX)};

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

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};
	shader_stages[0] = load_shader("logic_op_dynamic_state/baseline.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("logic_op_dynamic_state/baseline.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	/* Use the pNext to point to the rendering create structure */
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
	graphics_create.layout              = pipeline_layouts.baseline;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &graphics_create, VK_NULL_HANDLE, &pipeline.baseline));

	/* Setup for second pipeline */
	graphics_create.layout = pipeline_layouts.background;

	std::vector<VkDynamicState> dynamic_state_enables_background = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR,
	};

	/* Disable logic operations in background pipeline */
	color_blend_state.logicOpEnable = VK_FALSE;

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

	shader_stages[0] = load_shader("logic_op_dynamic_state/background.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("logic_op_dynamic_state/background.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &graphics_create, VK_NULL_HANDLE, &pipeline.background));
}

/**
 * 	@fn void LogicOpDynamicState::draw()
 *  @brief Preparing frame and submitting it to the present queue
 */
void LogicOpDynamicState::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

/**
 * 	@fn void LogicOpDynamicState::load_assets()
 *	@brief Loading extra models, textures from assets
 */
void LogicOpDynamicState::load_assets()
{
	/* Background model */
	background_model = load_model("scenes/cube.gltf");
	/* Load HDR cube map */
	textures.envmap = load_texture_cubemap("textures/uffizi_rgba16f_cube.ktx", vkb::sg::Image::Color);
}

/**
 * 	@fn void LogicOpDynamicState::create_descriptor_pool()
 * 	@brief Creating descriptor pool with size adjusted to use uniform buffer and image sampler
 */
void LogicOpDynamicState::create_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),
	};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(
	        static_cast<uint32_t>(pool_sizes.size()),
	        pool_sizes.data(),
	        2);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, VK_NULL_HANDLE, &descriptor_pool));
}

/**
 * 	@fn void LogicOpDynamicState::setup_descriptor_set_layout()
 * 	@brief Creating layout for descriptor sets
 */
void LogicOpDynamicState::setup_descriptor_set_layout()
{
	/* First descriptor set */
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        VK_SHADER_STAGE_VERTEX_BIT,
	        0),
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        VK_SHADER_STAGE_VERTEX_BIT,
	        1),
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, VK_NULL_HANDLE, &descriptor_set_layouts.baseline));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &descriptor_set_layouts.baseline,
	        1);

	/* Pass scene node information via push constants */
	VkPushConstantRange push_constant_range            = vkb::initializers::push_constant_range(VK_SHADER_STAGE_VERTEX_BIT, sizeof(push_const_block), 0);
	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, VK_NULL_HANDLE, &pipeline_layouts.baseline));

	/* Second descriptor set */
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
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, VK_NULL_HANDLE, &descriptor_set_layouts.background));

	pipeline_layout_create_info.pSetLayouts            = &descriptor_set_layouts.background;
	pipeline_layout_create_info.setLayoutCount         = 1;
	pipeline_layout_create_info.pushConstantRangeCount = 0;
	pipeline_layout_create_info.pPushConstantRanges    = VK_NULL_HANDLE;
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, VK_NULL_HANDLE, &pipeline_layouts.background));
}

/**
 * 	@fn void LogicOpDynamicState::create_descriptor_sets()
 * 	@brief Creating both descriptor set:
 * 		   1. Uniform buffer
 * 		   2. Image sampler
 */
void LogicOpDynamicState::create_descriptor_sets()
{
	/* First descriptor set */
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layouts.baseline,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.baseline));

	VkDescriptorBufferInfo matrix_common_buffer_descriptor   = create_descriptor(*uniform_buffers.common);
	VkDescriptorBufferInfo matrix_baseline_buffer_descriptor = create_descriptor(*uniform_buffers.baseline);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(
	        descriptor_sets.baseline,
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        0,
	        &matrix_common_buffer_descriptor),
	    vkb::initializers::write_descriptor_set(
	        descriptor_sets.baseline,
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        1,
	        &matrix_baseline_buffer_descriptor)};

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()),
	                       write_descriptor_sets.data(), 0, VK_NULL_HANDLE);

	/* Second descriptor set */
	alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layouts.background,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.background));

	VkDescriptorImageInfo background_image_descriptor = create_descriptor(textures.envmap);

	write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(
	        descriptor_sets.background,
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        0,
	        &matrix_common_buffer_descriptor),
	    vkb::initializers::write_descriptor_set(
	        descriptor_sets.background,
	        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        1,
	        &background_image_descriptor)};

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()),
	                       write_descriptor_sets.data(), 0, VK_NULL_HANDLE);
}

/**
 * 	@fn void LogicOpDynamicState::model_data_creation()
 * 	@brief Creating cube model
 */
void LogicOpDynamicState::model_data_creation()
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
	for (uint8_t i = 0; i < vertex_count; ++i)
	{
		vertices_pos[i] *= glm::vec3(8.0f, 8.0f, 8.0f);
		vertices_pos[i] += glm::vec3(0.0f, 1.0f, 5.0f);
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

	vkb::core::Buffer vertex_pos_staging  = vkb::core::Buffer::create_staging_buffer(get_device(), vertices_pos);
	vkb::core::Buffer vertex_norm_staging = vkb::core::Buffer::create_staging_buffer(get_device(), vertices_norm);
	vkb::core::Buffer index_staging       = vkb::core::Buffer::create_staging_buffer(get_device(), indices);

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
	VkCommandBuffer copy_command = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkBufferCopy copy_region = {};

	copy_region.size = vertex_buffer_size;
	vkCmdCopyBuffer(
	    copy_command,
	    vertex_pos_staging.get_handle(),
	    cube.vertices_pos->get_handle(),
	    1,
	    &copy_region);

	vkCmdCopyBuffer(
	    copy_command,
	    vertex_norm_staging.get_handle(),
	    cube.vertices_norm->get_handle(),
	    1,
	    &copy_region);

	copy_region.size = index_buffer_size;
	vkCmdCopyBuffer(
	    copy_command,
	    index_staging.get_handle(),
	    cube.indices->get_handle(),
	    1,
	    &copy_region);

	get_device().flush_command_buffer(copy_command, queue, true);
}

/**
 * 	@fn void LogicOpDynamicState::draw_created_model(VkCommandBuffer commandBuffer)
 * 	@brief Drawing cube model
 */
void LogicOpDynamicState::draw_created_model(VkCommandBuffer commandBuffer)
{
	VkDeviceSize offsets[1] = {0};
	push_const_block.color  = glm::vec4{0.75f, 1.0f, 1.0f, 1.0f};
	vkCmdPushConstants(commandBuffer, pipeline_layouts.baseline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_const_block), &push_const_block);
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, cube.vertices_pos->get(), offsets);
	vkCmdBindVertexBuffers(commandBuffer, 1, 1, cube.vertices_norm->get(), offsets);
	vkCmdBindIndexBuffer(commandBuffer, cube.indices->get_handle(), 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(commandBuffer, cube.index_count, 1, 0, 0, 0);
}

/**
 * @fn void LogicOpDynamicState::on_update_ui_overlay(vkb::Drawer &drawer)
 * @brief Projecting GUI and transferring data between GUI and application
 */
void LogicOpDynamicState::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.combo_box("Logic operation", &gui_settings.selected_operation, LogicOpDynamicState::GUI_settings::logic_op_names))
		{
			update_uniform_buffers();
		}
	}
}

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::C>> create_logic_op_dynamic_state()
{
	return std::make_unique<LogicOpDynamicState>();
}
