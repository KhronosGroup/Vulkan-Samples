/* Copyright (c) 2018-2023, Arm Limited and Contributors
 * Copyright (c) 2019-2023, Sascha Willems
 * Copyright (c) 2023, Mobica Limited
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

#include <cstdio>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <vk_mem_alloc.h>
#include <volk.h>

#define VK_FLAGS_NONE 0        // Custom define for better code readability

#define DEFAULT_FENCE_TIMEOUT 100000000000        // Default fence timeout in nanoseconds

template <class T>
using ShaderStageMap = std::map<VkShaderStageFlagBits, T>;

template <class T>
using BindingMap = std::map<uint32_t, std::map<uint32_t, T>>;

namespace vkb
{
/**
 * @brief Helper function to determine if a Vulkan format is depth only.
 * @param format Vulkan format to check.
 * @return True if format is a depth only, false otherwise.
 */
bool is_depth_only_format(VkFormat format);

/**
 * @brief Helper function to determine if a Vulkan format is depth or stencil.
 * @param format Vulkan format to check.
 * @return True if format is a depth or stencil, false otherwise.
 */
bool is_depth_stencil_format(VkFormat format);

/**
 * @brief Helper function to determine a suitable supported depth format based on a priority list
 * @param physical_device The physical device to check the depth formats against
 * @param depth_only (Optional) Wether to include the stencil component in the format or not
 * @param depth_format_priority_list (Optional) The list of depth formats to prefer over one another
 *		  By default we start with the highest precision packed format
 * @return The valid suited depth format
 */
VkFormat get_suitable_depth_format(VkPhysicalDevice             physical_device,
                                   bool                         depth_only                 = false,
                                   const std::vector<VkFormat> &depth_format_priority_list = {
                                       VK_FORMAT_D32_SFLOAT,
                                       VK_FORMAT_D24_UNORM_S8_UINT,
                                       VK_FORMAT_D16_UNORM});

/**
 * @brief Helper function to determine if a Vulkan descriptor type is a dynamic storage buffer or dynamic uniform buffer.
 * @param descriptor_type Vulkan descriptor type to check.
 * @return True if type is dynamic buffer, false otherwise.
 */
bool is_dynamic_buffer_descriptor_type(VkDescriptorType descriptor_type);

/**
 * @brief Helper function to determine if a Vulkan descriptor type is a buffer (either uniform or storage buffer, dynamic or not).
 * @param descriptor_type Vulkan descriptor type to check.
 * @return True if type is buffer, false otherwise.
 */
bool is_buffer_descriptor_type(VkDescriptorType descriptor_type);

/**
 * @brief Helper function to get the bits per pixel of a Vulkan format.
 * @param format Vulkan format to check.
 * @return The bits per pixel of the given format, -1 for invalid formats.
 */
int32_t get_bits_per_pixel(VkFormat format);

enum class ShaderSourceLanguage
{
	GLSL,
	HLSL,
	SPV,
};

/**
 * @brief Helper function to create a VkShaderModule
 * @param filename The shader location
 * @param device The logical device
 * @param stage The shader stage
 * @param src_language The shader language
 * @return The string to return.
 */
VkShaderModule load_shader(const std::string &filename, VkDevice device, VkShaderStageFlagBits stage, ShaderSourceLanguage src_language = ShaderSourceLanguage::GLSL);

/**
 * @brief Image memory barrier structure used to define
 *        memory access for an image view during command recording.
 */
struct ImageMemoryBarrier
{
	VkPipelineStageFlags src_stage_mask{VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};

	VkPipelineStageFlags dst_stage_mask{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};

	VkAccessFlags src_access_mask{0};

	VkAccessFlags dst_access_mask{0};

	VkImageLayout old_layout{VK_IMAGE_LAYOUT_UNDEFINED};

	VkImageLayout new_layout{VK_IMAGE_LAYOUT_UNDEFINED};

	uint32_t old_queue_family{VK_QUEUE_FAMILY_IGNORED};

	uint32_t new_queue_family{VK_QUEUE_FAMILY_IGNORED};
};

/**
 * @brief Buffer memory barrier structure used to define
 *        memory access for a buffer during command recording.
 */
struct BufferMemoryBarrier
{
	VkPipelineStageFlags src_stage_mask{VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};

	VkPipelineStageFlags dst_stage_mask{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};

	VkAccessFlags src_access_mask{0};

	VkAccessFlags dst_access_mask{0};
};

/**
 * @brief Put an image memory barrier for setting an image layout on the sub resource into the given command buffer
 */
void set_image_layout(
    VkCommandBuffer         command_buffer,
    VkImage                 image,
    VkImageLayout           old_layout,
    VkImageLayout           new_layout,
    VkImageSubresourceRange subresource_range,
    VkPipelineStageFlags    src_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VkPipelineStageFlags    dst_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

/**
 * @brief Uses a fixed sub resource layout with first mip level and layer
 */
void set_image_layout(
    VkCommandBuffer      command_buffer,
    VkImage              image,
    VkImageAspectFlags   aspect_mask,
    VkImageLayout        old_layout,
    VkImageLayout        new_layout,
    VkPipelineStageFlags src_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VkPipelineStageFlags dst_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

/**
 * @brief Insert an image memory barrier into the command buffer
 */
void insert_image_memory_barrier(
    VkCommandBuffer         command_buffer,
    VkImage                 image,
    VkAccessFlags           src_access_mask,
    VkAccessFlags           dst_access_mask,
    VkImageLayout           old_layout,
    VkImageLayout           new_layout,
    VkPipelineStageFlags    src_stage_mask,
    VkPipelineStageFlags    dst_stage_mask,
    VkImageSubresourceRange subresource_range);

/**
 * @brief Load and store info for a render pass attachment.
 */
struct LoadStoreInfo
{
	VkAttachmentLoadOp load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;

	VkAttachmentStoreOp store_op = VK_ATTACHMENT_STORE_OP_STORE;
};

namespace gbuffer
{
/**
 * @return Load store info to load all and store only the swapchain
 */
std::vector<LoadStoreInfo> get_load_all_store_swapchain();

/**
 * @return Load store info to clear all and store only the swapchain
 */
std::vector<LoadStoreInfo> get_clear_all_store_swapchain();

/**
 * @return Load store info to clear and store all images
 */
std::vector<LoadStoreInfo> get_clear_store_all();

/**
 * @return Default clear values for the G-buffer
 */
std::vector<VkClearValue> get_clear_value();
}        // namespace gbuffer

}        // namespace vkb
