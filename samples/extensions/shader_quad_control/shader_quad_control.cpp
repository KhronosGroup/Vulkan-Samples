/* Copyright (c) 2025, Holochip Inc.
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

#include "shader_quad_control.h"

ShaderQuadControl::ShaderQuadControl()
{
	title = "Shader quad control";
	set_api_version(VK_API_VERSION_1_2);

	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	// VK_KHR_shader_quad_control requires VK_KHR_shader_maximal_reconvergence per spec
	add_device_extension(VK_KHR_SHADER_MAXIMAL_RECONVERGENCE_EXTENSION_NAME);
	add_device_extension(VK_KHR_SHADER_QUAD_CONTROL_EXTENSION_NAME);
}

ShaderQuadControl::~ShaderQuadControl()
{
	if (has_device())
	{
		if (pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		}
		if (pipeline_layout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		}
	}
}

bool ShaderQuadControl::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	create_pipeline_layout();
	create_pipeline();
	build_command_buffers();

	prepared = true;
	return true;
}

void ShaderQuadControl::request_gpu_features(vkb::core::PhysicalDeviceC &gpu)
{
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceShaderQuadControlFeaturesKHR, shaderQuadControl);
}

void ShaderQuadControl::create_pipeline_layout()
{
	VkPipelineLayoutCreateInfo pipeline_layout_ci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_ci, nullptr, &pipeline_layout));
}

void ShaderQuadControl::create_pipeline()
{
	std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
	stages[0] = load_shader("shader_quad_control/glsl/quad_control.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	stages[1] = load_shader("shader_quad_control/glsl/quad_control.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	VkPipelineVertexInputStateCreateInfo vertex_input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

	VkPipelineInputAssemblyStateCreateInfo input_assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineViewportStateCreateInfo viewport_state{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	viewport_state.viewportCount = 1;
	viewport_state.scissorCount  = 1;

	VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	raster.polygonMode = VK_POLYGON_MODE_FILL;
	raster.cullMode    = VK_CULL_MODE_NONE;
	raster.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	raster.lineWidth   = 1.0f;

	VkPipelineMultisampleStateCreateInfo msaa{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
	msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depth_stencil{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};

	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo color_blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
	color_blend.attachmentCount = 1;
	color_blend.pAttachments    = &color_blend_attachment;

	std::array<VkDynamicState, 2>    dynamic_states{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic_state{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
	dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
	dynamic_state.pDynamicStates    = dynamic_states.data();

	VkGraphicsPipelineCreateInfo pipeline_ci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
	pipeline_ci.stageCount          = static_cast<uint32_t>(stages.size());
	pipeline_ci.pStages             = stages.data();
	pipeline_ci.pVertexInputState   = &vertex_input;
	pipeline_ci.pInputAssemblyState = &input_assembly;
	pipeline_ci.pViewportState      = &viewport_state;
	pipeline_ci.pRasterizationState = &raster;
	pipeline_ci.pMultisampleState   = &msaa;
	pipeline_ci.pDepthStencilState  = &depth_stencil;
	pipeline_ci.pColorBlendState    = &color_blend;
	pipeline_ci.pDynamicState       = &dynamic_state;
	pipeline_ci.layout              = pipeline_layout;
	pipeline_ci.renderPass          = render_pass;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &pipeline_ci, nullptr, &pipeline));
}

void ShaderQuadControl::build_command_buffers()
{
	VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	VkClearValue             clear_values[2];
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clear_values[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo render_pass_begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	render_pass_begin.renderPass        = render_pass;
	render_pass_begin.renderArea.offset = {0, 0};
	render_pass_begin.renderArea.extent = get_render_context().get_surface_extent();
	render_pass_begin.clearValueCount   = 2;
	render_pass_begin.pClearValues      = clear_values;

	for (size_t i = 0; i < draw_cmd_buffers.size(); i++)
	{
		render_pass_begin.framebuffer = framebuffers[i];
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &begin_info));

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.width    = static_cast<float>(get_render_context().get_surface_extent().width);
		viewport.height   = static_cast<float>(get_render_context().get_surface_extent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor{{0, 0}, get_render_context().get_surface_extent()};
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);

		draw_ui(draw_cmd_buffers[i]);
		vkCmdEndRenderPass(draw_cmd_buffers[i]);
		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void ShaderQuadControl::draw()
{
	prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	submit_frame();
}

void ShaderQuadControl::render(float)
{
	if (!prepared)
	{
		return;
	}
	draw();
}

std::unique_ptr<vkb::VulkanSampleC> create_shader_quad_control()
{
	return std::make_unique<ShaderQuadControl>();
}
