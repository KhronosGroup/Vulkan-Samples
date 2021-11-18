/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include "pipeline.h"

#include "device.h"
#include "pipeline_layout.h"
#include "shader_module.h"

namespace vkb
{
Pipeline::Pipeline(Device &device) :
    device{device}
{}

Pipeline::Pipeline(Pipeline &&other) :
    device{other.device},
    handle{other.handle},
    state{other.state}
{
	other.handle = VK_NULL_HANDLE;
}

Pipeline::~Pipeline()
{
	// Destroy pipeline
	if (handle != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(device.get_handle(), handle, nullptr);
	}
}

VkPipeline Pipeline::get_handle() const
{
	return handle;
}

const PipelineState &Pipeline::get_state() const
{
	return state;
}

ComputePipeline::ComputePipeline(Device &        device,
                                 VkPipelineCache pipeline_cache,
                                 PipelineState & pipeline_state) :
    Pipeline{device}
{
	const ShaderModule *shader_module = pipeline_state.get_pipeline_layout().get_shader_modules().front();

	if (shader_module->get_stage() != VK_SHADER_STAGE_COMPUTE_BIT)
	{
		throw VulkanException{VK_ERROR_INVALID_SHADER_NV, "Shader module stage is not compute"};
	}

	VkPipelineShaderStageCreateInfo stage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};

	stage.stage = shader_module->get_stage();
	stage.pName = shader_module->get_entry_point().c_str();

	// Create the Vulkan handle
	VkShaderModuleCreateInfo vk_create_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};

	vk_create_info.codeSize = shader_module->get_binary().size() * sizeof(uint32_t);
	vk_create_info.pCode    = shader_module->get_binary().data();

	VkResult result = vkCreateShaderModule(device.get_handle(), &vk_create_info, nullptr, &stage.module);
	if (result != VK_SUCCESS)
	{
		throw VulkanException{result};
	}

	device.get_debug_utils().set_debug_name(device.get_handle(),
	                                        VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<uint64_t>(stage.module),
	                                        shader_module->get_debug_name().c_str());

	// Create specialization info from tracked state.
	std::vector<uint8_t>                  data{};
	std::vector<VkSpecializationMapEntry> map_entries{};

	const auto specialization_constant_state = pipeline_state.get_specialization_constant_state().get_specialization_constant_state();

	for (const auto specialization_constant : specialization_constant_state)
	{
		map_entries.push_back({specialization_constant.first, to_u32(data.size()), specialization_constant.second.size()});
		data.insert(data.end(), specialization_constant.second.begin(), specialization_constant.second.end());
	}

	VkSpecializationInfo specialization_info{};
	specialization_info.mapEntryCount = to_u32(map_entries.size());
	specialization_info.pMapEntries   = map_entries.data();
	specialization_info.dataSize      = data.size();
	specialization_info.pData         = data.data();

	stage.pSpecializationInfo = &specialization_info;

	VkComputePipelineCreateInfo create_info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};

	create_info.layout = pipeline_state.get_pipeline_layout().get_handle();
	create_info.stage  = stage;

	result = vkCreateComputePipelines(device.get_handle(), pipeline_cache, 1, &create_info, nullptr, &handle);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create ComputePipelines"};
	}

	vkDestroyShaderModule(device.get_handle(), stage.module, nullptr);
}

GraphicsPipeline::GraphicsPipeline(Device &        device,
                                   VkPipelineCache pipeline_cache,
                                   PipelineState & pipeline_state) :
    Pipeline{device}
{
	std::vector<VkShaderModule> shader_modules;

	std::vector<VkPipelineShaderStageCreateInfo> stage_create_infos;

	// Create specialization info from tracked state. This is shared by all shaders.
	std::vector<uint8_t>                  data{};
	std::vector<VkSpecializationMapEntry> map_entries{};

	const auto specialization_constant_state = pipeline_state.get_specialization_constant_state().get_specialization_constant_state();

	for (const auto specialization_constant : specialization_constant_state)
	{
		map_entries.push_back({specialization_constant.first, to_u32(data.size()), specialization_constant.second.size()});
		data.insert(data.end(), specialization_constant.second.begin(), specialization_constant.second.end());
	}

	VkSpecializationInfo specialization_info{};
	specialization_info.mapEntryCount = to_u32(map_entries.size());
	specialization_info.pMapEntries   = map_entries.data();
	specialization_info.dataSize      = data.size();
	specialization_info.pData         = data.data();

	for (const ShaderModule *shader_module : pipeline_state.get_pipeline_layout().get_shader_modules())
	{
		VkPipelineShaderStageCreateInfo stage_create_info{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};

		stage_create_info.stage = shader_module->get_stage();
		stage_create_info.pName = shader_module->get_entry_point().c_str();

		// Create the Vulkan handle
		VkShaderModuleCreateInfo vk_create_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};

		vk_create_info.codeSize = shader_module->get_binary().size() * sizeof(uint32_t);
		vk_create_info.pCode    = shader_module->get_binary().data();

		VkResult result = vkCreateShaderModule(device.get_handle(), &vk_create_info, nullptr, &stage_create_info.module);
		if (result != VK_SUCCESS)
		{
			throw VulkanException{result};
		}

		device.get_debug_utils().set_debug_name(device.get_handle(),
		                                        VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<uint64_t>(stage_create_info.module),
		                                        shader_module->get_debug_name().c_str());

		stage_create_info.pSpecializationInfo = &specialization_info;

		stage_create_infos.push_back(stage_create_info);
		shader_modules.push_back(stage_create_info.module);
	}

	VkGraphicsPipelineCreateInfo create_info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};

	create_info.stageCount = to_u32(stage_create_infos.size());
	create_info.pStages    = stage_create_infos.data();

	VkPipelineVertexInputStateCreateInfo vertex_input_state{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

	vertex_input_state.pVertexAttributeDescriptions    = pipeline_state.get_vertex_input_state().attributes.data();
	vertex_input_state.vertexAttributeDescriptionCount = to_u32(pipeline_state.get_vertex_input_state().attributes.size());

	vertex_input_state.pVertexBindingDescriptions    = pipeline_state.get_vertex_input_state().bindings.data();
	vertex_input_state.vertexBindingDescriptionCount = to_u32(pipeline_state.get_vertex_input_state().bindings.size());

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};

	input_assembly_state.topology               = pipeline_state.get_input_assembly_state().topology;
	input_assembly_state.primitiveRestartEnable = pipeline_state.get_input_assembly_state().primitive_restart_enable;

	VkPipelineViewportStateCreateInfo viewport_state{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};

	viewport_state.viewportCount = pipeline_state.get_viewport_state().viewport_count;
	viewport_state.scissorCount  = pipeline_state.get_viewport_state().scissor_count;

	VkPipelineRasterizationStateCreateInfo rasterization_state{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};

	rasterization_state.depthClampEnable        = pipeline_state.get_rasterization_state().depth_clamp_enable;
	rasterization_state.rasterizerDiscardEnable = pipeline_state.get_rasterization_state().rasterizer_discard_enable;
	rasterization_state.polygonMode             = pipeline_state.get_rasterization_state().polygon_mode;
	rasterization_state.cullMode                = pipeline_state.get_rasterization_state().cull_mode;
	rasterization_state.frontFace               = pipeline_state.get_rasterization_state().front_face;
	rasterization_state.depthBiasEnable         = pipeline_state.get_rasterization_state().depth_bias_enable;
	rasterization_state.depthBiasClamp          = 1.0f;
	rasterization_state.depthBiasSlopeFactor    = 1.0f;
	rasterization_state.lineWidth               = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisample_state{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};

	multisample_state.sampleShadingEnable   = pipeline_state.get_multisample_state().sample_shading_enable;
	multisample_state.rasterizationSamples  = pipeline_state.get_multisample_state().rasterization_samples;
	multisample_state.minSampleShading      = pipeline_state.get_multisample_state().min_sample_shading;
	multisample_state.alphaToCoverageEnable = pipeline_state.get_multisample_state().alpha_to_coverage_enable;
	multisample_state.alphaToOneEnable      = pipeline_state.get_multisample_state().alpha_to_one_enable;

	if (pipeline_state.get_multisample_state().sample_mask)
	{
		multisample_state.pSampleMask = &pipeline_state.get_multisample_state().sample_mask;
	}

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};

	depth_stencil_state.depthTestEnable       = pipeline_state.get_depth_stencil_state().depth_test_enable;
	depth_stencil_state.depthWriteEnable      = pipeline_state.get_depth_stencil_state().depth_write_enable;
	depth_stencil_state.depthCompareOp        = pipeline_state.get_depth_stencil_state().depth_compare_op;
	depth_stencil_state.depthBoundsTestEnable = pipeline_state.get_depth_stencil_state().depth_bounds_test_enable;
	depth_stencil_state.stencilTestEnable     = pipeline_state.get_depth_stencil_state().stencil_test_enable;
	depth_stencil_state.front.failOp          = pipeline_state.get_depth_stencil_state().front.fail_op;
	depth_stencil_state.front.passOp          = pipeline_state.get_depth_stencil_state().front.pass_op;
	depth_stencil_state.front.depthFailOp     = pipeline_state.get_depth_stencil_state().front.depth_fail_op;
	depth_stencil_state.front.compareOp       = pipeline_state.get_depth_stencil_state().front.compare_op;
	depth_stencil_state.front.compareMask     = ~0U;
	depth_stencil_state.front.writeMask       = ~0U;
	depth_stencil_state.front.reference       = ~0U;
	depth_stencil_state.back.failOp           = pipeline_state.get_depth_stencil_state().back.fail_op;
	depth_stencil_state.back.passOp           = pipeline_state.get_depth_stencil_state().back.pass_op;
	depth_stencil_state.back.depthFailOp      = pipeline_state.get_depth_stencil_state().back.depth_fail_op;
	depth_stencil_state.back.compareOp        = pipeline_state.get_depth_stencil_state().back.compare_op;
	depth_stencil_state.back.compareMask      = ~0U;
	depth_stencil_state.back.writeMask        = ~0U;
	depth_stencil_state.back.reference        = ~0U;

	VkPipelineColorBlendStateCreateInfo color_blend_state{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};

	color_blend_state.logicOpEnable     = pipeline_state.get_color_blend_state().logic_op_enable;
	color_blend_state.logicOp           = pipeline_state.get_color_blend_state().logic_op;
	color_blend_state.attachmentCount   = to_u32(pipeline_state.get_color_blend_state().attachments.size());
	color_blend_state.pAttachments      = reinterpret_cast<const VkPipelineColorBlendAttachmentState *>(pipeline_state.get_color_blend_state().attachments.data());
	color_blend_state.blendConstants[0] = 1.0f;
	color_blend_state.blendConstants[1] = 1.0f;
	color_blend_state.blendConstants[2] = 1.0f;
	color_blend_state.blendConstants[3] = 1.0f;

	std::array<VkDynamicState, 9> dynamic_states{
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR,
	    VK_DYNAMIC_STATE_LINE_WIDTH,
	    VK_DYNAMIC_STATE_DEPTH_BIAS,
	    VK_DYNAMIC_STATE_BLEND_CONSTANTS,
	    VK_DYNAMIC_STATE_DEPTH_BOUNDS,
	    VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
	    VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
	    VK_DYNAMIC_STATE_STENCIL_REFERENCE,
	};

	VkPipelineDynamicStateCreateInfo dynamic_state{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};

	dynamic_state.pDynamicStates    = dynamic_states.data();
	dynamic_state.dynamicStateCount = to_u32(dynamic_states.size());

	create_info.pVertexInputState   = &vertex_input_state;
	create_info.pInputAssemblyState = &input_assembly_state;
	create_info.pViewportState      = &viewport_state;
	create_info.pRasterizationState = &rasterization_state;
	create_info.pMultisampleState   = &multisample_state;
	create_info.pDepthStencilState  = &depth_stencil_state;
	create_info.pColorBlendState    = &color_blend_state;
	create_info.pDynamicState       = &dynamic_state;

	create_info.layout     = pipeline_state.get_pipeline_layout().get_handle();
	create_info.renderPass = pipeline_state.get_render_pass()->get_handle();
	create_info.subpass    = pipeline_state.get_subpass_index();

	auto result = vkCreateGraphicsPipelines(device.get_handle(), pipeline_cache, 1, &create_info, nullptr, &handle);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create GraphicsPipelines"};
	}

	for (auto shader_module : shader_modules)
	{
		vkDestroyShaderModule(device.get_handle(), shader_module, nullptr);
	}

	state = pipeline_state;
}
}        // namespace vkb
