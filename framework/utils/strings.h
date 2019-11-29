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

#include <string>
#include <unordered_map>

#include <volk.h>

namespace vkb
{
enum class ShaderResourceType;

namespace sg
{
enum class AlphaMode;
}

namespace utils
{
extern std::string vk_result_to_string(VkResult result);

extern std::string to_string(VkFormat format);

extern std::string to_string(VkSampleCountFlagBits flags);

extern std::string to_string_shader_stage_flags(VkShaderStageFlags flags);

extern std::string to_string(VkPhysicalDeviceType type);

extern std::string to_string(VkSurfaceTransformFlagBitsKHR flags);

extern std::string to_string(VkPresentModeKHR mode);

extern std::string to_string_vk_image_usage_flags(VkImageUsageFlags flags);

extern std::string to_string_vk_image_aspect_flags(VkImageAspectFlags flags);

extern std::string to_string(VkImageTiling tiling);

extern std::string to_string(VkImageType type);

extern std::string to_string(VkExtent2D format);

extern std::string to_string(VkBlendFactor blend);

extern std::string to_string(VkVertexInputRate rate);

extern std::string to_string_vk_bool(VkBool32 state);

extern std::string to_string(VkPrimitiveTopology topology);

extern std::string to_string(VkFrontFace face);

extern std::string to_string(VkPolygonMode mode);

extern std::string to_string_vk_cull_mode_flags(VkCullModeFlags flags);

extern std::string to_string(VkCompareOp operation);

extern std::string to_string(VkStencilOp operation);

extern std::string to_string(VkLogicOp operation);

extern std::string to_string(VkBlendOp operation);

extern std::string to_string_vk_color_component_flags(VkColorComponentFlags operation);

extern std::string to_string(sg::AlphaMode mode);

extern std::string to_string(bool flag);

extern std::string to_string(ShaderResourceType type);

extern std::unordered_map<VkFormat, std::string> vk_format_strings;

}        // namespace utils
}        // namespace vkb