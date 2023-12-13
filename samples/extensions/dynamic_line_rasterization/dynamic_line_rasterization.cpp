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

#include "dynamic_line_rasterization.h"

DynamicLineRasterization::DynamicLineRasterization()
{
	add_device_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
	add_device_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
	add_device_extension(VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME);
}

DynamicLineRasterization::~DynamicLineRasterization()
{
	if (device)
	{
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);

		vkDestroyPipeline(get_device().get_handle(), pipelines.object, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipelines.grid, nullptr);

		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
		vkDestroyDescriptorPool(get_device().get_handle(), descriptor_pool, nullptr);
	}
}

bool DynamicLineRasterization::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position({0.0f, 1.0f, -5.0f});
	camera.set_rotation({-15.0f, 15.0f, 0.0f});
	camera.set_perspective(45.0f, static_cast<float>(width) / static_cast<float>(height), 128.0f, 0.1f);

	prepare_uniform_buffers();
	prepare_scene();
	setup_descriptor_pool();
	create_descriptor_set_layout();
	create_descriptor_set();
	create_pipelines();
	build_command_buffers();

	prepared = true;

	return true;
}

void DynamicLineRasterization::prepare_scene()
{
	std::vector<glm::vec3> vertices = {
	    {-1.0f, -1.0f, 1.0f},
	    {1.0f, -1.0f, 1.0f},
	    {1.0f, 1.0f, 1.0f},
	    {-1.0f, 1.0f, 1.0f},

	    {-1.0f, -1.0f, -1.0f},
	    {1.0f, -1.0f, -1.0f},
	    {1.0f, 1.0f, -1.0f},
	    {-1.0f, 1.0f, -1.0f}};

	std::vector<uint32_t> cube_indices = {
	    0, 1, 2,
	    2, 3, 0,

	    4, 5, 6,
	    6, 7, 4,

	    0, 3, 7,
	    7, 4, 0,

	    1, 5, 6,
	    6, 2, 1,

	    3, 2, 6,
	    6, 7, 3,

	    0, 4, 5,
	    5, 1, 0};

	// Indices of the edges of the cube
	std::vector<uint32_t> edges_indices = {
	    0, 1,
	    1, 2,
	    2, 3,
	    3, 0,

	    4, 5,
	    5, 6,
	    6, 7,
	    7, 4,

	    0, 4,
	    1, 5,
	    2, 6,
	    3, 7};

	cube_index_count                 = static_cast<uint32_t>(cube_indices.size());
	edges_index_count                = static_cast<uint32_t>(edges_indices.size());
	uint32_t vertex_buffer_size      = vertices.size() * sizeof(glm::vec3);
	uint32_t cube_index_buffer_size  = cube_indices.size() * sizeof(uint32_t);
	uint32_t edges_index_buffer_size = edges_indices.size() * sizeof(uint32_t);

	vertex_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                    vertex_buffer_size,
	                                                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                                                    VMA_MEMORY_USAGE_CPU_TO_GPU);
	vertex_buffer->update(vertices.data(), vertex_buffer_size);

	cube_index_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                        cube_index_buffer_size,
	                                                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	                                                        VMA_MEMORY_USAGE_CPU_TO_GPU);
	cube_index_buffer->update(cube_indices.data(), cube_index_buffer_size);

	edges_index_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                         edges_index_buffer_size,
	                                                         VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	                                                         VMA_MEMORY_USAGE_CPU_TO_GPU);
	edges_index_buffer->update(edges_indices.data(), edges_index_buffer_size);

	fill_color = glm::vec4(0.957f, 0.384f, 0.024f, 0.1f);
	edge_color = glm::vec4(0.957f, 0.384f, 0.024f, 1.0f);

	// Fill the first half of the stipple array with 'true' values for the initial stipple pattern.
	std::fill(gui_settings.stipple_pattern_arr.begin(), gui_settings.stipple_pattern_arr.begin() + 8, true);
	gui_settings.stipple_pattern = array_to_uint16(gui_settings.stipple_pattern_arr);
}

void DynamicLineRasterization::setup_descriptor_pool()
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

void DynamicLineRasterization::create_pipelines()
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

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(
	        VK_FALSE,
	        VK_FALSE,
	        VK_COMPARE_OP_NEVER);

	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(
	        VK_SAMPLE_COUNT_1_BIT,
	        0);

	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR,
	    VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY,
	    VK_DYNAMIC_STATE_POLYGON_MODE_EXT,
	    VK_DYNAMIC_STATE_LINE_RASTERIZATION_MODE_EXT,
	    VK_DYNAMIC_STATE_LINE_STIPPLE_ENABLE_EXT,
	    VK_DYNAMIC_STATE_LINE_STIPPLE_EXT,
	    VK_DYNAMIC_STATE_LINE_WIDTH};

	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0);

	const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};
	shader_stages[0] = load_shader("dynamic_line_rasterization/base.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("dynamic_line_rasterization/base.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

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
	                                   &pipelines.object));

	shader_stages[0]                  = load_shader("dynamic_line_rasterization/grid.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1]                  = load_shader("dynamic_line_rasterization/grid.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	graphics_create.pStages           = shader_stages.data();
	vertex_input_state                = vkb::initializers::pipeline_vertex_input_state_create_info();
	graphics_create.pVertexInputState = &vertex_input_state;

	vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &graphics_create, VK_NULL_HANDLE, &pipelines.grid);
}

void DynamicLineRasterization::prepare_uniform_buffers()
{
	camera_ubo = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(CameraUbo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
}

void DynamicLineRasterization::update_uniform_buffers()
{
	CameraUbo cam;
	cam.model      = glm::mat4(1.0f);
	cam.model      = glm::translate(cam.model, glm::vec3(0.0f));
	cam.view       = camera.matrices.view;
	cam.projection = camera.matrices.perspective;

	camera_ubo->convert_and_update(cam);

	rebuild_command_buffers();
}

void DynamicLineRasterization::create_descriptor_set()
{
	VkDescriptorSetAllocateInfo alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1u);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

	VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*camera_ubo);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, &buffer_descriptor),
	};

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0u, nullptr);
}

void DynamicLineRasterization::create_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1u)};

	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings);
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_set_layout_create_info, nullptr, &descriptor_set_layout));

	VkPushConstantRange push_constant_range =
	    vkb::initializers::push_constant_range(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::vec4), 0);

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layout);

	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void DynamicLineRasterization::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	VK_CHECK(vkQueueSubmit(queue, 1u, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();

	VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
}

void DynamicLineRasterization::render(float delta_time)
{
	if (!prepared)
		return;

	draw();

	if (camera.updated)
		update_uniform_buffers();
}

void DynamicLineRasterization::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	std::array<VkClearValue, 2> clear_values;
	clear_values[0].color        = {{0.05f, 0.05f, 0.05f, 1.0f}};
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

		vkCmdBindDescriptorSets(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0u, 1u, &descriptor_set, 0u, nullptr);

		// While dynamic parameterization is not utilized for the grid, it should be called before the first draw command to prevent validation layer warnings.
		vkCmdSetLineRasterizationModeEXT(cmd_buff, static_cast<VkLineRasterizationModeEXT>(gui_settings.selected_rasterization_mode));
		vkCmdSetLineWidth(cmd_buff, gui_settings.line_width);
		vkCmdSetLineStippleEnableEXT(cmd_buff, static_cast<VkBool32>(gui_settings.stipple_enabled));
		vkCmdSetLineStippleEXT(cmd_buff, gui_settings.stipple_factor, gui_settings.stipple_pattern);
		vkCmdSetPrimitiveTopologyEXT(cmd_buff, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		vkCmdSetPolygonModeEXT(cmd_buff, VK_POLYGON_MODE_FILL);

		// Draw the grid
		if (gui_settings.grid_enabled)
		{
			vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.grid);
			vkCmdDraw(cmd_buff, 6, 1, 0, 0);
		}

		vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.object);
		VkDeviceSize offsets[1] = {0};
		vkCmdBindVertexBuffers(cmd_buff, 0, 1, vertex_buffer->get(), offsets);

		// Fill the cube
		if (gui_settings.fill_enabled)
		{
			vkCmdBindIndexBuffer(cmd_buff, cube_index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdPushConstants(cmd_buff, pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec4), &fill_color);
			vkCmdSetPrimitiveTopologyEXT(cmd_buff, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
			vkCmdSetPolygonModeEXT(draw_cmd_buffers[i], VK_POLYGON_MODE_FILL);

			vkCmdDrawIndexed(cmd_buff, cube_index_count, 1, 0, 0, 0);
		}

		// Draw the cube edges
		vkCmdBindIndexBuffer(cmd_buff, edges_index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdPushConstants(cmd_buff, pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec4), &edge_color);
		vkCmdSetPrimitiveTopologyEXT(cmd_buff, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
		vkCmdSetPolygonModeEXT(cmd_buff, VK_POLYGON_MODE_LINE);

		vkCmdDrawIndexed(cmd_buff, edges_index_count, 1, 0, 0, 0);

		draw_ui(cmd_buff);

		vkCmdEndRenderPass(cmd_buff);

		VK_CHECK(vkEndCommandBuffer(cmd_buff));
	}
}

void DynamicLineRasterization::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	{
		auto &features = gpu.request_extension_features<VkPhysicalDeviceLineRasterizationFeaturesEXT>(
		    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT);
		features.smoothLines              = VK_TRUE;
		features.stippledSmoothLines      = VK_TRUE;
		features.bresenhamLines           = VK_TRUE;
		features.stippledBresenhamLines   = VK_TRUE;
		features.rectangularLines         = VK_TRUE;
		features.stippledRectangularLines = VK_TRUE;
	}
	{
		auto &features =
		    gpu.request_extension_features<VkPhysicalDeviceExtendedDynamicStateFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT);
		features.extendedDynamicState = VK_TRUE;
	}
	{
		auto &features =
		    gpu.request_extension_features<VkPhysicalDeviceExtendedDynamicState3FeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT);
		features.extendedDynamicState3PolygonMode           = VK_TRUE;
		features.extendedDynamicState3LineRasterizationMode = VK_TRUE;
		features.extendedDynamicState3LineStippleEnable     = VK_TRUE;
	}
	{
		auto &features            = gpu.get_mutable_requested_features();
		features.fillModeNonSolid = VK_TRUE;
		features.wideLines        = VK_TRUE;
	}
}

void DynamicLineRasterization::on_update_ui_overlay(vkb::Drawer &drawer)
{
	auto build_command_buffers_when = [this](bool drawer_action) {
		if (drawer_action)
			rebuild_command_buffers();
	};

	auto uint16_to_hex_string = [](const char *caption, uint16_t value) {
		std::stringstream stream;
		stream << caption << std::hex << value;
		return stream.str();
	};

	if (drawer.header("Primitive options"))
	{
		build_command_buffers_when(drawer.checkbox("Fill", &gui_settings.fill_enabled));
		build_command_buffers_when(drawer.checkbox("Grid", &gui_settings.grid_enabled));
		build_command_buffers_when(drawer.combo_box("Rasterization mode", &gui_settings.selected_rasterization_mode, gui_settings.rasterization_mode_names));
		build_command_buffers_when(drawer.slider_float("Line width", &gui_settings.line_width, 1.0f, 64.0f));
		build_command_buffers_when(drawer.checkbox("Stipple enabled", &gui_settings.stipple_enabled));
		// The stipple factor has a maximum value of 256. Here, a limit of 64 has been chosen to achieve a scroll step equal to 1.
		build_command_buffers_when(drawer.slider_int("Stipple factor", &gui_settings.stipple_factor, 1, 64));
		drawer.text(uint16_to_hex_string("Stipple pattern: ", gui_settings.stipple_pattern).c_str());

		for (int i = 0; i < 16; ++i)
		{
			ImGui::PushID(i);
			if (drawer.checkbox("", &(gui_settings.stipple_pattern_arr[i])))
			{
				gui_settings.stipple_pattern = array_to_uint16(gui_settings.stipple_pattern_arr);

				rebuild_command_buffers();
			}
			ImGui::PopID();
			if (i % 8 != 7)
				ImGui::SameLine();
		}
	}
}

uint16_t DynamicLineRasterization::array_to_uint16(const std::array<bool, 16> &array)
{
	uint16_t result = 0;
	for (int i = 0; i < 16; ++i)
		if (array[i])
			result |= (1 << i);
	return result;
}

bool DynamicLineRasterization::resize(const uint32_t width, const uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	update_uniform_buffers();
	return true;
}

std::unique_ptr<vkb::VulkanSample> create_dynamic_line_rasterization()
{
	return std::make_unique<DynamicLineRasterization>();
}
