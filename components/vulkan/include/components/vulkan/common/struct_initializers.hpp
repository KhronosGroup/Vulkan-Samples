/* Copyright (c) 2022, Arm Limited and Contributors
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

// DO NOT EDIT, THIS IS A GENERATED FILE!

#include <volk.h>

namespace components
{
namespace vulkan
{
template <typename Type>
VkStructureType get_structure_type_name()
{
	throw "function not implemented";
}

template <>
VkStructureType get_structure_type_name<VkBufferMemoryBarrier>()
{
	return VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
}
template <>
VkStructureType get_structure_type_name<VkImageMemoryBarrier>()
{
	return VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
}
template <>
VkStructureType get_structure_type_name<VkMemoryBarrier>()
{
	return VK_STRUCTURE_TYPE_MEMORY_BARRIER;
}
template <>
VkStructureType get_structure_type_name<VkApplicationInfo>()
{
	return VK_STRUCTURE_TYPE_APPLICATION_INFO;
}
template <>
VkStructureType get_structure_type_name<VkInstanceCreateInfo>()
{
	return VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkDeviceQueueCreateInfo>()
{
	return VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkDeviceCreateInfo>()
{
	return VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkSubmitInfo>()
{
	return VK_STRUCTURE_TYPE_SUBMIT_INFO;
}
template <>
VkStructureType get_structure_type_name<VkMappedMemoryRange>()
{
	return VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
}
template <>
VkStructureType get_structure_type_name<VkMemoryAllocateInfo>()
{
	return VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkBindSparseInfo>()
{
	return VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkFenceCreateInfo>()
{
	return VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkSemaphoreCreateInfo>()
{
	return VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkEventCreateInfo>()
{
	return VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkQueryPoolCreateInfo>()
{
	return VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkBufferCreateInfo>()
{
	return VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkBufferViewCreateInfo>()
{
	return VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkImageCreateInfo>()
{
	return VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkImageViewCreateInfo>()
{
	return VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkShaderModuleCreateInfo>()
{
	return VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPipelineCacheCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPipelineShaderStageCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkComputePipelineCreateInfo>()
{
	return VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPipelineVertexInputStateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPipelineInputAssemblyStateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPipelineTessellationStateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPipelineViewportStateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPipelineRasterizationStateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPipelineMultisampleStateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPipelineDepthStencilStateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPipelineColorBlendStateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPipelineDynamicStateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkGraphicsPipelineCreateInfo>()
{
	return VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPipelineLayoutCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkSamplerCreateInfo>()
{
	return VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkCopyDescriptorSet>()
{
	return VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
}
template <>
VkStructureType get_structure_type_name<VkDescriptorPoolCreateInfo>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkDescriptorSetAllocateInfo>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkDescriptorSetLayoutCreateInfo>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkWriteDescriptorSet>()
{
	return VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
}
template <>
VkStructureType get_structure_type_name<VkFramebufferCreateInfo>()
{
	return VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkRenderPassCreateInfo>()
{
	return VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkCommandPoolCreateInfo>()
{
	return VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkCommandBufferAllocateInfo>()
{
	return VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkCommandBufferInheritanceInfo>()
{
	return VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkCommandBufferBeginInfo>()
{
	return VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
}
template <>
VkStructureType get_structure_type_name<VkRenderPassBeginInfo>()
{
	return VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
}

template <>
VkStructureType get_structure_type_name<VkSwapchainCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPresentInfoKHR>()
{
	return VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkImageSwapchainCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_IMAGE_SWAPCHAIN_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkBindImageMemorySwapchainInfoKHR>()
{
	return VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_SWAPCHAIN_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkAcquireNextImageInfoKHR>()
{
	return VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkDeviceGroupPresentCapabilitiesKHR>()
{
	return VK_STRUCTURE_TYPE_DEVICE_GROUP_PRESENT_CAPABILITIES_KHR;
}
template <>
VkStructureType get_structure_type_name<VkDeviceGroupPresentInfoKHR>()
{
	return VK_STRUCTURE_TYPE_DEVICE_GROUP_PRESENT_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkDeviceGroupSwapchainCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_DEVICE_GROUP_SWAPCHAIN_CREATE_INFO_KHR;
}

}        // namespace vulkan
}        // namespace components
