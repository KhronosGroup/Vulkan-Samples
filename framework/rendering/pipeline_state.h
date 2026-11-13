/* Copyright (c) 2019-2026, Arm Limited and Contributors
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

#include "core/hpp_pipeline_layout.h"
#include "core/hpp_render_pass.h"
#include "core/pipeline_layout.h"
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace rendering
{
template <BindingType bindingType>
struct ColorBlendAttachmentState
{
	using BlendFactorType         = typename std::conditional<bindingType == BindingType::Cpp, vk::BlendFactor, VkBlendFactor>::type;
	using BlendOpType             = typename std::conditional<bindingType == BindingType::Cpp, vk::BlendOp, VkBlendOp>::type;
	using Bool32Type              = typename std::conditional<bindingType == BindingType::Cpp, vk::Bool32, VkBool32>::type;
	using ColorComponentFlagsType = typename std::conditional<bindingType == BindingType::Cpp, vk::ColorComponentFlags, VkColorComponentFlags>::type;

  private:
	constexpr BlendOpType default_blend_op() const
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			return vk::BlendOp::eAdd;
		}
		else
		{
			return VK_BLEND_OP_ADD;
		}
	}

	constexpr BlendFactorType default_dst_blend_factor() const
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			return vk::BlendFactor::eZero;
		}
		else
		{
			return VK_BLEND_FACTOR_ZERO;
		}
	}

	constexpr BlendFactorType default_src_blend_factor() const
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			return vk::BlendFactor::eOne;
		}
		else
		{
			return VK_BLEND_FACTOR_ONE;
		}
	}

	constexpr ColorComponentFlagsType default_write_mask() const
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			return vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		}
		else
		{
			return VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		}
	}

  public:
	Bool32Type              blend_enable           = false;
	BlendFactorType         src_color_blend_factor = default_src_blend_factor();
	BlendFactorType         dst_color_blend_factor = default_dst_blend_factor();
	BlendOpType             color_blend_op         = default_blend_op();
	BlendFactorType         src_alpha_blend_factor = default_src_blend_factor();
	BlendFactorType         dst_alpha_blend_factor = default_dst_blend_factor();
	BlendOpType             alpha_blend_op         = default_blend_op();
	ColorComponentFlagsType color_write_mask       = default_write_mask();
};

using ColorBlendAttachmentStateC   = ColorBlendAttachmentState<vkb::BindingType::C>;
using ColorBlendAttachmentStateCpp = ColorBlendAttachmentState<vkb::BindingType::Cpp>;

template <BindingType bindingType>
inline bool operator==(vkb::rendering::ColorBlendAttachmentState<bindingType> const &lhs, vkb::rendering::ColorBlendAttachmentState<bindingType> const &rhs)
{
	return lhs.blend_enable == rhs.blend_enable && lhs.src_color_blend_factor == rhs.src_color_blend_factor && lhs.dst_color_blend_factor == rhs.dst_color_blend_factor &&
	       lhs.color_blend_op == rhs.color_blend_op && lhs.src_alpha_blend_factor == rhs.src_alpha_blend_factor && lhs.dst_alpha_blend_factor == rhs.dst_alpha_blend_factor &&
	       lhs.alpha_blend_op == rhs.alpha_blend_op && lhs.color_write_mask == rhs.color_write_mask;
}

//==============================================================================

template <BindingType bindingType>
struct ColorBlendState
{
	using Bool32Type  = typename std::conditional<bindingType == BindingType::Cpp, vk::Bool32, VkBool32>::type;
	using LogicOpType = typename std::conditional<bindingType == BindingType::Cpp, vk::LogicOp, VkLogicOp>::type;

  private:
	constexpr LogicOpType default_logic_op() const
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			return vk::LogicOp::eClear;
		}
		else
		{
			return VK_LOGIC_OP_CLEAR;
		}
	}

  public:
	Bool32Type                                                          logic_op_enable = false;
	LogicOpType                                                         logic_op        = default_logic_op();
	std::vector<vkb::rendering::ColorBlendAttachmentState<bindingType>> attachments     = {};
};

using ColorBlendStateC   = ColorBlendState<vkb::BindingType::C>;
using ColorBlendStateCpp = ColorBlendState<vkb::BindingType::Cpp>;

template <BindingType bindingType>
inline bool operator!=(vkb::rendering::ColorBlendState<bindingType> const &lhs, vkb::rendering::ColorBlendState<bindingType> const &rhs)
{
	return lhs.logic_op_enable != rhs.logic_op_enable || lhs.logic_op != rhs.logic_op || lhs.attachments != rhs.attachments;
}

//==============================================================================

template <BindingType bindingType>
struct StencilOpState
{
	using CompareOpType = typename std::conditional<bindingType == BindingType::Cpp, vk::CompareOp, VkCompareOp>::type;
	using StencilOpType = typename std::conditional<bindingType == BindingType::Cpp, vk::StencilOp, VkStencilOp>::type;

  private:
	constexpr CompareOpType default_compare_op() const
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			return vk::CompareOp::eNever;
		}
		else
		{
			return VK_COMPARE_OP_NEVER;
		}
	}

	constexpr StencilOpType default_stencil_op() const
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			return vk::StencilOp::eReplace;
		}
		else
		{
			return VK_STENCIL_OP_REPLACE;
		}
	}

  public:
	StencilOpType fail_op       = default_stencil_op();
	StencilOpType pass_op       = default_stencil_op();
	StencilOpType depth_fail_op = default_stencil_op();
	CompareOpType compare_op    = default_compare_op();
};

using StencilOpStateC   = StencilOpState<vkb::BindingType::C>;
using StencilOpStateCpp = StencilOpState<vkb::BindingType::Cpp>;

template <BindingType bindingType>
inline bool operator!=(vkb::rendering::StencilOpState<bindingType> const &lhs, vkb::rendering::StencilOpState<bindingType> const &rhs)
{
	return lhs.fail_op != rhs.fail_op || lhs.pass_op != rhs.pass_op || lhs.depth_fail_op != rhs.depth_fail_op || lhs.compare_op != rhs.compare_op;
}

//==============================================================================

template <BindingType bindingType>
struct DepthStencilState
{
	using Bool32Type    = typename std::conditional<bindingType == BindingType::Cpp, vk::Bool32, VkBool32>::type;
	using CompareOpType = typename std::conditional<bindingType == BindingType::Cpp, vk::CompareOp, VkCompareOp>::type;

  private:
	constexpr CompareOpType default_depth_compare_op() const
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			return vk::CompareOp::eGreater;
		}
		else
		{
			return VK_COMPARE_OP_GREATER;
		}
	}

  public:
	Bool32Type                                  depth_test_enable        = true;
	Bool32Type                                  depth_write_enable       = true;
	CompareOpType                               depth_compare_op         = default_depth_compare_op();        // Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	Bool32Type                                  depth_bounds_test_enable = false;
	Bool32Type                                  stencil_test_enable      = false;
	vkb::rendering::StencilOpState<bindingType> front{};
	vkb::rendering::StencilOpState<bindingType> back{};
};

using DepthStencilStateC   = DepthStencilState<vkb::BindingType::C>;
using DepthStencilStateCpp = DepthStencilState<vkb::BindingType::Cpp>;

template <BindingType bindingType>
bool operator!=(vkb::rendering::DepthStencilState<bindingType> const &lhs, vkb::rendering::DepthStencilState<bindingType> const &rhs)
{
	return lhs.depth_test_enable != rhs.depth_test_enable || lhs.depth_write_enable != rhs.depth_write_enable || lhs.depth_compare_op != rhs.depth_compare_op ||
	       lhs.depth_bounds_test_enable != rhs.depth_bounds_test_enable || lhs.stencil_test_enable != rhs.stencil_test_enable || lhs.front != rhs.front || lhs.back != rhs.back;
}

//==============================================================================

template <BindingType bindingType>
struct InputAssemblyState
{
	using Bool32Type            = typename std::conditional<bindingType == BindingType::Cpp, vk::Bool32, VkBool32>::type;
	using PrimitiveTopologyType = typename std::conditional<bindingType == BindingType::Cpp, vk::PrimitiveTopology, VkPrimitiveTopology>::type;

  private:
	constexpr PrimitiveTopologyType default_topology() const
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			return vk::PrimitiveTopology::eTriangleList;
		}
		else
		{
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		}
	}

  public:
	PrimitiveTopologyType topology                 = default_topology();
	Bool32Type            primitive_restart_enable = false;
};

using InputAssemblyStateC   = InputAssemblyState<vkb::BindingType::C>;
using InputAssemblyStateCpp = InputAssemblyState<vkb::BindingType::Cpp>;

template <BindingType bindingType>
inline bool operator!=(vkb::rendering::InputAssemblyState<bindingType> const &lhs, vkb::rendering::InputAssemblyState<bindingType> const &rhs)
{
	return lhs.topology != rhs.topology || lhs.primitive_restart_enable != rhs.primitive_restart_enable;
}

//==============================================================================

template <BindingType bindingType>
struct MultisampleState
{
	using Bool32Type              = typename std::conditional<bindingType == BindingType::Cpp, vk::Bool32, VkBool32>::type;
	using SampleCountFlagBitsType = typename std::conditional<bindingType == BindingType::Cpp, vk::SampleCountFlagBits, VkSampleCountFlagBits>::type;
	using SampleMaskType          = typename std::conditional<bindingType == BindingType::Cpp, vk::SampleMask, VkSampleMask>::type;

  private:
	constexpr SampleCountFlagBitsType default_rasterization_samples() const
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			return vk::SampleCountFlagBits::e1;
		}
		else
		{
			return VK_SAMPLE_COUNT_1_BIT;
		}
	}

	constexpr SampleMaskType default_sample_mask() const
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			return {};
		}
		else
		{
			return 0;
		}
	}

  public:
	SampleCountFlagBitsType rasterization_samples    = default_rasterization_samples();
	Bool32Type              sample_shading_enable    = false;
	float                   min_sample_shading       = 0.0f;
	SampleMaskType          sample_mask              = default_sample_mask();
	Bool32Type              alpha_to_coverage_enable = false;
	Bool32Type              alpha_to_one_enable      = false;
};

using MultisampleStateC   = MultisampleState<vkb::BindingType::C>;
using MultisampleStateCpp = MultisampleState<vkb::BindingType::Cpp>;

template <BindingType bindingType>
inline bool operator!=(vkb::rendering::MultisampleState<bindingType> const &lhs, vkb::rendering::MultisampleState<bindingType> const &rhs)
{
	return lhs.rasterization_samples != rhs.rasterization_samples || lhs.sample_shading_enable != rhs.sample_shading_enable || lhs.min_sample_shading != rhs.min_sample_shading ||
	       lhs.sample_mask != rhs.sample_mask || lhs.alpha_to_coverage_enable != rhs.alpha_to_coverage_enable || lhs.alpha_to_one_enable != rhs.alpha_to_one_enable;
}

//==============================================================================

template <BindingType bindingType>
struct RasterizationState
{
	using Bool32Type        = typename std::conditional<bindingType == BindingType::Cpp, vk::Bool32, VkBool32>::type;
	using CullModeFlagsType = typename std::conditional<bindingType == BindingType::Cpp, vk::CullModeFlags, VkCullModeFlags>::type;
	using FrontFaceType     = typename std::conditional<bindingType == BindingType::Cpp, vk::FrontFace, VkFrontFace>::type;
	using PolygonModeType   = typename std::conditional<bindingType == BindingType::Cpp, vk::PolygonMode, VkPolygonMode>::type;

  private:
	constexpr CullModeFlagsType default_cull_mode() const
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			return vk::CullModeFlagBits::eBack;
		}
		else
		{
			return VK_CULL_MODE_BACK_BIT;
		}
	}

	constexpr FrontFaceType default_front_face() const
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			return vk::FrontFace::eCounterClockwise;
		}
		else
		{
			return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		}
	}

	constexpr PolygonModeType default_polygon_mode() const
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			return vk::PolygonMode::eFill;
		}
		else
		{
			return VK_POLYGON_MODE_FILL;
		}
	}

  public:
	Bool32Type        depth_clamp_enable        = false;
	Bool32Type        rasterizer_discard_enable = false;
	PolygonModeType   polygon_mode              = default_polygon_mode();
	CullModeFlagsType cull_mode                 = default_cull_mode();
	FrontFaceType     front_face                = default_front_face();
	Bool32Type        depth_bias_enable         = false;
};

using RasterizationStateC   = RasterizationState<vkb::BindingType::C>;
using RasterizationStateCpp = RasterizationState<vkb::BindingType::Cpp>;

template <BindingType bindingType>
inline bool operator!=(vkb::rendering::RasterizationState<bindingType> const &lhs, vkb::rendering::RasterizationState<bindingType> const &rhs)
{
	return lhs.depth_clamp_enable != rhs.depth_clamp_enable || lhs.rasterizer_discard_enable != rhs.rasterizer_discard_enable || lhs.polygon_mode != rhs.polygon_mode ||
	       lhs.cull_mode != rhs.cull_mode || lhs.front_face != rhs.front_face || lhs.depth_bias_enable != rhs.depth_bias_enable;
}

//==============================================================================

template <BindingType bindingType>
struct VertexInputState
{
	using VertexInputAttributeDescriptionType = typename std::conditional<bindingType == BindingType::Cpp, vk::VertexInputAttributeDescription, VkVertexInputAttributeDescription>::type;
	using VertexInputBindingDescriptionType   = typename std::conditional<bindingType == BindingType::Cpp, vk::VertexInputBindingDescription, VkVertexInputBindingDescription>::type;

	std::vector<VertexInputBindingDescriptionType>   bindings;
	std::vector<VertexInputAttributeDescriptionType> attributes;
};

using VertexInputStateC   = VertexInputState<vkb::BindingType::C>;
using VertexInputStateCpp = VertexInputState<vkb::BindingType::Cpp>;

template <BindingType bindingType>
inline bool operator!=(vkb::rendering::VertexInputState<bindingType> const &lhs, vkb::rendering::VertexInputState<bindingType> const &rhs)
{
	return lhs.bindings != rhs.bindings || lhs.attributes != rhs.attributes;
}

//==============================================================================

struct ViewportState
{
	uint32_t viewport_count = 1;
	uint32_t scissor_count  = 1;
};

inline bool operator!=(vkb::rendering::ViewportState const &lhs, vkb::rendering::ViewportState const &rhs)
{
	return lhs.viewport_count != rhs.viewport_count || lhs.scissor_count != rhs.scissor_count;
}

//==============================================================================

/// Helper class to create specialization constants for a Vulkan pipeline. The state tracks a pipeline globally, and not per shader. Two shaders using the same constant_id will have the same data.
class SpecializationConstantState
{
  public:
	void                                            clear_dirty();
	std::map<uint32_t, std::vector<uint8_t>> const &get_specialization_constant_state() const;
	bool                                            is_dirty() const;
	void                                            reset();
	template <class T>
	void set_constant(uint32_t constant_id, const T &data);
	void set_constant(uint32_t constant_id, const std::vector<uint8_t> &data);
	void set_specialization_constant_state(const std::map<uint32_t, std::vector<uint8_t>> &state);

  private:
	bool                                     dirty = false;
	std::map<uint32_t, std::vector<uint8_t>> specialization_constant_state;        // Map tracking state of the Specialization Constants
};

inline void SpecializationConstantState::clear_dirty()
{
	dirty = false;
}

inline std::map<uint32_t, std::vector<uint8_t>> const &SpecializationConstantState::get_specialization_constant_state() const
{
	return specialization_constant_state;
}

inline bool SpecializationConstantState::is_dirty() const
{
	return dirty;
}

inline void SpecializationConstantState::reset()
{
	if (dirty)
	{
		specialization_constant_state.clear();
	}

	dirty = false;
}

template <class T>
inline void SpecializationConstantState::set_constant(std::uint32_t constant_id, const T &data)
{
	set_constant(constant_id, to_bytes(static_cast<std::uint32_t>(data)));
}

template <>
inline void SpecializationConstantState::set_constant<bool>(std::uint32_t constant_id, const bool &data)
{
	set_constant(constant_id, to_bytes(static_cast<std::uint32_t>(data)));
}

inline void SpecializationConstantState::set_constant(uint32_t constant_id, const std::vector<uint8_t> &value)
{
	auto data = specialization_constant_state.find(constant_id);

	if (data != specialization_constant_state.end() && data->second == value)
	{
		return;
	}

	dirty = true;

	specialization_constant_state[constant_id] = value;
}

inline void SpecializationConstantState::set_specialization_constant_state(const std::map<uint32_t, std::vector<uint8_t>> &state)
{
	specialization_constant_state = state;
}

//==============================================================================

template <BindingType bindingType>
class PipelineState
{
	using PipelineLayoutType = typename std::conditional<bindingType == BindingType::Cpp, vkb::core::HPPPipelineLayout, vkb::PipelineLayout>::type;
	using RenderPassType     = typename std::conditional<bindingType == BindingType::Cpp, vkb::core::HPPRenderPass, vkb::RenderPass>::type;

  public:
	void                                                   clear_dirty();
	vkb::rendering::ColorBlendState<bindingType> const    &get_color_blend_state() const;
	vkb::rendering::DepthStencilState<bindingType> const  &get_depth_stencil_state() const;
	vkb::rendering::InputAssemblyState<bindingType> const &get_input_assembly_state() const;
	vkb::rendering::MultisampleState<bindingType> const   &get_multisample_state() const;
	PipelineLayoutType const                              &get_pipeline_layout() const;
	vkb::rendering::RasterizationState<bindingType> const &get_rasterization_state() const;
	RenderPassType const                                  *get_render_pass() const;
	vkb::rendering::SpecializationConstantState const     &get_specialization_constant_state() const;
	uint32_t                                               get_subpass_index() const;
	vkb::rendering::VertexInputState<bindingType> const   &get_vertex_input_state() const;
	vkb::rendering::ViewportState const                   &get_viewport_state() const;
	bool                                                   is_dirty() const;
	void                                                   reset();
	void                                                   set_color_blend_state(vkb::rendering::ColorBlendState<bindingType> const &color_blend_state);
	void                                                   set_depth_stencil_state(vkb::rendering::DepthStencilState<bindingType> const &depth_stencil_state);
	void                                                   set_input_assembly_state(vkb::rendering::InputAssemblyState<bindingType> const &input_assembly_state);
	void                                                   set_multisample_state(vkb::rendering::MultisampleState<bindingType> const &multisample_state);
	void                                                   set_pipeline_layout(PipelineLayoutType const &pipeline_layout);
	void                                                   set_rasterization_state(vkb::rendering::RasterizationState<bindingType> const &rasterization_state);
	void                                                   set_render_pass(RenderPassType const &render_pass);
	void                                                   set_specialization_constant(uint32_t constant_id, const std::vector<uint8_t> &data);
	void                                                   set_subpass_index(uint32_t subpass_index);
	void                                                   set_vertex_input_state(vkb::rendering::VertexInputState<bindingType> const &vertex_input_state);
	void                                                   set_viewport_state(vkb::rendering::ViewportState const &viewport_state);

  private:
	void set_color_blend_state_impl(vkb::rendering::ColorBlendStateCpp const &color_blend_state);
	void set_depth_stencil_state_impl(vkb::rendering::DepthStencilStateCpp const &depth_stencil_state);
	void set_input_assembly_state_impl(vkb::rendering::InputAssemblyStateCpp const &input_assembly_state);
	void set_multisample_state_impl(vkb::rendering::MultisampleStateCpp const &multisample_state);
	void set_pipeline_layout_impl(vkb::core::HPPPipelineLayout const &new_pipeline_layout);
	void set_render_pass_impl(vkb::core::HPPRenderPass const &new_render_pass);
	void set_rasterization_state_impl(vkb::rendering::RasterizationStateCpp const &rasterization_state);
	void set_vertex_input_state_impl(vkb::rendering::VertexInputStateCpp const &new_vertex_input_state);

  private:
	vkb::rendering::ColorBlendStateCpp          color_blend_state             = {};
	vkb::rendering::DepthStencilStateCpp        depth_stencil_state           = {};
	bool                                        dirty                         = false;
	vkb::rendering::InputAssemblyStateCpp       input_assembly_state          = {};
	vkb::rendering::MultisampleStateCpp         multisample_state             = {};
	vkb::core::HPPPipelineLayout const         *pipeline_layout               = nullptr;
	vkb::rendering::RasterizationStateCpp       rasterization_state           = {};
	vkb::core::HPPRenderPass const             *render_pass                   = nullptr;
	vkb::rendering::SpecializationConstantState specialization_constant_state = {};
	uint32_t                                    subpass_index                 = 0U;
	vkb::rendering::VertexInputStateCpp         vertex_input_state            = {};
	vkb::rendering::ViewportState               viewport_state                = {};
};

using PipelineStateC   = PipelineState<vkb::BindingType::C>;
using PipelineStateCpp = PipelineState<vkb::BindingType::Cpp>;

template <BindingType bindingType>
void PipelineState<bindingType>::clear_dirty()
{
	dirty = false;
	specialization_constant_state.clear_dirty();
}

template <BindingType bindingType>
inline vkb::rendering::ColorBlendState<bindingType> const &PipelineState<bindingType>::get_color_blend_state() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return color_blend_state;
	}
	else
	{
		return reinterpret_cast<vkb::rendering::ColorBlendStateC const &>(color_blend_state);
	}
}

template <BindingType bindingType>
inline vkb::rendering::DepthStencilState<bindingType> const &PipelineState<bindingType>::get_depth_stencil_state() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return depth_stencil_state;
	}
	else
	{
		return reinterpret_cast<vkb::rendering::DepthStencilStateC const &>(depth_stencil_state);
	}
}

template <BindingType bindingType>
inline vkb::rendering::InputAssemblyState<bindingType> const &PipelineState<bindingType>::get_input_assembly_state() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return input_assembly_state;
	}
	else
	{
		return reinterpret_cast<vkb::rendering::InputAssemblyStateC const &>(input_assembly_state);
	}
}

template <BindingType bindingType>
inline vkb::rendering::MultisampleState<bindingType> const &PipelineState<bindingType>::get_multisample_state() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return multisample_state;
	}
	else
	{
		return reinterpret_cast<vkb::rendering::MultisampleStateC const &>(multisample_state);
	}
}

template <BindingType bindingType>
inline typename PipelineState<bindingType>::PipelineLayoutType const &PipelineState<bindingType>::get_pipeline_layout() const
{
	assert(pipeline_layout && "Graphics state Pipeline layout is not set");
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *pipeline_layout;
	}
	else
	{
		return reinterpret_cast<vkb::PipelineLayout const &>(*pipeline_layout);
	}
}

template <BindingType bindingType>
inline vkb::rendering::RasterizationState<bindingType> const &PipelineState<bindingType>::get_rasterization_state() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return rasterization_state;
	}
	else
	{
		return reinterpret_cast<vkb::rendering::RasterizationStateC const &>(rasterization_state);
	}
}

template <BindingType bindingType>
inline typename PipelineState<bindingType>::RenderPassType const *PipelineState<bindingType>::get_render_pass() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return render_pass;
	}
	else
	{
		return reinterpret_cast<vkb::RenderPass const *>(render_pass);
	}
}

template <BindingType bindingType>
vkb::rendering::SpecializationConstantState const &PipelineState<bindingType>::get_specialization_constant_state() const
{
	return specialization_constant_state;
}

template <BindingType bindingType>
inline uint32_t PipelineState<bindingType>::get_subpass_index() const
{
	return subpass_index;
}

template <BindingType bindingType>
vkb::rendering::VertexInputState<bindingType> const &PipelineState<bindingType>::get_vertex_input_state() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return vertex_input_state;
	}
	else
	{
		return reinterpret_cast<vkb::rendering::VertexInputStateC const &>(vertex_input_state);
	}
}

template <BindingType bindingType>
inline vkb::rendering::ViewportState const &PipelineState<bindingType>::get_viewport_state() const
{
	return viewport_state;
}

template <BindingType bindingType>
inline bool PipelineState<bindingType>::is_dirty() const
{
	return dirty || specialization_constant_state.is_dirty();
}

template <BindingType bindingType>
inline void PipelineState<bindingType>::reset()
{
	clear_dirty();
	pipeline_layout = nullptr;
	render_pass     = nullptr;
	specialization_constant_state.reset();
	vertex_input_state   = {};
	input_assembly_state = {};
	rasterization_state  = {};
	multisample_state    = {};
	depth_stencil_state  = {};
	color_blend_state    = {};
	subpass_index        = {0U};
}

template <BindingType bindingType>
void PipelineState<bindingType>::set_color_blend_state(vkb::rendering::ColorBlendState<bindingType> const &new_color_blend_state)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		set_color_blend_state_impl(new_color_blend_state);
	}
	else
	{
		set_color_blend_state_impl(reinterpret_cast<vkb::rendering::ColorBlendStateCpp const &>(new_color_blend_state));
	}
}

template <BindingType bindingType>
void PipelineState<bindingType>::set_color_blend_state_impl(vkb::rendering::ColorBlendStateCpp const &new_color_blend_state)
{
	if (color_blend_state != new_color_blend_state)
	{
		color_blend_state = new_color_blend_state;
		dirty             = true;
	}
}

template <BindingType bindingType>
void PipelineState<bindingType>::set_depth_stencil_state(vkb::rendering::DepthStencilState<bindingType> const &new_depth_stencil_state)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		set_depth_stencil_state_impl(new_depth_stencil_state);
	}
	else
	{
		set_depth_stencil_state_impl(reinterpret_cast<vkb::rendering::DepthStencilStateCpp const &>(new_depth_stencil_state));
	}
}

template <BindingType bindingType>
void PipelineState<bindingType>::set_depth_stencil_state_impl(vkb::rendering::DepthStencilStateCpp const &new_depth_stencil_state)
{
	if (depth_stencil_state != new_depth_stencil_state)
	{
		depth_stencil_state = new_depth_stencil_state;
		dirty               = true;
	}
}

template <BindingType bindingType>
inline void PipelineState<bindingType>::set_input_assembly_state(vkb::rendering::InputAssemblyState<bindingType> const &new_input_assembly_state)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		set_input_assembly_state_impl(new_input_assembly_state);
	}
	else
	{
		set_input_assembly_state_impl(reinterpret_cast<vkb::rendering::InputAssemblyStateCpp const &>(new_input_assembly_state));
	}
}

template <BindingType bindingType>
inline void PipelineState<bindingType>::set_input_assembly_state_impl(vkb::rendering::InputAssemblyStateCpp const &new_input_assembly_state)
{
	if (input_assembly_state != new_input_assembly_state)
	{
		input_assembly_state = new_input_assembly_state;
		dirty                = true;
	}
}

template <BindingType bindingType>
inline void PipelineState<bindingType>::set_multisample_state(vkb::rendering::MultisampleState<bindingType> const &new_multisample_state)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		set_multisample_state_impl(new_multisample_state);
	}
	else
	{
		set_multisample_state_impl(reinterpret_cast<vkb::rendering::MultisampleStateCpp const &>(new_multisample_state));
	}
}

template <BindingType bindingType>
inline void PipelineState<bindingType>::set_multisample_state_impl(vkb::rendering::MultisampleStateCpp const &new_multisample_state)
{
	if (multisample_state != new_multisample_state)
	{
		multisample_state = new_multisample_state;
		dirty             = true;
	}
}

template <BindingType bindingType>
inline void PipelineState<bindingType>::set_pipeline_layout(PipelineLayoutType const &new_pipeline_layout)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		set_pipeline_layout_impl(new_pipeline_layout);
	}
	else
	{
		set_pipeline_layout_impl(reinterpret_cast<vkb::core::HPPPipelineLayout const &>(new_pipeline_layout));
	}
}

template <BindingType bindingType>
inline void PipelineState<bindingType>::set_pipeline_layout_impl(vkb::core::HPPPipelineLayout const &new_pipeline_layout)
{
	if (!pipeline_layout || pipeline_layout->get_handle() != new_pipeline_layout.get_handle())
	{
		pipeline_layout = &new_pipeline_layout;
		dirty           = true;
	}
}

template <BindingType bindingType>
inline void PipelineState<bindingType>::set_rasterization_state(vkb::rendering::RasterizationState<bindingType> const &new_rasterization_state)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		set_rasterization_state_impl(new_rasterization_state);
	}
	else
	{
		set_rasterization_state_impl(reinterpret_cast<vkb::rendering::RasterizationStateCpp const &>(new_rasterization_state));
	}
}

template <BindingType bindingType>
inline void PipelineState<bindingType>::set_rasterization_state_impl(vkb::rendering::RasterizationStateCpp const &new_rasterization_state)
{
	if (rasterization_state != new_rasterization_state)
	{
		rasterization_state = new_rasterization_state;
		dirty               = true;
	}
}

template <BindingType bindingType>
inline void PipelineState<bindingType>::set_render_pass(RenderPassType const &new_render_pass)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		set_render_pass_impl(new_render_pass);
	}
	else
	{
		set_render_pass_impl(reinterpret_cast<vkb::core::HPPRenderPass const &>(new_render_pass));
	}
}

template <BindingType bindingType>
inline void PipelineState<bindingType>::set_render_pass_impl(vkb::core::HPPRenderPass const &new_render_pass)
{
	if (!render_pass || render_pass->get_handle() != new_render_pass.get_handle())
	{
		render_pass = &new_render_pass;
		dirty       = true;
	}
}

template <BindingType bindingType>
inline void PipelineState<bindingType>::set_specialization_constant(uint32_t constant_id, std::vector<uint8_t> const &data)
{
	specialization_constant_state.set_constant(constant_id, data);

	if (specialization_constant_state.is_dirty())
	{
		dirty = true;
	}
}

template <BindingType bindingType>
inline void PipelineState<bindingType>::set_subpass_index(uint32_t new_subpass_index)
{
	if (subpass_index != new_subpass_index)
	{
		subpass_index = new_subpass_index;
		dirty         = true;
	}
}

template <BindingType bindingType>
inline void PipelineState<bindingType>::set_vertex_input_state(vkb::rendering::VertexInputState<bindingType> const &new_vertex_input_state)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		set_vertex_input_state_impl(new_vertex_input_state);
	}
	else
	{
		set_vertex_input_state_impl(reinterpret_cast<vkb::rendering::VertexInputStateCpp const &>(new_vertex_input_state));
	}
}

template <BindingType bindingType>
inline void PipelineState<bindingType>::set_vertex_input_state_impl(vkb::rendering::VertexInputStateCpp const &new_vertex_input_state)
{
	if (vertex_input_state != new_vertex_input_state)
	{
		vertex_input_state = new_vertex_input_state;
		dirty              = true;
	}
}

template <BindingType bindingType>
inline void PipelineState<bindingType>::set_viewport_state(vkb::rendering::ViewportState const &new_viewport_state)
{
	if (viewport_state != new_viewport_state)
	{
		viewport_state = new_viewport_state;
		dirty          = true;
	}
}
}        // namespace rendering

}        // namespace vkb
