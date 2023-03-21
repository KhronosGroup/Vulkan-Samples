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

#include "sparse_image.h"

SparseImage::SparseImage()
{
	title = "Sparse image";
}

SparseImage::~SparseImage()
{
	if (device)
	{
		object.reset();
		ubo.reset();

		vkDestroyPipeline(get_device().get_handle(), pipeline, VK_NULL_HANDLE);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, VK_NULL_HANDLE);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, VK_NULL_HANDLE);
	}
}

/**
 * 	@fn bool SparseImage::prepare(vkb::Platform &platform)
 * 	@brief Configuring all sample specific settings, creating descriptor sets/pool, pipelines, generating or loading models etc.
 */
bool SparseImage::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	prepare_pipeline();
	build_command_buffers();
	prepared = true;
	return true;
}

/**
 * @fn void SparseImage::request_gpu_features(vkb::PhysicalDevice &gpu)
 * @brief Enabling features related to Vulkan extensions
 */
void SparseImage::request_gpu_features(vkb::PhysicalDevice &gpu)
{
}

/**
 * @fn void SparseImage::on_update_ui_overlay(vkb::Drawer &drawer)
 * @brief Projecting GUI and transferring data between GUI and application
 */
void SparseImage::on_update_ui_overlay(vkb::Drawer &drawer)
{
}

/**
 * 	@fn void SparseImage::prepare_pipeline()
 * 	@brief Creating graphical pipeline
 */
void SparseImage::prepare_pipeline()
{
	// Create a blank pipeline layout.
	// We are not binding any resources to the pipeline in this sample.
	VkPipelineLayoutCreateInfo layout_info = vkb::initializers::pipeline_layout_create_info(nullptr, 0);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &layout_info, nullptr, &pipeline_layout));

	VkPipelineVertexInputStateCreateInfo vertex_input = vkb::initializers::pipeline_vertex_input_state_create_info();

	// Specify we will use triangle lists to draw geometry.
	VkPipelineInputAssemblyStateCreateInfo input_assembly = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	// Specify rasterization state.
	VkPipelineRasterizationStateCreateInfo raster = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);

	// Our attachment will write to all color channels, but no blending is enabled.
	VkPipelineColorBlendAttachmentState blend_attachment = vkb::initializers::pipeline_color_blend_attachment_state(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);

	VkPipelineColorBlendStateCreateInfo blend = vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment);

	// We will have one viewport and scissor box.
	VkPipelineViewportStateCreateInfo viewport = vkb::initializers::pipeline_viewport_state_create_info(1, 1);

	// Disable all depth testing.
	VkPipelineDepthStencilStateCreateInfo depth_stencil = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_FALSE, VK_FALSE, VK_COMPARE_OP_NEVER);

	// No multisampling.
	VkPipelineMultisampleStateCreateInfo multisample = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT);

	// Specify that these states will be dynamic, i.e. not part of pipeline state object.
	std::array<VkDynamicState, 2>    dynamics{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic = vkb::initializers::pipeline_dynamic_state_create_info(dynamics.data(), vkb::to_u32(dynamics.size()));

	// Load our SPIR-V shaders.
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};

	// Vertex stage of the pipeline
	shader_stages[0] = load_shader("triangle.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("triangle.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// We need to specify the pipeline layout and the render pass description up front as well.
	VkGraphicsPipelineCreateInfo pipe = vkb::initializers::pipeline_create_info(pipeline_layout, render_pass);
	pipe.stageCount                   = vkb::to_u32(shader_stages.size());
	pipe.pStages                      = shader_stages.data();
	pipe.pVertexInputState            = &vertex_input;
	pipe.pInputAssemblyState          = &input_assembly;
	pipe.pRasterizationState          = &raster;
	pipe.pColorBlendState             = &blend;
	pipe.pMultisampleState            = &multisample;
	pipe.pViewportState               = &viewport;
	pipe.pDepthStencilState           = &depth_stencil;
	pipe.pDynamicState                = &dynamic;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipe, nullptr, &pipeline));
}

/**
 * 	@fn void SparseImage::build_command_buffers()
 * 	@brief Creating command buffers and drawing background and model on window
 */
void SparseImage::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	// Clear color and depth values.
	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0};

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
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		// Set viewport dynamically
		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		// Set scissor dynamically
		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		// Draw three vertices with one instance.
		vkCmdDraw(cmd, 3, 1, 0, 0);

		// Draw user interface.
		draw_ui(draw_cmd_buffers[i]);

		// Complete render pass.
		vkCmdEndRenderPass(cmd);

		// Complete the command buffer.
		VK_CHECK(vkEndCommandBuffer(cmd));
	}
}

/**
 * 	@fn void SparseImage::render(float delta_time)
 * 	@brief Drawing frames and/or updating uniform buffers when camera position/rotation was changed
 */
void SparseImage::render(float delta_time)
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
 * 	@fn void SparseImage::draw()
 *  @brief Preparing frame and submitting it to the present queue
 */
void SparseImage::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

/**
 * 	@fn void SparseImage::load_assets()
 *	@brief Loading extra models, textures from assets
 */
void SparseImage::load_assets()
{}

/**
 * 	@fn void SparseImage::prepare_uniform_buffers()
 * 	@brief Preparing uniform buffer and updating UB data
 */
void SparseImage::prepare_uniform_buffers()
{}

/**
 * 	@fn void SparseImage::update_uniform_buffers()
 * 	@brief Updating data from application to GPU uniform buffer
 */
void SparseImage::update_uniform_buffers()
{}

/**
 * 	@fn void SparseImage::setup_descriptor_set_layout()
 * 	@brief Creating layout for descriptor sets
 */
void SparseImage::setup_descriptor_set_layout()
{}

/**
 * 	@fn void SparseImage::create_descriptor_sets()
 * 	@brief Creating both descriptor set:
 * 		   1. Uniform buffer
 * 		   2. Image sampler
 */
void SparseImage::create_descriptor_sets()
{}

/**
 * 	@fn void SparseImage::create_descriptor_pool()
 * 	@brief Creating descriptor pool with size adjusted to use uniform buffer and image sampler
 */
void SparseImage::create_descriptor_pool()
{}

std::unique_ptr<vkb::VulkanSample> create_sparse_image()
{
	return std::make_unique<SparseImage>();
}
