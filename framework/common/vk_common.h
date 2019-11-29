/* Copyright (c) 2018-2019, Arm Limited and Contributors
 * Copyright (c) 2019, Sascha Willems
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
 * @brief Helper function to determine a suitable supported depth format starting with 32 bit down to 16 bit
 * @param physical_device The physical device to check the depth formats against
 * @param depth_format The depth format (this can be modified)
 * @return false if none of the depth formats in the list is supported by the device
 */
VkBool32 get_supported_depth_format(VkPhysicalDevice physical_device, VkFormat *depth_format);

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

/**
 * @brief Helper function to convert a VkFormat enum to a string
 * @param format Vulkan format to convert.
 * @return The string to return.
 */
const std::string to_string(VkFormat format);

/**
 * @brief Helper function to convert a VkPresentModeKHR to a string
 * @param present_mode Vulkan present mode to convert.
 * @return The string to return.
 */
const std::string to_string(VkPresentModeKHR present_mode);

/**
 * @brief Helper function to convert a VkResult enum to a string
 * @param format Vulkan result to convert.
 * @return The string to return.
 */
const std::string to_string(VkResult result);

/**
 * @brief Helper function to convert a VkPhysicalDeviceType enum to a string
 * @param format Vulkan physical device type to convert.
 * @return The string to return.
 */
const std::string to_string(VkPhysicalDeviceType type);

/**
 * @brief Helper function to convert a VkSurfaceTransformFlagBitsKHR flag to a string
 * @param transform_flag Vulkan surface transform flag bit to convert.
 * @return The string to return.
 */
const std::string to_string(VkSurfaceTransformFlagBitsKHR transform_flag);

/**
 * @brief Helper function to convert a VkSurfaceFormatKHR format to a string
 * @param surface_format Vulkan surface format to convert.
 * @return The string to return.
 */
const std::string to_string(VkSurfaceFormatKHR surface_format);

/**
 * @brief Helper function to convert a VkCompositeAlphaFlagBitsKHR flag to a string
 * @param composite_alpha Vulkan composite alpha flag bit to convert.
 * @return The string to return.
 */
const std::string to_string(VkCompositeAlphaFlagBitsKHR composite_alpha);

/**
 * @brief Helper function to convert a VkImageUsageFlagBits flag to a string
 * @param image_usage Vulkan image usage flag bit to convert.
 * @return The string to return.
 */
const std::string to_string(VkImageUsageFlagBits image_usage);

/**
 * @brief Helper function to convert a VkExtent2D flag to a string
 * @param extent Vulkan extent to convert.
 * @return The string to return.
 */
const std::string to_string(VkExtent2D extent);

/**
 * @brief Helper function to create a VkShaderModule
 * @param filename The shader location
 * @param device The logical device
 * @param stage The shader stage
 * @return The string to return.
 */
VkShaderModule load_shader(const std::string &filename, VkDevice device, VkShaderStageFlagBits stage);

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

}        // namespace vkb
