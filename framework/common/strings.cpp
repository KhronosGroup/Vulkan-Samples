/* Copyright (c) 2018-2021, Arm Limited and Contributors
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
std::vector<std::string> split(const std::string &str, const std::string &delimiter)
{
	if (str.size() == 0)
	{
		return {};
	}

	std::vector<std::string> out;

	std::string buffer         = str;
	size_t      last_found_pos = 0;
	size_t      pos            = 0;
	while ((pos = buffer.find(delimiter)) != std::string::npos)
	{
		out.push_back(buffer.substr(0, pos));
		buffer.erase(0, pos + delimiter.length());
		last_found_pos = last_found_pos + pos + delimiter.length();
	}

	if (last_found_pos == str.size())
	{
		out.push_back("");
	}

	return out;
}

std::string join(const std::vector<std::string> &str, const std::string &seperator)
{
	std::stringstream out;

	for (auto it = str.begin(); it != str.end(); it++)
	{
		out << *it;

		if (it != str.end() - 1)
		{
			out << seperator;
		}
	}

	return out.str();
}

const std::string to_string(VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_R4G4_UNORM_PACK8:
			return "VK_FORMAT_R4G4_UNORM_PACK8";
		case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
			return "VK_FORMAT_R4G4B4A4_UNORM_PACK16";
		case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
			return "VK_FORMAT_B4G4R4A4_UNORM_PACK16";
		case VK_FORMAT_R5G6B5_UNORM_PACK16:
			return "VK_FORMAT_R5G6B5_UNORM_PACK16";
		case VK_FORMAT_B5G6R5_UNORM_PACK16:
			return "VK_FORMAT_B5G6R5_UNORM_PACK16";
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
			return "VK_FORMAT_R5G5B5A1_UNORM_PACK16";
		case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
			return "VK_FORMAT_B5G5R5A1_UNORM_PACK16";
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
			return "VK_FORMAT_A1R5G5B5_UNORM_PACK16";
		case VK_FORMAT_R8_UNORM:
			return "VK_FORMAT_R8_UNORM";
		case VK_FORMAT_R8_SNORM:
			return "VK_FORMAT_R8_SNORM";
		case VK_FORMAT_R8_USCALED:
			return "VK_FORMAT_R8_USCALED";
		case VK_FORMAT_R8_SSCALED:
			return "VK_FORMAT_R8_SSCALED";
		case VK_FORMAT_R8_UINT:
			return "VK_FORMAT_R8_UINT";
		case VK_FORMAT_R8_SINT:
			return "VK_FORMAT_R8_SINT";
		case VK_FORMAT_R8_SRGB:
			return "VK_FORMAT_R8_SRGB";
		case VK_FORMAT_R8G8_UNORM:
			return "VK_FORMAT_R8G8_UNORM";
		case VK_FORMAT_R8G8_SNORM:
			return "VK_FORMAT_R8G8_SNORM";
		case VK_FORMAT_R8G8_USCALED:
			return "VK_FORMAT_R8G8_USCALED";
		case VK_FORMAT_R8G8_SSCALED:
			return "VK_FORMAT_R8G8_SSCALED";
		case VK_FORMAT_R8G8_UINT:
			return "VK_FORMAT_R8G8_UINT";
		case VK_FORMAT_R8G8_SINT:
			return "VK_FORMAT_R8G8_SINT";
		case VK_FORMAT_R8G8_SRGB:
			return "VK_FORMAT_R8G8_SRGB";
		case VK_FORMAT_R8G8B8_UNORM:
			return "VK_FORMAT_R8G8B8_UNORM";
		case VK_FORMAT_R8G8B8_SNORM:
			return "VK_FORMAT_R8G8B8_SNORM";
		case VK_FORMAT_R8G8B8_USCALED:
			return "VK_FORMAT_R8G8B8_USCALED";
		case VK_FORMAT_R8G8B8_SSCALED:
			return "VK_FORMAT_R8G8B8_SSCALED";
		case VK_FORMAT_R8G8B8_UINT:
			return "VK_FORMAT_R8G8B8_UINT";
		case VK_FORMAT_R8G8B8_SINT:
			return "VK_FORMAT_R8G8B8_SINT";
		case VK_FORMAT_R8G8B8_SRGB:
			return "VK_FORMAT_R8G8B8_SRGB";
		case VK_FORMAT_B8G8R8_UNORM:
			return "VK_FORMAT_B8G8R8_UNORM";
		case VK_FORMAT_B8G8R8_SNORM:
			return "VK_FORMAT_B8G8R8_SNORM";
		case VK_FORMAT_B8G8R8_USCALED:
			return "VK_FORMAT_B8G8R8_USCALED";
		case VK_FORMAT_B8G8R8_SSCALED:
			return "VK_FORMAT_B8G8R8_SSCALED";
		case VK_FORMAT_B8G8R8_UINT:
			return "VK_FORMAT_B8G8R8_UINT";
		case VK_FORMAT_B8G8R8_SINT:
			return "VK_FORMAT_B8G8R8_SINT";
		case VK_FORMAT_B8G8R8_SRGB:
			return "VK_FORMAT_B8G8R8_SRGB";
		case VK_FORMAT_R8G8B8A8_UNORM:
			return "VK_FORMAT_R8G8B8A8_UNORM";
		case VK_FORMAT_R8G8B8A8_SNORM:
			return "VK_FORMAT_R8G8B8A8_SNORM";
		case VK_FORMAT_R8G8B8A8_USCALED:
			return "VK_FORMAT_R8G8B8A8_USCALED";
		case VK_FORMAT_R8G8B8A8_SSCALED:
			return "VK_FORMAT_R8G8B8A8_SSCALED";
		case VK_FORMAT_R8G8B8A8_UINT:
			return "VK_FORMAT_R8G8B8A8_UINT";
		case VK_FORMAT_R8G8B8A8_SINT:
			return "VK_FORMAT_R8G8B8A8_SINT";
		case VK_FORMAT_R8G8B8A8_SRGB:
			return "VK_FORMAT_R8G8B8A8_SRGB";
		case VK_FORMAT_B8G8R8A8_UNORM:
			return "VK_FORMAT_B8G8R8A8_UNORM";
		case VK_FORMAT_B8G8R8A8_SNORM:
			return "VK_FORMAT_B8G8R8A8_SNORM";
		case VK_FORMAT_B8G8R8A8_USCALED:
			return "VK_FORMAT_B8G8R8A8_USCALED";
		case VK_FORMAT_B8G8R8A8_SSCALED:
			return "VK_FORMAT_B8G8R8A8_SSCALED";
		case VK_FORMAT_B8G8R8A8_UINT:
			return "VK_FORMAT_B8G8R8A8_UINT";
		case VK_FORMAT_B8G8R8A8_SINT:
			return "VK_FORMAT_B8G8R8A8_SINT";
		case VK_FORMAT_B8G8R8A8_SRGB:
			return "VK_FORMAT_B8G8R8A8_SRGB";
		case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
			return "VK_FORMAT_A8B8G8R8_UNORM_PACK32";
		case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
			return "VK_FORMAT_A8B8G8R8_SNORM_PACK32";
		case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
			return "VK_FORMAT_A8B8G8R8_USCALED_PACK32";
		case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
			return "VK_FORMAT_A8B8G8R8_SSCALED_PACK32";
		case VK_FORMAT_A8B8G8R8_UINT_PACK32:
			return "VK_FORMAT_A8B8G8R8_UINT_PACK32";
		case VK_FORMAT_A8B8G8R8_SINT_PACK32:
			return "VK_FORMAT_A8B8G8R8_SINT_PACK32";
		case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
			return "VK_FORMAT_A8B8G8R8_SRGB_PACK32";
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
			return "VK_FORMAT_A2R10G10B10_UNORM_PACK32";
		case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
			return "VK_FORMAT_A2R10G10B10_SNORM_PACK32";
		case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
			return "VK_FORMAT_A2R10G10B10_USCALED_PACK32";
		case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
			return "VK_FORMAT_A2R10G10B10_SSCALED_PACK32";
		case VK_FORMAT_A2R10G10B10_UINT_PACK32:
			return "VK_FORMAT_A2R10G10B10_UINT_PACK32";
		case VK_FORMAT_A2R10G10B10_SINT_PACK32:
			return "VK_FORMAT_A2R10G10B10_SINT_PACK32";
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
			return "VK_FORMAT_A2B10G10R10_UNORM_PACK32";
		case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
			return "VK_FORMAT_A2B10G10R10_SNORM_PACK32";
		case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
			return "VK_FORMAT_A2B10G10R10_USCALED_PACK32";
		case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
			return "VK_FORMAT_A2B10G10R10_SSCALED_PACK32";
		case VK_FORMAT_A2B10G10R10_UINT_PACK32:
			return "VK_FORMAT_A2B10G10R10_UINT_PACK32";
		case VK_FORMAT_A2B10G10R10_SINT_PACK32:
			return "VK_FORMAT_A2B10G10R10_SINT_PACK32";
		case VK_FORMAT_R16_UNORM:
			return "VK_FORMAT_R16_UNORM";
		case VK_FORMAT_R16_SNORM:
			return "VK_FORMAT_R16_SNORM";
		case VK_FORMAT_R16_USCALED:
			return "VK_FORMAT_R16_USCALED";
		case VK_FORMAT_R16_SSCALED:
			return "VK_FORMAT_R16_SSCALED";
		case VK_FORMAT_R16_UINT:
			return "VK_FORMAT_R16_UINT";
		case VK_FORMAT_R16_SINT:
			return "VK_FORMAT_R16_SINT";
		case VK_FORMAT_R16_SFLOAT:
			return "VK_FORMAT_R16_SFLOAT";
		case VK_FORMAT_R16G16_UNORM:
			return "VK_FORMAT_R16G16_UNORM";
		case VK_FORMAT_R16G16_SNORM:
			return "VK_FORMAT_R16G16_SNORM";
		case VK_FORMAT_R16G16_USCALED:
			return "VK_FORMAT_R16G16_USCALED";
		case VK_FORMAT_R16G16_SSCALED:
			return "VK_FORMAT_R16G16_SSCALED";
		case VK_FORMAT_R16G16_UINT:
			return "VK_FORMAT_R16G16_UINT";
		case VK_FORMAT_R16G16_SINT:
			return "VK_FORMAT_R16G16_SINT";
		case VK_FORMAT_R16G16_SFLOAT:
			return "VK_FORMAT_R16G16_SFLOAT";
		case VK_FORMAT_R16G16B16_UNORM:
			return "VK_FORMAT_R16G16B16_UNORM";
		case VK_FORMAT_R16G16B16_SNORM:
			return "VK_FORMAT_R16G16B16_SNORM";
		case VK_FORMAT_R16G16B16_USCALED:
			return "VK_FORMAT_R16G16B16_USCALED";
		case VK_FORMAT_R16G16B16_SSCALED:
			return "VK_FORMAT_R16G16B16_SSCALED";
		case VK_FORMAT_R16G16B16_UINT:
			return "VK_FORMAT_R16G16B16_UINT";
		case VK_FORMAT_R16G16B16_SINT:
			return "VK_FORMAT_R16G16B16_SINT";
		case VK_FORMAT_R16G16B16_SFLOAT:
			return "VK_FORMAT_R16G16B16_SFLOAT";
		case VK_FORMAT_R16G16B16A16_UNORM:
			return "VK_FORMAT_R16G16B16A16_UNORM";
		case VK_FORMAT_R16G16B16A16_SNORM:
			return "VK_FORMAT_R16G16B16A16_SNORM";
		case VK_FORMAT_R16G16B16A16_USCALED:
			return "VK_FORMAT_R16G16B16A16_USCALED";
		case VK_FORMAT_R16G16B16A16_SSCALED:
			return "VK_FORMAT_R16G16B16A16_SSCALED";
		case VK_FORMAT_R16G16B16A16_UINT:
			return "VK_FORMAT_R16G16B16A16_UINT";
		case VK_FORMAT_R16G16B16A16_SINT:
			return "VK_FORMAT_R16G16B16A16_SINT";
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			return "VK_FORMAT_R16G16B16A16_SFLOAT";
		case VK_FORMAT_R32_UINT:
			return "VK_FORMAT_R32_UINT";
		case VK_FORMAT_R32_SINT:
			return "VK_FORMAT_R32_SINT";
		case VK_FORMAT_R32_SFLOAT:
			return "VK_FORMAT_R32_SFLOAT";
		case VK_FORMAT_R32G32_UINT:
			return "VK_FORMAT_R32G32_UINT";
		case VK_FORMAT_R32G32_SINT:
			return "VK_FORMAT_R32G32_SINT";
		case VK_FORMAT_R32G32_SFLOAT:
			return "VK_FORMAT_R32G32_SFLOAT";
		case VK_FORMAT_R32G32B32_UINT:
			return "VK_FORMAT_R32G32B32_UINT";
		case VK_FORMAT_R32G32B32_SINT:
			return "VK_FORMAT_R32G32B32_SINT";
		case VK_FORMAT_R32G32B32_SFLOAT:
			return "VK_FORMAT_R32G32B32_SFLOAT";
		case VK_FORMAT_R32G32B32A32_UINT:
			return "VK_FORMAT_R32G32B32A32_UINT";
		case VK_FORMAT_R32G32B32A32_SINT:
			return "VK_FORMAT_R32G32B32A32_SINT";
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return "VK_FORMAT_R32G32B32A32_SFLOAT";
		case VK_FORMAT_R64_UINT:
			return "VK_FORMAT_R64_UINT";
		case VK_FORMAT_R64_SINT:
			return "VK_FORMAT_R64_SINT";
		case VK_FORMAT_R64_SFLOAT:
			return "VK_FORMAT_R64_SFLOAT";
		case VK_FORMAT_R64G64_UINT:
			return "VK_FORMAT_R64G64_UINT";
		case VK_FORMAT_R64G64_SINT:
			return "VK_FORMAT_R64G64_SINT";
		case VK_FORMAT_R64G64_SFLOAT:
			return "VK_FORMAT_R64G64_SFLOAT";
		case VK_FORMAT_R64G64B64_UINT:
			return "VK_FORMAT_R64G64B64_UINT";
		case VK_FORMAT_R64G64B64_SINT:
			return "VK_FORMAT_R64G64B64_SINT";
		case VK_FORMAT_R64G64B64_SFLOAT:
			return "VK_FORMAT_R64G64B64_SFLOAT";
		case VK_FORMAT_R64G64B64A64_UINT:
			return "VK_FORMAT_R64G64B64A64_UINT";
		case VK_FORMAT_R64G64B64A64_SINT:
			return "VK_FORMAT_R64G64B64A64_SINT";
		case VK_FORMAT_R64G64B64A64_SFLOAT:
			return "VK_FORMAT_R64G64B64A64_SFLOAT";
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
			return "VK_FORMAT_B10G11R11_UFLOAT_PACK32";
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
			return "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32";
		case VK_FORMAT_D16_UNORM:
			return "VK_FORMAT_D16_UNORM";
		case VK_FORMAT_X8_D24_UNORM_PACK32:
			return "VK_FORMAT_X8_D24_UNORM_PACK32";
		case VK_FORMAT_D32_SFLOAT:
			return "VK_FORMAT_D32_SFLOAT";
		case VK_FORMAT_S8_UINT:
			return "VK_FORMAT_S8_UINT";
		case VK_FORMAT_D16_UNORM_S8_UINT:
			return "VK_FORMAT_D16_UNORM_S8_UINT";
		case VK_FORMAT_D24_UNORM_S8_UINT:
			return "VK_FORMAT_D24_UNORM_S8_UINT";
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return "VK_FORMAT_D32_SFLOAT_S8_UINT";
		case VK_FORMAT_UNDEFINED:
			return "VK_FORMAT_UNDEFINED";
		default:
			return "VK_FORMAT_INVALID";
	}
}

const std::string to_string(VkPresentModeKHR present_mode)
{
	switch (present_mode)
	{
		case VK_PRESENT_MODE_MAILBOX_KHR:
			return "VK_PRESENT_MODE_MAILBOX_KHR";
		case VK_PRESENT_MODE_IMMEDIATE_KHR:
			return "VK_PRESENT_MODE_IMMEDIATE_KHR";
		case VK_PRESENT_MODE_FIFO_KHR:
			return "VK_PRESENT_MODE_FIFO_KHR";
		case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
			return "VK_PRESENT_MODE_FIFO_RELAXED_KHR";
		case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR:
			return "VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR";
		case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:
			return "VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR";
		default:
			return "UNKNOWN_PRESENT_MODE";
	}
}

const std::string to_string(VkResult result)
{
	switch (result)
	{
#define STR(r)   \
	case VK_##r: \
		return #r
		STR(NOT_READY);
		STR(TIMEOUT);
		STR(EVENT_SET);
		STR(EVENT_RESET);
		STR(INCOMPLETE);
		STR(ERROR_OUT_OF_HOST_MEMORY);
		STR(ERROR_OUT_OF_DEVICE_MEMORY);
		STR(ERROR_INITIALIZATION_FAILED);
		STR(ERROR_DEVICE_LOST);
		STR(ERROR_MEMORY_MAP_FAILED);
		STR(ERROR_LAYER_NOT_PRESENT);
		STR(ERROR_EXTENSION_NOT_PRESENT);
		STR(ERROR_FEATURE_NOT_PRESENT);
		STR(ERROR_INCOMPATIBLE_DRIVER);
		STR(ERROR_TOO_MANY_OBJECTS);
		STR(ERROR_FORMAT_NOT_SUPPORTED);
		STR(ERROR_SURFACE_LOST_KHR);
		STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
		STR(SUBOPTIMAL_KHR);
		STR(ERROR_OUT_OF_DATE_KHR);
		STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
		STR(ERROR_VALIDATION_FAILED_EXT);
		STR(ERROR_INVALID_SHADER_NV);
#undef STR
		default:
			return "UNKNOWN_ERROR";
	}
}

const std::string to_string(VkSurfaceTransformFlagBitsKHR transform_flag)
{
	switch (transform_flag)
	{
		case VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR:
			return "SURFACE_TRANSFORM_IDENTITY";
		case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR:
			return "SURFACE_TRANSFORM_ROTATE_90";
		case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR:
			return "SURFACE_TRANSFORM_ROTATE_180";
		case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR:
			return "SURFACE_TRANSFORM_ROTATE_270";
		case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR:
			return "SURFACE_TRANSFORM_HORIZONTAL_MIRROR";
		case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR:
			return "SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90";
		case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR:
			return "SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180";
		case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR:
			return "SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270";
		case VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR:
			return "SURFACE_TRANSFORM_INHERIT";
		case VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR:
			return "SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM";
		default:
			return "[Unknown transform flag]";
	}
}

const std::string to_string(VkSurfaceFormatKHR surface_format)
{
	std::string surface_format_string = to_string(surface_format.format) + ", ";

	switch (surface_format.colorSpace)
	{
		case VK_COLORSPACE_SRGB_NONLINEAR_KHR:
			surface_format_string += "VK_COLORSPACE_SRGB_NONLINEAR_KHR";
			break;
		default:
			surface_format_string += "UNKNOWN COLOR SPACE";
	}
	return surface_format_string;
}

const std::string to_string(VkCompositeAlphaFlagBitsKHR composite_alpha)
{
	switch (composite_alpha)
	{
		case VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR:
			return "VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR";
		case VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR:
			return "VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR";
		case VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR:
			return "VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR";
		case VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR:
			return "VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR";
		case VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR:
			return "VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR";
		default:
			return "UNKNOWN COMPOSITE ALPHA FLAG";
	}
}

const std::string to_string(VkImageUsageFlagBits image_usage)
{
	switch (image_usage)
	{
		case VK_IMAGE_USAGE_TRANSFER_SRC_BIT:
			return "VK_IMAGE_USAGE_TRANSFER_SRC_BIT";
		case VK_IMAGE_USAGE_TRANSFER_DST_BIT:
			return "VK_IMAGE_USAGE_TRANSFER_DST_BIT";
		case VK_IMAGE_USAGE_SAMPLED_BIT:
			return "VK_IMAGE_USAGE_SAMPLED_BIT";
		case VK_IMAGE_USAGE_STORAGE_BIT:
			return "VK_IMAGE_USAGE_STORAGE_BIT";
		case VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT:
			return "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT";
		case VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT:
			return "VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT";
		case VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT:
			return "VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT";
		case VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT:
			return "VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT";
		case VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM:
			return "VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM";
		default:
			return "UNKNOWN IMAGE USAGE FLAG";
	}
}

const std::string to_string(VkExtent2D extent)
{
	return fmt::format("{}x{}", extent.width, extent.height);
}

const std::string to_string(VkSampleCountFlagBits flags)
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

const std::string to_string(VkPhysicalDeviceType type)
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

const std::string to_string(VkImageTiling tiling)
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

const std::string to_string(VkImageType type)
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

const std::string to_string(VkBlendFactor blend)
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

const std::string to_string(VkVertexInputRate rate)
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

const std::string to_string_vk_bool(VkBool32 state)
{
	if (state == VK_TRUE)
	{
		return "true";
	}

	return "false";
}

const std::string to_string(VkPrimitiveTopology topology)
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

const std::string to_string(VkFrontFace face)
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

const std::string to_string(VkPolygonMode mode)
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

const std::string to_string(VkCompareOp operation)
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

const std::string to_string(VkStencilOp operation)
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

const std::string to_string(VkLogicOp operation)
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

const std::string to_string(VkBlendOp operation)
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

const std::string to_string(sg::AlphaMode mode)
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

const std::string to_string(bool flag)
{
	if (flag == true)
	{
		return "true";
	}
	return "false";
}

const std::string to_string(ShaderResourceType type)
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

const std::string buffer_usage_to_string(VkBufferUsageFlags flags)
{
	return to_string<VkBufferUsageFlagBits>(flags,
	                                        {{VK_BUFFER_USAGE_TRANSFER_SRC_BIT, "VK_BUFFER_USAGE_TRANSFER_SRC_BIT"},
	                                         {VK_BUFFER_USAGE_TRANSFER_DST_BIT, "VK_BUFFER_USAGE_TRANSFER_DST_BIT"},
	                                         {VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, "VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT"},
	                                         {VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, "VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT"},
	                                         {VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, "VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT"},
	                                         {VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, "VK_BUFFER_USAGE_STORAGE_BUFFER_BIT"},
	                                         {VK_BUFFER_USAGE_INDEX_BUFFER_BIT, "VK_BUFFER_USAGE_INDEX_BUFFER_BIT"},
	                                         {VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, "VK_BUFFER_USAGE_VERTEX_BUFFER_BIT"},
	                                         {VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, "VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT"},
	                                         {VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, "VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT"},
	                                         {VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT, "VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT"},
	                                         {VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT, "VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT"},
	                                         {VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT, "VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT"},
	                                         {VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, "VK_BUFFER_USAGE_RAY_TRACING_BIT_NV"},
	                                         {VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT, "VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT"},
	                                         {VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR, "VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR"}});
}

const std::string shader_stage_to_string(VkShaderStageFlags flags)
{
	return to_string<VkShaderStageFlagBits>(flags,
	                                        {{VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, "TESSELLATION_CONTROL"},
	                                         {VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, "TESSELLATION_EVALUATION"},
	                                         {VK_SHADER_STAGE_GEOMETRY_BIT, "GEOMETRY"},
	                                         {VK_SHADER_STAGE_VERTEX_BIT, "VERTEX"},
	                                         {VK_SHADER_STAGE_FRAGMENT_BIT, "FRAGMENT"},
	                                         {VK_SHADER_STAGE_COMPUTE_BIT, "COMPUTE"},
	                                         {VK_SHADER_STAGE_ALL_GRAPHICS, "ALL GRAPHICS"}});
}

const std::string image_usage_to_string(VkImageUsageFlags flags)
{
	return to_string<VkImageUsageFlagBits>(flags,
	                                       {{VK_IMAGE_USAGE_TRANSFER_SRC_BIT, "VK_IMAGE_USAGE_TRANSFER_SRC_BIT"},
	                                        {VK_IMAGE_USAGE_TRANSFER_DST_BIT, "VK_IMAGE_USAGE_TRANSFER_DST_BIT"},
	                                        {VK_IMAGE_USAGE_SAMPLED_BIT, "VK_IMAGE_USAGE_SAMPLED_BIT"},
	                                        {VK_IMAGE_USAGE_STORAGE_BIT, "VK_IMAGE_USAGE_STORAGE_BIT"},
	                                        {VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT"},
	                                        {VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, "VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT"},
	                                        {VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, "VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT"},
	                                        {VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, "VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT"}});
}

const std::string image_aspect_to_string(VkImageAspectFlags flags)
{
	return to_string<VkImageAspectFlagBits>(flags,
	                                        {{VK_IMAGE_ASPECT_COLOR_BIT, "VK_IMAGE_ASPECT_COLOR_BIT"},
	                                         {VK_IMAGE_ASPECT_DEPTH_BIT, "VK_IMAGE_ASPECT_DEPTH_BIT"},
	                                         {VK_IMAGE_ASPECT_STENCIL_BIT, "VK_IMAGE_ASPECT_STENCIL_BIT"},
	                                         {VK_IMAGE_ASPECT_METADATA_BIT, "VK_IMAGE_ASPECT_METADATA_BIT"},
	                                         {VK_IMAGE_ASPECT_PLANE_0_BIT, "VK_IMAGE_ASPECT_PLANE_0_BIT"},
	                                         {VK_IMAGE_ASPECT_PLANE_1_BIT, "VK_IMAGE_ASPECT_PLANE_1_BIT"},
	                                         {VK_IMAGE_ASPECT_PLANE_2_BIT, "VK_IMAGE_ASPECT_PLANE_2_BIT"}});
}

const std::string cull_mode_to_string(VkCullModeFlags flags)
{
	return to_string<VkCullModeFlagBits>(flags,
	                                     {{VK_CULL_MODE_NONE, "VK_CULL_MODE_NONE"},
	                                      {VK_CULL_MODE_FRONT_BIT, "VK_CULL_MODE_FRONT_BIT"},
	                                      {VK_CULL_MODE_BACK_BIT, "VK_CULL_MODE_BACK_BIT"},
	                                      {VK_CULL_MODE_FRONT_AND_BACK, "VK_CULL_MODE_FRONT_AND_BACK"}});
}

const std::string color_component_to_string(VkColorComponentFlags flags)
{
	return to_string<VkColorComponentFlagBits>(flags,
	                                           {{VK_COLOR_COMPONENT_R_BIT, "R"},
	                                            {VK_COLOR_COMPONENT_G_BIT, "G"},
	                                            {VK_COLOR_COMPONENT_B_BIT, "B"},
	                                            {VK_COLOR_COMPONENT_A_BIT, "A"}});
}

std::vector<std::string> split(const std::string &input, char delim)
{
	std::vector<std::string> tokens;

	std::stringstream sstream(input);
	std::string       token;
	while (std::getline(sstream, token, delim))
	{
		tokens.push_back(token);
	}

	return tokens;
}
}        // namespace vkb
