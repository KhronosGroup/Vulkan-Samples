/* Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "common/vk_common.h"

#include "common/logging.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_format_traits.hpp"

namespace vkb
{
namespace common
{
/**
 * @brief facade helper functions and structs around the functions and structs in common/vk_common, providing a vulkan.hpp-based interface
 */

struct HPPBufferMemoryBarrier
{
	vk::PipelineStageFlags src_stage_mask  = vk::PipelineStageFlagBits::eBottomOfPipe;
	vk::PipelineStageFlags dst_stage_mask  = vk::PipelineStageFlagBits::eTopOfPipe;
	vk::AccessFlags        src_access_mask = {};
	vk::AccessFlags        dst_access_mask = {};
};

struct HPPImageMemoryBarrier
{
	vk::PipelineStageFlags src_stage_mask = vk::PipelineStageFlagBits::eBottomOfPipe;
	vk::PipelineStageFlags dst_stage_mask = vk::PipelineStageFlagBits::eTopOfPipe;
	vk::AccessFlags        src_access_mask;
	vk::AccessFlags        dst_access_mask;
	vk::ImageLayout        old_layout       = vk::ImageLayout::eUndefined;
	vk::ImageLayout        new_layout       = vk::ImageLayout::eUndefined;
	uint32_t               old_queue_family = VK_QUEUE_FAMILY_IGNORED;
	uint32_t               new_queue_family = VK_QUEUE_FAMILY_IGNORED;
};

struct HPPLoadStoreInfo
{
	vk::AttachmentLoadOp  load_op  = vk::AttachmentLoadOp::eClear;
	vk::AttachmentStoreOp store_op = vk::AttachmentStoreOp::eStore;
};

inline int32_t get_bits_per_pixel(vk::Format format)
{
	return vkb::get_bits_per_pixel(static_cast<VkFormat>(format));
}

inline vk::Format get_suitable_depth_format(vk::PhysicalDevice             physical_device,
                                            bool                           depth_only                 = false,
                                            const std::vector<vk::Format> &depth_format_priority_list = {
                                                vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint, vk::Format::eD16Unorm})
{
	return static_cast<vk::Format>(
	    vkb::get_suitable_depth_format(physical_device, depth_only, reinterpret_cast<std::vector<VkFormat> const &>(depth_format_priority_list)));
}

inline bool is_buffer_descriptor_type(vk::DescriptorType descriptor_type)
{
	return vkb::is_buffer_descriptor_type(static_cast<VkDescriptorType>(descriptor_type));
}

inline bool is_depth_only_format(vk::Format format)
{
	assert(vkb::is_depth_only_format(static_cast<VkFormat>(format)) ==
	       ((vk::componentCount(format) == 1) && (std::string(vk::componentName(format, 0)) == "D")));
	return vkb::is_depth_only_format(static_cast<VkFormat>(format));
}

inline bool is_depth_stencil_format(vk::Format format)
{
	assert(vkb::is_depth_stencil_format(static_cast<VkFormat>(format)) ==
	       ((vk::componentCount(format) == 2) && (std::string(vk::componentName(format, 0)) == "D") &&
	        (std::string(vk::componentName(format, 1)) == "S")));
	return vkb::is_depth_stencil_format(static_cast<VkFormat>(format));
}

inline bool is_depth_format(vk::Format format)
{
	assert(vkb::is_depth_format(static_cast<VkFormat>(format)) == (std::string(vk::componentName(format, 0)) == "D"));
	return vkb::is_depth_format(static_cast<VkFormat>(format));
}

inline bool is_dynamic_buffer_descriptor_type(vk::DescriptorType descriptor_type)
{
	return vkb::is_dynamic_buffer_descriptor_type(static_cast<VkDescriptorType>(descriptor_type));
}

inline vk::ShaderModule load_shader(const std::string &filename, vk::Device device, vk::ShaderStageFlagBits stage, ShaderSourceLanguage src_language = ShaderSourceLanguage::GLSL)
{
	return static_cast<vk::ShaderModule>(vkb::load_shader(filename, device, static_cast<VkShaderStageFlagBits>(stage), src_language));
}

inline void image_layout_transition(vk::CommandBuffer command_buffer,
                                    vk::Image         image,
                                    vk::ImageLayout   old_layout,
                                    vk::ImageLayout   new_layout)
{
	vkb::image_layout_transition(static_cast<VkCommandBuffer>(command_buffer),
	                             static_cast<VkImage>(image),
	                             static_cast<VkImageLayout>(old_layout),
	                             static_cast<VkImageLayout>(new_layout));
}

inline void image_layout_transition(vk::CommandBuffer         command_buffer,
                                    vk::Image                 image,
                                    vk::ImageLayout           old_layout,
                                    vk::ImageLayout           new_layout,
                                    vk::ImageSubresourceRange subresource_range)
{
	vkb::image_layout_transition(static_cast<VkCommandBuffer>(command_buffer),
	                             static_cast<VkImage>(image),
	                             static_cast<VkImageLayout>(old_layout),
	                             static_cast<VkImageLayout>(new_layout),
	                             static_cast<VkImageSubresourceRange>(subresource_range));
}

inline void image_layout_transition(vk::CommandBuffer                command_buffer,
                                    vk::Image                        image,
                                    vk::PipelineStageFlags           src_stage_mask,
                                    vk::PipelineStageFlags           dst_stage_mask,
                                    vk::AccessFlags                  src_access_mask,
                                    vk::AccessFlags                  dst_access_mask,
                                    vk::ImageLayout                  old_layout,
                                    vk::ImageLayout                  new_layout,
                                    vk::ImageSubresourceRange const &subresource_range)
{
	vkb::image_layout_transition(static_cast<VkCommandBuffer>(command_buffer),
	                             static_cast<VkImage>(image),
	                             static_cast<VkPipelineStageFlags>(src_stage_mask),
	                             static_cast<VkPipelineStageFlags>(dst_stage_mask),
	                             static_cast<VkAccessFlags>(src_access_mask),
	                             static_cast<VkAccessFlags>(dst_access_mask),
	                             static_cast<VkImageLayout>(old_layout),
	                             static_cast<VkImageLayout>(new_layout),
	                             static_cast<VkImageSubresourceRange const &>(subresource_range));
}

inline vk::SurfaceFormatKHR select_surface_format(vk::PhysicalDevice             gpu,
                                                  vk::SurfaceKHR                 surface,
                                                  std::vector<vk::Format> const &preferred_formats = {
                                                      vk::Format::eR8G8B8A8Srgb, vk::Format::eB8G8R8A8Srgb, vk::Format::eA8B8G8R8SrgbPack32})
{
	std::vector<vk::SurfaceFormatKHR> supported_surface_formats = gpu.getSurfaceFormatsKHR(surface);
	assert(!supported_surface_formats.empty());

	auto it = std::find_if(supported_surface_formats.begin(),
	                       supported_surface_formats.end(),
	                       [&preferred_formats](vk::SurfaceFormatKHR surface_format) {
		                       return std::any_of(preferred_formats.begin(),
		                                          preferred_formats.end(),
		                                          [&surface_format](vk::Format format) { return format == surface_format.format; });
	                       });

	// We use the first supported format as a fallback in case none of the preferred formats is available
	return it != supported_surface_formats.end() ? *it : supported_surface_formats[0];
}

inline vk::Format choose_blendable_format(vk::PhysicalDevice gpu, const std::vector<vk::Format> &format_priority_list)
{
	for (const auto &format : format_priority_list)
	{
		vk::FormatProperties fmt_props = gpu.getFormatProperties(format);

		if (fmt_props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachmentBlend)
			return format;
	}

	throw std::runtime_error("No suitable blendable format could be determined");
}

// helper functions not backed by vk_common.h
inline vk::CommandBuffer allocate_command_buffer(vk::Device device, vk::CommandPool command_pool, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary)
{
	vk::CommandBufferAllocateInfo command_buffer_allocate_info(command_pool, level, 1);
	return device.allocateCommandBuffers(command_buffer_allocate_info).front();
}

inline vk::DescriptorSet allocate_descriptor_set(vk::Device device, vk::DescriptorPool descriptor_pool, vk::DescriptorSetLayout descriptor_set_layout)
{
#if defined(ANDROID)
	vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptor_pool, 1, &descriptor_set_layout);
#else
	vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptor_pool, descriptor_set_layout);
#endif
	return device.allocateDescriptorSets(descriptor_set_allocate_info).front();
}

inline vk::Framebuffer create_framebuffer(vk::Device device, vk::RenderPass render_pass, std::vector<vk::ImageView> const &attachments, vk::Extent2D const &extent)
{
	vk::FramebufferCreateInfo framebuffer_create_info({}, render_pass, attachments, extent.width, extent.height, 1);
	return device.createFramebuffer(framebuffer_create_info);
}

inline vk::Pipeline create_graphics_pipeline(vk::Device                                                device,
                                             vk::PipelineCache                                         pipeline_cache,
                                             std::vector<vk::PipelineShaderStageCreateInfo> const     &shader_stages,
                                             vk::PipelineVertexInputStateCreateInfo const             &vertex_input_state,
                                             vk::PrimitiveTopology                                     primitive_topology,
                                             uint32_t                                                  patch_control_points,
                                             vk::PolygonMode                                           polygon_mode,
                                             vk::CullModeFlags                                         cull_mode,
                                             vk::FrontFace                                             front_face,
                                             std::vector<vk::PipelineColorBlendAttachmentState> const &blend_attachment_states,
                                             vk::PipelineDepthStencilStateCreateInfo const            &depth_stencil_state,
                                             vk::PipelineLayout                                        pipeline_layout,
                                             vk::RenderPass                                            render_pass)
{
	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state({}, primitive_topology, false);

	vk::PipelineTessellationStateCreateInfo tessellation_state({}, patch_control_points);

	vk::PipelineViewportStateCreateInfo viewport_state({}, 1, nullptr, 1, nullptr);

	vk::PipelineRasterizationStateCreateInfo rasterization_state;
	rasterization_state.polygonMode = polygon_mode;
	rasterization_state.cullMode    = cull_mode;
	rasterization_state.frontFace   = front_face;
	rasterization_state.lineWidth   = 1.0f;

	vk::PipelineMultisampleStateCreateInfo multisample_state({}, vk::SampleCountFlagBits::e1);

	vk::PipelineColorBlendStateCreateInfo color_blend_state({}, false, {}, blend_attachment_states);

	std::array<vk::DynamicState, 2>    dynamic_state_enables = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
	vk::PipelineDynamicStateCreateInfo dynamic_state({}, dynamic_state_enables);

	// Final fullscreen composition pass pipeline
	vk::GraphicsPipelineCreateInfo pipeline_create_info({},
	                                                    shader_stages,
	                                                    &vertex_input_state,
	                                                    &input_assembly_state,
	                                                    &tessellation_state,
	                                                    &viewport_state,
	                                                    &rasterization_state,
	                                                    &multisample_state,
	                                                    &depth_stencil_state,
	                                                    &color_blend_state,
	                                                    &dynamic_state,
	                                                    pipeline_layout,
	                                                    render_pass,
	                                                    {},
	                                                    {},
	                                                    -1);

	vk::Result   result;
	vk::Pipeline pipeline;
	std::tie(result, pipeline) = device.createGraphicsPipeline(pipeline_cache, pipeline_create_info);
	assert(result == vk::Result::eSuccess);
	return pipeline;
}

inline vk::ImageView create_image_view(vk::Device           device,
                                       vk::Image            image,
                                       vk::ImageViewType    view_type,
                                       vk::Format           format,
                                       vk::ImageAspectFlags aspect_mask      = vk::ImageAspectFlagBits::eColor,
                                       uint32_t             base_mip_level   = 0,
                                       uint32_t             level_count      = 1,
                                       uint32_t             base_array_layer = 0,
                                       uint32_t             layer_count      = 1)
{
	vk::ImageViewCreateInfo image_view_create_info;
	image_view_create_info.image                           = image;
	image_view_create_info.viewType                        = view_type;
	image_view_create_info.format                          = format;
	image_view_create_info.subresourceRange.aspectMask     = aspect_mask;
	image_view_create_info.subresourceRange.baseMipLevel   = base_mip_level;
	image_view_create_info.subresourceRange.levelCount     = level_count;
	image_view_create_info.subresourceRange.baseArrayLayer = base_array_layer;
	image_view_create_info.subresourceRange.layerCount     = layer_count;
	return device.createImageView(image_view_create_info);
}

inline vk::QueryPool create_query_pool(vk::Device device, vk::QueryType query_type, uint32_t query_count, vk::QueryPipelineStatisticFlags pipeline_statistics = {})
{
	vk::QueryPoolCreateInfo query_pool_create_info;
	query_pool_create_info.queryType          = query_type;
	query_pool_create_info.queryCount         = query_count;
	query_pool_create_info.pipelineStatistics = pipeline_statistics;
	return device.createQueryPool(query_pool_create_info);
}

inline vk::Sampler create_sampler(vk::PhysicalDevice gpu, vk::Device device, vk::Format format, vk::Filter filter,
                                  vk::SamplerAddressMode sampler_address_mode, float max_anisotropy, float max_LOD)
{
	const vk::FormatProperties fmt_props = gpu.getFormatProperties(format);

	bool has_linear_filter = !!(fmt_props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear);

	vk::SamplerCreateInfo sampler_create_info;
	sampler_create_info.magFilter               = has_linear_filter ? filter : vk::Filter::eNearest;
	sampler_create_info.minFilter               = has_linear_filter ? filter : vk::Filter::eNearest;
	sampler_create_info.mipmapMode              = has_linear_filter ? vk::SamplerMipmapMode::eLinear : vk::SamplerMipmapMode::eNearest;
	sampler_create_info.addressModeU            = sampler_address_mode;
	sampler_create_info.addressModeV            = sampler_address_mode;
	sampler_create_info.addressModeW            = sampler_address_mode;
	sampler_create_info.mipLodBias              = 0.0f;
	sampler_create_info.anisotropyEnable        = (1.0f < max_anisotropy);
	sampler_create_info.maxAnisotropy           = max_anisotropy;
	sampler_create_info.compareEnable           = false;
	sampler_create_info.compareOp               = vk::CompareOp::eNever;
	sampler_create_info.minLod                  = 0.0f;
	sampler_create_info.maxLod                  = max_LOD;
	sampler_create_info.borderColor             = vk::BorderColor::eFloatOpaqueWhite;
	sampler_create_info.unnormalizedCoordinates = false;
	return device.createSampler(sampler_create_info);
}

inline vk::ImageAspectFlags get_image_aspect_flags(vk::ImageUsageFlagBits usage, vk::Format format)
{
	vk::ImageAspectFlags image_aspect_flags;

	switch (usage)
	{
		case vk::ImageUsageFlagBits::eColorAttachment:
			assert(!vkb::common::is_depth_format(format));
			image_aspect_flags = vk::ImageAspectFlagBits::eColor;
			break;
		case vk::ImageUsageFlagBits::eDepthStencilAttachment:
			assert(vkb::common::is_depth_format(format));
			image_aspect_flags = vk::ImageAspectFlagBits::eDepth;
			// Stencil aspect should only be set on depth + stencil formats
			if (vkb::common::is_depth_stencil_format(format))
			{
				image_aspect_flags |= vk::ImageAspectFlagBits::eStencil;
			}
			break;
		default:
			assert(false);
	}

	return image_aspect_flags;
}

inline void submit_and_wait(vk::Device device, vk::Queue queue, std::vector<vk::CommandBuffer> command_buffers, std::vector<vk::Semaphore> semaphores = {})
{
	// Submit command_buffer
	vk::SubmitInfo submit_info(nullptr, {}, command_buffers, semaphores);

	// Create fence to ensure that command_buffer has finished executing
	vk::Fence fence = device.createFence({});

	// Submit to the queue
	queue.submit(submit_info, fence);

	// Wait for the fence to signal that command_buffer has finished executing
	vk::Result result = device.waitForFences(fence, true, DEFAULT_FENCE_TIMEOUT);
	if (result != vk::Result::eSuccess)
	{
		LOGE("Vulkan error on waitForFences: {}", vk::to_string(result));
		abort();
	}

	// Destroy the fence
	device.destroyFence(fence);
}

}        // namespace common
}        // namespace vkb
