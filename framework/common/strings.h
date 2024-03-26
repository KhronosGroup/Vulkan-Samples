/* Copyright (c) 2018-2024, Arm Limited and Contributors
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

#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <volk.h>

namespace vkb
{
enum class ShaderResourceType;

namespace sg
{
enum class AlphaMode;
}

std::vector<std::string> split(const std::string &str, const std::string &delimiter);
std::string              join(const std::vector<std::string> &str, const std::string &separator);

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
 * @param result Vulkan result to convert.
 * @return The string to return.
 */
const std::string to_string(VkResult result);

/**
 * @brief Helper function to convert a VkPhysicalDeviceType enum to a string
 * @param type Vulkan physical device type to convert.
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
 * @param format Vulkan format to convert.
 * @return The string to return.
 */
std::string to_string(VkExtent2D format);

/**
 * @brief Helper function to convert VkSampleCountFlagBits to a string
 * @param flags Vulkan sample count flags to convert
 * @return const std::string
 */
const std::string to_string(VkSampleCountFlagBits flags);

/**
 * @brief Helper function to convert VkImageTiling to a string
 * @param tiling Vulkan VkImageTiling to convert
 * @return The string to return
 */
const std::string to_string(VkImageTiling tiling);

/**
 * @brief Helper function to convert VkImageType to a string
 * @param type Vulkan VkImageType to convert
 * @return The string to return
 */
const std::string to_string(VkImageType type);

/**
 * @brief Helper function to convert VkBlendFactor to a string
 * @param blend Vulkan VkBlendFactor to convert
 * @return The string to return
 */
const std::string to_string(VkBlendFactor blend);

/**
 * @brief Helper function to convert VkVertexInputRate to a string
 * @param rate Vulkan VkVertexInputRate to convert
 * @return The string to return
 */
const std::string to_string(VkVertexInputRate rate);

/**
 * @brief Helper function to convert VkBool32 to a string
 * @param state Vulkan VkBool32 to convert
 * @return The string to return
 */
const std::string to_string_vk_bool(VkBool32 state);

/**
 * @brief Helper function to convert VkPrimitiveTopology to a string
 * @param topology Vulkan VkPrimitiveTopology to convert
 * @return The string to return
 */
const std::string to_string(VkPrimitiveTopology topology);

/**
 * @brief Helper function to convert VkFrontFace to a string
 * @param face Vulkan VkFrontFace to convert
 * @return The string to return
 */
const std::string to_string(VkFrontFace face);

/**
 * @brief Helper function to convert VkPolygonMode to a string
 * @param mode Vulkan VkPolygonMode to convert
 * @return The string to return
 */
const std::string to_string(VkPolygonMode mode);

/**
 * @brief Helper function to convert VkCompareOp to a string
 * @param operation Vulkan VkCompareOp to convert
 * @return The string to return
 */
const std::string to_string(VkCompareOp operation);

/**
 * @brief Helper function to convert VkStencilOp to a string
 * @param operation Vulkan VkStencilOp to convert
 * @return The string to return
 */
const std::string to_string(VkStencilOp operation);

/**
 * @brief Helper function to convert VkLogicOp to a string
 * @param operation Vulkan VkLogicOp to convert
 * @return The string to return
 */
const std::string to_string(VkLogicOp operation);

/**
 * @brief Helper function to convert VkBlendOp to a string
 * @param operation Vulkan VkBlendOp to convert
 * @return The string to return
 */
const std::string to_string(VkBlendOp operation);

/**
 * @brief Helper function to convert AlphaMode to a string
 * @param mode Vulkan AlphaMode to convert
 * @return The string to return
 */
const std::string to_string(sg::AlphaMode mode);

/**
 * @brief Helper function to convert bool to a string
 * @param flag Vulkan bool to convert (true/false)
 * @return The string to return
 */
const std::string to_string(bool flag);

/**
 * @brief Helper function to convert ShaderResourceType to a string
 * @param type Vulkan ShaderResourceType to convert
 * @return The string to return
 */
const std::string to_string(ShaderResourceType type);

/**
 * @brief Helper generic function to convert a bitmask to a string of its components
 * @param bitmask The bitmask to convert
 * @param string_map A map of bitmask bits to the string that describe the Vulkan flag
 * @returns A string of the enabled bits in the bitmask
 */
template <typename T>
inline const std::string to_string(uint32_t bitmask, const std::map<T, const char *> string_map)
{
	std::stringstream result;
	bool              append = false;
	for (const auto &s : string_map)
	{
		if (bitmask & s.first)
		{
			if (append)
			{
				result << " | ";
			}
			result << s.second;
			append = true;
		}
	}
	return result.str();
}

/**
 * @brief Helper function to convert VkBufferUsageFlags to a string
 * @param bitmask The buffer usage bitmask to convert to strings
 * @return The converted string to return
 */
const std::string buffer_usage_to_string(VkBufferUsageFlags bitmask);

/**
 * @brief Helper function to convert VkShaderStageFlags to a string
 * @param bitmask The shader stage bitmask to convert
 * @return The converted string to return
 */
const std::string shader_stage_to_string(VkShaderStageFlags bitmask);

/**
 * @brief Helper function to convert VkImageUsageFlags to a string
 * @param bitmask The image usage bitmask to convert
 * @return The converted string to return
 */
const std::string image_usage_to_string(VkImageUsageFlags bitmask);

/**
 * @brief Helper function to convert VkImageAspectFlags to a string
 * @param bitmask The image aspect bitmask to convert
 * @return The converted string to return
 */
const std::string image_aspect_to_string(VkImageAspectFlags bitmask);

/**
 * @brief Helper function to convert VkCullModeFlags to a string
 * @param bitmask The cull mode bitmask to convert
 * @return The converted string to return
 */
const std::string cull_mode_to_string(VkCullModeFlags bitmask);

/**
 * @brief Helper function to convert VkColorComponentFlags to a string
 * @param bitmask The color component bitmask to convert
 * @return The converted string to return
 */
const std::string color_component_to_string(VkColorComponentFlags bitmask);

/**
 * @brief Helper function to convert VkImageCompressionFlagsEXT to a string
 * @param flags The flags to convert
 * @return The converted string to return
 */
const std::string image_compression_flags_to_string(VkImageCompressionFlagsEXT flags);

/**
 * @brief Helper function to convert VkImageCompressionFixedRateFlagsEXT to a string
 * @param flags The flags to convert
 * @return The converted string to return
 */
const std::string image_compression_fixed_rate_flags_to_string(VkImageCompressionFixedRateFlagsEXT flags);

/**
 * @brief Helper function to split a single string into a vector of strings by a delimiter
 * @param input The input string to be split
 * @param delim The character to delimit by
 * @return The vector of tokenized strings
 */
std::vector<std::string> split(const std::string &input, char delim);
}        // namespace vkb
