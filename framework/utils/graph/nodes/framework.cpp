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

#include "utils/graph/nodes/framework.h"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>

#include <glm/gtx/string_cast.hpp>
#include <json.hpp>
#include <spdlog/fmt/fmt.h>

#include "utils/graph/node.h"
#include "utils/strings.h"

namespace vkb
{
namespace utils
{
std::unordered_map<FrameworkNodeType, std::string> FrameworkNode::framework_node_type_strings{
    {FrameworkNodeType::Text, "Text"},
    {FrameworkNodeType::RenderContext, "RenderContext"},
    {FrameworkNodeType::RenderFrame, "RenderFrame"},
    {FrameworkNodeType::SemaphorePool, "SemaphorePool"},
    {FrameworkNodeType::FencePool, "FencePool"},
    {FrameworkNodeType::CommandPool, "CommandPool"},
    {FrameworkNodeType::RenderTarget, "RenderTarget"},
    {FrameworkNodeType::Swapchain, "Swapchain"},
    {FrameworkNodeType::ImageView, "ImageView"},
    {FrameworkNodeType::Image, "Image"},
    {FrameworkNodeType::ResourceCache, "ResourceCache"},
    {FrameworkNodeType::ShaderModule, "ShaderModule"},
    {FrameworkNodeType::PipelineLayout, "PipelineLayout"},
    {FrameworkNodeType::DescriptorSetLayout, "DescriptorSetLayout"},
    {FrameworkNodeType::RenderPass, "RenderPass"},
    {FrameworkNodeType::GraphicsPipeline, "GraphicsPipeline"},
    {FrameworkNodeType::ComputePipeline, "ComputePipeline"},
    {FrameworkNodeType::DescriptorSet, "DescriptorSet"},
    {FrameworkNodeType::Framebuffer, "Framebuffer"},
    {FrameworkNodeType::ShaderResource, "ShaderResource"},
    {FrameworkNodeType::PipelineState, "PipelineState"},
    {FrameworkNodeType::SpecializationConstantState, "SpecializationConstantState"},
    {FrameworkNodeType::VertexInputState, "VertexInputState"},
    {FrameworkNodeType::InputAssemblyState, "InputAssemblyState"},
    {FrameworkNodeType::RasterizationState, "RasterizationState"},
    {FrameworkNodeType::ViewportState, "ViewportState"},
    {FrameworkNodeType::MultisampleState, "MultisampleState"},
    {FrameworkNodeType::DepthStencilState, "DepthStencilState"},
    {FrameworkNodeType::ColorBlendState, "ColorBlendState"},
    {FrameworkNodeType::ColorBlendAttachmentState, "ColorBlendAttachmentState"},
    {FrameworkNodeType::VkImage, "VkImage"},
    {FrameworkNodeType::VkImageView, "VkImageView"},
    {FrameworkNodeType::Device, "Device"}};

template <typename T>
std::string FrameworkNode::get_id(FrameworkNodeType type, T value)
{
	auto it = framework_node_type_strings.find(type);
	if (it != framework_node_type_strings.end())
	{
		return fmt::format("{}-{}", it->second, value);
	}
	throw std::runtime_error{"Type not in framework_node_type_strings map"};
}

std::string FrameworkNode::get_type_str(FrameworkNodeType type)
{
	auto it = framework_node_type_strings.find(type);
	if (it != framework_node_type_strings.end())
	{
		return it->second;
	}
	throw std::runtime_error{"Type not in framework_node_type_strings map"};
}

FrameworkNode::FrameworkNode(size_t id, const std::string &text)
{
	attributes["id"]    = id;
	attributes["label"] = text;
	attributes["group"] = "Text";
}

FrameworkNode::FrameworkNode(size_t id, const std::string &text, size_t owner)
{
	attributes["id"]    = id;
	attributes["label"] = text;
	attributes["group"] = "Text";
}

FrameworkNode::FrameworkNode(size_t id, const Device &device)
{
	attributes["id"]   = id;
	attributes["type"] = FrameworkNode::get_type_str(FrameworkNodeType::Device);

	auto pd_props = device.get_properties();

	nlohmann::json device_properties = {
	    {"deviceID", pd_props.deviceID},
	    {"deviceName", pd_props.deviceName},
	    {"deviceType", utils::to_string(pd_props.deviceType)},
	    {"driverVersion", pd_props.driverVersion},
	    {"apiVersion", pd_props.apiVersion},
	    {"vendorID", pd_props.vendorID}};

	attributes["data"] = nlohmann::json{{"VkPhysicalDeviceProperties", device_properties}};

	attributes["group"] = "Core";
}

FrameworkNode::FrameworkNode(size_t id, const RenderContext &context)
{
	attributes["id"]   = id;
	attributes["type"] = FrameworkNode::get_type_str(FrameworkNodeType::RenderContext);

	auto surface = context.get_surface_extent();

	attributes["data"] = nlohmann::json{{"VkExtent2D", {{"width", surface.width}, {"height", surface.height}}},
	                                    {"active_frame_index", context.get_active_frame_index()}};

	attributes["group"] = "Rendering";
}

FrameworkNode::FrameworkNode(size_t id, const RenderFrame &frame, std::string label)
{
	attributes["id"]    = id;
	attributes["label"] = label;
	attributes["type"]  = FrameworkNode::get_type_str(FrameworkNodeType::RenderFrame);
	attributes["group"] = "Rendering";
}

FrameworkNode::FrameworkNode(size_t id, const SemaphorePool &semaphore_pool)
{
	attributes["id"]    = id;
	attributes["type"]  = FrameworkNode::get_type_str(FrameworkNodeType::SemaphorePool);
	attributes["data"]  = nlohmann::json{{"active_semaphore_count", semaphore_pool.get_active_semaphore_count()}};
	attributes["group"] = "Framework";
}

FrameworkNode::FrameworkNode(size_t id, const FencePool &fence_pool)
{
	attributes["id"]    = id;
	attributes["type"]  = FrameworkNode::get_type_str(FrameworkNodeType::FencePool);
	attributes["group"] = "Framework";
}

FrameworkNode::FrameworkNode(size_t id, const RenderTarget &render_target)
{
	attributes["id"]   = id;
	attributes["type"] = FrameworkNode::get_type_str(FrameworkNodeType::RenderTarget);

	VkExtent2D surface = render_target.get_extent();

	attributes["data"] = nlohmann::json{{"VkExtent2D", {{"width", surface.width}, {"height", surface.height}}},
	                                    {"ImageView_count", render_target.get_views().size()},
	                                    {"Attachment_count", render_target.get_attachments().size()},
	                                    {"output_attachment_count", render_target.get_output_attachments().size()}};

	attributes["group"] = "Rendering";
}

FrameworkNode::FrameworkNode(size_t id, const core::ImageView &image_view)
{
	attributes["id"]   = id;
	attributes["type"] = FrameworkNode::get_type_str(FrameworkNodeType::ImageView);

	auto subresource_range  = image_view.get_subresource_range();
	auto subresource_layers = image_view.get_subresource_layers();

	attributes["data"] = nlohmann::json{{"VkFormat", utils::to_string(image_view.get_format())},
	                                    {"VkImageSubresourceRange",
	                                     {{"VkImageAspectFlags", utils::to_string_vk_image_aspect_flags(subresource_range.aspectMask)},
	                                      {"base_mip_level", subresource_range.baseMipLevel},
	                                      {"level_count", subresource_range.levelCount},
	                                      {"base_array_layer", subresource_range.baseArrayLayer},
	                                      {"layer_count", subresource_range.layerCount}}},
	                                    {"VkImageSubresourceLayers",
	                                     {{"VkImageAspectFlags", utils::to_string_vk_image_aspect_flags(subresource_layers.aspectMask)},
	                                      {"mip_level", subresource_layers.mipLevel},
	                                      {"base_array_layer", subresource_layers.baseArrayLayer},
	                                      {"layer_count", subresource_layers.layerCount}}}};

	attributes["group"] = "Core";
}

FrameworkNode::FrameworkNode(size_t id, const core::Image &image)
{
	attributes["id"]   = id;
	attributes["type"] = FrameworkNode::get_type_str(FrameworkNodeType::Image);

	auto subresource = image.get_subresource();

	attributes["data"] = nlohmann::json{{"VkExtent2D", {{"width", image.get_extent().width}, {"height", image.get_extent().height}}},
	                                    {"VkFormat", utils::to_string(image.get_format())},
	                                    {"VkImageUsageFlags", utils::to_string_vk_image_usage_flags(image.get_usage())},
	                                    {"VkSampleCountFlagBits", utils::to_string(image.get_sample_count())},
	                                    {"VkImageTiling", utils::to_string(image.get_tiling())},
	                                    {"VkImageType", utils::to_string(image.get_type())},
	                                    {"VkSubresource", {{"VkImageAspectFlags", utils::to_string_vk_image_aspect_flags(subresource.aspectMask)}, {"mip_level", subresource.mipLevel}, {"array_layer", subresource.arrayLayer}}}};

	attributes["group"] = "Core";
}

FrameworkNode::FrameworkNode(size_t id, const ResourceCache &resource_cache)
{
	attributes["id"]    = id;
	attributes["type"]  = FrameworkNode::get_type_str(FrameworkNodeType::ResourceCache);
	attributes["group"] = "Core";
}

FrameworkNode::FrameworkNode(size_t id, const DescriptorSetLayout &descriptor_set_layout, size_t hash)
{
	attributes["id"]   = id;
	attributes["type"] = FrameworkNode::get_type_str(FrameworkNodeType::DescriptorSetLayout);

	std::vector<nlohmann::json> bindings;

	auto it = descriptor_set_layout.get_bindings().begin();
	while (it != descriptor_set_layout.get_bindings().end())
	{
		bindings.push_back({{"binding", it->binding},
		                    {"descriptorCount", it->descriptorCount},
		                    // {"pImmutableSamplers", it->pImmutableSamplers},
		                    {"stageFlags", utils::to_string(it->stageFlags)}});
		it++;
	}

	attributes["data"] = nlohmann::json{{"hash", hash},
	                                    {"handle", Node::handle_to_uintptr_t(descriptor_set_layout.get_handle())},
	                                    {"VkDescriptorSetLayoutBinding", bindings}};

	attributes["group"] = "Core";
}

FrameworkNode::FrameworkNode(size_t id, const Framebuffer &framebuffer, size_t hash)
{
	attributes["id"]   = id;
	attributes["type"] = FrameworkNode::get_type_str(FrameworkNodeType::Framebuffer);

	attributes["data"] = nlohmann::json{{"hash", hash},
	                                    {"handle", Node::handle_to_uintptr_t(framebuffer.get_handle())}};

	attributes["group"] = "Core";
}

FrameworkNode::FrameworkNode(size_t id, const RenderPass &render_pass, size_t hash)
{
	attributes["id"]   = id;
	attributes["type"] = FrameworkNode::get_type_str(FrameworkNodeType::RenderPass);

	attributes["data"] = nlohmann::json{{"hash", hash},
	                                    {"handle", Node::handle_to_uintptr_t(render_pass.get_handle())}};

	attributes["group"] = "Rendering";
}

FrameworkNode::FrameworkNode(size_t id, const RenderPass &render_pass)
{
	attributes["id"]   = id;
	attributes["type"] = FrameworkNode::get_type_str(FrameworkNodeType::RenderPass);

	attributes["data"] = nlohmann::json{{"handle", Node::handle_to_uintptr_t(render_pass.get_handle())}};

	attributes["group"] = "Rendering";
}

FrameworkNode::FrameworkNode(size_t id, const ShaderModule &shader_module)
{
	attributes["id"]   = id;
	attributes["type"] = FrameworkNode::get_type_str(FrameworkNodeType::ShaderModule);

	std::string stage = utils::to_string_shader_stage_flags(shader_module.get_stage());
	std::transform(stage.begin(), stage.end(), stage.begin(),
	               [](unsigned char c) { return std::tolower(c); });

	attributes["label"] = stage;
	attributes["data"]  = nlohmann::json{{"stage", stage},
                                        {"infoLog", shader_module.get_info_log()},
                                        {"entry_point", shader_module.get_entry_point()},
                                        {"id", shader_module.get_id()}};

	attributes["group"] = "Rendering";
}

FrameworkNode::FrameworkNode(size_t id, const ShaderResource &shader_resource)
{
	attributes["id"]    = id;
	attributes["type"]  = FrameworkNode::get_type_str(FrameworkNodeType::ShaderResource);
	attributes["label"] = fmt::format("{}: {}", utils::to_string(shader_resource.type), shader_resource.name);

	attributes["data"]  = nlohmann::json{{"ShaderResourceType", utils::to_string(shader_resource.type)},
                                        {"VkShaderStageFlags", utils::to_string(shader_resource.stages)},
                                        {"set", shader_resource.set},
                                        {"binding", shader_resource.binding},
                                        {"location", shader_resource.location},
                                        {"input_attachment_index", shader_resource.input_attachment_index},
                                        {"vec_size", shader_resource.vec_size},
                                        {"columns", shader_resource.columns},
                                        {"array_size", shader_resource.array_size},
                                        {"offset", shader_resource.offset},
                                        {"size", shader_resource.size},
                                        {"constant_id", shader_resource.constant_id},
                                        {"dynamic", shader_resource.dynamic},
                                        {"name", shader_resource.name}};
	attributes["group"] = "Rendering";
}

FrameworkNode::FrameworkNode(size_t id, const PipelineLayout &pipeline_layout, size_t hash)
{
	attributes["id"]   = id;
	attributes["type"] = FrameworkNode::get_type_str(FrameworkNodeType::PipelineLayout);

	attributes["data"] = nlohmann::json{{"hash", hash},
	                                    {"handle", Node::handle_to_uintptr_t(pipeline_layout.get_handle())}};

	attributes["group"] = "Core";
}

FrameworkNode::FrameworkNode(size_t id, const PipelineLayout &pipeline_layout)
{
	attributes["id"]   = id;
	attributes["type"] = FrameworkNode::get_type_str(FrameworkNodeType::PipelineLayout);

	attributes["data"] = nlohmann::json{{"handle", Node::handle_to_uintptr_t(pipeline_layout.get_handle())}};

	attributes["group"] = "Core";
}

FrameworkNode::FrameworkNode(size_t id, const GraphicsPipeline &graphics_pipeline, size_t hash)
{
	attributes["id"]   = id;
	attributes["type"] = FrameworkNode::get_type_str(FrameworkNodeType::GraphicsPipeline);

	attributes["data"] = nlohmann::json{{"hash", hash},
	                                    {"handle", Node::handle_to_uintptr_t(graphics_pipeline.get_handle())}};

	attributes["group"] = "Core";
}
FrameworkNode::FrameworkNode(size_t id, const ComputePipeline &compute_pipeline, size_t hash)
{
	attributes["id"]   = id;
	attributes["type"] = FrameworkNode::get_type_str(FrameworkNodeType::ComputePipeline);

	attributes["data"] = nlohmann::json{{"hash", hash},
	                                    {"handle", Node::handle_to_uintptr_t(compute_pipeline.get_handle())}};

	attributes["group"] = "Core";
}

FrameworkNode::FrameworkNode(size_t id, const PipelineState &pipeline_state)
{
	attributes["id"]   = id;
	attributes["type"] = FrameworkNode::get_type_str(FrameworkNodeType::PipelineState);

	attributes["data"] = nlohmann::json{{"subpass_index", pipeline_state.get_subpass_index()}};

	attributes["group"] = "Rendering";
}

FrameworkNode::FrameworkNode(size_t id, const DescriptorSet &descriptor_set, size_t hash)
{
	attributes["id"]   = id;
	attributes["type"] = FrameworkNode::get_type_str(FrameworkNodeType::DescriptorSet);
	attributes["cid"]  = "ds";

	attributes["data"] = nlohmann::json{{"hash", hash},
	                                    {"handle", Node::handle_to_uintptr_t(descriptor_set.get_handle())}};

	attributes["group"] = "Core";
}

FrameworkNode::FrameworkNode(size_t id, const SpecializationConstantState &specialization_constant_state)
{
	attributes["id"]    = id;
	attributes["type"]  = FrameworkNode::get_type_str(FrameworkNodeType::SpecializationConstantState);
	attributes["label"] = FrameworkNode::get_type_str(FrameworkNodeType::SpecializationConstantState);

	nlohmann::json data = {};

	const auto &state = specialization_constant_state.get_specialization_constant_state();
	auto        it    = state.begin();
	while (it != state.end())
	{
		std::stringstream str;
		str << it->first;
		data.push_back({str.str(), it->second});
	}
	attributes["data"]  = data;
	attributes["group"] = "Core";
}

FrameworkNode::FrameworkNode(size_t id, const VertexInputState &vertex_input_state)
{
	attributes["id"]    = id;
	attributes["type"]  = FrameworkNode::get_type_str(FrameworkNodeType::VertexInputState);
	attributes["label"] = FrameworkNode::get_type_str(FrameworkNodeType::VertexInputState);

	std::vector<nlohmann::json> bindings;

	for (auto binding : vertex_input_state.bindings)
	{
		bindings.push_back({{"binding", binding.binding},
		                    {"stride", binding.stride},
		                    {"VkVertexInputRate", utils::to_string(binding.inputRate)}});
	}

	std::vector<nlohmann::json> binding;

	for (auto attribute : vertex_input_state.attributes)
	{
		binding.push_back({{"location", attribute.location},
		                   {"binding", attribute.binding},
		                   {"format", utils::to_string(attribute.format)},
		                   {"offset", attribute.offset}});
	}

	attributes["data"] = nlohmann::json{{"VkVertexInputBindingDescription", bindings},
	                                    {"VkVertexInputAttributeDescription", binding}};

	attributes["group"] = "Core";
}

FrameworkNode::FrameworkNode(size_t id, const InputAssemblyState &input_assembly_state)
{
	attributes["id"]    = id;
	attributes["type"]  = FrameworkNode::get_type_str(FrameworkNodeType::InputAssemblyState);
	attributes["label"] = FrameworkNode::get_type_str(FrameworkNodeType::InputAssemblyState);

	attributes["data"] = nlohmann::json{
	    {"VkPrimitiveTopology", utils::to_string(input_assembly_state.topology)},
	    {"primitive_restart_enabled", utils::to_string_vk_bool(input_assembly_state.primitive_restart_enable)}};

	attributes["group"] = "Core";
}
FrameworkNode::FrameworkNode(size_t id, const RasterizationState &rasterization_state)
{
	attributes["id"]    = id;
	attributes["type"]  = FrameworkNode::get_type_str(FrameworkNodeType::RasterizationState);
	attributes["label"] = FrameworkNode::get_type_str(FrameworkNodeType::RasterizationState);

	attributes["data"] = nlohmann::json{
	    {"depth_clamp_enable", utils::to_string_vk_bool(rasterization_state.depth_clamp_enable)},
	    {"rasterizer_discard_enable", utils::to_string_vk_bool(rasterization_state.rasterizer_discard_enable)},
	    {"polygon_mode", utils::to_string(rasterization_state.polygon_mode)},
	    {"cull_mode", utils::to_string_vk_cull_mode_flags(rasterization_state.cull_mode)},
	    {"front_face", utils::to_string(rasterization_state.front_face)},
	    {"depth_bias_enable", utils::to_string_vk_bool(rasterization_state.depth_bias_enable)},
	};

	attributes["group"] = "Core";
}
FrameworkNode::FrameworkNode(size_t id, const ViewportState &viewport_state)
{
	attributes["id"]    = id;
	attributes["type"]  = FrameworkNode::get_type_str(FrameworkNodeType::ViewportState);
	attributes["label"] = FrameworkNode::get_type_str(FrameworkNodeType::ViewportState);

	attributes["data"] = nlohmann::json{
	    {"viewport_count", viewport_state.viewport_count},
	    {"scissor_count", viewport_state.scissor_count}};
	attributes["group"] = "Core";
}
FrameworkNode::FrameworkNode(size_t id, const MultisampleState &multisample_state)
{
	attributes["id"]    = id;
	attributes["type"]  = FrameworkNode::get_type_str(FrameworkNodeType::MultisampleState);
	attributes["label"] = FrameworkNode::get_type_str(FrameworkNodeType::MultisampleState);

	attributes["data"] = nlohmann::json{
	    {"rasterization_samples", multisample_state.rasterization_samples},
	    {"sample_shading_enable", utils::to_string_vk_bool(multisample_state.sample_shading_enable)},
	    {"min_sample_shading", multisample_state.min_sample_shading},
	    {"sample_mask", multisample_state.sample_mask},
	    {"alpha_to_coverage_enable", utils::to_string_vk_bool(multisample_state.alpha_to_coverage_enable)},
	    {"alpha_to_one_enable", utils::to_string_vk_bool(multisample_state.alpha_to_one_enable)}};

	attributes["group"] = "Core";
}
FrameworkNode::FrameworkNode(size_t id, const DepthStencilState &depth_stencil_state)
{
	attributes["id"]    = id;
	attributes["type"]  = FrameworkNode::get_type_str(FrameworkNodeType::DepthStencilState);
	attributes["label"] = FrameworkNode::get_type_str(FrameworkNodeType::DepthStencilState);

	attributes["data"] = nlohmann::json{
	    {"depth_test_enable", utils::to_string_vk_bool(depth_stencil_state.depth_test_enable)},
	    {"depth_write_enable", utils::to_string_vk_bool(depth_stencil_state.depth_write_enable)},
	    {"depth_compare_op", utils::to_string(depth_stencil_state.depth_compare_op)},
	    {"depth_bounds_test_enable", utils::to_string_vk_bool(depth_stencil_state.depth_bounds_test_enable)},
	    {"stencil_test_enable", utils::to_string_vk_bool(depth_stencil_state.stencil_test_enable)},
	    {"front", {{"fail_op", utils::to_string(depth_stencil_state.front.fail_op)}, {"pass_op", utils::to_string(depth_stencil_state.front.pass_op)}, {"depth_fail_op", utils::to_string(depth_stencil_state.front.depth_fail_op)}, {"compare_op", utils::to_string(depth_stencil_state.front.compare_op)}}},
	    {"back", {{"fail_op", utils::to_string(depth_stencil_state.back.fail_op)}, {"pass_op", utils::to_string(depth_stencil_state.back.pass_op)}, {"depth_fail_op", utils::to_string(depth_stencil_state.back.depth_fail_op)}, {"compare_op", utils::to_string(depth_stencil_state.back.compare_op)}}}};

	attributes["group"] = "Core";
}
FrameworkNode::FrameworkNode(size_t id, const ColorBlendState &color_blend_state)
{
	attributes["id"]    = id;
	attributes["type"]  = FrameworkNode::get_type_str(FrameworkNodeType::ColorBlendState);
	attributes["label"] = FrameworkNode::get_type_str(FrameworkNodeType::ColorBlendState);

	attributes["data"] = nlohmann::json{
	    {"logic_op_enable", utils::to_string_vk_bool(color_blend_state.logic_op_enable)},
	    {"logic_op", utils::to_string(color_blend_state.logic_op)}};
	attributes["group"] = "Core";
}

FrameworkNode::FrameworkNode(size_t id, const ColorBlendAttachmentState &state)
{
	attributes["id"]    = id;
	attributes["type"]  = FrameworkNode::get_type_str(FrameworkNodeType::ColorBlendAttachmentState);
	attributes["label"] = FrameworkNode::get_type_str(FrameworkNodeType::ColorBlendAttachmentState);

	attributes["data"] = nlohmann::json{
	    {"blend_enable", utils::to_string_vk_bool(state.blend_enable)},
	    {"src_color_blend_factor", utils::to_string(state.src_color_blend_factor)},
	    {"dst_color_blend_factor", utils::to_string(state.dst_color_blend_factor)},
	    {"color_blend_op", utils::to_string(state.color_blend_op)},
	    {"src_alpha_blend_factor", utils::to_string(state.src_alpha_blend_factor)},
	    {"dst_alpha_blend_factor", utils::to_string(state.dst_alpha_blend_factor)},
	    {"alpha_blend_op", utils::to_string(state.alpha_blend_op)},
	    {"color_write_mask", utils::to_string_vk_color_component_flags(state.color_write_mask)}};
	attributes["group"] = "Core";
}

FrameworkNode::FrameworkNode(size_t id, const Swapchain &swapchain)
{
	attributes["id"]   = id;
	attributes["type"] = FrameworkNode::get_type_str(FrameworkNodeType::Swapchain);

	auto surface     = swapchain.get_extent();
	auto format      = swapchain.get_format();
	auto image_count = swapchain.get_images().size();

	attributes["data"] = nlohmann::json{{"VkExtent2D", {{"width", surface.width}, {"height", surface.height}}},
	                                    {"VkFormat", utils::to_string(format)},
	                                    {"image_count", image_count},
	                                    {"VkSurfaceTransformFlagBitsKHR", utils::to_string(swapchain.get_transform())},
	                                    {"VkPresentModeKHR", utils::to_string(swapchain.get_present_mode())},
	                                    {"VkImageUsageFlags", utils::to_string_vk_image_usage_flags(swapchain.get_usage())}};

	attributes["group"] = "Core";
}

}        // namespace utils
}        // namespace vkb