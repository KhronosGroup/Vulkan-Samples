/* Copyright (c) 2019, Arm Limited and Contributors
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

#include <vector>

#include "common/vk_common.h"
#include "core/pipeline_layout.h"
#include "core/render_pass.h"

namespace vkb
{
struct VertexInputState
{
	std::vector<VkVertexInputBindingDescription> bindings;

	std::vector<VkVertexInputAttributeDescription> attributes;
};

struct InputAssemblyState
{
	VkPrimitiveTopology topology{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};

	VkBool32 primitive_restart_enable{VK_FALSE};
};

struct RasterizationState
{
	VkBool32 depth_clamp_enable{VK_FALSE};

	VkBool32 rasterizer_discard_enable{VK_FALSE};

	VkPolygonMode polygon_mode{VK_POLYGON_MODE_FILL};

	VkCullModeFlags cull_mode{VK_CULL_MODE_BACK_BIT};

	VkFrontFace front_face{VK_FRONT_FACE_COUNTER_CLOCKWISE};

	VkBool32 depth_bias_enable{VK_FALSE};
};

struct ViewportState
{
	uint32_t viewport_count{1};

	uint32_t scissor_count{1};
};

struct MultisampleState
{
	VkSampleCountFlagBits rasterization_samples{VK_SAMPLE_COUNT_1_BIT};

	VkBool32 sample_shading_enable{VK_FALSE};

	float min_sample_shading{0.0f};

	VkSampleMask sample_mask{0};

	VkBool32 alpha_to_coverage_enable{VK_FALSE};

	VkBool32 alpha_to_one_enable{VK_FALSE};
};

struct StencilOpState
{
	VkStencilOp fail_op{VK_STENCIL_OP_REPLACE};

	VkStencilOp pass_op{VK_STENCIL_OP_REPLACE};

	VkStencilOp depth_fail_op{VK_STENCIL_OP_REPLACE};

	VkCompareOp compare_op{VK_COMPARE_OP_NEVER};
};

struct DepthStencilState
{
	VkBool32 depth_test_enable{VK_TRUE};

	VkBool32 depth_write_enable{VK_TRUE};

	VkCompareOp depth_compare_op{VK_COMPARE_OP_LESS_OR_EQUAL};

	VkBool32 depth_bounds_test_enable{VK_FALSE};

	VkBool32 stencil_test_enable{VK_FALSE};

	StencilOpState front{};

	StencilOpState back{};
};

struct ColorBlendAttachmentState
{
	VkBool32 blend_enable{VK_FALSE};

	VkBlendFactor src_color_blend_factor{VK_BLEND_FACTOR_ONE};

	VkBlendFactor dst_color_blend_factor{VK_BLEND_FACTOR_ZERO};

	VkBlendOp color_blend_op{VK_BLEND_OP_ADD};

	VkBlendFactor src_alpha_blend_factor{VK_BLEND_FACTOR_ONE};

	VkBlendFactor dst_alpha_blend_factor{VK_BLEND_FACTOR_ZERO};

	VkBlendOp alpha_blend_op{VK_BLEND_OP_ADD};

	VkColorComponentFlags color_write_mask{VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};
};

struct ColorBlendState
{
	VkBool32 logic_op_enable{VK_FALSE};

	VkLogicOp logic_op{VK_LOGIC_OP_CLEAR};

	std::vector<ColorBlendAttachmentState> attachments;
};

/// Helper class to create specialization constants for a Vulkan pipeline. The state tracks a pipeline globally, and not per shader. Two shaders using the same constant_id will have the same data.
class SpecializationConstantState
{
  public:
	void reset();

	bool is_dirty() const;

	void clear_dirty();

	template <class T>
	void set_constant(uint32_t constant_id, const T &data);

	void set_constant(uint32_t constant_id, const std::vector<uint8_t> &data);

	void set_specialization_constant_state(const std::map<uint32_t, std::vector<uint8_t>> &state);

	const std::map<uint32_t, std::vector<uint8_t>> &get_specialization_constant_state() const;

  private:
	bool dirty{false};
	// Map tracking state of the Specialization Constants
	std::map<uint32_t, std::vector<uint8_t>> specialization_constant_state;
};

template <class T>
inline void SpecializationConstantState::set_constant(std::uint32_t constant_id, const T &data)
{
	std::uint32_t value = static_cast<std::uint32_t>(data);

	set_constant(constant_id,
	             {reinterpret_cast<const uint8_t *>(&value),
	              reinterpret_cast<const uint8_t *>(&value) + sizeof(T)});
}

template <>
inline void SpecializationConstantState::set_constant<bool>(std::uint32_t constant_id, const bool &data_)
{
	std::uint32_t value = static_cast<std::uint32_t>(data_);

	set_constant(constant_id,
	             {reinterpret_cast<const uint8_t *>(&value),
	              reinterpret_cast<const uint8_t *>(&value) + sizeof(std::uint32_t)});
}

class PipelineState
{
  public:
	void reset();

	void set_pipeline_layout(PipelineLayout &pipeline_layout);

	void set_render_pass(const RenderPass &render_pass);

	void set_specialization_constant(uint32_t constant_id, const std::vector<uint8_t> &data);

	void set_vertex_input_state(const VertexInputState &vertex_input_sate);

	void set_input_assembly_state(const InputAssemblyState &input_assembly_state);

	void set_rasterization_state(const RasterizationState &rasterization_state);

	void set_viewport_state(const ViewportState &viewport_state);

	void set_multisample_state(const MultisampleState &multisample_state);

	void set_depth_stencil_state(const DepthStencilState &depth_stencil_state);

	void set_color_blend_state(const ColorBlendState &color_blend_state);

	void set_subpass_index(uint32_t subpass_index);

	const PipelineLayout &get_pipeline_layout() const;

	const RenderPass *get_render_pass() const;

	const SpecializationConstantState &get_specialization_constant_state() const;

	const VertexInputState &get_vertex_input_state() const;

	const InputAssemblyState &get_input_assembly_state() const;

	const RasterizationState &get_rasterization_state() const;

	const ViewportState &get_viewport_state() const;

	const MultisampleState &get_multisample_state() const;

	const DepthStencilState &get_depth_stencil_state() const;

	const ColorBlendState &get_color_blend_state() const;

	uint32_t get_subpass_index() const;

	bool is_dirty() const;

	void clear_dirty();

  private:
	bool dirty{false};

	PipelineLayout *pipeline_layout{nullptr};

	const RenderPass *render_pass{nullptr};

	SpecializationConstantState specialization_constant_state{};

	VertexInputState vertex_input_sate{};

	InputAssemblyState input_assembly_state{};

	RasterizationState rasterization_state{};

	ViewportState viewport_state{};

	MultisampleState multisample_state{};

	DepthStencilState depth_stencil_state{};

	ColorBlendState color_blend_state{};

	uint32_t subpass_index{0U};
};
}        // namespace vkb
