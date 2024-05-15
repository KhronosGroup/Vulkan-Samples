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

#include "fragment_shader_barycentric.h"

FragmentShaderBarycentric::FragmentShaderBarycentric()
{
	title = "Fragment shader barycentric";

	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME);
}

FragmentShaderBarycentric::~FragmentShaderBarycentric()
{
	if (has_device())
	{
		vkDestroySampler(get_device().get_handle(), textures.envmap.sampler, VK_NULL_HANDLE);
		vkDestroySampler(get_device().get_handle(), textures.cube.sampler, VK_NULL_HANDLE);
		textures = {};
		skybox.reset();
		object.reset();
		ubo.reset();

		vkDestroyPipeline(get_device().get_handle(), pipelines.object, VK_NULL_HANDLE);
		vkDestroyPipeline(get_device().get_handle(), pipelines.skybox, VK_NULL_HANDLE);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, VK_NULL_HANDLE);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, VK_NULL_HANDLE);
	}
}
/**
 * 	@fn bool FragmentShaderBarycentric::prepare(const vkb::ApplicationOptions &options)
 * 	@brief Configuring all sample specific settings, creating descriptor sets/pool, pipelines, generating or loading models etc.
 */
bool FragmentShaderBarycentric::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	/* Set up camera properties */
	camera.type = vkb::CameraType::LookAt;
	camera.set_position({0.f, 1.0f, -6.0f});
	camera.set_rotation({0.f, 0.f, 0.f});
	camera.set_perspective(60.f, static_cast<float>(width) / static_cast<float>(height), 256.f, 0.1f);

	load_assets();
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
 * 	@fn void FragmentShaderBarycentric::load_assets()
 *	@brief Loading extra models, textures from assets
 */
void FragmentShaderBarycentric::load_assets()
{
	// Loading models
	skybox = load_model("scenes/cube.gltf");                      // background
	object = load_model("scenes/textured_unit_cube.gltf");        // cube in the center of the scene

	// Loading textures
	textures.envmap = load_texture_cubemap("textures/uffizi_rgba16f_cube.ktx", vkb::sg::Image::Color);
	textures.cube   = load_texture("textures/checkerboard_rgba.ktx", vkb::sg::Image::Color);
}

/**
 * 	@fn void FragmentShaderBarycentric::prepare_uniform_buffers()
 * 	@brief Preparing uniform buffer and updating UB data
 */
void FragmentShaderBarycentric::prepare_uniform_buffers()
{
	ubo = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(ubo_vs), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

/**
 * 	@fn void FragmentShaderBarycentric::create_descriptor_pool()
 * 	@brief Creating descriptor pool with size adjusted to use uniform buffer and image sampler
 */
void FragmentShaderBarycentric::create_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2)};
	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), 2);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

/**
 * 	@fn void FragmentShaderBarycentric::update_uniform_buffers()
 * 	@brief Updating data from application to GPU uniform buffer
 */
void FragmentShaderBarycentric::update_uniform_buffers()
{
	ubo_vs.projection = camera.matrices.perspective;
	ubo_vs.modelview  = camera.matrices.view;
	ubo->convert_and_update(ubo_vs);
}

/**
 * 	@fn void FragmentShaderBarycentric::setup_descriptor_set_layout()
 * 	@brief Creating layout for descriptor sets
 */
void FragmentShaderBarycentric::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &descriptor_set_layout,
	        1);

	// Pass selected effect information via push constants
	VkPushConstantRange push_constant_range =
	    vkb::initializers::push_constant_range(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(gui_settings.selected_effect), 0);
	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

/**
 * 	@fn void FragmentShaderBarycentric::create_descriptor_sets()
 * 	@brief Creating descriptor sets for two models
 */
void FragmentShaderBarycentric::create_descriptor_sets()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layout,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.skybox));

	VkDescriptorBufferInfo            matrix_buffer_descriptor     = create_descriptor(*ubo);
	VkDescriptorImageInfo             environment_image_descriptor = create_descriptor(textures.envmap);
	std::vector<VkWriteDescriptorSet> write_descriptor_sets        = {
        vkb::initializers::write_descriptor_set(descriptor_sets.skybox, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matrix_buffer_descriptor),
        vkb::initializers::write_descriptor_set(descriptor_sets.skybox, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &environment_image_descriptor)};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.object));
	VkDescriptorImageInfo cube_image_descriptor = create_descriptor(textures.cube);
	write_descriptor_sets                       = {
        vkb::initializers::write_descriptor_set(descriptor_sets.object, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matrix_buffer_descriptor),
        vkb::initializers::write_descriptor_set(descriptor_sets.object, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &cube_image_descriptor)};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

/**
 * 	@fn void FragmentShaderBarycentric::create_pipeline()
 * 	@brief Creating graphical pipeline
 */
void FragmentShaderBarycentric::create_pipeline()
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

	// Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(
	        VK_FALSE,
	        VK_FALSE,
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

	// Vertex bindings an attributes for model rendering
	// Binding description
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};

	// Attribute descriptions
	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)        // Position
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};
	shader_stages[0] = load_shader("fragment_shader_barycentric/skybox.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("fragment_shader_barycentric/skybox.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Use the pNext to point to the rendering create struct
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
	graphics_create.layout              = pipeline_layout;

	// Skybox pipeline (background cube)
	vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &graphics_create, VK_NULL_HANDLE, &pipelines.skybox);

	// Object pipeline
	shader_stages[0] = load_shader("fragment_shader_barycentric/object.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("fragment_shader_barycentric/object.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Flip cull mode
	rasterization_state.cullMode = VK_CULL_MODE_FRONT_BIT;
	vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &graphics_create, VK_NULL_HANDLE, &pipelines.object);
}

/**
 *  @fn void FragmentShaderBarycentric::draw()
 *  @brief Preparing frame and submitting it to the present queue
 */
void FragmentShaderBarycentric::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

/**
 * 	@fn void FragmentShaderBarycentric::build_command_buffers()
 * 	@brief Creating command buffers and drawing background and model on window
 */
void FragmentShaderBarycentric::build_command_buffers()
{
	std::array<VkClearValue, 2> clear_values{};
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0};

	int i = -1;
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

		vkCmdBindDescriptorSets(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets.skybox, 0, nullptr);

		// skybox
		vkCmdBindPipeline(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skybox);
		draw_model(skybox, draw_cmd_buffer);

		// object
		vkCmdBindDescriptorSets(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets.object, 0, nullptr);
		vkCmdPushConstants(draw_cmd_buffer, pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(gui_settings.selected_effect), &gui_settings.selected_effect);
		vkCmdBindPipeline(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.object);
		draw_model(object, draw_cmd_buffer);

		// UI
		draw_ui(draw_cmd_buffer);

		vkCmdEndRenderPass(draw_cmd_buffer);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffer));
	}
}

/**
 * 	@fn void FragmentShaderBarycentric::render(float delta_time)
 * 	@brief Drawing frames and/or updating uniform buffers when camera position/rotation was changed
 */
void FragmentShaderBarycentric::render(float delta_time)
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
 * @fn void FragmentShaderBarycentric::on_update_ui_overlay(vkb::Drawer &drawer)
 * @brief Projecting GUI and transferring data between GUI and application
 */
void FragmentShaderBarycentric::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.combo_box("Effects", &gui_settings.selected_effect, gui_settings.effect_names))
		{
			rebuild_command_buffers();
		}
	}
}

/**
 * @fn void FragmentShaderBarycentric::request_gpu_features(vkb::PhysicalDevice &gpu)
 * @brief Enabling features related to Vulkan extensions
 */
void FragmentShaderBarycentric::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	auto &requested_fragment_shader_barycentric_features =
	    gpu.request_extension_features<VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR);
	requested_fragment_shader_barycentric_features.fragmentShaderBarycentric = VK_TRUE;

	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = true;
	}
}

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::C>> create_fragment_shader_barycentric()
{
	return std::make_unique<FragmentShaderBarycentric>();
}
