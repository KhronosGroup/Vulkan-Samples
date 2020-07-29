/* Copyright (c) 2018-2020, Arm Limited and Contributors
 * Copyright (c) 2019-2020, Sascha Willems
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

#include <spdlog/fmt/fmt.h>

#include "glsl_compiler.h"
#include "platform/filesystem.h"

std::ostream &operator<<(std::ostream &os, const VkResult result)
{
#define WRITE_VK_ENUM(r) \
	case VK_##r:         \
		os << #r;        \
		break;

	switch (result)
	{
		WRITE_VK_ENUM(NOT_READY);
		WRITE_VK_ENUM(TIMEOUT);
		WRITE_VK_ENUM(EVENT_SET);
		WRITE_VK_ENUM(EVENT_RESET);
		WRITE_VK_ENUM(INCOMPLETE);
		WRITE_VK_ENUM(ERROR_OUT_OF_HOST_MEMORY);
		WRITE_VK_ENUM(ERROR_OUT_OF_DEVICE_MEMORY);
		WRITE_VK_ENUM(ERROR_INITIALIZATION_FAILED);
		WRITE_VK_ENUM(ERROR_DEVICE_LOST);
		WRITE_VK_ENUM(ERROR_MEMORY_MAP_FAILED);
		WRITE_VK_ENUM(ERROR_LAYER_NOT_PRESENT);
		WRITE_VK_ENUM(ERROR_EXTENSION_NOT_PRESENT);
		WRITE_VK_ENUM(ERROR_FEATURE_NOT_PRESENT);
		WRITE_VK_ENUM(ERROR_INCOMPATIBLE_DRIVER);
		WRITE_VK_ENUM(ERROR_TOO_MANY_OBJECTS);
		WRITE_VK_ENUM(ERROR_FORMAT_NOT_SUPPORTED);
		WRITE_VK_ENUM(ERROR_SURFACE_LOST_KHR);
		WRITE_VK_ENUM(ERROR_NATIVE_WINDOW_IN_USE_KHR);
		WRITE_VK_ENUM(SUBOPTIMAL_KHR);
		WRITE_VK_ENUM(ERROR_OUT_OF_DATE_KHR);
		WRITE_VK_ENUM(ERROR_INCOMPATIBLE_DISPLAY_KHR);
		WRITE_VK_ENUM(ERROR_VALIDATION_FAILED_EXT);
		WRITE_VK_ENUM(ERROR_INVALID_SHADER_NV);
		default:
			os << "UNKNOWN_ERROR";
	}

#undef WRITE_VK_ENUM

	return os;
}

namespace vkb
{
namespace
{
VkShaderStageFlagBits find_shader_stage(const std::string &ext)
{
	if (ext == "vert")
	{
		return VK_SHADER_STAGE_VERTEX_BIT;
	}
	else if (ext == "frag")
	{
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	}
	else if (ext == "comp")
	{
		return VK_SHADER_STAGE_COMPUTE_BIT;
	}
	else if (ext == "geom")
	{
		return VK_SHADER_STAGE_GEOMETRY_BIT;
	}
	else if (ext == "tesc")
	{
		return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	}
	else if (ext == "tese")
	{
		return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	}
	else if (ext == "rgen")
	{
		return VK_SHADER_STAGE_RAYGEN_BIT_NV;
	}
	else if (ext == "rmiss")
	{
		return VK_SHADER_STAGE_MISS_BIT_NV;
	}
	else if (ext == "rchit")
	{
		return VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
	}

	throw std::runtime_error("File extension `" + ext + "` does not have a vulkan shader stage.");
}
}        // namespace
bool is_depth_only_format(VkFormat format)
{
	return format == VK_FORMAT_D16_UNORM ||
	       format == VK_FORMAT_D32_SFLOAT;
}

bool is_depth_stencil_format(VkFormat format)
{
	return format == VK_FORMAT_D16_UNORM_S8_UINT ||
	       format == VK_FORMAT_D24_UNORM_S8_UINT ||
	       format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
	       is_depth_only_format(format);
}

VkFormat get_suitable_depth_format(VkPhysicalDevice physical_device, bool depth_only, const std::vector<VkFormat> &depth_format_priority_list)
{
	VkFormat depth_format{VK_FORMAT_UNDEFINED};

	for (auto &format : depth_format_priority_list)
	{
		if (depth_only && !is_depth_only_format(format))
		{
			continue;
		}

		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(physical_device, format, &properties);

		// Format must support depth stencil attachment for optimal tiling
		if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			depth_format = format;
			break;
		}
	}

	if (depth_format != VK_FORMAT_UNDEFINED)
	{
		LOGI("Depth format selected: {}", to_string(depth_format));
		return depth_format;
	}

	throw std::runtime_error("No suitable depth format could be determined");
}

bool is_dynamic_buffer_descriptor_type(VkDescriptorType descriptor_type)
{
	return descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
	       descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
}

bool is_buffer_descriptor_type(VkDescriptorType descriptor_type)
{
	return descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
	       descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
	       is_dynamic_buffer_descriptor_type(descriptor_type);
}

int32_t get_bits_per_pixel(VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_R4G4_UNORM_PACK8:
			return 8;
		case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
		case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
		case VK_FORMAT_R5G6B5_UNORM_PACK16:
		case VK_FORMAT_B5G6R5_UNORM_PACK16:
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
		case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
			return 16;
		case VK_FORMAT_R8_UNORM:
		case VK_FORMAT_R8_SNORM:
		case VK_FORMAT_R8_USCALED:
		case VK_FORMAT_R8_SSCALED:
		case VK_FORMAT_R8_UINT:
		case VK_FORMAT_R8_SINT:
		case VK_FORMAT_R8_SRGB:
			return 8;
		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8_SNORM:
		case VK_FORMAT_R8G8_USCALED:
		case VK_FORMAT_R8G8_SSCALED:
		case VK_FORMAT_R8G8_UINT:
		case VK_FORMAT_R8G8_SINT:
		case VK_FORMAT_R8G8_SRGB:
			return 16;
		case VK_FORMAT_R8G8B8_UNORM:
		case VK_FORMAT_R8G8B8_SNORM:
		case VK_FORMAT_R8G8B8_USCALED:
		case VK_FORMAT_R8G8B8_SSCALED:
		case VK_FORMAT_R8G8B8_UINT:
		case VK_FORMAT_R8G8B8_SINT:
		case VK_FORMAT_R8G8B8_SRGB:
		case VK_FORMAT_B8G8R8_UNORM:
		case VK_FORMAT_B8G8R8_SNORM:
		case VK_FORMAT_B8G8R8_USCALED:
		case VK_FORMAT_B8G8R8_SSCALED:
		case VK_FORMAT_B8G8R8_UINT:
		case VK_FORMAT_B8G8R8_SINT:
		case VK_FORMAT_B8G8R8_SRGB:
			return 24;
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_SNORM:
		case VK_FORMAT_R8G8B8A8_USCALED:
		case VK_FORMAT_R8G8B8A8_SSCALED:
		case VK_FORMAT_R8G8B8A8_UINT:
		case VK_FORMAT_R8G8B8A8_SINT:
		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_SNORM:
		case VK_FORMAT_B8G8R8A8_USCALED:
		case VK_FORMAT_B8G8R8A8_SSCALED:
		case VK_FORMAT_B8G8R8A8_UINT:
		case VK_FORMAT_B8G8R8A8_SINT:
		case VK_FORMAT_B8G8R8A8_SRGB:
		case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
		case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
		case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
		case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
		case VK_FORMAT_A8B8G8R8_UINT_PACK32:
		case VK_FORMAT_A8B8G8R8_SINT_PACK32:
		case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
			return 32;
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
		case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
		case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
		case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
		case VK_FORMAT_A2R10G10B10_UINT_PACK32:
		case VK_FORMAT_A2R10G10B10_SINT_PACK32:
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
		case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
		case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
		case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
		case VK_FORMAT_A2B10G10R10_UINT_PACK32:
		case VK_FORMAT_A2B10G10R10_SINT_PACK32:
			return 32;
		case VK_FORMAT_R16_UNORM:
		case VK_FORMAT_R16_SNORM:
		case VK_FORMAT_R16_USCALED:
		case VK_FORMAT_R16_SSCALED:
		case VK_FORMAT_R16_UINT:
		case VK_FORMAT_R16_SINT:
		case VK_FORMAT_R16_SFLOAT:
			return 16;
		case VK_FORMAT_R16G16_UNORM:
		case VK_FORMAT_R16G16_SNORM:
		case VK_FORMAT_R16G16_USCALED:
		case VK_FORMAT_R16G16_SSCALED:
		case VK_FORMAT_R16G16_UINT:
		case VK_FORMAT_R16G16_SINT:
		case VK_FORMAT_R16G16_SFLOAT:
			return 32;
		case VK_FORMAT_R16G16B16_UNORM:
		case VK_FORMAT_R16G16B16_SNORM:
		case VK_FORMAT_R16G16B16_USCALED:
		case VK_FORMAT_R16G16B16_SSCALED:
		case VK_FORMAT_R16G16B16_UINT:
		case VK_FORMAT_R16G16B16_SINT:
		case VK_FORMAT_R16G16B16_SFLOAT:
			return 48;
		case VK_FORMAT_R16G16B16A16_UNORM:
		case VK_FORMAT_R16G16B16A16_SNORM:
		case VK_FORMAT_R16G16B16A16_USCALED:
		case VK_FORMAT_R16G16B16A16_SSCALED:
		case VK_FORMAT_R16G16B16A16_UINT:
		case VK_FORMAT_R16G16B16A16_SINT:
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			return 64;
		case VK_FORMAT_R32_UINT:
		case VK_FORMAT_R32_SINT:
		case VK_FORMAT_R32_SFLOAT:
			return 32;
		case VK_FORMAT_R32G32_UINT:
		case VK_FORMAT_R32G32_SINT:
		case VK_FORMAT_R32G32_SFLOAT:
			return 64;
		case VK_FORMAT_R32G32B32_UINT:
		case VK_FORMAT_R32G32B32_SINT:
		case VK_FORMAT_R32G32B32_SFLOAT:
			return 96;
		case VK_FORMAT_R32G32B32A32_UINT:
		case VK_FORMAT_R32G32B32A32_SINT:
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return 128;
		case VK_FORMAT_R64_UINT:
		case VK_FORMAT_R64_SINT:
		case VK_FORMAT_R64_SFLOAT:
			return 64;
		case VK_FORMAT_R64G64_UINT:
		case VK_FORMAT_R64G64_SINT:
		case VK_FORMAT_R64G64_SFLOAT:
			return 128;
		case VK_FORMAT_R64G64B64_UINT:
		case VK_FORMAT_R64G64B64_SINT:
		case VK_FORMAT_R64G64B64_SFLOAT:
			return 192;
		case VK_FORMAT_R64G64B64A64_UINT:
		case VK_FORMAT_R64G64B64A64_SINT:
		case VK_FORMAT_R64G64B64A64_SFLOAT:
			return 256;
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
			return 32;
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
			return 32;
		case VK_FORMAT_D16_UNORM:
			return 16;
		case VK_FORMAT_X8_D24_UNORM_PACK32:
			return 32;
		case VK_FORMAT_D32_SFLOAT:
			return 32;
		case VK_FORMAT_S8_UINT:
			return 8;
		case VK_FORMAT_D16_UNORM_S8_UINT:
			return 24;
		case VK_FORMAT_D24_UNORM_S8_UINT:
			return 32;
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return 40;
		case VK_FORMAT_UNDEFINED:
		default:
			return -1;
	}
}

VkShaderModule load_shader(const std::string &filename, VkDevice device, VkShaderStageFlagBits stage)
{
	vkb::GLSLCompiler glsl_compiler;

	auto buffer = vkb::fs::read_shader_binary(filename);

	std::string file_ext = filename;

	// Extract extension name from the glsl shader file
	file_ext = file_ext.substr(file_ext.find_last_of(".") + 1);

	std::vector<uint32_t> spirv;
	std::string           info_log;

	// Compile the GLSL source
	if (!glsl_compiler.compile_to_spirv(vkb::find_shader_stage(file_ext), buffer, "main", {}, spirv, info_log))
	{
		LOGE("Failed to compile shader, Error: {}", info_log.c_str());
		return VK_NULL_HANDLE;
	}

	VkShaderModule           shader_module;
	VkShaderModuleCreateInfo module_create_info{};
	module_create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	module_create_info.codeSize = spirv.size() * sizeof(uint32_t);
	module_create_info.pCode    = spirv.data();

	VK_CHECK(vkCreateShaderModule(device, &module_create_info, NULL, &shader_module));

	return shader_module;
}

// Create an image memory barrier for changing the layout of
// an image and put it into an active command buffer
// See chapter 11.4 "Image Layout" for details

void set_image_layout(
    VkCommandBuffer         command_buffer,
    VkImage                 image,
    VkImageLayout           old_layout,
    VkImageLayout           new_layout,
    VkImageSubresourceRange subresource_range,
    VkPipelineStageFlags    src_mask,
    VkPipelineStageFlags    dst_mask)
{
	// Create an image barrier object
	VkImageMemoryBarrier barrier{};
	barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.oldLayout           = old_layout;
	barrier.newLayout           = new_layout;
	barrier.image               = image;
	barrier.subresourceRange    = subresource_range;

	// Source layouts (old)
	// Source access mask controls actions that have to be finished on the old layout
	// before it will be transitioned to the new layout
	switch (old_layout)
	{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			// Image layout is undefined (or does not matter)
			// Only valid as initial layout
			// No flags required, listed only for completeness
			barrier.srcAccessMask = 0;
			break;

		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			// Image is preinitialized
			// Only valid as initial layout for linear images, preserves memory contents
			// Make sure host writes have been finished
			barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image is a color attachment
			// Make sure any writes to the color buffer have been finished
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image is a depth/stencil attachment
			// Make sure any writes to the depth/stencil buffer have been finished
			barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image is a transfer source
			// Make sure any reads from the image have been finished
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image is a transfer destination
			// Make sure any writes to the image have been finished
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image is read by a shader
			// Make sure any shader reads from the image have been finished
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
	}

	// Target layouts (new)
	// Destination access mask controls the dependency for the new image layout
	switch (new_layout)
	{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image will be used as a transfer destination
			// Make sure any writes to the image have been finished
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image will be used as a transfer source
			// Make sure any reads from the image have been finished
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image will be used as a color attachment
			// Make sure any writes to the color buffer have been finished
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image layout will be used as a depth/stencil attachment
			// Make sure any writes to depth/stencil buffer have been finished
			barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image will be read in a shader (sampler, input attachment)
			// Make sure any writes to the image have been finished
			if (barrier.srcAccessMask == 0)
			{
				barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
	}

	// Put barrier inside setup command buffer
	vkCmdPipelineBarrier(
	    command_buffer,
	    src_mask,
	    dst_mask,
	    0,
	    0, nullptr,
	    0, nullptr,
	    1, &barrier);
}

// Fixed sub resource on first mip level and layer
void set_image_layout(
    VkCommandBuffer      command_buffer,
    VkImage              image,
    VkImageAspectFlags   aspect_mask,
    VkImageLayout        old_layout,
    VkImageLayout        new_layout,
    VkPipelineStageFlags src_mask,
    VkPipelineStageFlags dst_mask)
{
	VkImageSubresourceRange subresource_range = {};
	subresource_range.aspectMask              = aspect_mask;
	subresource_range.baseMipLevel            = 0;
	subresource_range.levelCount              = 1;
	subresource_range.layerCount              = 1;
	set_image_layout(command_buffer, image, old_layout, new_layout, subresource_range, src_mask, dst_mask);
}

void insert_image_memory_barrier(
    VkCommandBuffer         command_buffer,
    VkImage                 image,
    VkAccessFlags           src_access_mask,
    VkAccessFlags           dst_access_mask,
    VkImageLayout           old_layout,
    VkImageLayout           new_layout,
    VkPipelineStageFlags    src_stage_mask,
    VkPipelineStageFlags    dst_stage_mask,
    VkImageSubresourceRange subresource_range)
{
	VkImageMemoryBarrier barrier{};
	barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.srcAccessMask       = src_access_mask;
	barrier.dstAccessMask       = dst_access_mask;
	barrier.oldLayout           = old_layout;
	barrier.newLayout           = new_layout;
	barrier.image               = image;
	barrier.subresourceRange    = subresource_range;

	vkCmdPipelineBarrier(
	    command_buffer,
	    src_stage_mask,
	    dst_stage_mask,
	    0,
	    0, nullptr,
	    0, nullptr,
	    1, &barrier);
}
namespace gbuffer
{
std::vector<LoadStoreInfo> get_load_all_store_swapchain()
{
	// Load every attachment and store only swapchain
	std::vector<LoadStoreInfo> load_store{4};

	// Swapchain
	load_store[0].load_op  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	load_store[0].store_op = VK_ATTACHMENT_STORE_OP_STORE;

	// Depth
	load_store[1].load_op  = VK_ATTACHMENT_LOAD_OP_LOAD;
	load_store[1].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Albedo
	load_store[2].load_op  = VK_ATTACHMENT_LOAD_OP_LOAD;
	load_store[2].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Normal
	load_store[3].load_op  = VK_ATTACHMENT_LOAD_OP_LOAD;
	load_store[3].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	return load_store;
}

std::vector<LoadStoreInfo> get_clear_all_store_swapchain()
{
	// Clear every attachment and store only swapchain
	std::vector<LoadStoreInfo> load_store{4};

	// Swapchain
	load_store[0].load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	load_store[0].store_op = VK_ATTACHMENT_STORE_OP_STORE;

	// Depth
	load_store[1].load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	load_store[1].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Albedo
	load_store[2].load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	load_store[2].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Normal
	load_store[3].load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	load_store[3].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	return load_store;
}

std::vector<LoadStoreInfo> get_clear_store_all()
{
	// Clear and store every attachment
	std::vector<LoadStoreInfo> load_store{4};

	// Swapchain
	load_store[0].load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	load_store[0].store_op = VK_ATTACHMENT_STORE_OP_STORE;

	// Depth
	load_store[1].load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	load_store[1].store_op = VK_ATTACHMENT_STORE_OP_STORE;

	// Albedo
	load_store[2].load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	load_store[2].store_op = VK_ATTACHMENT_STORE_OP_STORE;

	// Normal
	load_store[3].load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	load_store[3].store_op = VK_ATTACHMENT_STORE_OP_STORE;

	return load_store;
}

std::vector<VkClearValue> get_clear_value()
{
	// Clear values
	std::vector<VkClearValue> clear_value{4};
	clear_value[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clear_value[1].depthStencil = {0.0f, ~0U};
	clear_value[2].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clear_value[3].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};

	return clear_value;
}
}        // namespace gbuffer

}        // namespace vkb
