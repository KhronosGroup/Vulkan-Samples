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

#include "pipeline_state.h"

#include <vulkan/vulkan.hpp>

namespace vkb
{
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

struct HPPStencilOpState
{
	vk::StencilOp fail_op       = vk::StencilOp::eReplace;
	vk::StencilOp pass_op       = vk::StencilOp::eReplace;
	vk::StencilOp depth_fail_op = vk::StencilOp::eReplace;
	vk::CompareOp compare_op    = vk::CompareOp::eNever;
};

struct HPPDepthStencilState
{
	vk::Bool32     depth_test_enable        = true;
	vk::Bool32     depth_write_enable       = true;
	vk::CompareOp  depth_compare_op         = vk::CompareOp::eGreater;        // Note: Using Reversed depth-buffer for increased precision, so Greater depth values are kept
	vk::Bool32     depth_bounds_test_enable = false;
	vk::Bool32     stencil_test_enable      = false;
	StencilOpState front;
	StencilOpState back;
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

struct HPPVertexInputState
{
	std::vector<vk::VertexInputBindingDescription>   bindings;
	std::vector<vk::VertexInputAttributeDescription> attributes;
};
}        // namespace rendering
}        // namespace vkb
