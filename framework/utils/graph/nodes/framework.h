/* Copyright (c) 2018-2019, Arm Limited and Contributors
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

#include <iostream>
#include <map>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

#include "core/device.h"
#include "core/image_view.h"
#include "core/shader_module.h"
#include "core/swapchain.h"
#include "fence_pool.h"
#include "rendering/render_context.h"
#include "rendering/render_frame.h"
#include "rendering/render_target.h"
#include "resource_cache.h"
#include "semaphore_pool.h"
#include "utils/graph/node.h"

namespace vkb
{
namespace utils
{
enum class FrameworkNodeType
{
	Text,
	RenderContext,
	RenderFrame,
	SemaphorePool,
	FencePool,
	CommandPool,
	RenderTarget,
	Swapchain,
	ImageView,
	Image,
	ResourceCache,
	ShaderModule,
	PipelineLayout,
	DescriptorSetLayout,
	RenderPass,
	GraphicsPipeline,
	ComputePipeline,
	DescriptorSet,
	Framebuffer,
	ShaderResource,
	PipelineState,
	SpecializationConstantState,
	VertexInputState,
	InputAssemblyState,
	RasterizationState,
	ViewportState,
	MultisampleState,
	DepthStencilState,
	ColorBlendState,
	ColorBlendAttachmentState,
	VkImage,
	Device,
	VkImageView
};

/**
 * @brief FrameworkNode is a node type used by utils::Graph to create different node variants for different types of framework components.
 * This structure allows for minimum code cluttering when using the graph api.
 * Note: if you want to add a new framework node definition to the graph it must also be defined here
 */
class FrameworkNode : public Node
{
	enum class Group
	{
		Default,
		Core,
		Rendering,
		Framework
	};

  public:
	FrameworkNode() = default;

	FrameworkNode(size_t id, const std::string &text);
	FrameworkNode(size_t id, const std::string &text, size_t owner);
	FrameworkNode(size_t id, const Device &device);
	FrameworkNode(size_t id, const RenderContext &context);
	FrameworkNode(size_t id, const SemaphorePool &semaphore_pool);
	FrameworkNode(size_t id, const FencePool &fence_pool);
	FrameworkNode(size_t id, const RenderFrame &frame, std::string label);
	FrameworkNode(size_t id, const RenderTarget &render_target);
	FrameworkNode(size_t id, const core::ImageView &image_view);
	FrameworkNode(size_t id, const core::Image &image);
	FrameworkNode(size_t id, const Swapchain &swapchain);
	FrameworkNode(size_t id, const ResourceCache &resource_cache);
	FrameworkNode(size_t id, const DescriptorSetLayout &descriptor_set_layouts, size_t hash);
	FrameworkNode(size_t id, const Framebuffer &framebuffers, size_t hash);
	FrameworkNode(size_t id, const RenderPass &render_passes, size_t hash);
	FrameworkNode(size_t id, const RenderPass &render_passes);
	FrameworkNode(size_t id, const ShaderModule &shader_modules);
	FrameworkNode(size_t id, const ShaderResource &shader_resource);
	FrameworkNode(size_t id, const PipelineLayout &pipeline_layouts, size_t hash);
	FrameworkNode(size_t id, const PipelineLayout &pipeline_layouts);
	FrameworkNode(size_t id, const GraphicsPipeline &graphics_pipelines, size_t hash);
	FrameworkNode(size_t id, const ComputePipeline &compute_pipelines, size_t hash);
	FrameworkNode(size_t id, const PipelineState &pipeline_state);
	FrameworkNode(size_t id, const DescriptorSet &descriptor_sets, size_t hash);
	FrameworkNode(size_t id, const SpecializationConstantState &specialization_constant_state);
	FrameworkNode(size_t id, const VertexInputState &vertex_input_state);
	FrameworkNode(size_t id, const InputAssemblyState &input_assembly_state);
	FrameworkNode(size_t id, const RasterizationState &rasterization_state);
	FrameworkNode(size_t id, const ViewportState &viewport_state);
	FrameworkNode(size_t id, const MultisampleState &multisample_state);
	FrameworkNode(size_t id, const DepthStencilState &depth_stencil_state);
	FrameworkNode(size_t id, const ColorBlendState &color_blend_state);
	FrameworkNode(size_t id, const ColorBlendAttachmentState &state);

	template <typename T>
	static std::string get_id(FrameworkNodeType type, T value);

	static std::string get_type_str(FrameworkNodeType type);

  private:
	static std::unordered_map<FrameworkNodeType, std::string> framework_node_type_strings;
};
}        // namespace utils
}        // namespace vkb