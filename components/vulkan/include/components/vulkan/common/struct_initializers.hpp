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
VkStructureType get_structure_type_name<VkPhysicalDeviceSubgroupProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkBindBufferMemoryInfo>()
{
	return VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO;
}
template <>
VkStructureType get_structure_type_name<VkBindImageMemoryInfo>()
{
	return VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDevice16BitStorageFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkMemoryDedicatedRequirements>()
{
	return VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS;
}
template <>
VkStructureType get_structure_type_name<VkMemoryDedicatedAllocateInfo>()
{
	return VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkMemoryAllocateFlagsInfo>()
{
	return VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
}
template <>
VkStructureType get_structure_type_name<VkDeviceGroupRenderPassBeginInfo>()
{
	return VK_STRUCTURE_TYPE_DEVICE_GROUP_RENDER_PASS_BEGIN_INFO;
}
template <>
VkStructureType get_structure_type_name<VkDeviceGroupCommandBufferBeginInfo>()
{
	return VK_STRUCTURE_TYPE_DEVICE_GROUP_COMMAND_BUFFER_BEGIN_INFO;
}
template <>
VkStructureType get_structure_type_name<VkDeviceGroupSubmitInfo>()
{
	return VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO;
}
template <>
VkStructureType get_structure_type_name<VkDeviceGroupBindSparseInfo>()
{
	return VK_STRUCTURE_TYPE_DEVICE_GROUP_BIND_SPARSE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkBindBufferMemoryDeviceGroupInfo>()
{
	return VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_DEVICE_GROUP_INFO;
}
template <>
VkStructureType get_structure_type_name<VkBindImageMemoryDeviceGroupInfo>()
{
	return VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_DEVICE_GROUP_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceGroupProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkDeviceGroupDeviceCreateInfo>()
{
	return VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkBufferMemoryRequirementsInfo2>()
{
	return VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;
}
template <>
VkStructureType get_structure_type_name<VkImageMemoryRequirementsInfo2>()
{
	return VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
}
template <>
VkStructureType get_structure_type_name<VkImageSparseMemoryRequirementsInfo2>()
{
	return VK_STRUCTURE_TYPE_IMAGE_SPARSE_MEMORY_REQUIREMENTS_INFO_2;
}
template <>
VkStructureType get_structure_type_name<VkMemoryRequirements2>()
{
	return VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
}
template <>
VkStructureType get_structure_type_name<VkSparseImageMemoryRequirements2>()
{
	return VK_STRUCTURE_TYPE_SPARSE_IMAGE_MEMORY_REQUIREMENTS_2;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceFeatures2>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceProperties2>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
}
template <>
VkStructureType get_structure_type_name<VkFormatProperties2>()
{
	return VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
}
template <>
VkStructureType get_structure_type_name<VkImageFormatProperties2>()
{
	return VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceImageFormatInfo2>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
}
template <>
VkStructureType get_structure_type_name<VkQueueFamilyProperties2>()
{
	return VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceMemoryProperties2>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
}
template <>
VkStructureType get_structure_type_name<VkSparseImageFormatProperties2>()
{
	return VK_STRUCTURE_TYPE_SPARSE_IMAGE_FORMAT_PROPERTIES_2;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceSparseImageFormatInfo2>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SPARSE_IMAGE_FORMAT_INFO_2;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDevicePointClippingProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_POINT_CLIPPING_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkRenderPassInputAttachmentAspectCreateInfo>()
{
	return VK_STRUCTURE_TYPE_RENDER_PASS_INPUT_ATTACHMENT_ASPECT_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkImageViewUsageCreateInfo>()
{
	return VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPipelineTessellationDomainOriginStateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkRenderPassMultiviewCreateInfo>()
{
	return VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceMultiviewFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceMultiviewProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceVariablePointersFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceProtectedMemoryFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceProtectedMemoryProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkDeviceQueueInfo2>()
{
	return VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
}
template <>
VkStructureType get_structure_type_name<VkProtectedSubmitInfo>()
{
	return VK_STRUCTURE_TYPE_PROTECTED_SUBMIT_INFO;
}
template <>
VkStructureType get_structure_type_name<VkSamplerYcbcrConversionCreateInfo>()
{
	return VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkSamplerYcbcrConversionInfo>()
{
	return VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO;
}
template <>
VkStructureType get_structure_type_name<VkBindImagePlaneMemoryInfo>()
{
	return VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO;
}
template <>
VkStructureType get_structure_type_name<VkImagePlaneMemoryRequirementsInfo>()
{
	return VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceSamplerYcbcrConversionFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkSamplerYcbcrConversionImageFormatProperties>()
{
	return VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_IMAGE_FORMAT_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkDescriptorUpdateTemplateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceExternalImageFormatInfo>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO;
}
template <>
VkStructureType get_structure_type_name<VkExternalImageFormatProperties>()
{
	return VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceExternalBufferInfo>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_BUFFER_INFO;
}
template <>
VkStructureType get_structure_type_name<VkExternalBufferProperties>()
{
	return VK_STRUCTURE_TYPE_EXTERNAL_BUFFER_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceIDProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkExternalMemoryImageCreateInfo>()
{
	return VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkExternalMemoryBufferCreateInfo>()
{
	return VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkExportMemoryAllocateInfo>()
{
	return VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceExternalFenceInfo>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_FENCE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkExternalFenceProperties>()
{
	return VK_STRUCTURE_TYPE_EXTERNAL_FENCE_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkExportFenceCreateInfo>()
{
	return VK_STRUCTURE_TYPE_EXPORT_FENCE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkExportSemaphoreCreateInfo>()
{
	return VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceExternalSemaphoreInfo>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkExternalSemaphoreProperties>()
{
	return VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceMaintenance3Properties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkDescriptorSetLayoutSupport>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_SUPPORT;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShaderDrawParametersFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
}

template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceVulkan11Features>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceVulkan11Properties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceVulkan12Features>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceVulkan12Properties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkImageFormatListCreateInfo>()
{
	return VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkAttachmentDescription2>()
{
	return VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
}
template <>
VkStructureType get_structure_type_name<VkAttachmentReference2>()
{
	return VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
}
template <>
VkStructureType get_structure_type_name<VkSubpassDescription2>()
{
	return VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
}
template <>
VkStructureType get_structure_type_name<VkSubpassDependency2>()
{
	return VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
}
template <>
VkStructureType get_structure_type_name<VkRenderPassCreateInfo2>()
{
	return VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
}
template <>
VkStructureType get_structure_type_name<VkSubpassBeginInfo>()
{
	return VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO;
}
template <>
VkStructureType get_structure_type_name<VkSubpassEndInfo>()
{
	return VK_STRUCTURE_TYPE_SUBPASS_END_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDevice8BitStorageFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceDriverProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShaderAtomicInt64Features>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShaderFloat16Int8Features>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceFloatControlsProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkDescriptorSetLayoutBindingFlagsCreateInfo>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceDescriptorIndexingFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceDescriptorIndexingProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkDescriptorSetVariableDescriptorCountAllocateInfo>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkDescriptorSetVariableDescriptorCountLayoutSupport>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_LAYOUT_SUPPORT;
}
template <>
VkStructureType get_structure_type_name<VkSubpassDescriptionDepthStencilResolve>()
{
	return VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceDepthStencilResolveProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceScalarBlockLayoutFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkImageStencilUsageCreateInfo>()
{
	return VK_STRUCTURE_TYPE_IMAGE_STENCIL_USAGE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkSamplerReductionModeCreateInfo>()
{
	return VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceSamplerFilterMinmaxProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_FILTER_MINMAX_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceVulkanMemoryModelFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceImagelessFramebufferFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkFramebufferAttachmentImageInfo>()
{
	return VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkFramebufferAttachmentsCreateInfo>()
{
	return VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkRenderPassAttachmentBeginInfo>()
{
	return VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceUniformBufferStandardLayoutFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkAttachmentReferenceStencilLayout>()
{
	return VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_STENCIL_LAYOUT;
}
template <>
VkStructureType get_structure_type_name<VkAttachmentDescriptionStencilLayout>()
{
	return VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_STENCIL_LAYOUT;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceHostQueryResetFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceTimelineSemaphoreFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceTimelineSemaphoreProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkSemaphoreTypeCreateInfo>()
{
	return VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkTimelineSemaphoreSubmitInfo>()
{
	return VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
}
template <>
VkStructureType get_structure_type_name<VkSemaphoreWaitInfo>()
{
	return VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
}
template <>
VkStructureType get_structure_type_name<VkSemaphoreSignalInfo>()
{
	return VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceBufferDeviceAddressFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkBufferDeviceAddressInfo>()
{
	return VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
}
template <>
VkStructureType get_structure_type_name<VkBufferOpaqueCaptureAddressCreateInfo>()
{
	return VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkMemoryOpaqueCaptureAddressAllocateInfo>()
{
	return VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkDeviceMemoryOpaqueCaptureAddressInfo>()
{
	return VK_STRUCTURE_TYPE_DEVICE_MEMORY_OPAQUE_CAPTURE_ADDRESS_INFO;
}

template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceVulkan13Features>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceVulkan13Properties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkPipelineCreationFeedbackCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShaderTerminateInvocationFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TERMINATE_INVOCATION_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceToolProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TOOL_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDevicePrivateDataFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIVATE_DATA_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkDevicePrivateDataCreateInfo>()
{
	return VK_STRUCTURE_TYPE_DEVICE_PRIVATE_DATA_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPrivateDataSlotCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PRIVATE_DATA_SLOT_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDevicePipelineCreationCacheControlFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkMemoryBarrier2>()
{
	return VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
}
template <>
VkStructureType get_structure_type_name<VkBufferMemoryBarrier2>()
{
	return VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
}
template <>
VkStructureType get_structure_type_name<VkImageMemoryBarrier2>()
{
	return VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
}
template <>
VkStructureType get_structure_type_name<VkDependencyInfo>()
{
	return VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
}
template <>
VkStructureType get_structure_type_name<VkSemaphoreSubmitInfo>()
{
	return VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
}
template <>
VkStructureType get_structure_type_name<VkCommandBufferSubmitInfo>()
{
	return VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
}
template <>
VkStructureType get_structure_type_name<VkSubmitInfo2>()
{
	return VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceSynchronization2Features>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ZERO_INITIALIZE_WORKGROUP_MEMORY_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceImageRobustnessFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ROBUSTNESS_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkBufferCopy2>()
{
	return VK_STRUCTURE_TYPE_BUFFER_COPY_2;
}
template <>
VkStructureType get_structure_type_name<VkCopyBufferInfo2>()
{
	return VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
}
template <>
VkStructureType get_structure_type_name<VkImageCopy2>()
{
	return VK_STRUCTURE_TYPE_IMAGE_COPY_2;
}
template <>
VkStructureType get_structure_type_name<VkCopyImageInfo2>()
{
	return VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2;
}
template <>
VkStructureType get_structure_type_name<VkBufferImageCopy2>()
{
	return VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
}
template <>
VkStructureType get_structure_type_name<VkCopyBufferToImageInfo2>()
{
	return VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
}
template <>
VkStructureType get_structure_type_name<VkCopyImageToBufferInfo2>()
{
	return VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2;
}
template <>
VkStructureType get_structure_type_name<VkImageBlit2>()
{
	return VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
}
template <>
VkStructureType get_structure_type_name<VkBlitImageInfo2>()
{
	return VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
}
template <>
VkStructureType get_structure_type_name<VkImageResolve2>()
{
	return VK_STRUCTURE_TYPE_IMAGE_RESOLVE_2;
}
template <>
VkStructureType get_structure_type_name<VkResolveImageInfo2>()
{
	return VK_STRUCTURE_TYPE_RESOLVE_IMAGE_INFO_2;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceSubgroupSizeControlFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceSubgroupSizeControlProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkPipelineShaderStageRequiredSubgroupSizeCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceInlineUniformBlockFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceInlineUniformBlockProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkWriteDescriptorSetInlineUniformBlock>()
{
	return VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK;
}
template <>
VkStructureType get_structure_type_name<VkDescriptorPoolInlineUniformBlockCreateInfo>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_INLINE_UNIFORM_BLOCK_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceTextureCompressionASTCHDRFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXTURE_COMPRESSION_ASTC_HDR_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkRenderingAttachmentInfo>()
{
	return VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
}
template <>
VkStructureType get_structure_type_name<VkRenderingInfo>()
{
	return VK_STRUCTURE_TYPE_RENDERING_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPipelineRenderingCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceDynamicRenderingFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkCommandBufferInheritanceRenderingInfo>()
{
	return VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShaderIntegerDotProductFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShaderIntegerDotProductProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceTexelBufferAlignmentProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkFormatProperties3>()
{
	return VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceMaintenance4Features>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceMaintenance4Properties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES;
}
template <>
VkStructureType get_structure_type_name<VkDeviceBufferMemoryRequirements>()
{
	return VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS;
}
template <>
VkStructureType get_structure_type_name<VkDeviceImageMemoryRequirements>()
{
	return VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS;
}

}        // namespace vulkan
}        // namespace components
