/* Copyright (c) 2018-2023, Arm Limited and Contributors
 * Copyright (c) 2019-2023, Sascha Willems
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

#include <fmt/format.h>

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
		return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	}
	else if (ext == "rahit")
	{
		return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
	}
	else if (ext == "rchit")
	{
		return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	}
	else if (ext == "rmiss")
	{
		return VK_SHADER_STAGE_MISS_BIT_KHR;
	}
	else if (ext == "rint")
	{
		return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
	}
	else if (ext == "rcall")
	{
		return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
	}
	else if (ext == "mesh")
	{
		return VK_SHADER_STAGE_MESH_BIT_EXT;
	}
	else if (ext == "task")
	{
		return VK_SHADER_STAGE_TASK_BIT_EXT;
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
	       format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}

bool is_depth_format(VkFormat format)
{
	return is_depth_only_format(format) || is_depth_stencil_format(format);
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

	return load_shader(spirv, device, stage);
}

VkShaderModule load_shader(const std::vector<uint32_t> &spirv, VkDevice device, VkShaderStageFlagBits stage)
{
	VkShaderModule           shader_module;
	VkShaderModuleCreateInfo module_create_info{};
	module_create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	module_create_info.codeSize = spirv.size() * sizeof(uint32_t);
	module_create_info.pCode    = spirv.data();

	VK_CHECK(vkCreateShaderModule(device, &module_create_info, NULL, &shader_module));

	return shader_module;
}

VkAccessFlags getAccessFlags(VkImageLayout layout)
{
	switch (layout)
	{
		case VK_IMAGE_LAYOUT_UNDEFINED:
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			return 0;
		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			return VK_ACCESS_HOST_WRITE_BIT;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
			return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
			return VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			return VK_ACCESS_TRANSFER_READ_BIT;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			return VK_ACCESS_TRANSFER_WRITE_BIT;
		case VK_IMAGE_LAYOUT_GENERAL:
			assert(false && "Don't know how to get a meaningful VkAccessFlags for VK_IMAGE_LAYOUT_GENERAL! Don't use it!");
			return 0;
		default:
			assert(false);
			return 0;
	}
}

VkPipelineStageFlags getPipelineStageFlags(VkImageLayout layout)
{
	switch (layout)
	{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			return VK_PIPELINE_STAGE_HOST_BIT;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			return VK_PIPELINE_STAGE_TRANSFER_BIT;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
			return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
			return VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		case VK_IMAGE_LAYOUT_GENERAL:
			assert(false && "Don't know how to get a meaningful VkPipelineStageFlags for VK_IMAGE_LAYOUT_GENERAL! Don't use it!");
			return 0;
		default:
			assert(false);
			return 0;
	}
}

// Create an image memory barrier for changing the layout of
// an image and put it into an active command buffer
// See chapter 12.4 "Image Layout" for details

void image_layout_transition(VkCommandBuffer                command_buffer,
                             VkImage                        image,
                             VkPipelineStageFlags           src_stage_mask,
                             VkPipelineStageFlags           dst_stage_mask,
                             VkAccessFlags                  src_access_mask,
                             VkAccessFlags                  dst_access_mask,
                             VkImageLayout                  old_layout,
                             VkImageLayout                  new_layout,
                             VkImageSubresourceRange const &subresource_range)
{
	// Create an image barrier object
	VkImageMemoryBarrier image_memory_barrier{};
	image_memory_barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.srcAccessMask       = src_access_mask;
	image_memory_barrier.dstAccessMask       = dst_access_mask;
	image_memory_barrier.oldLayout           = old_layout;
	image_memory_barrier.newLayout           = new_layout;
	image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.image               = image;
	image_memory_barrier.subresourceRange    = subresource_range;

	// Put barrier inside setup command buffer
	vkCmdPipelineBarrier(command_buffer, src_stage_mask, dst_stage_mask, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
}

void image_layout_transition(VkCommandBuffer                command_buffer,
                             VkImage                        image,
                             VkImageLayout                  old_layout,
                             VkImageLayout                  new_layout,
                             VkImageSubresourceRange const &subresource_range)
{
	VkPipelineStageFlags src_stage_mask  = getPipelineStageFlags(old_layout);
	VkPipelineStageFlags dst_stage_mask  = getPipelineStageFlags(new_layout);
	VkAccessFlags        src_access_mask = getAccessFlags(old_layout);
	VkAccessFlags        dst_access_mask = getAccessFlags(new_layout);

	image_layout_transition(command_buffer, image, src_stage_mask, dst_stage_mask, src_access_mask, dst_access_mask, old_layout, new_layout, subresource_range);
}

// Fixed sub resource on first mip level and layer
void image_layout_transition(VkCommandBuffer command_buffer,
                             VkImage         image,
                             VkImageLayout   old_layout,
                             VkImageLayout   new_layout)
{
	VkImageSubresourceRange subresource_range = {};
	subresource_range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource_range.baseMipLevel            = 0;
	subresource_range.levelCount              = 1;
	subresource_range.baseArrayLayer          = 0;
	subresource_range.layerCount              = 1;
	image_layout_transition(command_buffer, image, old_layout, new_layout, subresource_range);
}

void image_layout_transition(VkCommandBuffer                                                 command_buffer,
                             std::vector<std::pair<VkImage, VkImageSubresourceRange>> const &imagesAndRanges,
                             VkImageLayout                                                   old_layout,
                             VkImageLayout                                                   new_layout)
{
	VkPipelineStageFlags src_stage_mask  = getPipelineStageFlags(old_layout);
	VkPipelineStageFlags dst_stage_mask  = getPipelineStageFlags(new_layout);
	VkAccessFlags        src_access_mask = getAccessFlags(old_layout);
	VkAccessFlags        dst_access_mask = getAccessFlags(new_layout);

	// Create image barrier objects
	std::vector<VkImageMemoryBarrier> image_memory_barriers;
	image_memory_barriers.reserve(imagesAndRanges.size());
	for (size_t i = 0; i < imagesAndRanges.size(); i++)
	{
		image_memory_barriers.emplace_back(VkImageMemoryBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		                                                        nullptr,
		                                                        src_access_mask,
		                                                        dst_access_mask,
		                                                        old_layout,
		                                                        new_layout,
		                                                        VK_QUEUE_FAMILY_IGNORED,
		                                                        VK_QUEUE_FAMILY_IGNORED,
		                                                        imagesAndRanges[i].first,
		                                                        imagesAndRanges[i].second});
	}

	// Put barriers inside setup command buffer
	vkCmdPipelineBarrier(command_buffer,
	                     src_stage_mask,
	                     dst_stage_mask,
	                     0,
	                     0,
	                     nullptr,
	                     0,
	                     nullptr,
	                     static_cast<uint32_t>(image_memory_barriers.size()),
	                     image_memory_barriers.data());
}

VkSurfaceFormatKHR select_surface_format(VkPhysicalDevice gpu, VkSurfaceKHR surface, std::vector<VkFormat> const &preferred_formats)
{
	uint32_t surface_format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &surface_format_count, nullptr);
	assert(0 < surface_format_count);
	std::vector<VkSurfaceFormatKHR> supported_surface_formats(surface_format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &surface_format_count, supported_surface_formats.data());

	auto it = std::find_if(supported_surface_formats.begin(),
	                       supported_surface_formats.end(),
	                       [&preferred_formats](VkSurfaceFormatKHR surface_format) {
		                       return std::any_of(preferred_formats.begin(),
		                                          preferred_formats.end(),
		                                          [&surface_format](VkFormat format) { return format == surface_format.format; });
	                       });

	// We use the first supported format as a fallback in case none of the preferred formats is available
	return it != supported_surface_formats.end() ? *it : supported_surface_formats[0];
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
