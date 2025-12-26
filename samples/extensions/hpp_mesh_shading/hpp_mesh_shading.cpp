/* Copyright (c) 2024-2025, NVIDIA CORPORATION. All rights reserved.
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
 * Basic example for VK_EXT_mesh_shader, using Vulkan-Hpp.
 * There is only a mesh shader and a fragment shader. The mesh shader creates the vertices for a single triangle.
 */

#include "hpp_mesh_shading.h"

HPPMeshShading::HPPMeshShading()
{
	title = "Mesh shading";

	// VK_EXT_mesh_shader depends on VK_KHR_spirv_1_4, which in turn depends on Vulkan 1.1 and VK_KHR_shader_float_controls
	set_api_version(VK_API_VERSION_1_1);

	add_device_extension(VK_EXT_MESH_SHADER_EXTENSION_NAME);
	add_device_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
	add_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
}

HPPMeshShading::~HPPMeshShading()
{
	if (has_device() && get_device().get_handle())
	{
		vk::Device const &device = get_device().get_handle();

		device.destroyPipeline(pipeline);
		device.destroyPipelineLayout(pipeline_layout);
		device.destroyDescriptorSetLayout(descriptor_set_layout);
	}
}

bool HPPMeshShading::prepare(const vkb::ApplicationOptions &options)
{
	assert(!prepared);

	if (HPPApiVulkanSample::prepare(options))
	{
		vk::Device device = get_device().get_handle();

		descriptor_pool       = device.createDescriptorPool({.maxSets = 2});
		descriptor_set_layout = device.createDescriptorSetLayout({});
		descriptor_set =
		    device.allocateDescriptorSets({.descriptorPool = descriptor_pool, .descriptorSetCount = 1, .pSetLayouts = &descriptor_set_layout}).front();

		// Create a blank pipeline layout.
		// We are not binding any resources to the pipeline in this first sample.
		pipeline_layout = device.createPipelineLayout({.setLayoutCount = 1, .pSetLayouts = &descriptor_set_layout});

		pipeline = create_pipeline();

		build_command_buffers();

		prepared = true;
	}

	return prepared;
}

void HPPMeshShading::request_gpu_features(vkb::core::PhysicalDeviceCpp &gpu)
{
	// Enable extension features required by this sample
	// These are passed to device creation via a pNext structure chain
	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceMeshShaderFeaturesEXT, meshShader);
}

/*
    Command buffer generation
*/
void HPPMeshShading::build_command_buffers()
{
	vk::CommandBufferBeginInfo command_buffer_begin_info;

	std::array<vk::ClearValue, 2> clear_values = {{default_clear_color, vk::ClearDepthStencilValue{0.0f, 0}}};

	vk::RenderPassBeginInfo render_pass_begin_info{.renderPass      = render_pass,
	                                               .renderArea      = {{0, 0}, extent},
	                                               .clearValueCount = static_cast<uint32_t>(clear_values.size()),
	                                               .pClearValues    = clear_values.data()};

	for (size_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		render_pass_begin_info.framebuffer = framebuffers[i];

		vk::CommandBuffer command_buffer = draw_cmd_buffers[i];

		command_buffer.begin(command_buffer_begin_info);
		command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
		command_buffer.setViewport(0, {{0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f}});
		command_buffer.setScissor(0, {{{0, 0}, extent}});
		command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, descriptor_set, {});
		command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

		// Mesh shaders need the vkCmdDrawMeshTasksExt
		command_buffer.drawMeshTasksEXT(1, 1, 1);

		draw_ui(command_buffer);

		command_buffer.endRenderPass();
		command_buffer.end();
	}
}

void HPPMeshShading::render(float delta_time)
{
	if (prepared)
	{
		draw();
	}
}

vk::Pipeline HPPMeshShading::create_pipeline()
{
	// Load our SPIR-V shaders.
	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = {load_shader("mesh_shading", "ms.mesh.spv", vk::ShaderStageFlagBits::eMeshEXT),
	                                                                load_shader("mesh_shading", "ps.frag.spv", vk::ShaderStageFlagBits::eFragment)};

	// We will have one viewport and scissor box.
	vk::PipelineViewportStateCreateInfo viewport_state{.viewportCount = 1, .scissorCount = 1};

	vk::PipelineRasterizationStateCreateInfo rasterization_state{
	    .polygonMode = vk::PolygonMode::eFill, .cullMode = vk::CullModeFlagBits::eNone, .frontFace = vk::FrontFace::eCounterClockwise, .lineWidth = 1.0f};

	// No multisampling.
	vk::PipelineMultisampleStateCreateInfo multisample_state{.rasterizationSamples = vk::SampleCountFlagBits::e1};

	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state{
	    .depthTestEnable = false, .depthWriteEnable = true, .depthCompareOp = vk::CompareOp::eGreater, .back = {.compareOp = vk::CompareOp::eGreater}};

	vk::PipelineColorBlendAttachmentState blend_attachment_state{.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
	                                                                               vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

	vk::PipelineColorBlendStateCreateInfo color_blend_state{.attachmentCount = 1, .pAttachments = &blend_attachment_state};

	// Specify that these states will be dynamic, i.e. not part of pipeline state object.
	std::array<vk::DynamicState, 2>    dynamic_state_enables = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
	vk::PipelineDynamicStateCreateInfo dynamic_state{.dynamicStateCount = static_cast<uint32_t>(dynamic_state_enables.size()),
	                                                 .pDynamicStates    = dynamic_state_enables.data()};

	vk::GraphicsPipelineCreateInfo graphics_pipeline_create_info{.stageCount          = static_cast<uint32_t>(shader_stages.size()),
	                                                             .pStages             = shader_stages.data(),
	                                                             .pViewportState      = &viewport_state,
	                                                             .pRasterizationState = &rasterization_state,
	                                                             .pMultisampleState   = &multisample_state,
	                                                             .pDepthStencilState  = &depth_stencil_state,
	                                                             .pColorBlendState    = &color_blend_state,
	                                                             .pDynamicState       = &dynamic_state,
	                                                             .layout              = pipeline_layout,
	                                                             .renderPass          = render_pass};

	vk::Result   result;
	vk::Pipeline pipeline;
	std::tie(result, pipeline) = get_device().get_handle().createGraphicsPipeline(pipeline_cache, graphics_pipeline_create_info);
	assert(result == vk::Result::eSuccess);

	return pipeline;
}

void HPPMeshShading::draw()
{
	HPPApiVulkanSample::prepare_frame();

	// Submit to queue
	submit_info.setCommandBuffers(draw_cmd_buffers[current_buffer]);
	queue.submit(submit_info);

	HPPApiVulkanSample::submit_frame();
}

std::unique_ptr<vkb::VulkanSampleCpp> create_hpp_mesh_shading()
{
	return std::make_unique<HPPMeshShading>();
}
