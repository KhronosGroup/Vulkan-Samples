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

#include "hello_triangle_framework.h"

HelloTriangleFramework::HelloTriangleFramework()
{
}

HelloTriangleFramework::~HelloTriangleFramework()
{
	if (device)
	{
		vkDestroyPipeline(get_device().get_handle(), triangle_pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), triangle_pipeline_layout, nullptr);
	}
}

bool HelloTriangleFramework::prepare(vkb::Platform &platform)
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

void HelloTriangleFramework::prepare_pipelines()
{
	// Create a blank pipeline layout.
	// We are not binding any resources to the pipeline in this first sample.
	VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &layout_info, nullptr, &triangle_pipeline_layout));

	VkPipelineVertexInputStateCreateInfo vertex_input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

	// Specify we will use triangle lists to draw geometry.
	VkPipelineInputAssemblyStateCreateInfo input_assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Specify rasterization state.
	VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	raster.cullMode  = VK_CULL_MODE_BACK_BIT;
	raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
	raster.lineWidth = 1.0f;

	// Our attachment will write to all color channels, but no blending is enabled.
	VkPipelineColorBlendAttachmentState blend_attachment{};
	blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
	blend.attachmentCount = 1;
	blend.pAttachments    = &blend_attachment;

	// We will have one viewport and scissor box.
	VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	viewport.viewportCount = 1;
	viewport.scissorCount  = 1;

	// Disable all depth testing.
	VkPipelineDepthStencilStateCreateInfo depth_stencil{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};

	// No multisampling.
	VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Specify that these states will be dynamic, i.e. not part of pipeline state object.
	std::array<VkDynamicState, 2> dynamics{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
	dynamic.pDynamicStates    = dynamics.data();
	dynamic.dynamicStateCount = vkb::to_u32(dynamics.size());

	// Load our SPIR-V shaders.
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};

	// Vertex stage of the pipeline
	shader_stages[0] = load_shader("triangle.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("triangle.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	VkGraphicsPipelineCreateInfo pipe{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
	pipe.stageCount          = vkb::to_u32(shader_stages.size());
	pipe.pStages             = shader_stages.data();
	pipe.pVertexInputState   = &vertex_input;
	pipe.pInputAssemblyState = &input_assembly;
	pipe.pRasterizationState = &raster;
	pipe.pColorBlendState    = &blend;
	pipe.pMultisampleState   = &multisample;
	pipe.pViewportState      = &viewport;
	pipe.pDepthStencilState  = &depth_stencil;
	pipe.pDynamicState       = &dynamic;

	// We need to specify the pipeline layout and the render pass description up front as well.
	pipe.renderPass = render_pass;
	pipe.layout     = triangle_pipeline_layout;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &pipe, nullptr, &triangle_pipeline));
}

void HelloTriangleFramework::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass            = render_pass;
	render_pass_begin_info.renderArea.offset.x   = 0;
	render_pass_begin_info.renderArea.offset.y   = 0;
	render_pass_begin_info.clearValueCount       = 2;
	render_pass_begin_info.pClearValues          = clear_values;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		auto cmd = draw_cmd_buffers[i];
		vkBeginCommandBuffer(cmd, &command_buffer_begin_info);

		// Begin the render pass.
		VkRenderPassBeginInfo rp_begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
		rp_begin.renderPass               = render_pass;
		rp_begin.framebuffer              = framebuffers[i];
		rp_begin.renderArea.extent.width  = width;
		rp_begin.renderArea.extent.height = height;
		rp_begin.clearValueCount          = 2;
		rp_begin.pClearValues             = clear_values;
		// We will add draw commands in the same command buffer.
		vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

		// Bind the graphics pipeline.
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle_pipeline);

		VkViewport vp{};
		vp.width    = static_cast<float>(width);
		vp.height   = static_cast<float>(height);
		vp.minDepth = 0.0f;
		vp.maxDepth = 1.0f;
		// Set viewport dynamically
		vkCmdSetViewport(cmd, 0, 1, &vp);

		VkRect2D scissor{};
		scissor.extent.width  = width;
		scissor.extent.height = height;
		// Set scissor dynamically
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		// Draw three vertices with one instance.
		vkCmdDraw(cmd, 3, 1, 0, 0);

		// Traw user interface.
		draw_ui(draw_cmd_buffers[i]);

		// Complete render pass.
		vkCmdEndRenderPass(cmd);

		// Complete the command buffer.
		VK_CHECK(vkEndCommandBuffer(cmd));
	}
}

void HelloTriangleFramework::render(float delta_time)
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

std::unique_ptr<vkb::Application> create_hello_triangle_framework()
{
	return std::make_unique<HelloTriangleFramework>();
}
