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

#include "strings.h"

#include <spdlog/fmt/fmt.h>

#include "core/shader_module.h"
#include "scene_graph/components/material.h"

namespace vkb
{
namespace utils
{
std::string vk_result_to_string(VkResult result)
{
	if (result == VK_SUCCESS)
	{
		return "VK_SUCCESS";
	}
	if (result == VK_NOT_READY)
	{
		return "VK_NOT_READY";
	}
	if (result == VK_TIMEOUT)
	{
		return "VK_TIMEOUT";
	}
	if (result == VK_EVENT_SET)
	{
		return "VK_EVENT_SET";
	}
	if (result == VK_EVENT_RESET)
	{
		return "VK_EVENT_RESET";
	}
	if (result == VK_INCOMPLETE)
	{
		return "VK_INCOMPLETE";
	}
	if (result == VK_ERROR_OUT_OF_HOST_MEMORY)
	{
		return "VK_ERROR_OUT_OF_HOST_MEMORY";
	}
	if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
	{
		return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
	}
	if (result == VK_ERROR_INITIALIZATION_FAILED)
	{
		return "VK_ERROR_INITIALIZATION_FAILED";
	}
	if (result == VK_ERROR_DEVICE_LOST)
	{
		return "VK_ERROR_DEVICE_LOST";
	}
	if (result == VK_ERROR_MEMORY_MAP_FAILED)
	{
		return "VK_ERROR_MEMORY_MAP_FAILED";
	}
	if (result == VK_ERROR_LAYER_NOT_PRESENT)
	{
		return "VK_ERROR_LAYER_NOT_PRESENT";
	}
	if (result == VK_ERROR_EXTENSION_NOT_PRESENT)
	{
		return "VK_ERROR_EXTENSION_NOT_PRESENT";
	}
	if (result == VK_ERROR_FEATURE_NOT_PRESENT)
	{
		return "VK_ERROR_FEATURE_NOT_PRESENT";
	}
	if (result == VK_ERROR_INCOMPATIBLE_DRIVER)
	{
		return "VK_ERROR_INCOMPATIBLE_DRIVER";
	}
	if (result == VK_ERROR_TOO_MANY_OBJECTS)
	{
		return "VK_ERROR_TOO_MANY_OBJECTS";
	}
	if (result == VK_ERROR_FORMAT_NOT_SUPPORTED)
	{
		return "VK_ERROR_FORMAT_NOT_SUPPORTED";
	}
	if (result == VK_ERROR_FRAGMENTED_POOL)
	{
		return "VK_ERROR_FRAGMENTED_POOL";
	}
	if (result == VK_ERROR_OUT_OF_POOL_MEMORY)
	{
		return "VK_ERROR_OUT_OF_POOL_MEMORY";
	}
	if (result == VK_ERROR_INVALID_EXTERNAL_HANDLE)
	{
		return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
	}
	if (result == VK_ERROR_SURFACE_LOST_KHR)
	{
		return "VK_ERROR_SURFACE_LOST_KHR";
	}
	if (result == VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
	{
		return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
	}
	if (result == VK_SUBOPTIMAL_KHR)
	{
		return "VK_SUBOPTIMAL_KHR";
	}
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		return "VK_ERROR_OUT_OF_DATE_KHR";
	}
	if (result == VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)
	{
		return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
	}
	if (result == VK_ERROR_VALIDATION_FAILED_EXT)
	{
		return "VK_ERROR_VALIDATION_FAILED_EXT";
	}
	if (result == VK_ERROR_INVALID_SHADER_NV)
	{
		return "VK_ERROR_INVALID_SHADER_NV";
	}
	if (result == VK_ERROR_NOT_PERMITTED_EXT)
	{
		return "VK_ERROR_NOT_PERMITTED_EXT";
	}
	return "UNKNOWN";
}

std::unordered_map<VkFormat, std::string> vk_format_strings{
    {VK_FORMAT_UNDEFINED, "VK_FORMAT_UNDEFINED"},
    {VK_FORMAT_R4G4_UNORM_PACK8, "VK_FORMAT_R4G4_UNORM_PACK8"},
    {VK_FORMAT_R4G4B4A4_UNORM_PACK16, "VK_FORMAT_R4G4B4A4_UNORM_PACK16"},
    {VK_FORMAT_B4G4R4A4_UNORM_PACK16, "VK_FORMAT_B4G4R4A4_UNORM_PACK16"},
    {VK_FORMAT_R5G6B5_UNORM_PACK16, "VK_FORMAT_R5G6B5_UNORM_PACK16"},
    {VK_FORMAT_B5G6R5_UNORM_PACK16, "VK_FORMAT_B5G6R5_UNORM_PACK16"},
    {VK_FORMAT_R5G5B5A1_UNORM_PACK16, "VK_FORMAT_R5G5B5A1_UNORM_PACK16"},
    {VK_FORMAT_B5G5R5A1_UNORM_PACK16, "VK_FORMAT_B5G5R5A1_UNORM_PACK16"},
    {VK_FORMAT_A1R5G5B5_UNORM_PACK16, "VK_FORMAT_A1R5G5B5_UNORM_PACK16"},
    {VK_FORMAT_R8_UNORM, "VK_FORMAT_R8_UNORM"},
    {VK_FORMAT_R8_SNORM, "VK_FORMAT_R8_SNORM"},
    {VK_FORMAT_R8_USCALED, "VK_FORMAT_R8_USCALED"},
    {VK_FORMAT_R8_SSCALED, "VK_FORMAT_R8_SSCALED"},
    {VK_FORMAT_R8_UINT, "VK_FORMAT_R8_UINT"},
    {VK_FORMAT_R8_SINT, "VK_FORMAT_R8_SINT"},
    {VK_FORMAT_R8_SRGB, "VK_FORMAT_R8_SRGB"},
    {VK_FORMAT_R8G8_UNORM, "VK_FORMAT_R8G8_UNORM"},
    {VK_FORMAT_R8G8_SNORM, "VK_FORMAT_R8G8_SNORM"},
    {VK_FORMAT_R8G8_USCALED, "VK_FORMAT_R8G8_USCALED"},
    {VK_FORMAT_R8G8_SSCALED, "VK_FORMAT_R8G8_SSCALED"},
    {VK_FORMAT_R8G8_UINT, "VK_FORMAT_R8G8_UINT"},
    {VK_FORMAT_R8G8_SINT, "VK_FORMAT_R8G8_SINT"},
    {VK_FORMAT_R8G8_SRGB, "VK_FORMAT_R8G8_SRGB"},
    {VK_FORMAT_R8G8B8_UNORM, "VK_FORMAT_R8G8B8_UNORM"},
    {VK_FORMAT_R8G8B8_SNORM, "VK_FORMAT_R8G8B8_SNORM"},
    {VK_FORMAT_R8G8B8_USCALED, "VK_FORMAT_R8G8B8_USCALED"},
    {VK_FORMAT_R8G8B8_SSCALED, "VK_FORMAT_R8G8B8_SSCALED"},
    {VK_FORMAT_R8G8B8_UINT, "VK_FORMAT_R8G8B8_UINT"},
    {VK_FORMAT_R8G8B8_SINT, "VK_FORMAT_R8G8B8_SINT"},
    {VK_FORMAT_R8G8B8_SRGB, "VK_FORMAT_R8G8B8_SRGB"},
    {VK_FORMAT_B8G8R8_UNORM, "VK_FORMAT_B8G8R8_UNORM"},
    {VK_FORMAT_B8G8R8_SNORM, "VK_FORMAT_B8G8R8_SNORM"},
    {VK_FORMAT_B8G8R8_USCALED, "VK_FORMAT_B8G8R8_USCALED"},
    {VK_FORMAT_B8G8R8_SSCALED, "VK_FORMAT_B8G8R8_SSCALED"},
    {VK_FORMAT_B8G8R8_UINT, "VK_FORMAT_B8G8R8_UINT"},
    {VK_FORMAT_B8G8R8_SINT, "VK_FORMAT_B8G8R8_SINT"},
    {VK_FORMAT_B8G8R8_SRGB, "VK_FORMAT_B8G8R8_SRGB"},
    {VK_FORMAT_R8G8B8A8_UNORM, "VK_FORMAT_R8G8B8A8_UNORM"},
    {VK_FORMAT_R8G8B8A8_SNORM, "VK_FORMAT_R8G8B8A8_SNORM"},
    {VK_FORMAT_R8G8B8A8_USCALED, "VK_FORMAT_R8G8B8A8_USCALED"},
    {VK_FORMAT_R8G8B8A8_SSCALED, "VK_FORMAT_R8G8B8A8_SSCALED"},
    {VK_FORMAT_R8G8B8A8_UINT, "VK_FORMAT_R8G8B8A8_UINT"},
    {VK_FORMAT_R8G8B8A8_SINT, "VK_FORMAT_R8G8B8A8_SINT"},
    {VK_FORMAT_R8G8B8A8_SRGB, "VK_FORMAT_R8G8B8A8_SRGB"},
    {VK_FORMAT_B8G8R8A8_UNORM, "VK_FORMAT_B8G8R8A8_UNORM"},
    {VK_FORMAT_B8G8R8A8_SNORM, "VK_FORMAT_B8G8R8A8_SNORM"},
    {VK_FORMAT_B8G8R8A8_USCALED, "VK_FORMAT_B8G8R8A8_USCALED"},
    {VK_FORMAT_B8G8R8A8_SSCALED, "VK_FORMAT_B8G8R8A8_SSCALED"},
    {VK_FORMAT_B8G8R8A8_UINT, "VK_FORMAT_B8G8R8A8_UINT"},
    {VK_FORMAT_B8G8R8A8_SINT, "VK_FORMAT_B8G8R8A8_SINT"},
    {VK_FORMAT_B8G8R8A8_SRGB, "VK_FORMAT_B8G8R8A8_SRGB"},
    {VK_FORMAT_A8B8G8R8_UNORM_PACK32, "VK_FORMAT_A8B8G8R8_UNORM_PACK32"},
    {VK_FORMAT_A8B8G8R8_SNORM_PACK32, "VK_FORMAT_A8B8G8R8_SNORM_PACK32"},
    {VK_FORMAT_A8B8G8R8_USCALED_PACK32, "VK_FORMAT_A8B8G8R8_USCALED_PACK32"},
    {VK_FORMAT_A8B8G8R8_SSCALED_PACK32, "VK_FORMAT_A8B8G8R8_SSCALED_PACK32"},
    {VK_FORMAT_A8B8G8R8_UINT_PACK32, "VK_FORMAT_A8B8G8R8_UINT_PACK32"},
    {VK_FORMAT_A8B8G8R8_SINT_PACK32, "VK_FORMAT_A8B8G8R8_SINT_PACK32"},
    {VK_FORMAT_A8B8G8R8_SRGB_PACK32, "VK_FORMAT_A8B8G8R8_SRGB_PACK32"},
    {VK_FORMAT_A2R10G10B10_UNORM_PACK32, "VK_FORMAT_A2R10G10B10_UNORM_PACK32"},
    {VK_FORMAT_A2R10G10B10_SNORM_PACK32, "VK_FORMAT_A2R10G10B10_SNORM_PACK32"},
    {VK_FORMAT_A2R10G10B10_USCALED_PACK32, "VK_FORMAT_A2R10G10B10_USCALED_PACK32"},
    {VK_FORMAT_A2R10G10B10_SSCALED_PACK32, "VK_FORMAT_A2R10G10B10_SSCALED_PACK32"},
    {VK_FORMAT_A2R10G10B10_UINT_PACK32, "VK_FORMAT_A2R10G10B10_UINT_PACK32"},
    {VK_FORMAT_A2R10G10B10_SINT_PACK32, "VK_FORMAT_A2R10G10B10_SINT_PACK32"},
    {VK_FORMAT_A2B10G10R10_UNORM_PACK32, "VK_FORMAT_A2B10G10R10_UNORM_PACK32"},
    {VK_FORMAT_A2B10G10R10_SNORM_PACK32, "VK_FORMAT_A2B10G10R10_SNORM_PACK32"},
    {VK_FORMAT_A2B10G10R10_USCALED_PACK32, "VK_FORMAT_A2B10G10R10_USCALED_PACK32"},
    {VK_FORMAT_A2B10G10R10_SSCALED_PACK32, "VK_FORMAT_A2B10G10R10_SSCALED_PACK32"},
    {VK_FORMAT_A2B10G10R10_UINT_PACK32, "VK_FORMAT_A2B10G10R10_UINT_PACK32"},
    {VK_FORMAT_A2B10G10R10_SINT_PACK32, "VK_FORMAT_A2B10G10R10_SINT_PACK32"},
    {VK_FORMAT_R16_UNORM, "VK_FORMAT_R16_UNORM"},
    {VK_FORMAT_R16_SNORM, "VK_FORMAT_R16_SNORM"},
    {VK_FORMAT_R16_USCALED, "VK_FORMAT_R16_USCALED"},
    {VK_FORMAT_R16_SSCALED, "VK_FORMAT_R16_SSCALED"},
    {VK_FORMAT_R16_UINT, "VK_FORMAT_R16_UINT"},
    {VK_FORMAT_R16_SINT, "VK_FORMAT_R16_SINT"},
    {VK_FORMAT_R16_SFLOAT, "VK_FORMAT_R16_SFLOAT"},
    {VK_FORMAT_R16G16_UNORM, "VK_FORMAT_R16G16_UNORM"},
    {VK_FORMAT_R16G16_SNORM, "VK_FORMAT_R16G16_SNORM"},
    {VK_FORMAT_R16G16_USCALED, "VK_FORMAT_R16G16_USCALED"},
    {VK_FORMAT_R16G16_SSCALED, "VK_FORMAT_R16G16_SSCALED"},
    {VK_FORMAT_R16G16_UINT, "VK_FORMAT_R16G16_UINT"},
    {VK_FORMAT_R16G16_SINT, "VK_FORMAT_R16G16_SINT"},
    {VK_FORMAT_R16G16_SFLOAT, "VK_FORMAT_R16G16_SFLOAT"},
    {VK_FORMAT_R16G16B16_UNORM, "VK_FORMAT_R16G16B16_UNORM"},
    {VK_FORMAT_R16G16B16_SNORM, "VK_FORMAT_R16G16B16_SNORM"},
    {VK_FORMAT_R16G16B16_USCALED, "VK_FORMAT_R16G16B16_USCALED"},
    {VK_FORMAT_R16G16B16_SSCALED, "VK_FORMAT_R16G16B16_SSCALED"},
    {VK_FORMAT_R16G16B16_UINT, "VK_FORMAT_R16G16B16_UINT"},
    {VK_FORMAT_R16G16B16_SINT, "VK_FORMAT_R16G16B16_SINT"},
    {VK_FORMAT_R16G16B16_SFLOAT, "VK_FORMAT_R16G16B16_SFLOAT"},
    {VK_FORMAT_R16G16B16A16_UNORM, "VK_FORMAT_R16G16B16A16_UNORM"},
    {VK_FORMAT_R16G16B16A16_SNORM, "VK_FORMAT_R16G16B16A16_SNORM"},
    {VK_FORMAT_R16G16B16A16_USCALED, "VK_FORMAT_R16G16B16A16_USCALED"},
    {VK_FORMAT_R16G16B16A16_SSCALED, "VK_FORMAT_R16G16B16A16_SSCALED"},
    {VK_FORMAT_R16G16B16A16_UINT, "VK_FORMAT_R16G16B16A16_UINT"},
    {VK_FORMAT_R16G16B16A16_SINT, "VK_FORMAT_R16G16B16A16_SINT"},
    {VK_FORMAT_R16G16B16A16_SFLOAT, "VK_FORMAT_R16G16B16A16_SFLOAT"},
    {VK_FORMAT_R32_UINT, "VK_FORMAT_R32_UINT"},
    {VK_FORMAT_R32_SINT, "VK_FORMAT_R32_SINT"},
    {VK_FORMAT_R32_SFLOAT, "VK_FORMAT_R32_SFLOAT"},
    {VK_FORMAT_R32G32_UINT, "VK_FORMAT_R32G32_UINT"},
    {VK_FORMAT_R32G32_SINT, "VK_FORMAT_R32G32_SINT"},
    {VK_FORMAT_R32G32_SFLOAT, "VK_FORMAT_R32G32_SFLOAT"},
    {VK_FORMAT_R32G32B32_UINT, "VK_FORMAT_R32G32B32_UINT"},
    {VK_FORMAT_R32G32B32_SINT, "VK_FORMAT_R32G32B32_SINT"},
    {VK_FORMAT_R32G32B32_SFLOAT, "VK_FORMAT_R32G32B32_SFLOAT"},
    {VK_FORMAT_R32G32B32A32_UINT, "VK_FORMAT_R32G32B32A32_UINT"},
    {VK_FORMAT_R32G32B32A32_SINT, "VK_FORMAT_R32G32B32A32_SINT"},
    {VK_FORMAT_R32G32B32A32_SFLOAT, "VK_FORMAT_R32G32B32A32_SFLOAT"},
    {VK_FORMAT_R64_UINT, "VK_FORMAT_R64_UINT"},
    {VK_FORMAT_R64_SINT, "VK_FORMAT_R64_SINT"},
    {VK_FORMAT_R64_SFLOAT, "VK_FORMAT_R64_SFLOAT"},
    {VK_FORMAT_R64G64_UINT, "VK_FORMAT_R64G64_UINT"},
    {VK_FORMAT_R64G64_SINT, "VK_FORMAT_R64G64_SINT"},
    {VK_FORMAT_R64G64_SFLOAT, "VK_FORMAT_R64G64_SFLOAT"},
    {VK_FORMAT_R64G64B64_UINT, "VK_FORMAT_R64G64B64_UINT"},
    {VK_FORMAT_R64G64B64_SINT, "VK_FORMAT_R64G64B64_SINT"},
    {VK_FORMAT_R64G64B64_SFLOAT, "VK_FORMAT_R64G64B64_SFLOAT"},
    {VK_FORMAT_R64G64B64A64_UINT, "VK_FORMAT_R64G64B64A64_UINT"},
    {VK_FORMAT_R64G64B64A64_SINT, "VK_FORMAT_R64G64B64A64_SINT"},
    {VK_FORMAT_R64G64B64A64_SFLOAT, "VK_FORMAT_R64G64B64A64_SFLOAT"},
    {VK_FORMAT_B10G11R11_UFLOAT_PACK32, "VK_FORMAT_B10G11R11_UFLOAT_PACK32"},
    {VK_FORMAT_E5B9G9R9_UFLOAT_PACK32, "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32"},
    {VK_FORMAT_D16_UNORM, "VK_FORMAT_D16_UNORM"},
    {VK_FORMAT_X8_D24_UNORM_PACK32, "VK_FORMAT_X8_D24_UNORM_PACK32"},
    {VK_FORMAT_D32_SFLOAT, "VK_FORMAT_D32_SFLOAT"},
    {VK_FORMAT_S8_UINT, "VK_FORMAT_S8_UINT"},
    {VK_FORMAT_D16_UNORM_S8_UINT, "VK_FORMAT_D16_UNORM_S8_UINT"},
    {VK_FORMAT_D24_UNORM_S8_UINT, "VK_FORMAT_D24_UNORM_S8_UINT"},
    {VK_FORMAT_D32_SFLOAT_S8_UINT, "VK_FORMAT_D32_SFLOAT_S8_UINT"},
    {VK_FORMAT_BC1_RGB_UNORM_BLOCK, "VK_FORMAT_BC1_RGB_UNORM_BLOCK"},
    {VK_FORMAT_BC1_RGB_SRGB_BLOCK, "VK_FORMAT_BC1_RGB_SRGB_BLOCK"},
    {VK_FORMAT_BC1_RGBA_UNORM_BLOCK, "VK_FORMAT_BC1_RGBA_UNORM_BLOCK"},
    {VK_FORMAT_BC1_RGBA_SRGB_BLOCK, "VK_FORMAT_BC1_RGBA_SRGB_BLOCK"},
    {VK_FORMAT_BC2_UNORM_BLOCK, "VK_FORMAT_BC2_UNORM_BLOCK"},
    {VK_FORMAT_BC2_SRGB_BLOCK, "VK_FORMAT_BC2_SRGB_BLOCK"},
    {VK_FORMAT_BC3_UNORM_BLOCK, "VK_FORMAT_BC3_UNORM_BLOCK"},
    {VK_FORMAT_BC3_SRGB_BLOCK, "VK_FORMAT_BC3_SRGB_BLOCK"},
    {VK_FORMAT_BC4_UNORM_BLOCK, "VK_FORMAT_BC4_UNORM_BLOCK"},
    {VK_FORMAT_BC4_SNORM_BLOCK, "VK_FORMAT_BC4_SNORM_BLOCK"},
    {VK_FORMAT_BC5_UNORM_BLOCK, "VK_FORMAT_BC5_UNORM_BLOCK"},
    {VK_FORMAT_BC5_SNORM_BLOCK, "VK_FORMAT_BC5_SNORM_BLOCK"},
    {VK_FORMAT_BC6H_UFLOAT_BLOCK, "VK_FORMAT_BC6H_UFLOAT_BLOCK"},
    {VK_FORMAT_BC6H_SFLOAT_BLOCK, "VK_FORMAT_BC6H_SFLOAT_BLOCK"},
    {VK_FORMAT_BC7_UNORM_BLOCK, "VK_FORMAT_BC7_UNORM_BLOCK"},
    {VK_FORMAT_BC7_SRGB_BLOCK, "VK_FORMAT_BC7_SRGB_BLOCK"},
    {VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, "VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK"},
    {VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK, "VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK"},
    {VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, "VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK"},
    {VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK, "VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK"},
    {VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, "VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK"},
    {VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK, "VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK"},
    {VK_FORMAT_EAC_R11_UNORM_BLOCK, "VK_FORMAT_EAC_R11_UNORM_BLOCK"},
    {VK_FORMAT_EAC_R11_SNORM_BLOCK, "VK_FORMAT_EAC_R11_SNORM_BLOCK"},
    {VK_FORMAT_EAC_R11G11_UNORM_BLOCK, "VK_FORMAT_EAC_R11G11_UNORM_BLOCK"},
    {VK_FORMAT_EAC_R11G11_SNORM_BLOCK, "VK_FORMAT_EAC_R11G11_SNORM_BLOCK"},
    {VK_FORMAT_ASTC_4x4_UNORM_BLOCK, "VK_FORMAT_ASTC_4x4_UNORM_BLOCK"},
    {VK_FORMAT_ASTC_4x4_SRGB_BLOCK, "VK_FORMAT_ASTC_4x4_SRGB_BLOCK"},
    {VK_FORMAT_ASTC_5x4_UNORM_BLOCK, "VK_FORMAT_ASTC_5x4_UNORM_BLOCK"},
    {VK_FORMAT_ASTC_5x4_SRGB_BLOCK, "VK_FORMAT_ASTC_5x4_SRGB_BLOCK"},
    {VK_FORMAT_ASTC_5x5_UNORM_BLOCK, "VK_FORMAT_ASTC_5x5_UNORM_BLOCK"},
    {VK_FORMAT_ASTC_5x5_SRGB_BLOCK, "VK_FORMAT_ASTC_5x5_SRGB_BLOCK"},
    {VK_FORMAT_ASTC_6x5_UNORM_BLOCK, "VK_FORMAT_ASTC_6x5_UNORM_BLOCK"},
    {VK_FORMAT_ASTC_6x5_SRGB_BLOCK, "VK_FORMAT_ASTC_6x5_SRGB_BLOCK"},
    {VK_FORMAT_ASTC_6x6_UNORM_BLOCK, "VK_FORMAT_ASTC_6x6_UNORM_BLOCK"},
    {VK_FORMAT_ASTC_6x6_SRGB_BLOCK, "VK_FORMAT_ASTC_6x6_SRGB_BLOCK"},
    {VK_FORMAT_ASTC_8x5_UNORM_BLOCK, "VK_FORMAT_ASTC_8x5_UNORM_BLOCK"},
    {VK_FORMAT_ASTC_8x5_SRGB_BLOCK, "VK_FORMAT_ASTC_8x5_SRGB_BLOCK"},
    {VK_FORMAT_ASTC_8x6_UNORM_BLOCK, "VK_FORMAT_ASTC_8x6_UNORM_BLOCK"},
    {VK_FORMAT_ASTC_8x6_SRGB_BLOCK, "VK_FORMAT_ASTC_8x6_SRGB_BLOCK"},
    {VK_FORMAT_ASTC_8x8_UNORM_BLOCK, "VK_FORMAT_ASTC_8x8_UNORM_BLOCK"},
    {VK_FORMAT_ASTC_8x8_SRGB_BLOCK, "VK_FORMAT_ASTC_8x8_SRGB_BLOCK"},
    {VK_FORMAT_ASTC_10x5_UNORM_BLOCK, "VK_FORMAT_ASTC_10x5_UNORM_BLOCK"},
    {VK_FORMAT_ASTC_10x5_SRGB_BLOCK, "VK_FORMAT_ASTC_10x5_SRGB_BLOCK"},
    {VK_FORMAT_ASTC_10x6_UNORM_BLOCK, "VK_FORMAT_ASTC_10x6_UNORM_BLOCK"},
    {VK_FORMAT_ASTC_10x6_SRGB_BLOCK, "VK_FORMAT_ASTC_10x6_SRGB_BLOCK"},
    {VK_FORMAT_ASTC_10x8_UNORM_BLOCK, "VK_FORMAT_ASTC_10x8_UNORM_BLOCK"},
    {VK_FORMAT_ASTC_10x8_SRGB_BLOCK, "VK_FORMAT_ASTC_10x8_SRGB_BLOCK"},
    {VK_FORMAT_ASTC_10x10_UNORM_BLOCK, "VK_FORMAT_ASTC_10x10_UNORM_BLOCK"},
    {VK_FORMAT_ASTC_10x10_SRGB_BLOCK, "VK_FORMAT_ASTC_10x10_SRGB_BLOCK"},
    {VK_FORMAT_ASTC_12x10_UNORM_BLOCK, "VK_FORMAT_ASTC_12x10_UNORM_BLOCK"},
    {VK_FORMAT_ASTC_12x10_SRGB_BLOCK, "VK_FORMAT_ASTC_12x10_SRGB_BLOCK"},
    {VK_FORMAT_ASTC_12x12_UNORM_BLOCK, "VK_FORMAT_ASTC_12x12_UNORM_BLOCK"},
    {VK_FORMAT_ASTC_12x12_SRGB_BLOCK, "VK_FORMAT_ASTC_12x12_SRGB_BLOCK"},
    {VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG, "VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG"},
    {VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG, "VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG"},
    {VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG, "VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG"},
    {VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG, "VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG"},
    {VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG, "VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG"},
    {VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG, "VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG"},
    {VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG, "VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG"},
    {VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG, "VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG"}};

std::string to_string(VkFormat format)
{
	auto it = vk_format_strings.find(format);
	if (it != vk_format_strings.end())
	{
		return it->second;
	}
	throw std::runtime_error{"INVALID_FORMAT"};
}

std::string to_string(VkSampleCountFlagBits flags)
{
	std::string result{""};
	bool        append = false;
	if (flags & VK_SAMPLE_COUNT_1_BIT)
	{
		result += "1";
		append = true;
	}
	if (flags & VK_SAMPLE_COUNT_2_BIT)
	{
		result += append ? "/" : "";
		result += "2";
		append = true;
	}
	if (flags & VK_SAMPLE_COUNT_4_BIT)
	{
		result += append ? "/" : "";
		result += "4";
		append = true;
	}
	if (flags & VK_SAMPLE_COUNT_8_BIT)
	{
		result += append ? "/" : "";
		result += "8";
		append = true;
	}
	if (flags & VK_SAMPLE_COUNT_16_BIT)
	{
		result += append ? "/" : "";
		result += "16";
		append = true;
	}
	if (flags & VK_SAMPLE_COUNT_32_BIT)
	{
		result += append ? "/" : "";
		result += "32";
		append = true;
	}
	if (flags & VK_SAMPLE_COUNT_64_BIT)
	{
		result += append ? "/" : "";
		result += "64";
	}
	return result;
}

std::string to_string_shader_stage_flags(VkShaderStageFlags flags)
{
	if (flags == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
	{
		return "TESSELLATION_CONTROL";
	}
	if (flags == VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
	{
		return "TESSELLATION_EVALUATION";
	}
	if (flags == VK_SHADER_STAGE_GEOMETRY_BIT)
	{
		return "GEOMETRY";
	}
	if (flags == VK_SHADER_STAGE_FRAGMENT_BIT)
	{
		return "FRAGMENT";
	}
	if (flags == VK_SHADER_STAGE_COMPUTE_BIT)
	{
		return "COMPUTE";
	}
	if (flags == VK_SHADER_STAGE_ALL_GRAPHICS)
	{
		return "ALL_GRAPHICS";
	}
	if (flags == VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM)
	{
		return "FLAG_BITS_MAX_ENUM";
	}
	if (flags & VK_SHADER_STAGE_VERTEX_BIT)
	{
		return "VERTEX";
	}
	return "Unknown Stage";
}

std::string to_string(VkPhysicalDeviceType type)
{
	switch (type)
	{
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
			return "VK_PHYSICAL_DEVICE_TYPE_OTHER";
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			return "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU";
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			return "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU";
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			return "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU";
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			return "VK_PHYSICAL_DEVICE_TYPE_CPU";
		default:
			return "UNKNOWN_DEVICE_TYPE";
	}
}

std::string to_string(VkSurfaceTransformFlagBitsKHR flags)
{
	std::string result{""};
	bool        append = false;
	if (flags & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		result += "IDENTITY";
		append = true;
	}
	if (flags & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR)
	{
		result += append ? "/" : "";
		result += "ROTATE_90";
		append = true;
	}
	if (flags & VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR)
	{
		result += append ? "/" : "";
		result += "ROTATE_180";
		append = true;
	}
	if (flags & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR)
	{
		result += append ? "/" : "";
		result += "ROTATE_270";
		append = true;
	}
	if (flags & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR)
	{
		result += append ? "/" : "";
		result += "HORIZONTAL_MIRROR";
		append = true;
	}
	if (flags & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR)
	{
		result += append ? "/" : "";
		result += "HORIZONTAL_MIRROR_ROTATE_90";
		append = true;
	}
	if (flags & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR)
	{
		result += append ? "/" : "";
		result += "HORIZONTAL_MIRROR_ROTATE_180";
		append = true;
	}
	if (flags & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR)
	{
		result += append ? "/" : "";
		result += "HORIZONTAL_MIRROR_ROTATE_270";
		append = true;
	}
	if (flags & VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR)
	{
		result += append ? "/" : "";
		result += "INHERIT";
	}
	return result;
}

std::string to_string(VkPresentModeKHR mode)
{
	switch (mode)
	{
		case VK_PRESENT_MODE_IMMEDIATE_KHR:
			return "VK_PRESENT_MODE_IMMEDIATE_KHR";
		case VK_PRESENT_MODE_MAILBOX_KHR:
			return "VK_PRESENT_MODE_MAILBOX_KHR";
		case VK_PRESENT_MODE_FIFO_KHR:
			return "VK_PRESENT_MODE_FIFO_KHR";
		case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
			return "VK_PRESENT_MODE_FIFO_RELAXED_KHR";
		case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:
			return "VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR";
		case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR:
			return "VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR";
		default:
			return "UNKNOWN_PRESENT_MODE";
	}
}

std::string to_string_vk_image_usage_flags(VkImageUsageFlags flags)
{
	std::string result{""};
	bool        append = false;
	if (flags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
	{
		result += append ? "/" : "";
		result += "VK_IMAGE_USAGE_TRANSFER_SRC_BIT";
		append = true;
	}
	if (flags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	{
		result += append ? "/" : "";
		result += "VK_IMAGE_USAGE_TRANSFER_DST_BIT";
		append = true;
	}
	if (flags & VK_IMAGE_USAGE_SAMPLED_BIT)
	{
		result += append ? "/" : "";
		result += "VK_IMAGE_USAGE_SAMPLED_BIT";
		append = true;
	}
	if (flags & VK_IMAGE_USAGE_STORAGE_BIT)
	{
		result += append ? "/" : "";
		result += "VK_IMAGE_USAGE_STORAGE_BIT";
		append = true;
	}
	if (flags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
	{
		result += append ? "/" : "";
		result += "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT";
		append = true;
	}
	if (flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		result += append ? "/" : "";
		result += "VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT";
		append = true;
	}
	if (flags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
	{
		result += append ? "/" : "";
		result += "VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT";
		append = true;
	}
	if (flags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
	{
		result += append ? "/" : "";
		result += "VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT";
		append = true;
	}
	return result;
}

std::string to_string_vk_image_aspect_flags(VkImageAspectFlags flags)
{
	std::string result = "";
	bool        append = false;

	if (flags & VK_IMAGE_ASPECT_COLOR_BIT)
	{
		result += append ? "/" : "";
		result += "VK_IMAGE_ASPECT_COLOR_BIT";
		append = true;
	}
	if (flags & VK_IMAGE_ASPECT_DEPTH_BIT)
	{
		result += append ? "/" : "";
		result += "VK_IMAGE_ASPECT_DEPTH_BIT";
		append = true;
	}
	if (flags & VK_IMAGE_ASPECT_STENCIL_BIT)
	{
		result += append ? "/" : "";
		result += "VK_IMAGE_ASPECT_STENCIL_BIT";
		append = true;
	}
	if (flags & VK_IMAGE_ASPECT_METADATA_BIT)
	{
		result += append ? "/" : "";
		result += "VK_IMAGE_ASPECT_METADATA_BIT";
		append = true;
	}
	if (flags & VK_IMAGE_ASPECT_PLANE_0_BIT)
	{
		result += append ? "/" : "";
		result += "VK_IMAGE_ASPECT_PLANE_0_BIT";
		append = true;
	}
	if (flags & VK_IMAGE_ASPECT_PLANE_1_BIT)
	{
		result += append ? "/" : "";
		result += "VK_IMAGE_ASPECT_PLANE_1_BIT";
		append = true;
	}
	if (flags & VK_IMAGE_ASPECT_PLANE_2_BIT)
	{
		result += append ? "/" : "";
		result += "VK_IMAGE_ASPECT_PLANE_2_BIT";
		append = true;
	}
	return result;
}

std::string to_string(VkImageTiling tiling)
{
	switch (tiling)
	{
		case VK_IMAGE_TILING_OPTIMAL:
			return "VK_IMAGE_TILING_OPTIMAL";
		case VK_IMAGE_TILING_LINEAR:
			return "VK_IMAGE_TILING_LINEAR";
		default:
			return "UNKOWN_TILING_METHOD";
	}
}

std::string to_string(VkImageType type)
{
	switch (type)
	{
		case VK_IMAGE_TYPE_1D:
			return "VK_IMAGE_TYPE_1D";
		case VK_IMAGE_TYPE_2D:
			return "VK_IMAGE_TYPE_2D";
		case VK_IMAGE_TYPE_3D:
			return "VK_IMAGE_TYPE_3D";
		default:
			return "UNKOWN_IMAGE_TYPE";
	}
}

std::string to_string(VkExtent2D extent)
{
	return fmt::format("{}x{}", extent.width, extent.height);
}

std::string to_string(VkBlendFactor blend)
{
	switch (blend)
	{
		case VK_BLEND_FACTOR_ZERO:
			return "VK_BLEND_FACTOR_ZERO";
		case VK_BLEND_FACTOR_ONE:
			return "VK_BLEND_FACTOR_ONE";
		case VK_BLEND_FACTOR_SRC_COLOR:
			return "VK_BLEND_FACTOR_SRC_COLOR";
		case VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
			return "VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR";
		case VK_BLEND_FACTOR_DST_COLOR:
			return "VK_BLEND_FACTOR_DST_COLOR";
		case VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR:
			return "VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR";
		case VK_BLEND_FACTOR_SRC_ALPHA:
			return "VK_BLEND_FACTOR_SRC_ALPHA";
		case VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
			return "VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA";
		case VK_BLEND_FACTOR_DST_ALPHA:
			return "VK_BLEND_FACTOR_DST_ALPHA";
		case VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
			return "VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA";
		case VK_BLEND_FACTOR_CONSTANT_COLOR:
			return "VK_BLEND_FACTOR_CONSTANT_COLOR";
		case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:
			return "VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR";
		case VK_BLEND_FACTOR_CONSTANT_ALPHA:
			return "VK_BLEND_FACTOR_CONSTANT_ALPHA";
		case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA:
			return "VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA";
		case VK_BLEND_FACTOR_SRC_ALPHA_SATURATE:
			return "VK_BLEND_FACTOR_SRC_ALPHA_SATURATE";
		case VK_BLEND_FACTOR_SRC1_COLOR:
			return "VK_BLEND_FACTOR_SRC1_COLOR";
		case VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR:
			return "VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR";
		case VK_BLEND_FACTOR_SRC1_ALPHA:
			return "VK_BLEND_FACTOR_SRC1_ALPHA";
		case VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA:
			return "VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA";
		default:
			return "Unknown Blend Factor";
	}
}

std::string to_string(VkVertexInputRate rate)
{
	switch (rate)
	{
		case VK_VERTEX_INPUT_RATE_VERTEX:
			return "VK_VERTEX_INPUT_RATE_VERTEX";
		case VK_VERTEX_INPUT_RATE_INSTANCE:
			return "VK_VERTEX_INPUT_RATE_INSTANCE";
		default:
			return "Unknown Rate";
	}
}

std::string to_string_vk_bool(VkBool32 state)
{
	if (state == VK_TRUE)
	{
		return "true";
	}

	return "false";
}

std::string to_string(VkPrimitiveTopology topology)
{
	if (topology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
	{
		return "VK_PRIMITIVE_TOPOLOGY_POINT_LIST";
	}
	if (topology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
	{
		return "VK_PRIMITIVE_TOPOLOGY_LINE_LIST";
	}
	if (topology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP)
	{
		return "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP";
	}
	if (topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
	{
		return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST";
	}
	if (topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)
	{
		return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP";
	}
	if (topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN)
	{
		return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN";
	}
	if (topology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY)
	{
		return "VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY";
	}
	if (topology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY)
	{
		return "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY";
	}
	if (topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY)
	{
		return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY";
	}
	if (topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY)
	{
		return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY";
	}
	if (topology == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST)
	{
		return "VK_PRIMITIVE_TOPOLOGY_PATCH_LIST";
	}

	return "UNKOWN TOPOLOGY";
}

std::string to_string(VkFrontFace face)
{
	if (face == VK_FRONT_FACE_COUNTER_CLOCKWISE)
	{
		return "VK_FRONT_FACE_COUNTER_CLOCKWISE";
	}
	if (face == VK_FRONT_FACE_CLOCKWISE)
	{
		return "VK_FRONT_FACE_CLOCKWISE";
	}
	return "UNKOWN";
}

std::string to_string(VkPolygonMode mode)
{
	if (mode == VK_POLYGON_MODE_FILL)
	{
		return "VK_POLYGON_MODE_FILL";
	}
	if (mode == VK_POLYGON_MODE_LINE)
	{
		return "VK_POLYGON_MODE_LINE";
	}
	if (mode == VK_POLYGON_MODE_POINT)
	{
		return "VK_POLYGON_MODE_POINT";
	}
	if (mode == VK_POLYGON_MODE_FILL_RECTANGLE_NV)
	{
		return "VK_POLYGON_MODE_FILL_RECTANGLE_NV";
	}
	return "UNKOWN";
}

std::string to_string_vk_cull_mode_flags(VkCullModeFlags flags)
{
	if (flags == VK_CULL_MODE_NONE)
	{
		return "VK_CULL_MODE_NONE";
	}
	if (flags == VK_CULL_MODE_FRONT_BIT)
	{
		return "VK_CULL_MODE_FRONT_BIT";
	}
	if (flags == VK_CULL_MODE_BACK_BIT)
	{
		return "VK_CULL_MODE_BACK_BIT";
	}
	if (flags == VK_CULL_MODE_FRONT_AND_BACK)
	{
		return "VK_CULL_MODE_FRONT_AND_BACK";
	}
	return "Unkown Cull Mode";
}

std::string to_string(VkCompareOp operation)
{
	if (operation == VK_COMPARE_OP_NEVER)
	{
		return "NEVER";
	}
	if (operation == VK_COMPARE_OP_LESS)
	{
		return "LESS";
	}
	if (operation == VK_COMPARE_OP_EQUAL)
	{
		return "EQUAL";
	}
	if (operation == VK_COMPARE_OP_LESS_OR_EQUAL)
	{
		return "LESS_OR_EQUAL";
	}
	if (operation == VK_COMPARE_OP_GREATER)
	{
		return "GREATER";
	}
	if (operation == VK_COMPARE_OP_NOT_EQUAL)
	{
		return "NOT_EQUAL";
	}
	if (operation == VK_COMPARE_OP_GREATER_OR_EQUAL)
	{
		return "GREATER_OR_EQUAL";
	}
	if (operation == VK_COMPARE_OP_ALWAYS)
	{
		return "ALWAYS";
	}
	return "Unkown";
}

std::string to_string(VkStencilOp operation)
{
	if (operation == VK_STENCIL_OP_KEEP)
	{
		return "KEEP";
	}
	if (operation == VK_STENCIL_OP_ZERO)
	{
		return "ZERO";
	}
	if (operation == VK_STENCIL_OP_REPLACE)
	{
		return "REPLACE";
	}
	if (operation == VK_STENCIL_OP_INCREMENT_AND_CLAMP)
	{
		return "INCREMENT_AND_CLAMP";
	}
	if (operation == VK_STENCIL_OP_DECREMENT_AND_CLAMP)
	{
		return "DECREMENT_AND_CLAMP";
	}
	if (operation == VK_STENCIL_OP_INVERT)
	{
		return "INVERT";
	}
	if (operation == VK_STENCIL_OP_INCREMENT_AND_WRAP)
	{
		return "INCREMENT_AND_WRAP";
	}
	if (operation == VK_STENCIL_OP_DECREMENT_AND_WRAP)
	{
		return "DECREMENT_AND_WRAP";
	}
	return "Unkown";
}

std::string to_string(VkLogicOp operation)
{
	if (operation == VK_LOGIC_OP_CLEAR)
	{
		return "CLEAR";
	}
	if (operation == VK_LOGIC_OP_AND)
	{
		return "AND";
	}
	if (operation == VK_LOGIC_OP_AND_REVERSE)
	{
		return "AND_REVERSE";
	}
	if (operation == VK_LOGIC_OP_COPY)
	{
		return "COPY";
	}
	if (operation == VK_LOGIC_OP_AND_INVERTED)
	{
		return "AND_INVERTED";
	}
	if (operation == VK_LOGIC_OP_NO_OP)
	{
		return "NO_OP";
	}
	if (operation == VK_LOGIC_OP_XOR)
	{
		return "XOR";
	}
	if (operation == VK_LOGIC_OP_OR)
	{
		return "OR";
	}
	if (operation == VK_LOGIC_OP_NOR)
	{
		return "NOR";
	}
	if (operation == VK_LOGIC_OP_EQUIVALENT)
	{
		return "EQUIVALENT";
	}
	if (operation == VK_LOGIC_OP_INVERT)
	{
		return "INVERT";
	}
	if (operation == VK_LOGIC_OP_OR_REVERSE)
	{
		return "OR_REVERSE";
	}
	if (operation == VK_LOGIC_OP_COPY_INVERTED)
	{
		return "COPY_INVERTED";
	}
	if (operation == VK_LOGIC_OP_OR_INVERTED)
	{
		return "OR_INVERTED";
	}
	if (operation == VK_LOGIC_OP_NAND)
	{
		return "NAND";
	}
	if (operation == VK_LOGIC_OP_SET)
	{
		return "SET";
	}
	return "Unkown";
}

std::string to_string(VkBlendOp operation)
{
	if (operation == VK_BLEND_OP_ADD)
	{
		return "ADD";
	}

	if (operation == VK_BLEND_OP_SUBTRACT)
	{
		return "SUBTRACT";
	}

	if (operation == VK_BLEND_OP_REVERSE_SUBTRACT)
	{
		return "REVERSE_SUBTRACT";
	}

	if (operation == VK_BLEND_OP_MIN)
	{
		return "MIN";
	}

	if (operation == VK_BLEND_OP_MAX)
	{
		return "MAX";
	}

	if (operation == VK_BLEND_OP_ZERO_EXT)
	{
		return "ZERO_EXT";
	}

	if (operation == VK_BLEND_OP_SRC_EXT)
	{
		return "SRC_EXT";
	}

	if (operation == VK_BLEND_OP_DST_EXT)
	{
		return "DST_EXT";
	}

	if (operation == VK_BLEND_OP_SRC_OVER_EXT)
	{
		return "SRC_OVER_EXT";
	}

	if (operation == VK_BLEND_OP_DST_OVER_EXT)
	{
		return "DST_OVER_EXT";
	}

	if (operation == VK_BLEND_OP_SRC_IN_EXT)
	{
		return "SRC_IN_EXT";
	}

	if (operation == VK_BLEND_OP_DST_IN_EXT)
	{
		return "DST_IN_EXT";
	}

	if (operation == VK_BLEND_OP_SRC_OUT_EXT)
	{
		return "SRC_OUT_EXT";
	}

	if (operation == VK_BLEND_OP_DST_OUT_EXT)
	{
		return "DST_OUT_EXT";
	}

	if (operation == VK_BLEND_OP_SRC_ATOP_EXT)
	{
		return "SRC_ATOP_EXT";
	}

	if (operation == VK_BLEND_OP_DST_ATOP_EXT)
	{
		return "DST_ATOP_EXT";
	}

	if (operation == VK_BLEND_OP_XOR_EXT)
	{
		return "XOR_EXT";
	}

	if (operation == VK_BLEND_OP_MULTIPLY_EXT)
	{
		return "MULTIPLY_EXT";
	}

	if (operation == VK_BLEND_OP_SCREEN_EXT)
	{
		return "SCREEN_EXT";
	}

	if (operation == VK_BLEND_OP_OVERLAY_EXT)
	{
		return "OVERLAY_EXT";
	}

	if (operation == VK_BLEND_OP_DARKEN_EXT)
	{
		return "DARKEN_EXT";
	}

	if (operation == VK_BLEND_OP_LIGHTEN_EXT)
	{
		return "LIGHTEN_EXT";
	}

	if (operation == VK_BLEND_OP_COLORDODGE_EXT)
	{
		return "COLORDODGE_EXT";
	}

	if (operation == VK_BLEND_OP_COLORBURN_EXT)
	{
		return "COLORBURN_EXT";
	}

	if (operation == VK_BLEND_OP_HARDLIGHT_EXT)
	{
		return "HARDLIGHT_EXT";
	}

	if (operation == VK_BLEND_OP_SOFTLIGHT_EXT)
	{
		return "SOFTLIGHT_EXT";
	}

	if (operation == VK_BLEND_OP_DIFFERENCE_EXT)
	{
		return "DIFFERENCE_EXT";
	}

	if (operation == VK_BLEND_OP_EXCLUSION_EXT)
	{
		return "EXCLUSION_EXT";
	}

	if (operation == VK_BLEND_OP_INVERT_EXT)
	{
		return "INVERT_EXT";
	}

	if (operation == VK_BLEND_OP_INVERT_RGB_EXT)
	{
		return "INVERT_RGB_EXT";
	}

	if (operation == VK_BLEND_OP_LINEARDODGE_EXT)
	{
		return "LINEARDODGE_EXT";
	}

	if (operation == VK_BLEND_OP_LINEARBURN_EXT)
	{
		return "LINEARBURN_EXT";
	}

	if (operation == VK_BLEND_OP_VIVIDLIGHT_EXT)
	{
		return "VIVIDLIGHT_EXT";
	}

	if (operation == VK_BLEND_OP_LINEARLIGHT_EXT)
	{
		return "LINEARLIGHT_EXT";
	}

	if (operation == VK_BLEND_OP_PINLIGHT_EXT)
	{
		return "PINLIGHT_EXT";
	}

	if (operation == VK_BLEND_OP_HARDMIX_EXT)
	{
		return "HARDMIX_EXT";
	}

	if (operation == VK_BLEND_OP_HSL_HUE_EXT)
	{
		return "HSL_HUE_EXT";
	}

	if (operation == VK_BLEND_OP_HSL_SATURATION_EXT)
	{
		return "HSL_SATURATION_EXT";
	}

	if (operation == VK_BLEND_OP_HSL_COLOR_EXT)
	{
		return "HSL_COLOR_EXT";
	}

	if (operation == VK_BLEND_OP_HSL_LUMINOSITY_EXT)
	{
		return "HSL_LUMINOSITY_EXT";
	}

	if (operation == VK_BLEND_OP_PLUS_EXT)
	{
		return "PLUS_EXT";
	}

	if (operation == VK_BLEND_OP_PLUS_CLAMPED_EXT)
	{
		return "PLUS_CLAMPED_EXT";
	}

	if (operation == VK_BLEND_OP_PLUS_CLAMPED_ALPHA_EXT)
	{
		return "PLUS_CLAMPED_ALPHA_EXT";
	}

	if (operation == VK_BLEND_OP_PLUS_DARKER_EXT)
	{
		return "PLUS_DARKER_EXT";
	}

	if (operation == VK_BLEND_OP_MINUS_EXT)
	{
		return "MINUS_EXT";
	}

	if (operation == VK_BLEND_OP_MINUS_CLAMPED_EXT)
	{
		return "MINUS_CLAMPED_EXT";
	}

	if (operation == VK_BLEND_OP_CONTRAST_EXT)
	{
		return "CONTRAST_EXT";
	}

	if (operation == VK_BLEND_OP_INVERT_OVG_EXT)
	{
		return "INVERT_OVG_EXT";
	}

	if (operation == VK_BLEND_OP_RED_EXT)
	{
		return "RED_EXT";
	}

	if (operation == VK_BLEND_OP_GREEN_EXT)
	{
		return "GREEN_EXT";
	}

	if (operation == VK_BLEND_OP_BLUE_EXT)
	{
		return "BLUE_EXT";
	}
	return "Unkown";
}

std::string to_string_vk_color_component_flags(VkColorComponentFlags flags)
{
	std::string result = "";
	if (flags & VK_COLOR_COMPONENT_R_BIT)
	{
		result += "R";
	}
	if (flags & VK_COLOR_COMPONENT_G_BIT)
	{
		result += "G";
	}
	if (flags & VK_COLOR_COMPONENT_B_BIT)
	{
		result += "B";
	}
	if (flags & VK_COLOR_COMPONENT_A_BIT)
	{
		result += "A";
	}
	return result.empty() ? "No Color Component" : result;
}

std::string to_string(sg::AlphaMode mode)
{
	if (mode == sg::AlphaMode::Blend)
	{
		return "Blend";
	}
	else if (mode == sg::AlphaMode::Mask)
	{
		return "Mask";
	}
	else if (mode == sg::AlphaMode::Opaque)
	{
		return "Opaque";
	}
	return "Unkown";
}

std::string to_string(bool flag)
{
	if (flag == true)
	{
		return "true";
	}
	return "false";
}

std::string to_string(ShaderResourceType type)
{
	switch (type)
	{
		case ShaderResourceType::Input:
			return "Input";
		case ShaderResourceType::InputAttachment:
			return "InputAttachment";
		case ShaderResourceType::Output:
			return "Output";
		case ShaderResourceType::Image:
			return "Image";
		case ShaderResourceType::ImageSampler:
			return "ImageSampler";
		case ShaderResourceType::ImageStorage:
			return "ImageStorage";
		case ShaderResourceType::Sampler:
			return "Sampler";
		case ShaderResourceType::BufferUniform:
			return "BufferUniform";
		case ShaderResourceType::BufferStorage:
			return "BufferStorage";
		case ShaderResourceType::PushConstant:
			return "PushConstant";
		case ShaderResourceType::SpecializationConstant:
			return "SpecializationConstant";
		default:
			return "Unkown Type";
	}
}

}        // namespace utils
}        // namespace vkb
