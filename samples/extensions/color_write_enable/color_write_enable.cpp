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

#include "color_write_enable.h"

ColorWriteEnable::ColorWriteEnable()
{
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_EXT_COLOR_WRITE_ENABLE_EXTENSION_NAME);
	add_device_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
}

ColorWriteEnable::~ColorWriteEnable()
{
	if (device)
	{
		vkDestroyPipeline(get_device().get_handle(), triangle_pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), triangle_pipeline_layout, nullptr);
	}
}

bool ColorWriteEnable::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	prepare_pipelines();
	build_command_buffers();
	prepared = true;
	return true;
}

void ColorWriteEnable::prepare_pipelines()
{
	// Create a blank pipeline layout.
	// We are not binding any resources to the pipeline in this sample.
	VkPipelineLayoutCreateInfo layout_info = vkb::initializers::pipeline_layout_create_info(nullptr, 0);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &layout_info, nullptr, &triangle_pipeline_layout));

	VkPipelineVertexInputStateCreateInfo vertex_input = vkb::initializers::pipeline_vertex_input_state_create_info();

	// Specify we will use triangle lists to draw geometry.
	VkPipelineInputAssemblyStateCreateInfo input_assembly = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	// Specify rasterization state.
	VkPipelineRasterizationStateCreateInfo raster = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);

	// Prepare separate attachment for each color channel.
	VkPipelineColorBlendAttachmentState red_blend_attachment   = vkb::initializers::pipeline_color_blend_attachment_state(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
	VkPipelineColorBlendAttachmentState green_blend_attachment = vkb::initializers::pipeline_color_blend_attachment_state(VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
	VkPipelineColorBlendAttachmentState blue_blend_attachment  = vkb::initializers::pipeline_color_blend_attachment_state(VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
	VkPipelineColorBlendAttachmentState blend_attachment[3]    = {red_blend_attachment, green_blend_attachment, blue_blend_attachment};

	// Define separate color_write_enable toggle for each color attachment.
	VkPipelineColorWriteCreateInfoEXT color_write_info    = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_WRITE_CREATE_INFO_EXT};
	std::array<VkBool32, 3>           color_write_enables = {r_bit_enabled, g_bit_enabled, b_bit_enabled};
	color_write_info.attachmentCount                      = static_cast<uint32_t>(color_write_enables.size());
	color_write_info.pColorWriteEnables                   = color_write_enables.data();

	// Define color blend with an attachment for each color. Chain it with color_write_info.
	VkPipelineColorBlendStateCreateInfo color_blend_state = vkb::initializers::pipeline_color_blend_state_create_info(3, blend_attachment);
	color_blend_state.pNext                               = &color_write_info;

	// We will have one viewport and scissor box.
	VkPipelineViewportStateCreateInfo viewport = vkb::initializers::pipeline_viewport_state_create_info(1, 1);

	// No multisampling.
	VkPipelineMultisampleStateCreateInfo multisample = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT);

	// Specify that these states will be dynamic, i.e. not part of pipeline state object.
	std::array<VkDynamicState, 3>    dynamics{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_COLOR_WRITE_ENABLE_EXT};
	VkPipelineDynamicStateCreateInfo dynamic = vkb::initializers::pipeline_dynamic_state_create_info(dynamics.data(), vkb::to_u32(dynamics.size()));

	// Load our SPIR-V shaders.
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {};

	// Vertex stage of the pipeline
	shader_stages[0] = load_shader("color_write_enable/triangle_separate_channels.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("color_write_enable/triangle_separate_channels.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// We need to specify the pipeline layout and the render pass description up front as well.
	VkGraphicsPipelineCreateInfo pipe = vkb::initializers::pipeline_create_info(triangle_pipeline_layout, render_pass);
	pipe.stageCount                   = vkb::to_u32(shader_stages.size());
	pipe.pStages                      = shader_stages.data();
	pipe.pVertexInputState            = &vertex_input;
	pipe.pInputAssemblyState          = &input_assembly;
	pipe.pRasterizationState          = &raster;
	pipe.pColorBlendState             = &color_blend_state;
	pipe.pMultisampleState            = &multisample;
	pipe.pViewportState               = &viewport;
	pipe.pDynamicState                = &dynamic;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipe, nullptr, &triangle_pipeline));
}

void ColorWriteEnable::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	auto &features            = gpu.request_extension_features<VkPhysicalDeviceColorWriteEnableFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COLOR_WRITE_ENABLE_FEATURES_EXT);
	features.colorWriteEnable = VK_TRUE;
}

void ColorWriteEnable::setup_render_pass()
{
	std::array<VkAttachmentDescription, 3> attachments = {};
	// 3 color attachments
	for (uint32_t i = 0; i < 3; ++i)
	{
		attachments[i].format         = render_context->get_format();
		attachments[i].samples        = VK_SAMPLE_COUNT_1_BIT;
		attachments[i].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[i].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[i].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[i].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[i].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}

	std::array<VkAttachmentReference, 3> color_reference = {};
	for (uint32_t i = 0; i < color_reference.size(); ++i)
	{
		color_reference[i].attachment = i;
		color_reference[i].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	VkSubpassDescription subpass_description    = {};
	subpass_description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_description.colorAttachmentCount    = static_cast<uint32_t>(color_reference.size());
	subpass_description.pColorAttachments       = color_reference.data();
	subpass_description.inputAttachmentCount    = 0;
	subpass_description.pInputAttachments       = nullptr;
	subpass_description.preserveAttachmentCount = 0;
	subpass_description.pPreserveAttachments    = nullptr;
	subpass_description.pResolveAttachments     = nullptr;

	// Subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies = {};

	dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass      = 0;
	dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass      = 0;
	dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo render_pass_create_info = {};
	render_pass_create_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount        = static_cast<uint32_t>(attachments.size());
	render_pass_create_info.pAttachments           = attachments.data();
	render_pass_create_info.subpassCount           = 1;
	render_pass_create_info.pSubpasses             = &subpass_description;
	render_pass_create_info.dependencyCount        = static_cast<uint32_t>(dependencies.size());
	render_pass_create_info.pDependencies          = dependencies.data();

	VK_CHECK(vkCreateRenderPass(device->get_handle(), &render_pass_create_info, nullptr, &render_pass));
}

void ColorWriteEnable::setup_framebuffer()
{
	std::array<VkImageView, 3> attachments = {};

	VkFramebufferCreateInfo framebuffer_create_info = {};
	framebuffer_create_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_create_info.pNext                   = NULL;
	framebuffer_create_info.renderPass              = render_pass;
	framebuffer_create_info.attachmentCount         = static_cast<uint32_t>(attachments.size());
	framebuffer_create_info.pAttachments            = attachments.data();
	framebuffer_create_info.width                   = get_render_context().get_surface_extent().width;
	framebuffer_create_info.height                  = get_render_context().get_surface_extent().height;
	framebuffer_create_info.layers                  = 1;

	// Delete existing frame buffers
	for (uint32_t i = 0; i < framebuffers.size(); i++)
	{
		if (framebuffers[i] != VK_NULL_HANDLE)
		{
			vkDestroyFramebuffer(device->get_handle(), framebuffers[i], nullptr);
		}
	}

	// Create frame buffers for every swap chain image
	framebuffers.resize(render_context->get_render_frames().size());
	for (uint32_t i = 0; i < framebuffers.size(); i++)
	{
		attachments[0] = swapchain_buffers[i].view;
		attachments[1] = swapchain_buffers[i].view;
		attachments[2] = swapchain_buffers[i].view;
		VK_CHECK(vkCreateFramebuffer(device->get_handle(), &framebuffer_create_info, nullptr, &framebuffers[i]));
	}
}

void ColorWriteEnable::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	// Clear color values.
	std::array<VkClearValue, 3> clear_values = {};
	clear_values[0].color                    = {background_r_value, background_g_value, background_b_value, 0.0f};
	clear_values[1].color                    = {background_r_value, background_g_value, background_b_value, 0.0f};
	clear_values[2].color                    = {background_r_value, background_g_value, background_b_value, 0.0f};

	// Begin the render pass.
	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.offset.x      = 0;
	render_pass_begin_info.renderArea.offset.y      = 0;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = clear_values.size();
	render_pass_begin_info.pClearValues             = clear_values.data();

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
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle_pipeline);

		// Set viewport dynamically
		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		// Set scissor dynamically
		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		// Toggle color_write_enable for each attachment dynamically
		std::array<VkBool32, 3> color_write_enables = {r_bit_enabled, g_bit_enabled, b_bit_enabled};
		vkCmdSetColorWriteEnableEXT(cmd, static_cast<uint32_t>(color_write_enables.size()), color_write_enables.data());

		// Draw three vertices with one instance.
		vkCmdDraw(cmd, 3, 1, 0, 0);

		// Draw user interface.
		draw_ui(cmd);

		// Complete render pass.
		vkCmdEndRenderPass(cmd);

		// Complete the command buffer.
		VK_CHECK(vkEndCommandBuffer(cmd));
	}
}

void ColorWriteEnable::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Background color"))
	{
		if (drawer.slider_float("Red", &background_r_value, 0.0f, 1.0f))
		{
			build_command_buffers();
		}
		if (drawer.slider_float("Green", &background_g_value, 0.0f, 1.0f))
		{
			build_command_buffers();
		}
		if (drawer.slider_float("Blue", &background_b_value, 0.0f, 1.0f))
		{
			build_command_buffers();
		}
	}

	if (drawer.header("Enabled attachment"))
	{
		if (drawer.checkbox("Red bit", &r_bit_enabled))
		{
			build_command_buffers();
		}
		if (drawer.checkbox("Green bit", &g_bit_enabled))
		{
			build_command_buffers();
		}
		if (drawer.checkbox("Blue bit", &b_bit_enabled))
		{
			build_command_buffers();
		}
	}
}

void ColorWriteEnable::render(float delta_time)
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
}

std::unique_ptr<vkb::Application> create_color_write_enable()
{
	return std::make_unique<ColorWriteEnable>();
}
