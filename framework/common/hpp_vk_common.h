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
 * @brief facade helper functions around the functions in common/vk_commmon, providing a vulkan.hpp-based interface
 */
vk::Format get_suitable_depth_format(vk::PhysicalDevice             physical_device,
                                     bool                           depth_only                 = false,
                                     const std::vector<vk::Format> &depth_format_priority_list = {
                                         vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint, vk::Format::eD16Unorm})
{
	std::vector<VkFormat> dfpl;
	dfpl.reserve(depth_format_priority_list.size());
	for (auto const &format : depth_format_priority_list)
	{
		dfpl.push_back(static_cast<VkFormat>(format));
	}
	return static_cast<vk::Format>(vkb::get_suitable_depth_format(physical_device, depth_only, dfpl));
}

bool is_depth_only_format(vk::Format format)
{
	return format == vk::Format::eD16Unorm || format == vk::Format::eD32Sfloat;
}

bool is_depth_stencil_format(vk::Format format)
{
	return format == vk::Format::eD16UnormS8Uint || format == vk::Format::eD24UnormS8Uint || format == vk::Format::eD32SfloatS8Uint ||
	       is_depth_only_format(format);
}

vk::ShaderModule load_shader(const std::string &filename, vk::Device device, vk::ShaderStageFlagBits stage)
{
	return vkb::load_shader(filename, device, static_cast<VkShaderStageFlagBits>(stage));
}

vk::ImageLayout map_descriptor_type_to_image_layout(vk::DescriptorType descriptor_type, vk::Format format)
{
	switch (descriptor_type)
	{
		case vk::DescriptorType::eCombinedImageSampler:
		case vk::DescriptorType::eInputAttachment:
			if (is_depth_stencil_format(format))
			{
				return vk::ImageLayout::eDepthStencilReadOnlyOptimal;
			}
			else
			{
				return vk::ImageLayout::eShaderReadOnlyOptimal;
			}
			break;
		case vk::DescriptorType::eStorageImage:
			return vk::ImageLayout::eGeneral;
			break;
		default:
			return vk::ImageLayout::eUndefined;
			break;
	}
}

void set_image_layout(vk::CommandBuffer         command_buffer,
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
