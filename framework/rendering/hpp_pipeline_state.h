/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#pragma once

#include "rendering/pipeline_state.h"
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace core
{
class HPPPipelineLayout;
class HPPRenderPass;
}        // namespace core

namespace rendering
{
struct HPPColorBlendAttachmentState
{
	vk::Bool32              blend_enable           = false;
	vk::BlendFactor         src_color_blend_factor = vk::BlendFactor::eOne;
	vk::BlendFactor         dst_color_blend_factor = vk::BlendFactor::eZero;
	vk::BlendOp             color_blend_op         = vk::BlendOp::eAdd;
	vk::BlendFactor         src_alpha_blend_factor = vk::BlendFactor::eOne;
	vk::BlendFactor         dst_alpha_blend_factor = vk::BlendFactor::eZero;
	vk::BlendOp             alpha_blend_op         = vk::BlendOp::eAdd;
	vk::ColorComponentFlags color_write_mask       = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
};

struct HPPColorBlendState
{
	vk::Bool32                                logic_op_enable = false;
	vk::LogicOp                               logic_op        = vk::LogicOp::eClear;
	std::vector<HPPColorBlendAttachmentState> attachments;
};

struct HPPDepthStencilState
{
	vk::Bool32     depth_test_enable        = true;
	vk::Bool32     depth_write_enable       = true;
	vk::CompareOp  depth_compare_op         = vk::CompareOp::eGreater;        // Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	vk::Bool32     depth_bounds_test_enable = false;
	vk::Bool32     stencil_test_enable      = false;
	StencilOpState front;
	StencilOpState back;
};

struct HPPInputAssemblyState
{
	vk::PrimitiveTopology topology                 = vk::PrimitiveTopology::eTriangleList;
	vk::Bool32            primitive_restart_enable = false;
};

struct HPPMultisampleState
{
	vk::SampleCountFlagBits rasterization_samples    = vk::SampleCountFlagBits::e1;
	vk::Bool32              sample_shading_enable    = false;
	float                   min_sample_shading       = 0.0f;
	vk::SampleMask          sample_mask              = 0;
	vk::Bool32              alpha_to_coverage_enable = false;
	vk::Bool32              alpha_to_one_enable      = false;
};

struct HPPRasterizationState
{
	vk::Bool32        depth_clamp_enable        = false;
	vk::Bool32        rasterizer_discard_enable = false;
	vk::PolygonMode   polygon_mode              = vk::PolygonMode::eFill;
	vk::CullModeFlags cull_mode                 = vk::CullModeFlagBits::eBack;
	vk::FrontFace     front_face                = vk::FrontFace::eCounterClockwise;
	vk::Bool32        depth_bias_enable         = false;
};

class HPPSpecializationConstantState : private vkb::SpecializationConstantState
{};

struct HPPStencilOpState
{
	vk::StencilOp fail_op       = vk::StencilOp::eReplace;
	vk::StencilOp pass_op       = vk::StencilOp::eReplace;
	vk::StencilOp depth_fail_op = vk::StencilOp::eReplace;
	vk::CompareOp compare_op    = vk::CompareOp::eNever;
};

struct HPPVertexInputState
{
	std::vector<vk::VertexInputBindingDescription>   bindings;
	std::vector<vk::VertexInputAttributeDescription> attributes;
};

struct HPPViewportState
{
	uint32_t viewport_count = 1;
	uint32_t scissor_count  = 1;
};

class HPPPipelineState : private vkb::PipelineState
{
  public:
	using vkb::PipelineState::clear_dirty;
	using vkb::PipelineState::get_subpass_index;
	using vkb::PipelineState::is_dirty;
	using vkb::PipelineState::reset;
	using vkb::PipelineState::set_specialization_constant;
	using vkb::PipelineState::set_subpass_index;

  public:
	const vkb::rendering::HPPColorBlendState &get_color_blend_state() const
	{
		return reinterpret_cast<vkb::rendering::HPPColorBlendState const &>(vkb::PipelineState::get_color_blend_state());
	}

	const vkb::core::HPPPipelineLayout &get_pipeline_layout() const
	{
		return reinterpret_cast<vkb::core::HPPPipelineLayout const &>(vkb::PipelineState::get_pipeline_layout());
	}

	const vkb::core::HPPRenderPass *get_render_pass() const
	{
		return reinterpret_cast<vkb::core::HPPRenderPass const *>(vkb::PipelineState::get_render_pass());
	}

	const vkb::rendering::HPPSpecializationConstantState &get_specialization_constant_state() const
	{
		return reinterpret_cast<vkb::rendering::HPPSpecializationConstantState const &>(vkb::PipelineState::get_specialization_constant_state());
	}

	void set_color_blend_state(const vkb::rendering::HPPColorBlendState &color_blend_state)
	{
		vkb::PipelineState::set_color_blend_state(reinterpret_cast<vkb::ColorBlendState const &>(color_blend_state));
	}

	void set_depth_stencil_state(const vkb::rendering::HPPDepthStencilState &depth_stencil_state)
	{
		vkb::PipelineState::set_depth_stencil_state(reinterpret_cast<vkb::DepthStencilState const &>(depth_stencil_state));
	}

	void set_input_assembly_state(const vkb::rendering::HPPInputAssemblyState &input_assembly_state)
	{
		vkb::PipelineState::set_input_assembly_state(reinterpret_cast<vkb::InputAssemblyState const &>(input_assembly_state));
	}

	void set_multisample_state(const vkb::rendering::HPPMultisampleState &multisample_state)
	{
		vkb::PipelineState::set_multisample_state(reinterpret_cast<vkb::MultisampleState const &>(multisample_state));
	}

	void set_pipeline_layout(vkb::core::HPPPipelineLayout &pipeline_layout)
	{
		vkb::PipelineState::set_pipeline_layout(reinterpret_cast<vkb::PipelineLayout &>(pipeline_layout));
	}

	void set_rasterization_state(const vkb::rendering::HPPRasterizationState &rasterization_state)
	{
		vkb::PipelineState::set_rasterization_state(reinterpret_cast<vkb::RasterizationState const &>(rasterization_state));
	}

	void set_render_pass(const vkb::core::HPPRenderPass &render_pass)
	{
		vkb::PipelineState::set_render_pass(reinterpret_cast<vkb::RenderPass const &>(render_pass));
	}

	void set_vertex_input_state(const vkb::rendering::HPPVertexInputState &vertex_input_state)
	{
		vkb::PipelineState::set_vertex_input_state(reinterpret_cast<vkb::VertexInputState const &>(vertex_input_state));
	}

	void set_viewport_state(const vkb::rendering::HPPViewportState &viewport_state)
	{
		vkb::PipelineState::set_viewport_state(reinterpret_cast<vkb::ViewportState const &>(viewport_state));
	}
};

}        // namespace rendering
}        // namespace vkb
