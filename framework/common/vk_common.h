/* Copyright (c) 2018-2026, Arm Limited and Contributors
 * Copyright (c) 2019-2026, Sascha Willems
 * Copyright (c) 2024-2026, Mobica Limited
 * Copyright (c) 2026, NVIDIA CORPORATION. All rights reserved.
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
#include <string>
#include <type_traits>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_format_traits.hpp>

#define VK_FLAGS_NONE 0        // Custom define for better code readability

#define DEFAULT_FENCE_TIMEOUT 100000000000        // Default fence timeout in nanoseconds

template <class T>
using BindingMap = std::map<uint32_t, std::map<uint32_t, T>>;

namespace vkb
{
enum class BindingType
{
	C,
	Cpp
};

enum class CommandBufferResetMode
{
	ResetPool,
	ResetIndividually,
	AlwaysAllocate,
};

enum class RequestMode
{
	Optional,
	Required
};

enum class ShadingLanguage
{
	GLSL,
	HLSL,
	SLANG,
};

namespace common
{
namespace
{
// Some helper functions to convert between C and C++ Vulkan types, for use in the common namespace.
template <vkb::BindingType bindingType>
inline constexpr std::conditional<bindingType == vkb::BindingType::Cpp, vk::AttachmentLoadOp, VkAttachmentLoadOp>::type
    attachment_load_op_clear()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return vk::AttachmentLoadOp::eClear;
	}
	else
	{
		return VK_ATTACHMENT_LOAD_OP_CLEAR;
	}
}

template <vkb::BindingType bindingType>
inline constexpr std::conditional<bindingType == vkb::BindingType::Cpp, vk::AttachmentStoreOp, VkAttachmentStoreOp>::
    type
    attachment_store_op_store()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return vk::AttachmentStoreOp::eStore;
	}
	else
	{
		return VK_ATTACHMENT_STORE_OP_STORE;
	}
}

template <vkb::BindingType bindingType>
inline constexpr std::conditional<bindingType == vkb::BindingType::Cpp, vk::CommandBufferLevel, VkCommandBufferLevel>::
    type
    command_buffer_level_primary()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return vk::CommandBufferLevel::ePrimary;
	}
	else
	{
		return VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	}
}

template <vkb::BindingType bindingType>
inline constexpr std::vector<
    typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Format, VkFormat>::type>
    default_depth_format_priority_list()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return {vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint, vk::Format::eD16Unorm};
	}
	else
	{
		return {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM};
	}
}

inline constexpr bool default_depth_only()
{
	return false;
}

template <vkb::BindingType bindingType>
inline constexpr std::vector<
    typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Format, VkFormat>::type>
    default_format_priority_list()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return {vk::Format::eR8G8B8A8Unorm, vk::Format::eB8G8R8A8Unorm, vk::Format::eR5G6B5UnormPack16};
	}
	else
	{
		return {VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R5G6B5_UNORM_PACK16};
	}
}

template <vkb::BindingType bindingType>
inline constexpr std::
    conditional<bindingType == vkb::BindingType::Cpp, vk::ImageAspectFlagBits, VkImageAspectFlagBits>::type
    image_aspect_color_bit()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return vk::ImageAspectFlagBits::eColor;
	}
	else
	{
		return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}

template <vkb::BindingType bindingType>
inline constexpr std::conditional<bindingType == vkb::BindingType::Cpp, vk::ImageLayout, VkImageLayout>::type
    image_layout_undefined()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return vk::ImageLayout::eUndefined;
	}
	else
	{
		return VK_IMAGE_LAYOUT_UNDEFINED;
	}
}

template <vkb::BindingType bindingType>
inline constexpr std::conditional<bindingType == vkb::BindingType::Cpp, vk::PipelineStageFlags, VkPipelineStageFlags>::
    type
    pipeline_stage_flag_bits_bottom_of_pipe()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return vk::PipelineStageFlagBits::eBottomOfPipe;
	}
	else
	{
		return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}
}

template <vkb::BindingType bindingType>
inline constexpr std::conditional<bindingType == vkb::BindingType::Cpp, vk::PipelineStageFlags, VkPipelineStageFlags>::
    type
    pipeline_stage_flag_bits_top_of_pipe()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return vk::PipelineStageFlagBits::eTopOfPipe;
	}
	else
	{
		return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}
}

template <vkb::BindingType bindingType>
inline constexpr uint32_t queue_family_ignored()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return vk::QueueFamilyIgnored;
	}
	else
	{
		return VK_QUEUE_FAMILY_IGNORED;
	}
}
}        // namespace

vk::CommandBuffer
    allocate_command_buffer(vk::Device             device,
                            vk::CommandPool        command_pool,
                            vk::CommandBufferLevel level = command_buffer_level_primary<vkb::BindingType::Cpp>());

vk::DescriptorSet allocate_descriptor_set(vk::Device              device,
                                          vk::DescriptorPool      descriptor_pool,
                                          vk::DescriptorSetLayout descriptor_set_layout);

vk::Framebuffer create_framebuffer(vk::Device                        device,
                                   vk::RenderPass                    render_pass,
                                   std::vector<vk::ImageView> const &attachments,
                                   vk::Extent2D const               &extent);

vk::Pipeline create_graphics_pipeline(vk::Device                                                device,
                                      vk::PipelineCache                                         pipeline_cache,
                                      std::vector<vk::PipelineShaderStageCreateInfo> const     &shader_stages,
                                      vk::PipelineVertexInputStateCreateInfo const             &vertex_input_state,
                                      vk::PrimitiveTopology                                     primitive_topology,
                                      uint32_t                                                  patch_control_points,
                                      vk::PolygonMode                                           polygon_mode,
                                      vk::CullModeFlags                                         cull_mode,
                                      vk::FrontFace                                             front_face,
                                      std::vector<vk::PipelineColorBlendAttachmentState> const &blend_attachment_states,
                                      vk::PipelineDepthStencilStateCreateInfo const            &depth_stencil_state,
                                      vk::PipelineLayout                                        pipeline_layout,
                                      vk::RenderPass                                            render_pass);

vk::ImageView create_image_view(vk::Device           device,
                                vk::Image            image,
                                vk::ImageViewType    view_type,
                                vk::Format           format,
                                vk::ImageAspectFlags aspect_mask      = image_aspect_color_bit<vkb::BindingType::Cpp>(),
                                uint32_t             base_mip_level   = 0,
                                uint32_t             level_count      = 1,
                                uint32_t             base_array_layer = 0,
                                uint32_t             layer_count      = 1);

vk::QueryPool create_query_pool(vk::Device                      device,
                                vk::QueryType                   query_type,
                                uint32_t                        query_count,
                                vk::QueryPipelineStatisticFlags pipeline_statistics = {});

vk::Sampler create_sampler(vk::Device             device,
                           vk::Filter             mag_filter,
                           vk::Filter             min_filter,
                           vk::SamplerMipmapMode  mipmap_mode,
                           vk::SamplerAddressMode sampler_address_mode,
                           float                  max_anisotropy,
                           float                  max_LOD);

vk::Sampler create_sampler(vk::PhysicalDevice     gpu,
                           vk::Device             device,
                           vk::Format             format,
                           vk::Filter             filter,
                           vk::SamplerAddressMode sampler_address_mode,
                           float                  max_anisotropy,
                           float                  max_LOD);

/**
 * @brief Helper function to get the bits per pixel of a Vulkan format.
 * @param format vk::Format to check.
 * @return The bits per pixel of the given format, 0 for invalid formats.
 */
constexpr int32_t get_bits_per_pixel(vk::Format format);

constexpr vk::ImageAspectFlags get_image_aspect_flags(vk::ImageUsageFlagBits usage, vk::Format format);

/**
 * @brief Put an image memory barrier for a layout transition of an image, using explicitly given transition parameters.
 * @param command_buffer The vk::CommandBuffer to record the barrier.
 * @param image The vk::Image to transition.
 * @param src_stage_mask The vk::PipelineStageFlags to use as source.
 * @param dst_stage_mask The vk::PipelineStageFlags to use as destination.
 * @param src_access_mask The vk::AccessFlags to use as source.
 * @param dst_access_mask The vk::AccessFlags to use as destination.
 * @param old_layout The vk::ImageLayout to transition from.
 * @param new_layout The vk::ImageLayout to transition to.
 * @param subresource_range The vk::ImageSubresourceRange to use with the transition.
 */
void image_layout_transition(vk::CommandBuffer                command_buffer,
                             vk::Image                        image,
                             vk::PipelineStageFlags           src_stage_mask,
                             vk::PipelineStageFlags           dst_stage_mask,
                             vk::AccessFlags                  src_access_mask,
                             vk::AccessFlags                  dst_access_mask,
                             vk::ImageLayout                  old_layout,
                             vk::ImageLayout                  new_layout,
                             vk::ImageSubresourceRange const &subresource_range);

/**
 * @brief Put an image memory barrier for a layout transition of an image, on a given subresource range.
 *
 * The src_stage_mask, dst_stage_mask, src_access_mask, and dst_access_mask used are determined from old_layout and new_layout.
 *
 * @param command_buffer The vk::CommandBuffer to record the barrier.
 * @param image The vk::Image to transition.
 * @param old_layout The vk::ImageLayout to transition from.
 * @param new_layout The vk::ImageLayout to transition to.
 * @param subresource_range The vk::ImageSubresourceRange to use with the transition.
 */
void image_layout_transition(vk::CommandBuffer         command_buffer,
                             vk::Image                 image,
                             vk::ImageLayout           old_layout,
                             vk::ImageLayout           new_layout,
                             vk::ImageSubresourceRange subresource_range);

/**
 * @brief Put an image memory barrier for a layout transition of an image, on a fixed subresource with first mip level and layer.
 *
 * The src_stage_mask, dst_stage_mask, src_access_mask, and dst_access_mask used are determined from old_layout and new_layout.
 *
 * @param command_buffer The vk::CommandBuffer to record the barrier.
 * @param image The vk::Image to transition.
 * @param old_layout The vk::ImageLayout to transition from.
 * @param new_layout The vk::ImageLayout to transition to.
 */
void image_layout_transition(vk::CommandBuffer command_buffer,
                             vk::Image         image,
                             vk::ImageLayout   old_layout,
                             vk::ImageLayout   new_layout);

/**
 * @brief Put an image memory barrier for a layout transition of a vector of images, with a given subresource range per image.
 *
 * The src_stage_mask, dst_stage_mask, src_access_mask, and dst_access_mask used are determined from old_layout and new_layout.
 *
 * @param command_buffer The vk::CommandBuffer to record the barrier.
 * @param imagesAndRanges The images to transition, with accompanying subresource ranges.
 * @param old_layout The vk::ImageLayout to transition from.
 * @param new_layout The vk::ImageLayout to transition to.
 */
void image_layout_transition(vk::CommandBuffer                                                   command_buffer,
                             std::vector<std::pair<vk::Image, vk::ImageSubresourceRange>> const &imagesAndRanges,
                             vk::ImageLayout                                                     old_layout,
                             vk::ImageLayout                                                     new_layout);

/**
 * @brief Helper function to determine if a Vulkan descriptor type is a buffer (either uniform or storage buffer, dynamic or not).
 * @param descriptor_type vk::Descriptor type to check.
 * @return True if type is buffer, false otherwise.
 */
constexpr bool is_buffer_descriptor_type(vk::DescriptorType descriptor_type);

/**
 * @brief Helper function to determine if a Vulkan format is depth.
 * @param format vk::Format to check.
 * @return True if format is a depth, false otherwise.
 */
constexpr bool is_depth_format(vk::Format format);

/**
 * @brief Helper function to determine if a Vulkan format is depth only.
 * @param format vk::Format to check.
 * @return True if format is a depth only, false otherwise.
 */
constexpr bool is_depth_only_format(vk::Format format);

/**
 * @brief Helper function to determine if a Vulkan format is depth with stencil.
 * @param format vk::Format to check.
 * @return True if format is a depth with stencil, false otherwise.
 */
constexpr bool is_depth_stencil_format(vk::Format format);

/**
 * @brief Helper function to determine if a Vulkan descriptor type is a dynamic storage buffer or dynamic uniform buffer.
 * @param descriptor_type vk::DescriptorType to check.
 * @return True if type is dynamic buffer, false otherwise.
 */
constexpr bool is_dynamic_buffer_descriptor_type(vk::DescriptorType descriptor_type);

/**
 * @brief Helper function to create a vk::ShaderModule from a SPIR-V shader file
 * @param filename The shader location
 * @param device The logical device
 * @param stage The shader stage
 * @return The vk::ShaderModule containing the loaded shader
 */
vk::ShaderModule load_shader(std::string const &filename, vk::Device device, vk::ShaderStageFlagBits stage);

/**
 * @brief Helper function to create a vk::ShaderModule from a SPIR-V vector
 * @param spirv The SPIR-V code in vector format
 * @param device The logical device
 * @return The vk::ShaderModule containing the loaded shader
 */
vk::ShaderModule load_shader_from_vector(const std::vector<uint32_t> &spirv, vk::Device device);

/**
 * @brief Helper function to check support for linear filtering and adjust its parameters if required
 * @param physical_device The vk::PhysicalDevice to check the depth formats against
 * @param format The vk::Format to check against
 * @param filter The preferred vk::Filter to adjust
 * @param mipmapMode (OPTIONAL) The preferred vk::SamplerMipmapMode to adjust
 */
void make_filters_valid(vk::PhysicalDevice     physical_device,
                        vk::Format             format,
                        vk::Filter            *filter,
                        vk::SamplerMipmapMode *mipmapMode = nullptr);

template <typename StructExtension>
    requires(!!vk::StructExtends<StructExtension, vk::SubresourceLayout2>::value)
StructExtension query_image_subresource_layout_extension(vk::Device device, vk::Image image);

/**
 * @brief Helper function to pick a blendable format from a priority ordered list
 * @param physical_device The vk::PhysicalDevice to check the formats against
 * @param format_priority_list List of vk::Formats in order of priority
 * @return The selected vk::Format
 */
vk::Format select_blendable_format(vk::PhysicalDevice             physical_device,
                                   std::vector<vk::Format> const &format_priority_list);

uint32_t select_queue_family_index(std::vector<vk::QueueFamilyProperties> const &queue_family_properties,
                                   vk::QueueFlagBits                             queue_flag);

/**
 * @brief Helper function to determine a suitable supported depth format based on a priority list
 * @param physical_device The vk::PhysicalDevice to check the depth formats against
 * @param depth_only Wether to include the stencil component in the format or not
 * @param depth_format_priority_list The list of depth formats to prefer over one another
 * @return The valid suited depth format
 */
vk::Format select_suitable_depth_format(vk::PhysicalDevice             physical_device,
                                        bool                           depth_only = default_depth_only(),
                                        std::vector<vk::Format> const &depth_format_priority_list =
                                            default_depth_format_priority_list<vkb::BindingType::Cpp>());

/**
 * @brief Helper function to select a vk::SurfaceFormatKHR
 * @param gpu The vk::PhysicalDevice to select a format for.
 * @param surface The vk::SurfaceKHR to select a format for.
 * @param format_priority_list The list of preferred vk::Formats to select from.
 * @return The selected surface format.
 */
vk::SurfaceFormatKHR select_surface_format(
    vk::PhysicalDevice             gpu,
    vk::SurfaceKHR                 surface,
    std::vector<vk::Format> const &format_priority_list = default_format_priority_list<vkb::BindingType::Cpp>());

void submit_and_wait(vk::Device                     device,
                     vk::Queue                      queue,
                     std::vector<vk::CommandBuffer> command_buffers,
                     std::vector<vk::Semaphore>     semaphores = {});

template <typename FlagBitsType>
constexpr std::vector<FlagBitsType> vectorize_flags(vk::Flags<FlagBitsType> flags);

VkCommandBuffer
    allocate_command_buffer(VkDevice             device,
                            VkCommandPool        command_pool,
                            VkCommandBufferLevel level = command_buffer_level_primary<vkb::BindingType::C>());

VkDescriptorSet allocate_descriptor_set(VkDevice              device,
                                        VkDescriptorPool      descriptor_pool,
                                        VkDescriptorSetLayout descriptor_set_layout);

VkFramebuffer create_framebuffer(VkDevice                        device,
                                 VkRenderPass                    render_pass,
                                 std::vector<VkImageView> const &attachments,
                                 VkExtent2D const               &extent);

VkPipeline create_graphics_pipeline(VkDevice                                                device,
                                    VkPipelineCache                                         pipeline_cache,
                                    std::vector<VkPipelineShaderStageCreateInfo> const     &shader_stages,
                                    VkPipelineVertexInputStateCreateInfo const             &vertex_input_state,
                                    VkPrimitiveTopology                                     primitive_topology,
                                    uint32_t                                                patch_control_points,
                                    VkPolygonMode                                           polygon_mode,
                                    VkCullModeFlags                                         cull_mode,
                                    VkFrontFace                                             front_face,
                                    std::vector<VkPipelineColorBlendAttachmentState> const &blend_attachment_states,
                                    VkPipelineDepthStencilStateCreateInfo const            &depth_stencil_state,
                                    VkPipelineLayout                                        pipeline_layout,
                                    VkRenderPass                                            render_pass);

VkImageView create_image_view(VkDevice           device,
                              VkImage            image,
                              VkImageViewType    view_type,
                              VkFormat           format,
                              VkImageAspectFlags aspect_mask      = image_aspect_color_bit<vkb::BindingType::C>(),
                              uint32_t           base_mip_level   = 0,
                              uint32_t           level_count      = 1,
                              uint32_t           base_array_layer = 0,
                              uint32_t           layer_count      = 1);

VkQueryPool create_query_pool(VkDevice                      device,
                              VkQueryType                   query_type,
                              uint32_t                      query_count,
                              VkQueryPipelineStatisticFlags pipeline_statistics = {});

VkSampler create_sampler(VkDevice             device,
                         VkFilter             mag_filter,
                         VkFilter             min_filter,
                         VkSamplerMipmapMode  mipmap_mode,
                         VkSamplerAddressMode sampler_address_mode,
                         float                max_anisotropy,
                         float                max_LOD);

VkSampler create_sampler(VkPhysicalDevice     gpu,
                         VkDevice             device,
                         VkFormat             format,
                         VkFilter             filter,
                         VkSamplerAddressMode sampler_address_mode,
                         float                max_anisotropy,
                         float                max_LOD);

/**
 * @brief Helper function to get the bits per pixel of a Vulkan format.
 * @param format VkFormat to check.
 * @return The bits per pixel of the given format, 0 for invalid formats.
 */
constexpr int32_t get_bits_per_pixel(VkFormat format);

constexpr VkImageAspectFlags get_image_aspect_flags(VkImageUsageFlagBits usage, VkFormat format);

/**
 * @brief Put an image memory barrier for a layout transition of an image, using explicitly given transition parameters.
 * @param command_buffer The VkCommandBuffer to record the barrier.
 * @param image The VkImage to transition.
 * @param src_stage_mask The VkPipelineStageFlags to use as source.
 * @param dst_stage_mask The VkPipelineStageFlags to use as destination.
 * @param src_access_mask The VkAccessFlags to use as source.
 * @param dst_access_mask The VkAccessFlags to use as destination.
 * @param old_layout The VkImageLayout to transition from.
 * @param new_layout The VkImageLayout to transition to.
 * @param subresource_range The VkImageSubresourceRange to use with the transition.
 */
void image_layout_transition(VkCommandBuffer                command_buffer,
                             VkImage                        image,
                             VkPipelineStageFlags           src_stage_mask,
                             VkPipelineStageFlags           dst_stage_mask,
                             VkAccessFlags                  src_access_mask,
                             VkAccessFlags                  dst_access_mask,
                             VkImageLayout                  old_layout,
                             VkImageLayout                  new_layout,
                             VkImageSubresourceRange const &subresource_range);

/**
 * @brief Put an image memory barrier for a layout transition of an image, on a given subresource range.
 *
 * The src_stage_mask, dst_stage_mask, src_access_mask, and dst_access_mask used are determined from old_layout and new_layout.
 *
 * @param command_buffer The vk::CommandBuffer to record the barrier.
 * @param image The vk::Image to transition.
 * @param old_layout The vk::ImageLayout to transition from.
 * @param new_layout The vk::ImageLayout to transition to.
 * @param subresource_range The vk::ImageSubresourceRange to use with the transition.
 */
void image_layout_transition(VkCommandBuffer                command_buffer,
                             VkImage                        image,
                             VkImageLayout                  old_layout,
                             VkImageLayout                  new_layout,
                             VkImageSubresourceRange const &subresource_range);

/**
 * @brief Put an image memory barrier for a layout transition of an image, on a fixed subresource with first mip level and layer.
 *
 * The src_stage_mask, dst_stage_mask, src_access_mask, and dst_access_mask used are determined from old_layout and new_layout.
 *
 * @param command_buffer The VkCommandBuffer to record the barrier.
 * @param image The VkImage to transition.
 * @param old_layout The VkImageLayout to transition from.
 * @param new_layout The VkImageLayout to transition to.
 */
void image_layout_transition(VkCommandBuffer command_buffer,
                             VkImage         image,
                             VkImageLayout   old_layout,
                             VkImageLayout   new_layout);

/**
 * @brief Put an image memory barrier for a layout transition of a vector of images, with a given subresource range per image.
 *
 * The src_stage_mask, dst_stage_mask, src_access_mask, and dst_access_mask used are determined from old_layout and new_layout.
 *
 * @param command_buffer The VkCommandBuffer to record the barrier.
 * @param imagesAndRanges The images to transition, with accompanying subresource ranges.
 * @param old_layout The VkImageLayout to transition from.
 * @param new_layout The VkImageLayout to transition to.
 */
void image_layout_transition(VkCommandBuffer                                                 command_buffer,
                             std::vector<std::pair<VkImage, VkImageSubresourceRange>> const &imagesAndRanges,
                             VkImageLayout                                                   old_layout,
                             VkImageLayout                                                   new_layout);

/**
 * @brief Helper function to determine if a Vulkan descriptor type is a buffer (either uniform or storage buffer, dynamic or not).
 * @param descriptor_type VkDescriptor type to check.
 * @return True if type is buffer, false otherwise.
 */
constexpr bool is_buffer_descriptor_type(VkDescriptorType descriptor_type);

/**
 * @brief Helper function to determine if a Vulkan format is depth.
 * @param format VkFormat to check.
 * @return True if format is a depth, false otherwise.
 */
constexpr bool is_depth_format(VkFormat format);

/**
 * @brief Helper function to determine if a Vulkan format is depth only.
 * @param format VkFormat to check.
 * @return True if format is a depth only, false otherwise.
 */
constexpr bool is_depth_only_format(VkFormat format);

/**
 * @brief Helper function to determine if a Vulkan format is depth with stencil.
 * @param format VkFormat to check.
 * @return True if format is a depth with stencil, false otherwise.
 */
constexpr bool is_depth_stencil_format(VkFormat format);

/**
 * @brief Helper function to determine if a Vulkan descriptor type is a dynamic storage buffer or dynamic uniform buffer.
 * @param descriptor_type VkDescriptorType to check.
 * @return True if type is dynamic buffer, false otherwise.
 */
constexpr bool is_dynamic_buffer_descriptor_type(VkDescriptorType descriptor_type);

/**
 * @brief Helper function to create a VkShaderModule from a SPIR-V shader file
 * @param filename The shader location
 * @param device The logical device
 * @param stage The shader stage
 * @return The VkShaderModule containing the loaded shader
 */
VkShaderModule load_shader(std::string const &filename, VkDevice device, VkShaderStageFlagBits stage);

/**
 * @brief Helper function to create a VkShaderModule from a SPIR-V vector
 * @param spirv The SPIR-V code in vector format
 * @param device The logical device
 * @return The VkShaderModule containing the loaded shader
 */
VkShaderModule load_shader_from_vector(const std::vector<uint32_t> &spirv, VkDevice device);

/**
 * @brief Helper function to check support for linear filtering and adjust its parameters if required
 * @param physical_device The VkPhysicalDevice to check the depth formats against
 * @param format The VkFormat to check against
 * @param filter The preferred VkFilter to adjust
 * @param mipmapMode (OPTIONAL) The preferred VkSamplerMipmapMode to adjust
 */
void make_filters_valid(VkPhysicalDevice     physical_device,
                        VkFormat             format,
                        VkFilter            *filter,
                        VkSamplerMipmapMode *mipmapMode = nullptr);

template <typename StructExtension>
StructExtension query_image_subresource_layout_extension(VkDevice device, VkImage image);

/**
 * @brief Helper function to pick a blendable format from a priority ordered list
 * @param physical_device The VkPhysicalDevice to check the formats against
 * @param format_priority_list List of VkFormats in order of priority
 * @return The selected VkFormat
 */
VkFormat select_blendable_format(VkPhysicalDevice physical_device, std::vector<VkFormat> const &format_priority_list);

uint32_t select_queue_family_index(std::vector<VkQueueFamilyProperties> const &queue_family_properties,
                                   VkQueueFlagBits                             queue_flag);

/**
 * @brief Helper function to determine a suitable supported depth format based on a priority list
 * @param physical_device The VkPhysicalDevice to check the depth formats against
 * @param depth_only Wether to include the stencil component in the format or not
 * @param depth_format_priority_list The list of depth formats to prefer over one another
 * @return The valid suited depth format
 */
VkFormat select_suitable_depth_format(VkPhysicalDevice             physical_device,
                                      bool                         depth_only = default_depth_only(),
                                      std::vector<VkFormat> const &depth_format_priority_list =
                                          default_depth_format_priority_list<vkb::BindingType::C>());

/**
 * @brief Helper function to select a VkSurfaceFormatKHR
 * @param gpu The VkPhysicalDevice to select a format for.
 * @param surface The VkSurfaceKHR to select a format for.
 * @param format_priority_list The list of preferred VkFormats to select from.
 * @return The selected surface format.
 */
VkSurfaceFormatKHR select_surface_format(
    VkPhysicalDevice             gpu,
    VkSurfaceKHR                 surface,
    std::vector<VkFormat> const &format_priority_list = default_format_priority_list<vkb::BindingType::C>());

void submit_and_wait(VkDevice                     device,
                     VkQueue                      queue,
                     std::vector<VkCommandBuffer> command_buffers,
                     std::vector<VkSemaphore>     semaphores = {});

/**
 * @brief Buffer memory barrier structure used to define
 *        memory access for a buffer during command recording.
 */
template <vkb::BindingType bindingType>
struct BufferMemoryBarrier
{
	using AccessFlagsType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::AccessFlags, VkAccessFlags>::type;
	using PipelineStageFlagsType = typename std::
	    conditional<bindingType == vkb::BindingType::Cpp, vk::PipelineStageFlags, VkPipelineStageFlags>::type;

	PipelineStageFlagsType src_stage_mask  = pipeline_stage_flag_bits_bottom_of_pipe<bindingType>();
	PipelineStageFlagsType dst_stage_mask  = pipeline_stage_flag_bits_top_of_pipe<bindingType>();
	AccessFlagsType        src_access_mask = {};
	AccessFlagsType        dst_access_mask = {};
};

using BufferMemoryBarrierC   = BufferMemoryBarrier<vkb::BindingType::C>;
using BufferMemoryBarrierCpp = BufferMemoryBarrier<vkb::BindingType::Cpp>;

/**
 * @brief Image memory barrier structure used to define
 *        memory access for an image view during command recording.
 */
template <vkb::BindingType bindingType>
struct ImageMemoryBarrier
{
	using AccessFlagsType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::AccessFlags, VkAccessFlags>::type;
	using PipelineStageFlagsType = typename std::
	    conditional<bindingType == vkb::BindingType::Cpp, vk::PipelineStageFlags, VkPipelineStageFlags>::type;
	using ImageLayoutType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::ImageLayout, VkImageLayout>::type;

  private:
  public:
	PipelineStageFlagsType src_stage_mask   = pipeline_stage_flag_bits_bottom_of_pipe<bindingType>();
	PipelineStageFlagsType dst_stage_mask   = pipeline_stage_flag_bits_top_of_pipe<bindingType>();
	AccessFlagsType        src_access_mask  = {};
	AccessFlagsType        dst_access_mask  = {};
	ImageLayoutType        old_layout       = image_layout_undefined<bindingType>();
	ImageLayoutType        new_layout       = image_layout_undefined<bindingType>();
	uint32_t               src_queue_family = queue_family_ignored<bindingType>();
	uint32_t               dst_queue_family = queue_family_ignored<bindingType>();
};

using ImageMemoryBarrierC   = ImageMemoryBarrier<vkb::BindingType::C>;
using ImageMemoryBarrierCpp = ImageMemoryBarrier<vkb::BindingType::Cpp>;

/**
 * @brief Load and store info for a render pass attachment.
 */
template <vkb::BindingType bindingType>
struct LoadStoreInfo
{
	using AttachmentLoadOpType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::AttachmentLoadOp, VkAttachmentLoadOp>::type;
	using AttachmentStoreOpType = typename std::
	    conditional<bindingType == vkb::BindingType::Cpp, vk::AttachmentStoreOp, VkAttachmentStoreOp>::type;

	AttachmentLoadOpType  load_op  = attachment_load_op_clear<bindingType>();
	AttachmentStoreOpType store_op = attachment_store_op_store<bindingType>();
};

using LoadStoreInfoC   = LoadStoreInfo<vkb::BindingType::C>;
using LoadStoreInfoCpp = LoadStoreInfo<vkb::BindingType::Cpp>;

// ==== inlined helper functions ====

inline constexpr int32_t get_bits_per_pixel(vk::Format format)
{
	int32_t bits = 0;
	for (uint8_t i = 0; i < vk::componentCount(format); ++i)
	{
		bits += vk::componentBits(format, i);
	}
	return bits;
}

inline constexpr vk::ImageAspectFlags get_image_aspect_flags(vk::ImageUsageFlagBits usage, vk::Format format)
{
	vk::ImageAspectFlags image_aspect_flags;

	switch (usage)
	{
		case vk::ImageUsageFlagBits::eColorAttachment:
			assert(!vkb::common::is_depth_format(format));
			image_aspect_flags = vk::ImageAspectFlagBits::eColor;
			break;
		case vk::ImageUsageFlagBits::eDepthStencilAttachment:
			assert(vkb::common::is_depth_format(format));
			image_aspect_flags = vk::ImageAspectFlagBits::eDepth;
			// Stencil aspect should only be set on depth + stencil formats
			if (vkb::common::is_depth_stencil_format(format))
			{
				image_aspect_flags |= vk::ImageAspectFlagBits::eStencil;
			}
			break;
		default:
			assert(false);
	}

	return image_aspect_flags;
}

inline constexpr bool is_buffer_descriptor_type(vk::DescriptorType descriptor_type)
{
	return descriptor_type == vk::DescriptorType::eStorageBuffer ||
	       descriptor_type == vk::DescriptorType::eUniformBuffer || is_dynamic_buffer_descriptor_type(descriptor_type);
}

inline constexpr bool is_depth_format(vk::Format format)
{
	return (std::string(vk::componentName(format, 0)) == "D");
}

inline constexpr bool is_depth_only_format(vk::Format format)
{
	return (vk::componentCount(format) == 1) && (std::string(vk::componentName(format, 0)) == "D");
}

inline constexpr bool is_depth_stencil_format(vk::Format format)
{
	return (vk::componentCount(format) == 2) && (std::string(vk::componentName(format, 0)) == "D") &&
	       (std::string(vk::componentName(format, 1)) == "S");
}

inline constexpr bool is_dynamic_buffer_descriptor_type(vk::DescriptorType descriptor_type)
{
	return descriptor_type == vk::DescriptorType::eStorageBufferDynamic ||
	       descriptor_type == vk::DescriptorType::eUniformBufferDynamic;
}

template <typename StructExtension>
    requires(!!vk::StructExtends<StructExtension, vk::SubresourceLayout2>::value)
inline StructExtension query_image_subresource_layout_extension(vk::Device device, vk::Image image)
{
	vk::ImageSubresource2EXT image_subresource{.imageSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor}};

	return device.getImageSubresourceLayout2EXT<vk::SubresourceLayout2EXT, StructExtension>(image, image_subresource)
	    .template get<StructExtension>();
}

template <typename FlagBitsType>
inline constexpr std::vector<FlagBitsType> vectorize_flags(vk::Flags<FlagBitsType> flags)
{
	std::vector<FlagBitsType>                  flag_bits;
	typename vk::Flags<FlagBitsType>::MaskType mask = 1;
	while (flags)
	{
		FlagBitsType flag_bit = static_cast<FlagBitsType>(mask);
		if (flags & flag_bit)
		{
			flag_bits.push_back(flag_bit);
			flags ^= flag_bit;
		}
		mask <<= 1;
	}
	return flag_bits;
}

inline VkCommandBuffer allocate_command_buffer(VkDevice device, VkCommandPool command_pool, VkCommandBufferLevel level)
{
	return static_cast<VkCommandBuffer>(allocate_command_buffer(static_cast<vk::Device>(device),
	                                                            static_cast<vk::CommandPool>(command_pool),
	                                                            static_cast<vk::CommandBufferLevel>(level)));
}

inline VkDescriptorSet allocate_descriptor_set(VkDevice              device,
                                               VkDescriptorPool      descriptor_pool,
                                               VkDescriptorSetLayout descriptor_set_layout)
{
	return static_cast<VkDescriptorSet>(
	    allocate_descriptor_set(static_cast<vk::Device>(device),
	                            static_cast<vk::DescriptorPool>(descriptor_pool),
	                            static_cast<vk::DescriptorSetLayout>(descriptor_set_layout)));
}

inline VkFramebuffer create_framebuffer(VkDevice                        device,
                                        VkRenderPass                    render_pass,
                                        std::vector<VkImageView> const &attachments,
                                        VkExtent2D const               &extent)
{
	return static_cast<VkFramebuffer>(
	    create_framebuffer(static_cast<vk::Device>(device),
	                       static_cast<vk::RenderPass>(render_pass),
	                       reinterpret_cast<std::vector<vk::ImageView> const &>(attachments),
	                       reinterpret_cast<vk::Extent2D const &>(extent)));
}

inline VkPipeline
    create_graphics_pipeline(VkDevice                                                device,
                             VkPipelineCache                                         pipeline_cache,
                             std::vector<VkPipelineShaderStageCreateInfo> const     &shader_stages,
                             VkPipelineVertexInputStateCreateInfo const             &vertex_input_state,
                             VkPrimitiveTopology                                     primitive_topology,
                             uint32_t                                                patch_control_points,
                             VkPolygonMode                                           polygon_mode,
                             VkCullModeFlags                                         cull_mode,
                             VkFrontFace                                             front_face,
                             std::vector<VkPipelineColorBlendAttachmentState> const &blend_attachment_states,
                             VkPipelineDepthStencilStateCreateInfo const            &depth_stencil_state,
                             VkPipelineLayout                                        pipeline_layout,
                             VkRenderPass                                            render_pass)
{
	return static_cast<VkPipeline>(create_graphics_pipeline(
	    static_cast<vk::Device>(device),
	    static_cast<vk::PipelineCache>(pipeline_cache),
	    reinterpret_cast<std::vector<vk::PipelineShaderStageCreateInfo> const &>(shader_stages),
	    reinterpret_cast<vk::PipelineVertexInputStateCreateInfo const &>(vertex_input_state),
	    static_cast<vk::PrimitiveTopology>(primitive_topology),
	    patch_control_points,
	    static_cast<vk::PolygonMode>(polygon_mode),
	    static_cast<vk::CullModeFlags>(cull_mode),
	    static_cast<vk::FrontFace>(front_face),
	    reinterpret_cast<std::vector<vk::PipelineColorBlendAttachmentState> const &>(blend_attachment_states),
	    reinterpret_cast<vk::PipelineDepthStencilStateCreateInfo const &>(depth_stencil_state),
	    static_cast<vk::PipelineLayout>(pipeline_layout),
	    static_cast<vk::RenderPass>(render_pass)));
}

inline VkImageView create_image_view(VkDevice           device,
                                     VkImage            image,
                                     VkImageViewType    view_type,
                                     VkFormat           format,
                                     VkImageAspectFlags aspect_mask,
                                     uint32_t           base_mip_level,
                                     uint32_t           level_count,
                                     uint32_t           base_array_layer,
                                     uint32_t           layer_count)
{
	return static_cast<VkImageView>(create_image_view(static_cast<vk::Device>(device),
	                                                  static_cast<vk::Image>(image),
	                                                  static_cast<vk::ImageViewType>(view_type),
	                                                  static_cast<vk::Format>(format),
	                                                  static_cast<vk::ImageAspectFlags>(aspect_mask),
	                                                  base_mip_level,
	                                                  level_count,
	                                                  base_array_layer,
	                                                  layer_count));
}

inline VkQueryPool create_query_pool(VkDevice                      device,
                                     VkQueryType                   query_type,
                                     uint32_t                      query_count,
                                     VkQueryPipelineStatisticFlags pipeline_statistics)
{
	return static_cast<VkQueryPool>(
	    create_query_pool(static_cast<vk::Device>(device),
	                      static_cast<vk::QueryType>(query_type),
	                      query_count,
	                      static_cast<vk::QueryPipelineStatisticFlags>(pipeline_statistics)));
}

inline VkSampler create_sampler(VkDevice             device,
                                VkFilter             mag_filter,
                                VkFilter             min_filter,
                                VkSamplerMipmapMode  mipmap_mode,
                                VkSamplerAddressMode sampler_address_mode,
                                float                max_anisotropy,
                                float                max_LOD)
{
	return static_cast<VkSampler>(create_sampler(static_cast<vk::Device>(device),
	                                             static_cast<vk::Filter>(mag_filter),
	                                             static_cast<vk::Filter>(min_filter),
	                                             static_cast<vk::SamplerMipmapMode>(mipmap_mode),
	                                             static_cast<vk::SamplerAddressMode>(sampler_address_mode),
	                                             max_anisotropy,
	                                             max_LOD));
}

inline VkSampler create_sampler(VkPhysicalDevice     gpu,
                                VkDevice             device,
                                VkFormat             format,
                                VkFilter             filter,
                                VkSamplerAddressMode sampler_address_mode,
                                float                max_anisotropy,
                                float                max_LOD)
{
	return static_cast<VkSampler>(create_sampler(static_cast<vk::PhysicalDevice>(gpu),
	                                             static_cast<vk::Device>(device),
	                                             static_cast<vk::Format>(format),
	                                             static_cast<vk::Filter>(filter),
	                                             static_cast<vk::SamplerAddressMode>(sampler_address_mode),
	                                             max_anisotropy,
	                                             max_LOD));
}

inline constexpr int32_t get_bits_per_pixel(VkFormat format)
{
	return get_bits_per_pixel(static_cast<vk::Format>(format));
}

inline constexpr VkImageAspectFlags get_image_aspect_flags(VkImageUsageFlagBits usage, VkFormat format)
{
	return static_cast<VkImageAspectFlags>(
	    get_image_aspect_flags(static_cast<vk::ImageUsageFlagBits>(usage), static_cast<vk::Format>(format)));
}

inline void image_layout_transition(VkCommandBuffer                command_buffer,
                                    VkImage                        image,
                                    VkPipelineStageFlags           src_stage_mask,
                                    VkPipelineStageFlags           dst_stage_mask,
                                    VkAccessFlags                  src_access_mask,
                                    VkAccessFlags                  dst_access_mask,
                                    VkImageLayout                  old_layout,
                                    VkImageLayout                  new_layout,
                                    VkImageSubresourceRange const &subresource_range)
{
	image_layout_transition(static_cast<vk::CommandBuffer>(command_buffer),
	                        static_cast<vk::Image>(image),
	                        static_cast<vk::PipelineStageFlags>(src_stage_mask),
	                        static_cast<vk::PipelineStageFlags>(dst_stage_mask),
	                        static_cast<vk::AccessFlags>(src_access_mask),
	                        static_cast<vk::AccessFlags>(dst_access_mask),
	                        static_cast<vk::ImageLayout>(old_layout),
	                        static_cast<vk::ImageLayout>(new_layout),
	                        reinterpret_cast<vk::ImageSubresourceRange const &>(subresource_range));
}

inline void image_layout_transition(VkCommandBuffer                command_buffer,
                                    VkImage                        image,
                                    VkImageLayout                  old_layout,
                                    VkImageLayout                  new_layout,
                                    VkImageSubresourceRange const &subresource_range)
{
	image_layout_transition(static_cast<vk::CommandBuffer>(command_buffer),
	                        static_cast<vk::Image>(image),
	                        static_cast<vk::ImageLayout>(old_layout),
	                        static_cast<vk::ImageLayout>(new_layout),
	                        reinterpret_cast<vk::ImageSubresourceRange const &>(subresource_range));
}

inline void image_layout_transition(VkCommandBuffer command_buffer,
                                    VkImage         image,
                                    VkImageLayout   old_layout,
                                    VkImageLayout   new_layout)
{
	image_layout_transition(static_cast<vk::CommandBuffer>(command_buffer),
	                        static_cast<vk::Image>(image),
	                        static_cast<vk::ImageLayout>(old_layout),
	                        static_cast<vk::ImageLayout>(new_layout));
}

inline void image_layout_transition(VkCommandBuffer                                                 command_buffer,
                                    std::vector<std::pair<VkImage, VkImageSubresourceRange>> const &imagesAndRanges,
                                    VkImageLayout                                                   old_layout,
                                    VkImageLayout                                                   new_layout)
{
	image_layout_transition(
	    static_cast<vk::CommandBuffer>(command_buffer),
	    reinterpret_cast<std::vector<std::pair<vk::Image, vk::ImageSubresourceRange>> const &>(imagesAndRanges),
	    static_cast<vk::ImageLayout>(old_layout),
	    static_cast<vk::ImageLayout>(new_layout));
}

inline constexpr bool is_buffer_descriptor_type(VkDescriptorType descriptor_type)
{
	return is_buffer_descriptor_type(static_cast<vk::DescriptorType>(descriptor_type));
}

inline constexpr bool is_depth_format(VkFormat format)
{
	return is_depth_format(static_cast<vk::Format>(format));
}

inline constexpr bool is_depth_only_format(VkFormat format)
{
	return is_depth_only_format(static_cast<vk::Format>(format));
}

inline constexpr bool is_depth_stencil_format(VkFormat format)
{
	return is_depth_stencil_format(static_cast<vk::Format>(format));
}

inline constexpr bool is_dynamic_buffer_descriptor_type(VkDescriptorType descriptor_type)
{
	return is_dynamic_buffer_descriptor_type(static_cast<vk::DescriptorType>(descriptor_type));
}

inline VkShaderModule load_shader(std::string const &filename, VkDevice device, VkShaderStageFlagBits stage)
{
	return static_cast<VkShaderModule>(
	    load_shader(filename, static_cast<vk::Device>(device), static_cast<vk::ShaderStageFlagBits>(stage)));
}

inline VkShaderModule load_shader_from_vector(const std::vector<uint32_t> &spirv, VkDevice device)
{
	return static_cast<VkShaderModule>(load_shader_from_vector(spirv, static_cast<vk::Device>(device)));
}

inline void make_filters_valid(VkPhysicalDevice     physical_device,
                               VkFormat             format,
                               VkFilter            *filter,
                               VkSamplerMipmapMode *mipmapMode)
{
	make_filters_valid(static_cast<vk::PhysicalDevice>(physical_device),
	                   static_cast<vk::Format>(format),
	                   reinterpret_cast<vk::Filter *>(filter),
	                   reinterpret_cast<vk::SamplerMipmapMode *>(mipmapMode));
}

template <typename StructExtension>
inline StructExtension query_image_subresource_layout_extension(VkDevice device, VkImage image)
{
	return static_cast<StructExtension>(
	    query_image_subresource_layout_extension<typename vk::CppType<StructExtension>::Type>(
	        static_cast<vk::Device>(device), static_cast<vk::Image>(image)));
}

inline VkFormat select_blendable_format(VkPhysicalDevice             physical_device,
                                        std::vector<VkFormat> const &format_priority_list)
{
	return static_cast<VkFormat>(
	    select_blendable_format(static_cast<vk::PhysicalDevice>(physical_device),
	                            reinterpret_cast<std::vector<vk::Format> const &>(format_priority_list)));
}

inline uint32_t select_queue_family_index(std::vector<VkQueueFamilyProperties> const &queue_family_properties,
                                          VkQueueFlagBits                             queue_flag)
{
	return select_queue_family_index(
	    reinterpret_cast<std::vector<vk::QueueFamilyProperties> const &>(queue_family_properties),
	    static_cast<vk::QueueFlagBits>(queue_flag));
}

inline VkFormat select_suitable_depth_format(VkPhysicalDevice             physical_device,
                                             bool                         depth_only,
                                             std::vector<VkFormat> const &depth_format_priority_list)
{
	return static_cast<VkFormat>(
	    select_suitable_depth_format(static_cast<vk::PhysicalDevice>(physical_device),
	                                 depth_only,
	                                 reinterpret_cast<std::vector<vk::Format> const &>(depth_format_priority_list)));
}

inline VkSurfaceFormatKHR
    select_surface_format(VkPhysicalDevice gpu, VkSurfaceKHR surface, std::vector<VkFormat> const &format_priority_list)
{
	return static_cast<VkSurfaceFormatKHR>(
	    select_surface_format(static_cast<vk::PhysicalDevice>(gpu),
	                          static_cast<vk::SurfaceKHR>(surface),
	                          reinterpret_cast<std::vector<vk::Format> const &>(format_priority_list)));
}

inline void submit_and_wait(VkDevice                     device,
                            VkQueue                      queue,
                            std::vector<VkCommandBuffer> command_buffers,
                            std::vector<VkSemaphore>     semaphores)
{
	return submit_and_wait(static_cast<vk::Device>(device),
	                       static_cast<vk::Queue>(queue),
	                       reinterpret_cast<std::vector<vk::CommandBuffer> const &>(command_buffers),
	                       reinterpret_cast<std::vector<vk::Semaphore> const &>(semaphores));
}

}        // namespace common
}        // namespace vkb
