/* Copyright (c) 2019-2024, Sascha Willems
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

#include "volk.h"
#include <vector>

namespace vkb
{
namespace initializers
{
inline VkMemoryAllocateInfo memory_allocate_info()
{
	VkMemoryAllocateInfo memory_allocation{};
	memory_allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	return memory_allocation;
}

inline VkMappedMemoryRange mapped_memory_range()
{
	VkMappedMemoryRange mapped_memory_range{};
	mapped_memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	return mapped_memory_range;
}

inline VkCommandBufferAllocateInfo command_buffer_allocate_info(
    VkCommandPool        command_pool,
    VkCommandBufferLevel level,
    uint32_t             buffer_count)
{
	VkCommandBufferAllocateInfo command_buffer_allocate_info{};
	command_buffer_allocate_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.commandPool        = command_pool;
	command_buffer_allocate_info.level              = level;
	command_buffer_allocate_info.commandBufferCount = buffer_count;
	return command_buffer_allocate_info;
}

inline VkCommandPoolCreateInfo command_pool_create_info()
{
	VkCommandPoolCreateInfo command_pool_create_info{};
	command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	return command_pool_create_info;
}

inline VkCommandBufferBeginInfo command_buffer_begin_info()
{
	VkCommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	return cmdBufferBeginInfo;
}

inline VkCommandBufferInheritanceInfo command_buffer_inheritance_info()
{
	VkCommandBufferInheritanceInfo command_buffer_inheritance_info{};
	command_buffer_inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	return command_buffer_inheritance_info;
}

inline VkRenderPassBeginInfo render_pass_begin_info()
{
	VkRenderPassBeginInfo render_pass_begin_info{};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	return render_pass_begin_info;
}

inline VkRenderPassCreateInfo render_pass_create_info()
{
	VkRenderPassCreateInfo render_pass_create_info{};
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	return render_pass_create_info;
}

/** @brief Initialize rendering_attachment_info */
inline VkRenderingAttachmentInfoKHR rendering_attachment_info()
{
	VkRenderingAttachmentInfoKHR attachment_info{};
	attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	attachment_info.pNext = VK_NULL_HANDLE;
	return attachment_info;
}

/** @brief Initialize VkRenderingInfoKHR, e.g. for use with dynamic rendering extension */
inline VkRenderingInfoKHR rendering_info(VkRect2D                            render_area            = {},
                                         uint32_t                            color_attachment_count = 0,
                                         const VkRenderingAttachmentInfoKHR *pColorAttachments      = VK_NULL_HANDLE,
                                         VkRenderingFlagsKHR                 flags                  = 0)
{
	VkRenderingInfoKHR rendering_info   = {};
	rendering_info.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	rendering_info.pNext                = VK_NULL_HANDLE;
	rendering_info.flags                = flags;
	rendering_info.renderArea           = render_area;
	rendering_info.layerCount           = 0;
	rendering_info.viewMask             = 0;
	rendering_info.colorAttachmentCount = color_attachment_count;
	rendering_info.pColorAttachments    = pColorAttachments;
	rendering_info.pDepthAttachment     = VK_NULL_HANDLE;
	rendering_info.pStencilAttachment   = VK_NULL_HANDLE;
	return rendering_info;
}

/** @brief Initialize an image memory barrier with no image transfer ownership */
inline VkImageMemoryBarrier image_memory_barrier()
{
	VkImageMemoryBarrier image_memory_barrier{};
	image_memory_barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	return image_memory_barrier;
}

/** @brief Initialize a buffer memory barrier with no image transfer ownership */
inline VkBufferMemoryBarrier buffer_memory_barrier()
{
	VkBufferMemoryBarrier buffer_memory_barrier{};
	buffer_memory_barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	buffer_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	buffer_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	return buffer_memory_barrier;
}

inline VkMemoryBarrier memory_barrier()
{
	VkMemoryBarrier memory_barrier{};
	memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	return memory_barrier;
}

inline VkImageCreateInfo image_create_info()
{
	VkImageCreateInfo image_create_info{};
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	return image_create_info;
}

inline VkSamplerCreateInfo sampler_create_info()
{
	VkSamplerCreateInfo sampler_create_info{};
	sampler_create_info.sType         = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_create_info.maxAnisotropy = 1.0f;
	return sampler_create_info;
}

inline VkImageViewCreateInfo image_view_create_info()
{
	VkImageViewCreateInfo image_view_create_info{};
	image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	return image_view_create_info;
}

inline VkFramebufferCreateInfo framebuffer_create_info()
{
	VkFramebufferCreateInfo framebuffer_create_info{};
	framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	return framebuffer_create_info;
}

inline VkSemaphoreCreateInfo semaphore_create_info()
{
	VkSemaphoreCreateInfo semaphore_create_info{};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	return semaphore_create_info;
}

inline VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0)
{
	VkFenceCreateInfo fence_create_info{};
	fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_create_info.flags = flags;
	return fence_create_info;
}

inline VkEventCreateInfo event_create_info()
{
	VkEventCreateInfo event_create_info{};
	event_create_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
	return event_create_info;
}

inline VkSubmitInfo submit_info()
{
	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	return submit_info;
}

inline VkViewport viewport(
    float width,
    float height,
    float min_depth,
    float max_depth)
{
	VkViewport viewport{};
	viewport.width    = width;
	viewport.height   = height;
	viewport.minDepth = min_depth;
	viewport.maxDepth = max_depth;
	return viewport;
}

inline VkRect2D rect2D(
    int32_t width,
    int32_t height,
    int32_t offset_x,
    int32_t offset_y)
{
	VkRect2D rect2D{};
	rect2D.extent.width  = width;
	rect2D.extent.height = height;
	rect2D.offset.x      = offset_x;
	rect2D.offset.y      = offset_y;
	return rect2D;
}

inline VkBufferCreateInfo buffer_create_info()
{
	VkBufferCreateInfo buffer_create_info{};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	return buffer_create_info;
}

inline VkBufferCreateInfo buffer_create_info(
    VkBufferUsageFlags usage,
    VkDeviceSize       size)
{
	VkBufferCreateInfo buffer_create_info{};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.usage = usage;
	buffer_create_info.size  = size;
	return buffer_create_info;
}

inline VkDescriptorPoolCreateInfo descriptor_pool_create_info(
    uint32_t              count,
    VkDescriptorPoolSize *pool_sizes,
    uint32_t              max_sets)
{
	VkDescriptorPoolCreateInfo descriptor_pool_info{};
	descriptor_pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_info.poolSizeCount = count;
	descriptor_pool_info.pPoolSizes    = pool_sizes;
	descriptor_pool_info.maxSets       = max_sets;
	return descriptor_pool_info;
}

inline VkDescriptorPoolCreateInfo descriptor_pool_create_info(
    const std::vector<VkDescriptorPoolSize> &pool_sizes,
    uint32_t                                 max_sets)
{
	VkDescriptorPoolCreateInfo descriptor_pool_info{};
	descriptor_pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	descriptor_pool_info.pPoolSizes    = pool_sizes.data();
	descriptor_pool_info.maxSets       = max_sets;
	return descriptor_pool_info;
}

inline VkDescriptorPoolSize descriptor_pool_size(
    VkDescriptorType type,
    uint32_t         count)
{
	VkDescriptorPoolSize descriptor_pool_size{};
	descriptor_pool_size.type            = type;
	descriptor_pool_size.descriptorCount = count;
	return descriptor_pool_size;
}

inline VkDescriptorSetLayoutBinding descriptor_set_layout_binding(
    VkDescriptorType   type,
    VkShaderStageFlags flags,
    uint32_t           binding,
    uint32_t           count = 1)
{
	VkDescriptorSetLayoutBinding set_layout_binding{};
	set_layout_binding.descriptorType  = type;
	set_layout_binding.stageFlags      = flags;
	set_layout_binding.binding         = binding;
	set_layout_binding.descriptorCount = count;
	return set_layout_binding;
}

inline VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info(
    const VkDescriptorSetLayoutBinding *bindings,
    uint32_t                            binding_count)
{
	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{};
	descriptor_set_layout_create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_set_layout_create_info.pBindings    = bindings;
	descriptor_set_layout_create_info.bindingCount = binding_count;
	return descriptor_set_layout_create_info;
}

inline VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info(
    const std::vector<VkDescriptorSetLayoutBinding> &bindings)
{
	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{};
	descriptor_set_layout_create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_set_layout_create_info.pBindings    = bindings.data();
	descriptor_set_layout_create_info.bindingCount = static_cast<uint32_t>(bindings.size());
	return descriptor_set_layout_create_info;
}

inline VkPipelineLayoutCreateInfo pipeline_layout_create_info(
    const VkDescriptorSetLayout *set_layouts,
    uint32_t                     set_layout_count = 1)
{
	VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
	pipeline_layout_create_info.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount = set_layout_count;
	pipeline_layout_create_info.pSetLayouts    = set_layouts;
	return pipeline_layout_create_info;
}

inline VkPipelineLayoutCreateInfo pipeline_layout_create_info(
    uint32_t set_layout_count = 1)
{
	VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
	pipeline_layout_create_info.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount = set_layout_count;
	return pipeline_layout_create_info;
}

inline VkDescriptorSetAllocateInfo descriptor_set_allocate_info(
    VkDescriptorPool             descriptor_pool,
    const VkDescriptorSetLayout *set_layouts,
    uint32_t                     descriptor_set_count)
{
	VkDescriptorSetAllocateInfo descriptor_set_allocate_info{};
	descriptor_set_allocate_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_allocate_info.descriptorPool     = descriptor_pool;
	descriptor_set_allocate_info.pSetLayouts        = set_layouts;
	descriptor_set_allocate_info.descriptorSetCount = descriptor_set_count;
	return descriptor_set_allocate_info;
}

inline VkDescriptorImageInfo descriptor_image_info(VkSampler sampler, VkImageView image_view, VkImageLayout image_layout)
{
	VkDescriptorImageInfo descriptor_image_info{};
	descriptor_image_info.sampler     = sampler;
	descriptor_image_info.imageView   = image_view;
	descriptor_image_info.imageLayout = image_layout;
	return descriptor_image_info;
}

inline VkWriteDescriptorSet write_descriptor_set(
    VkDescriptorSet         dst_set,
    VkDescriptorType        type,
    uint32_t                binding,
    VkDescriptorBufferInfo *buffer_info,
    uint32_t                descriptor_count = 1)
{
	VkWriteDescriptorSet write_descriptor_set{};
	write_descriptor_set.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set.dstSet          = dst_set;
	write_descriptor_set.descriptorType  = type;
	write_descriptor_set.dstBinding      = binding;
	write_descriptor_set.pBufferInfo     = buffer_info;
	write_descriptor_set.descriptorCount = descriptor_count;
	return write_descriptor_set;
}

inline VkWriteDescriptorSet write_descriptor_set(
    VkDescriptorSet        dst_set,
    VkDescriptorType       type,
    uint32_t               binding,
    VkDescriptorImageInfo *image_info,
    uint32_t               descriptor_count = 1)
{
	VkWriteDescriptorSet write_descriptor_set{};
	write_descriptor_set.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set.dstSet          = dst_set;
	write_descriptor_set.descriptorType  = type;
	write_descriptor_set.dstBinding      = binding;
	write_descriptor_set.pImageInfo      = image_info;
	write_descriptor_set.descriptorCount = descriptor_count;
	return write_descriptor_set;
}

inline VkVertexInputBindingDescription vertex_input_binding_description(
    uint32_t          binding,
    uint32_t          stride,
    VkVertexInputRate input_rate)
{
	VkVertexInputBindingDescription vertex_input_binding_description{};
	vertex_input_binding_description.binding   = binding;
	vertex_input_binding_description.stride    = stride;
	vertex_input_binding_description.inputRate = input_rate;
	return vertex_input_binding_description;
}

inline VkVertexInputBindingDescription2EXT vertex_input_binding_description2ext(
    uint32_t          binding,
    uint32_t          stride,
    VkVertexInputRate input_rate,
    uint32_t          divisor)
{
	VkVertexInputBindingDescription2EXT vertex_input_binding_description2ext{};
	vertex_input_binding_description2ext.sType     = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
	vertex_input_binding_description2ext.binding   = binding;
	vertex_input_binding_description2ext.stride    = stride;
	vertex_input_binding_description2ext.inputRate = input_rate;
	vertex_input_binding_description2ext.divisor   = divisor;
	return vertex_input_binding_description2ext;
}

inline VkVertexInputAttributeDescription vertex_input_attribute_description(
    uint32_t binding,
    uint32_t location,
    VkFormat format,
    uint32_t offset)
{
	VkVertexInputAttributeDescription vertex_input_attribute_description{};
	vertex_input_attribute_description.location = location;
	vertex_input_attribute_description.binding  = binding;
	vertex_input_attribute_description.format   = format;
	vertex_input_attribute_description.offset   = offset;
	return vertex_input_attribute_description;
}

inline VkVertexInputAttributeDescription2EXT vertex_input_attribute_description2ext(
    uint32_t binding,
    uint32_t location,
    VkFormat format,
    uint32_t offset)
{
	VkVertexInputAttributeDescription2EXT vertex_input_attribute_description2ext{};
	vertex_input_attribute_description2ext.sType    = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
	vertex_input_attribute_description2ext.location = location;
	vertex_input_attribute_description2ext.binding  = binding;
	vertex_input_attribute_description2ext.format   = format;
	vertex_input_attribute_description2ext.offset   = offset;
	return vertex_input_attribute_description2ext;
}

inline VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info()
{
	VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info{};
	pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	return pipeline_vertex_input_state_create_info;
}

inline VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info(
    VkPrimitiveTopology                     topology,
    VkPipelineInputAssemblyStateCreateFlags flags,
    VkBool32                                primitive_restart_enable)
{
	VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info{};
	pipeline_input_assembly_state_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipeline_input_assembly_state_create_info.topology               = topology;
	pipeline_input_assembly_state_create_info.flags                  = flags;
	pipeline_input_assembly_state_create_info.primitiveRestartEnable = primitive_restart_enable;
	return pipeline_input_assembly_state_create_info;
}

inline VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info(
    VkPolygonMode                           polygon_mode,
    VkCullModeFlags                         cull_mode,
    VkFrontFace                             front_face,
    VkPipelineRasterizationStateCreateFlags flags = 0)
{
	VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info{};
	pipeline_rasterization_state_create_info.sType            = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipeline_rasterization_state_create_info.polygonMode      = polygon_mode;
	pipeline_rasterization_state_create_info.cullMode         = cull_mode;
	pipeline_rasterization_state_create_info.frontFace        = front_face;
	pipeline_rasterization_state_create_info.flags            = flags;
	pipeline_rasterization_state_create_info.depthClampEnable = VK_FALSE;
	pipeline_rasterization_state_create_info.lineWidth        = 1.0f;
	return pipeline_rasterization_state_create_info;
}

inline VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state(
    VkColorComponentFlags color_write_mask,
    VkBool32              blend_enable)
{
	VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state{};
	pipeline_color_blend_attachment_state.colorWriteMask = color_write_mask;
	pipeline_color_blend_attachment_state.blendEnable    = blend_enable;
	return pipeline_color_blend_attachment_state;
}

inline VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info(
    uint32_t                                   attachment_count,
    const VkPipelineColorBlendAttachmentState *attachments)
{
	VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info{};
	pipeline_color_blend_state_create_info.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pipeline_color_blend_state_create_info.attachmentCount = attachment_count;
	pipeline_color_blend_state_create_info.pAttachments    = attachments;
	return pipeline_color_blend_state_create_info;
}

inline VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info(
    VkBool32    depth_test_enable,
    VkBool32    depth_write_enable,
    VkCompareOp depth_compare_op)
{
	VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info{};
	pipeline_depth_stencil_state_create_info.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipeline_depth_stencil_state_create_info.depthTestEnable  = depth_test_enable;
	pipeline_depth_stencil_state_create_info.depthWriteEnable = depth_write_enable;
	pipeline_depth_stencil_state_create_info.depthCompareOp   = depth_compare_op;
	pipeline_depth_stencil_state_create_info.front            = pipeline_depth_stencil_state_create_info.back;
	pipeline_depth_stencil_state_create_info.back.compareOp   = VK_COMPARE_OP_ALWAYS;
	return pipeline_depth_stencil_state_create_info;
}

inline VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info(
    uint32_t                           viewport_count,
    uint32_t                           scissor_count,
    VkPipelineViewportStateCreateFlags flags = 0)
{
	VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info{};
	pipeline_viewport_state_create_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipeline_viewport_state_create_info.viewportCount = viewport_count;
	pipeline_viewport_state_create_info.scissorCount  = scissor_count;
	pipeline_viewport_state_create_info.flags         = flags;
	return pipeline_viewport_state_create_info;
}

inline VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info(
    VkSampleCountFlagBits                 rasterization_samples,
    VkPipelineMultisampleStateCreateFlags flags = 0)
{
	VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info{};
	pipeline_multisample_state_create_info.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipeline_multisample_state_create_info.rasterizationSamples = rasterization_samples;
	pipeline_multisample_state_create_info.flags                = flags;
	return pipeline_multisample_state_create_info;
}

inline VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info(
    const VkDynamicState             *dynamic_states,
    uint32_t                          dynamicStateCount,
    VkPipelineDynamicStateCreateFlags flags = 0)
{
	VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info{};
	pipeline_dynamic_state_create_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipeline_dynamic_state_create_info.pDynamicStates    = dynamic_states;
	pipeline_dynamic_state_create_info.dynamicStateCount = dynamicStateCount;
	pipeline_dynamic_state_create_info.flags             = flags;
	return pipeline_dynamic_state_create_info;
}

inline VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info(
    const std::vector<VkDynamicState> &dynamic_states,
    VkPipelineDynamicStateCreateFlags  flags = 0)
{
	VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info{};
	pipeline_dynamic_state_create_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipeline_dynamic_state_create_info.pDynamicStates    = dynamic_states.data();
	pipeline_dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
	pipeline_dynamic_state_create_info.flags             = flags;
	return pipeline_dynamic_state_create_info;
}

inline VkPipelineTessellationStateCreateInfo pipeline_tessellation_state_create_info(uint32_t patch_control_points)
{
	VkPipelineTessellationStateCreateInfo pipeline_tessellation_state_create_info{};
	pipeline_tessellation_state_create_info.sType              = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	pipeline_tessellation_state_create_info.patchControlPoints = patch_control_points;
	return pipeline_tessellation_state_create_info;
}

inline VkGraphicsPipelineCreateInfo pipeline_create_info(
    VkPipelineLayout      layout,
    VkRenderPass          render_pass,
    VkPipelineCreateFlags flags = 0)
{
	VkGraphicsPipelineCreateInfo pipeline_create_info{};
	pipeline_create_info.sType              = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_create_info.layout             = layout;
	pipeline_create_info.renderPass         = render_pass;
	pipeline_create_info.flags              = flags;
	pipeline_create_info.basePipelineIndex  = -1;
	pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	return pipeline_create_info;
}

inline VkGraphicsPipelineCreateInfo pipeline_create_info()
{
	VkGraphicsPipelineCreateInfo pipeline_create_info{};
	pipeline_create_info.sType              = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_create_info.basePipelineIndex  = -1;
	pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	return pipeline_create_info;
}

inline VkComputePipelineCreateInfo compute_pipeline_create_info(
    VkPipelineLayout      layout,
    VkPipelineCreateFlags flags = 0)
{
	VkComputePipelineCreateInfo compute_pipeline_create_info{};
	compute_pipeline_create_info.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	compute_pipeline_create_info.layout = layout;
	compute_pipeline_create_info.flags  = flags;
	return compute_pipeline_create_info;
}

inline VkPushConstantRange push_constant_range(
    VkShaderStageFlags stage_flags,
    uint32_t           size,
    uint32_t           offset)
{
	VkPushConstantRange push_constant_range{};
	push_constant_range.stageFlags = stage_flags;
	push_constant_range.offset     = offset;
	push_constant_range.size       = size;
	return push_constant_range;
}

inline VkBindSparseInfo bind_sparse_info()
{
	VkBindSparseInfo bind_sparse_info{};
	bind_sparse_info.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
	return bind_sparse_info;
}

/** @brief Initialize a map entry for a shader specialization constant */
inline VkSpecializationMapEntry specialization_map_entry(uint32_t constant_id, uint32_t offset, size_t size)
{
	VkSpecializationMapEntry specialization_map_entry{};
	specialization_map_entry.constantID = constant_id;
	specialization_map_entry.offset     = offset;
	specialization_map_entry.size       = size;
	return specialization_map_entry;
}

/** @brief Initialize a specialization constant info structure to pass to a shader stage */
inline VkSpecializationInfo specialization_info(uint32_t map_entry_count, const VkSpecializationMapEntry *map_entries, size_t data_size, const void *data)
{
	VkSpecializationInfo specialization_info{};
	specialization_info.mapEntryCount = map_entry_count;
	specialization_info.pMapEntries   = map_entries;
	specialization_info.dataSize      = data_size;
	specialization_info.pData         = data;
	return specialization_info;
}

inline VkTimelineSemaphoreSubmitInfo timeline_semaphore_submit_info(uint32_t wait_value_count, uint64_t *wait_values, uint32_t signal_value_count, uint64_t *signal_values)
{
	return VkTimelineSemaphoreSubmitInfo{
	    VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
	    NULL,
	    wait_value_count,
	    wait_values,
	    signal_value_count,
	    signal_values};
}
}        // namespace initializers
}        // namespace vkb
