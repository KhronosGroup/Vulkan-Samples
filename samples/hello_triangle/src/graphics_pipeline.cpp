/* Copyright (c) 2022, Arm Limited and Contributors
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

#include "graphics_pipeline.hpp"

#include <components/common/error.hpp>

VKBP_DISABLE_WARNINGS()
#include <glslang/Public/ShaderLang.h>

#include <SPIRV/GLSL.std.450.h>
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/ResourceLimits.h>
#include <glslang/Include/ShHandle.h>
#include <glslang/OSDependent/osinclude.h>
VKBP_ENABLE_WARNINGS()

#include <array>

void init_render_pass(Context &context)
{
	VkAttachmentDescription attachment{};
	attachment.flags = 0;
	// Backbuffer format.
	attachment.format = context.swapchain_dimensions.format;
	// Not multisampled.
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// When starting the frame, we want tiles to be cleared.
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// When ending the frame, we want tiles to be written out.
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	// Don't care about stencil since we're not using it.
	attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// The image layout will be undefined when the render pass begins.
	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// After the render pass is complete, we will transition to PRESENT_SRC_KHR layout.
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// We have one subpass. This subpass has one color attachment.
	// While executing this subpass, the attachment will be in attachment optimal layout.
	VkAttachmentReference color_ref = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

	// We will end up with two transitions.
	// The first one happens right before we start subpass #0, where
	// UNDEFINED is transitioned into COLOR_ATTACHMENT_OPTIMAL.
	// The final layout in the render pass attachment states PRESENT_SRC_KHR, so we
	// will get a final transition from COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR.
	VkSubpassDescription subpass{};
	subpass.flags                = 0;
	subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments    = &color_ref;

	// Create a dependency to external events.
	// We need to wait for the WSI semaphore to signal.
	// Only pipeline stages which depend on COLOR_ATTACHMENT_OUTPUT_BIT will
	// actually wait for the semaphore, so we must also wait for that pipeline stage.
	VkSubpassDependency dependency{};
	dependency.dependencyFlags = 0;
	dependency.srcSubpass      = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass      = 0;
	dependency.srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	// Since we changed the image layout, we need to make the memory visible to
	// color attachment to modify.
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// Finally, create the renderpass.
	VkRenderPassCreateInfo rp_info{};
	rp_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rp_info.attachmentCount = 1;
	rp_info.pAttachments    = &attachment;
	rp_info.subpassCount    = 1;
	rp_info.pSubpasses      = &subpass;
	rp_info.dependencyCount = 1;
	rp_info.pDependencies   = &dependency;

	VK_CHECK(vkCreateRenderPass(context.device, &rp_info, nullptr, &context.render_pass));
}

void init_pipeline(Context &context, const std::vector<uint8_t> &vertex_shader, const std::vector<uint8_t> &fragment_shader)
{
	// Create a blank pipeline layout.
	// We are not binding any resources to the pipeline in this first sample.
	VkPipelineLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	VK_CHECK(vkCreatePipelineLayout(context.device, &layout_info, nullptr, &context.pipeline_layout));

	VkPipelineVertexInputStateCreateInfo vertex_input{};
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	// Specify we will use triangle lists to draw geometry.
	VkPipelineInputAssemblyStateCreateInfo input_assembly{};
	input_assembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Specify rasterization state.
	VkPipelineRasterizationStateCreateInfo raster{};
	raster.sType     = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raster.cullMode  = VK_CULL_MODE_BACK_BIT;
	raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
	raster.lineWidth = 1.0f;

	// Our attachment will write to all color channels, but no blending is enabled.
	VkPipelineColorBlendAttachmentState blend_attachment{};
	blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo blend{};
	blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend.attachmentCount = 1;
	blend.pAttachments    = &blend_attachment;

	// We will have one viewport and scissor box.
	VkPipelineViewportStateCreateInfo viewport{};
	viewport.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport.viewportCount = 1;
	viewport.scissorCount  = 1;

	// Disable all depth testing.
	VkPipelineDepthStencilStateCreateInfo depth_stencil{};
	depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

	// No multisampling.
	VkPipelineMultisampleStateCreateInfo multisample{};
	multisample.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Specify that these states will be dynamic, i.e. not part of pipeline state object.
	std::array<VkDynamicState, 2> dynamics{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamic{};
	dynamic.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic.pDynamicStates    = dynamics.data();
	dynamic.dynamicStateCount = static_cast<uint32_t>(dynamics.size());

	// Load our SPIR-V shaders.
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};

	// Vertex stage of the pipeline
	shader_stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
	shader_stages[0].module = load_shader_module(context, VK_SHADER_STAGE_VERTEX_BIT, vertex_shader);
	shader_stages[0].pName  = "main";

	// Fragment stage of the pipeline
	shader_stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_stages[1].module = load_shader_module(context, VK_SHADER_STAGE_FRAGMENT_BIT, fragment_shader);
	shader_stages[1].pName  = "main";

	VkGraphicsPipelineCreateInfo pipe{};
	pipe.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipe.stageCount          = static_cast<uint32_t>(shader_stages.size());
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
	pipe.renderPass = context.render_pass;
	pipe.layout     = context.pipeline_layout;

	VK_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipe, nullptr, &context.pipeline));

	// Pipeline is baked, we can delete the shader modules now.
	vkDestroyShaderModule(context.device, shader_stages[0].module, nullptr);
	vkDestroyShaderModule(context.device, shader_stages[1].module, nullptr);
}

inline EShLanguage find_shader_language(VkShaderStageFlagBits stage)
{
	switch (stage)
	{
		case VK_SHADER_STAGE_VERTEX_BIT:
			return EShLangVertex;

		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
			return EShLangTessControl;

		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
			return EShLangTessEvaluation;

		case VK_SHADER_STAGE_GEOMETRY_BIT:
			return EShLangGeometry;

		case VK_SHADER_STAGE_FRAGMENT_BIT:
			return EShLangFragment;

		case VK_SHADER_STAGE_COMPUTE_BIT:
			return EShLangCompute;

		case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
			return EShLangRayGen;

		case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
			return EShLangAnyHit;

		case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
			return EShLangClosestHit;

		case VK_SHADER_STAGE_MISS_BIT_KHR:
			return EShLangMiss;

		case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
			return EShLangIntersect;

		case VK_SHADER_STAGE_CALLABLE_BIT_KHR:
			return EShLangCallable;

		default:
			return EShLangVertex;
	}
}

VkShaderModule load_shader_module(Context &context, VkShaderStageFlagBits stage, const std::vector<uint8_t> &glsl_source)
{
	glslang::InitializeProcess();
	EShMessages messages = static_cast<EShMessages>(EShMsgDefault | EShMsgVulkanRules | EShMsgSpvRules);

	EShLanguage language = find_shader_language(stage);
	std::string source   = std::string(glsl_source.begin(), glsl_source.end());

	const char *file_name_list[1] = {""};
	const char *shader_source     = reinterpret_cast<const char *>(source.data());

	glslang::TShader shader(language);
	shader.setStringsWithLengthsAndNames(&shader_source, nullptr, file_name_list, 1);
	shader.setEntryPoint("main");
	shader.setSourceEntryPoint("main");

	std::string info_log;

	if (!shader.parse(&glslang::DefaultTBuiltInResource, 100, false, messages))
	{
		info_log = std::string(shader.getInfoLog()) + "\n" + std::string(shader.getInfoDebugLog());
		return VK_NULL_HANDLE;
	}

	// Add shader to new program object.
	glslang::TProgram program;
	program.addShader(&shader);

	// Link program.
	if (!program.link(messages))
	{
		info_log = std::string(program.getInfoLog()) + "\n" + std::string(program.getInfoDebugLog());
		return VK_NULL_HANDLE;
	}

	// Save any info log that was generated.
	if (shader.getInfoLog())
	{
		info_log += std::string(shader.getInfoLog()) + "\n" + std::string(shader.getInfoDebugLog()) + "\n";
	}

	if (program.getInfoLog())
	{
		info_log += std::string(program.getInfoLog()) + "\n" + std::string(program.getInfoDebugLog());
	}

	glslang::TIntermediate *intermediate = program.getIntermediate(language);

	// Translate to SPIRV.
	if (!intermediate)
	{
		info_log += "Failed to get shared intermediate code.\n";
		return VK_NULL_HANDLE;
	}

	spv::SpvBuildLogger logger;

	std::vector<uint32_t> spirv;
	glslang::GlslangToSpv(*intermediate, spirv, &logger);

	info_log += logger.getAllMessages() + "\n";

	glslang::FinalizeProcess();

	if (info_log.size() > 0)
	{
		LOGI(info_log);
	}

	VkShaderModuleCreateInfo module_info{};
	module_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	module_info.codeSize = spirv.size() * sizeof(uint32_t);
	module_info.pCode    = spirv.data();

	VkShaderModule shader_module;
	VK_CHECK(vkCreateShaderModule(context.device, &module_info, nullptr, &shader_module));

	return shader_module;
}