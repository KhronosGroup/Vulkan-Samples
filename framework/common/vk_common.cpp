/* Copyright (c) 2018-2026, Arm Limited and Contributors
 * Copyright (c) 2019-2026, Sascha Willems
 * Copyright (c) 2026, NVIDIA CORPORATION. All rights reserved.
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

#include "vk_common.h"
#include "core/util/logging.hpp"
#include "filesystem/legacy.h"

constexpr vk::AccessFlags getAccessFlags(vk::ImageLayout layout)
{
	switch (layout)
	{
		case vk::ImageLayout::eUndefined:
		case vk::ImageLayout::ePresentSrcKHR:
			return {};
		case vk::ImageLayout::ePreinitialized:
			return vk::AccessFlagBits::eHostWrite;
		case vk::ImageLayout::eColorAttachmentOptimal:
			return vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
		case vk::ImageLayout::eDepthAttachmentOptimal:
			return vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		case vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR:
			return vk::AccessFlagBits::eFragmentShadingRateAttachmentReadKHR;
		case vk::ImageLayout::eShaderReadOnlyOptimal:
			return vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eInputAttachmentRead;
		case vk::ImageLayout::eTransferSrcOptimal:
			return vk::AccessFlagBits::eTransferRead;
		case vk::ImageLayout::eTransferDstOptimal:
			return vk::AccessFlagBits::eTransferWrite;
		case vk::ImageLayout::eGeneral:
			assert(false &&
			       "Don't know how to get a meaningful vk::AccessFlags for vk::ImageLayout::eGeneral! Don't use it!");
			return {};
		default:
			assert(false);
			return {};
	}
}

constexpr vk::PipelineStageFlags getPipelineStageFlags(vk::ImageLayout layout)
{
	switch (layout)
	{
		case vk::ImageLayout::eUndefined:
			return vk::PipelineStageFlagBits::eTopOfPipe;
		case vk::ImageLayout::ePreinitialized:
			return vk::PipelineStageFlagBits::eHost;
		case vk::ImageLayout::eTransferDstOptimal:
		case vk::ImageLayout::eTransferSrcOptimal:
			return vk::PipelineStageFlagBits::eTransfer;
		case vk::ImageLayout::eColorAttachmentOptimal:
			return vk::PipelineStageFlagBits::eColorAttachmentOutput;
		case vk::ImageLayout::eDepthAttachmentOptimal:
			return vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
		case vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR:
			return vk::PipelineStageFlagBits::eFragmentShadingRateAttachmentKHR;
		case vk::ImageLayout::eShaderReadOnlyOptimal:
			return vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader;
		case vk::ImageLayout::ePresentSrcKHR:
			return vk::PipelineStageFlagBits::eBottomOfPipe;
		case vk::ImageLayout::eGeneral:
			assert(false && "Don't know how to get a meaningful vk::PipelineStageFlags for vk::ImageLayout::eGeneral! "
			                "Don't use it!");
			return {};
		default:
			assert(false);
			return {};
	}
}

namespace vkb
{
namespace common
{
vk::CommandBuffer allocate_command_buffer(vk::Device device, vk::CommandPool command_pool, vk::CommandBufferLevel level)
{
	vk::CommandBufferAllocateInfo command_buffer_allocate_info{
	    .commandPool = command_pool, .level = level, .commandBufferCount = 1};
	return device.allocateCommandBuffers(command_buffer_allocate_info).front();
}

vk::DescriptorSet allocate_descriptor_set(vk::Device              device,
                                          vk::DescriptorPool      descriptor_pool,
                                          vk::DescriptorSetLayout descriptor_set_layout)
{
	vk::DescriptorSetAllocateInfo descriptor_set_allocate_info{
	    .descriptorPool = descriptor_pool, .descriptorSetCount = 1, .pSetLayouts = &descriptor_set_layout};
	return device.allocateDescriptorSets(descriptor_set_allocate_info).front();
}

vk::Framebuffer create_framebuffer(vk::Device                        device,
                                   vk::RenderPass                    render_pass,
                                   std::vector<vk::ImageView> const &attachments,
                                   vk::Extent2D const               &extent)
{
	vk::FramebufferCreateInfo framebuffer_create_info{.renderPass      = render_pass,
	                                                  .attachmentCount = static_cast<uint32_t>(attachments.size()),
	                                                  .pAttachments    = attachments.data(),
	                                                  .width           = extent.width,
	                                                  .height          = extent.height,
	                                                  .layers          = 1};
	return device.createFramebuffer(framebuffer_create_info);
}

vk::Pipeline create_graphics_pipeline(vk::Device                                                device,
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
	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state{.topology = primitive_topology};

	vk::PipelineTessellationStateCreateInfo tessellation_state{.patchControlPoints = patch_control_points};

	vk::PipelineViewportStateCreateInfo viewport_state{.viewportCount = 1, .scissorCount = 1};

	vk::PipelineRasterizationStateCreateInfo rasterization_state{
	    .polygonMode = polygon_mode, .cullMode = cull_mode, .frontFace = front_face, .lineWidth = 1.0f};

	vk::PipelineMultisampleStateCreateInfo multisample_state{.rasterizationSamples = vk::SampleCountFlagBits::e1};

	vk::PipelineColorBlendStateCreateInfo color_blend_state{.attachmentCount =
	                                                            static_cast<uint32_t>(blend_attachment_states.size()),
	                                                        .pAttachments = blend_attachment_states.data()};

	std::array<vk::DynamicState, 2> dynamic_state_enables = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
	vk::PipelineDynamicStateCreateInfo dynamic_state{.dynamicStateCount =
	                                                     static_cast<uint32_t>(dynamic_state_enables.size()),
	                                                 .pDynamicStates = dynamic_state_enables.data()};

	// Final fullscreen composition pass pipeline
	vk::GraphicsPipelineCreateInfo pipeline_create_info{.stageCount = static_cast<uint32_t>(shader_stages.size()),
	                                                    .pStages    = shader_stages.data(),
	                                                    .pVertexInputState   = &vertex_input_state,
	                                                    .pInputAssemblyState = &input_assembly_state,
	                                                    .pTessellationState  = &tessellation_state,
	                                                    .pViewportState      = &viewport_state,
	                                                    .pRasterizationState = &rasterization_state,
	                                                    .pMultisampleState   = &multisample_state,
	                                                    .pDepthStencilState  = &depth_stencil_state,
	                                                    .pColorBlendState    = &color_blend_state,
	                                                    .pDynamicState       = &dynamic_state,
	                                                    .layout              = pipeline_layout,
	                                                    .renderPass          = render_pass,
	                                                    .basePipelineIndex   = -1};

	vk::Result   result;
	vk::Pipeline pipeline;
	std::tie(result, pipeline) = device.createGraphicsPipeline(pipeline_cache, pipeline_create_info);
	assert(result == vk::Result::eSuccess);
	return pipeline;
}

vk::ImageView create_image_view(vk::Device           device,
                                vk::Image            image,
                                vk::ImageViewType    view_type,
                                vk::Format           format,
                                vk::ImageAspectFlags aspect_mask,
                                uint32_t             base_mip_level,
                                uint32_t             level_count,
                                uint32_t             base_array_layer,
                                uint32_t             layer_count)
{
	vk::ImageViewCreateInfo image_view_create_info{.image            = image,
	                                               .viewType         = view_type,
	                                               .format           = format,
	                                               .subresourceRange = {.aspectMask     = aspect_mask,
	                                                                    .baseMipLevel   = base_mip_level,
	                                                                    .levelCount     = level_count,
	                                                                    .baseArrayLayer = base_array_layer,
	                                                                    .layerCount     = layer_count}};
	return device.createImageView(image_view_create_info);
}

vk::QueryPool create_query_pool(vk::Device                      device,
                                vk::QueryType                   query_type,
                                uint32_t                        query_count,
                                vk::QueryPipelineStatisticFlags pipeline_statistics)
{
	vk::QueryPoolCreateInfo query_pool_create_info{
	    .queryType = query_type, .queryCount = query_count, .pipelineStatistics = pipeline_statistics};
	return device.createQueryPool(query_pool_create_info);
}

vk::Sampler create_sampler(vk::Device             device,
                           vk::Filter             mag_filter,
                           vk::Filter             min_filter,
                           vk::SamplerMipmapMode  mipmap_mode,
                           vk::SamplerAddressMode sampler_address_mode,
                           float                  max_anisotropy,
                           float                  max_LOD)
{
	vk::SamplerCreateInfo sampler_create_info{.magFilter        = mag_filter,
	                                          .minFilter        = min_filter,
	                                          .mipmapMode       = mipmap_mode,
	                                          .addressModeU     = sampler_address_mode,
	                                          .addressModeV     = sampler_address_mode,
	                                          .addressModeW     = sampler_address_mode,
	                                          .anisotropyEnable = (1.0f < max_anisotropy),
	                                          .maxAnisotropy    = max_anisotropy,
	                                          .compareOp        = vk::CompareOp::eNever,
	                                          .minLod           = 0.0f,
	                                          .maxLod           = max_LOD,
	                                          .borderColor      = vk::BorderColor::eFloatOpaqueWhite};
	return device.createSampler(sampler_create_info);
}

vk::Sampler create_sampler(vk::PhysicalDevice     gpu,
                           vk::Device             device,
                           vk::Format             format,
                           vk::Filter             filter,
                           vk::SamplerAddressMode sampler_address_mode,
                           float                  max_anisotropy,
                           float                  max_LOD)
{
	const vk::FormatProperties fmt_props = gpu.getFormatProperties(format);

	bool has_linear_filter = !!(fmt_props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear);

	return create_sampler(device,
	                      has_linear_filter ? filter : vk::Filter::eNearest,
	                      has_linear_filter ? filter : vk::Filter::eNearest,
	                      has_linear_filter ? vk::SamplerMipmapMode::eLinear : vk::SamplerMipmapMode::eNearest,
	                      sampler_address_mode,
	                      max_anisotropy,
	                      max_LOD);
}

void image_layout_transition(vk::CommandBuffer                command_buffer,
                             vk::Image                        image,
                             vk::PipelineStageFlags           src_stage_mask,
                             vk::PipelineStageFlags           dst_stage_mask,
                             vk::AccessFlags                  src_access_mask,
                             vk::AccessFlags                  dst_access_mask,
                             vk::ImageLayout                  old_layout,
                             vk::ImageLayout                  new_layout,
                             vk::ImageSubresourceRange const &subresource_range)
{
	// Create an image barrier object
	vk::ImageMemoryBarrier image_memory_barrier{.srcAccessMask       = src_access_mask,
	                                            .dstAccessMask       = dst_access_mask,
	                                            .oldLayout           = old_layout,
	                                            .newLayout           = new_layout,
	                                            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
	                                            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
	                                            .image               = image,
	                                            .subresourceRange    = subresource_range};

	// Put barrier inside setup command buffer
	command_buffer.pipelineBarrier(src_stage_mask, dst_stage_mask, {}, {}, {}, {image_memory_barrier});
}

void image_layout_transition(vk::CommandBuffer         command_buffer,
                             vk::Image                 image,
                             vk::ImageLayout           old_layout,
                             vk::ImageLayout           new_layout,
                             vk::ImageSubresourceRange subresource_range)
{
	image_layout_transition(command_buffer,
	                        image,
	                        getPipelineStageFlags(old_layout),
	                        getPipelineStageFlags(new_layout),
	                        getAccessFlags(old_layout),
	                        getAccessFlags(new_layout),
	                        old_layout,
	                        new_layout,
	                        subresource_range);
}

void image_layout_transition(vk::CommandBuffer command_buffer,
                             vk::Image         image,
                             vk::ImageLayout   old_layout,
                             vk::ImageLayout   new_layout)
{
	vk::ImageSubresourceRange subresource_range = {
	    .aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1};
	image_layout_transition(command_buffer, image, old_layout, new_layout, subresource_range);
}

void image_layout_transition(vk::CommandBuffer                                                   command_buffer,
                             std::vector<std::pair<vk::Image, vk::ImageSubresourceRange>> const &imagesAndRanges,
                             vk::ImageLayout                                                     old_layout,
                             vk::ImageLayout                                                     new_layout)
{
	vk::AccessFlags src_access_mask = getAccessFlags(old_layout);
	vk::AccessFlags dst_access_mask = getAccessFlags(new_layout);

	// Create image barrier objects
	std::vector<vk::ImageMemoryBarrier> image_memory_barriers;
	image_memory_barriers.reserve(imagesAndRanges.size());
	for (auto const &[image, range] : imagesAndRanges)
	{
		image_memory_barriers.emplace_back(vk::ImageMemoryBarrier{.srcAccessMask       = src_access_mask,
		                                                          .dstAccessMask       = dst_access_mask,
		                                                          .oldLayout           = old_layout,
		                                                          .newLayout           = new_layout,
		                                                          .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
		                                                          .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
		                                                          .image               = image,
		                                                          .subresourceRange    = range});
	}

	// Put barriers inside setup command buffer
	command_buffer.pipelineBarrier(
	    getPipelineStageFlags(old_layout), getPipelineStageFlags(new_layout), {}, {}, {}, image_memory_barriers);
}

vk::ShaderModule load_shader(std::string const &filename, vk::Device device, vk::ShaderStageFlagBits stage)
{
	auto spirv = vkb::fs::read_shader_binary_u32(filename);
	return load_shader_from_vector(spirv, device);
}

vk::ShaderModule load_shader_from_vector(const std::vector<uint32_t> &spirv, vk::Device device)
{
	vk::ShaderModuleCreateInfo module_create_info{.codeSize = spirv.size() * sizeof(uint32_t), .pCode = spirv.data()};
	return device.createShaderModule(module_create_info);
}

void make_filters_valid(vk::PhysicalDevice     physical_device,
                        vk::Format             format,
                        vk::Filter            *filter,
                        vk::SamplerMipmapMode *mipmapMode)
{
	assert(filter);

	// Not all formats support linear filtering, so we need to adjust them if they don't
	if ((*filter == vk::Filter::eNearest) && (!mipmapMode || (*mipmapMode == vk::SamplerMipmapMode::eNearest)))
	{
		return;        // These must already be valid
	}

	vk::FormatProperties properties = physical_device.getFormatProperties(format);
	if (!(properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
	{
		*filter = vk::Filter::eNearest;
		if (mipmapMode)
		{
			*mipmapMode = vk::SamplerMipmapMode::eNearest;
		}
	}
}

vk::Format select_blendable_format(vk::PhysicalDevice             physical_device,
                                   std::vector<vk::Format> const &format_priority_list)
{
	for (auto const &format : format_priority_list)
	{
		vk::FormatProperties properties = physical_device.getFormatProperties(format);
		if (properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment)
		{
			return format;
		}
	}

	throw std::runtime_error("No suitable blendable format could be determined");
}

uint32_t select_queue_family_index(std::vector<vk::QueueFamilyProperties> const &queue_family_properties,
                                   vk::QueueFlagBits                             queue_flag)
{
	// Dedicated queue for compute
	// Try to find a queue family index that supports compute but not graphics
	if (queue_flag == vk::QueueFlagBits::eCompute)
	{
		auto propertyIt =
		    std::ranges::find_if(queue_family_properties, [queue_flag](vk::QueueFamilyProperties const &property) {
			    return (property.queueFlags & queue_flag) && !(property.queueFlags & vk::QueueFlagBits::eGraphics);
		    });
		if (propertyIt != queue_family_properties.end())
		{
			return static_cast<uint32_t>(std::distance(queue_family_properties.begin(), propertyIt));
		}
	}

	// Dedicated queue for transfer
	// Try to find a queue family index that supports transfer but not graphics and compute
	if (queue_flag == vk::QueueFlagBits::eTransfer)
	{
		auto propertyIt =
		    std::ranges::find_if(queue_family_properties, [queue_flag](vk::QueueFamilyProperties const &property) {
			    return (property.queueFlags & queue_flag) &&
			           !(property.queueFlags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute));
		    });
		if (propertyIt != queue_family_properties.end())
		{
			return static_cast<uint32_t>(std::distance(queue_family_properties.begin(), propertyIt));
		}
	}

	// For other queue types or if no separate compute or transfer queue is present, return the first one to support the requested flags
	auto propertyIt =
	    std::ranges::find_if(queue_family_properties, [queue_flag](vk::QueueFamilyProperties const &property) {
		    return (property.queueFlags & queue_flag) == queue_flag;
	    });
	if (propertyIt != queue_family_properties.end())
	{
		return static_cast<uint32_t>(std::distance(queue_family_properties.begin(), propertyIt));
	}

	throw std::runtime_error("Could not find a matching queue family index");
}

vk::Format select_suitable_depth_format(vk::PhysicalDevice             physical_device,
                                        bool                           depth_only,
                                        std::vector<vk::Format> const &depth_format_priority_list)
{
	for (auto format : depth_format_priority_list)
	{
		if (!depth_only || is_depth_only_format(format))
		{
			vk::FormatProperties properties = physical_device.getFormatProperties(format);

			// Format must support depth stencil attachment for optimal tiling
			if (properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
			{
				LOGI("Depth format selected: {}", vk::to_string(format));
				return format;
			}
		}
	}

	throw std::runtime_error("No suitable depth format could be determined");
}

vk::SurfaceFormatKHR select_surface_format(vk::PhysicalDevice             gpu,
                                           vk::SurfaceKHR                 surface,
                                           std::vector<vk::Format> const &format_priority_list)
{
	std::vector<vk::SurfaceFormatKHR> supported_surface_formats = gpu.getSurfaceFormatsKHR(surface);
	assert(!supported_surface_formats.empty());

	auto it =
	    std::ranges::find_if(supported_surface_formats, [&format_priority_list](vk::SurfaceFormatKHR surface_format) {
		    return std::ranges::any_of(
		        format_priority_list, [&surface_format](vk::Format format) { return format == surface_format.format; });
	    });

	// We use the first supported format as a fallback in case none of the preferred formats is available
	return it != supported_surface_formats.end() ? *it : supported_surface_formats[0];
}

void submit_and_wait(vk::Device                     device,
                     vk::Queue                      queue,
                     std::vector<vk::CommandBuffer> command_buffers,
                     std::vector<vk::Semaphore>     semaphores)
{
	// Submit command_buffer
	vk::SubmitInfo submit_info{.commandBufferCount   = static_cast<uint32_t>(command_buffers.size()),
	                           .pCommandBuffers      = command_buffers.data(),
	                           .signalSemaphoreCount = static_cast<uint32_t>(semaphores.size()),
	                           .pSignalSemaphores    = semaphores.data()};

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
