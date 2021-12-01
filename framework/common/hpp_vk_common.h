/* Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
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

#include <common/vk_common.h>
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace common
{
/**
 * @brief facade helper functions and structs around the functions and structs in common/vk_commmon, providing a vulkan.hpp-based interface
 */
struct HPPImageMemoryBarrier
{
	vk::PipelineStageFlags src_stage_mask{vk::PipelineStageFlagBits::eBottomOfPipe};

	vk::PipelineStageFlags dst_stage_mask{vk::PipelineStageFlagBits::eTopOfPipe};

	vk::AccessFlags src_access_mask{};

	vk::AccessFlags dst_access_mask{};

	vk::ImageLayout old_layout{vk::ImageLayout::eUndefined};

	vk::ImageLayout new_layout{vk::ImageLayout::eUndefined};

	uint32_t old_queue_family{VK_QUEUE_FAMILY_IGNORED};

	uint32_t new_queue_family{VK_QUEUE_FAMILY_IGNORED};
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

inline bool is_depth_only_format(vk::Format format)
{
	return vkb::is_depth_only_format(static_cast<VkFormat>(format));
}

inline bool is_depth_stencil_format(vk::Format format)
{
	return vkb::is_depth_stencil_format(static_cast<VkFormat>(format));
}

inline vk::ShaderModule load_shader(const std::string &filename, vk::Device device, vk::ShaderStageFlagBits stage)
{
	return vkb::load_shader(filename, device, static_cast<VkShaderStageFlagBits>(stage));
}

inline void set_image_layout(vk::CommandBuffer         command_buffer,
                             vk::Image                 image,
                             vk::ImageLayout           old_layout,
                             vk::ImageLayout           new_layout,
                             vk::ImageSubresourceRange subresource_range,
                             vk::PipelineStageFlags    src_mask = vk::PipelineStageFlagBits::eAllCommands,
                             vk::PipelineStageFlags    dst_mask = vk::PipelineStageFlagBits::eAllCommands)
{
	vkb::set_image_layout(command_buffer,
	                      image,
	                      static_cast<VkImageLayout>(old_layout),
	                      static_cast<VkImageLayout>(new_layout),
	                      subresource_range,
	                      static_cast<VkPipelineStageFlags>(src_mask),
	                      static_cast<VkPipelineStageFlags>(dst_mask));
}

}        // namespace common
}        // namespace vkb
