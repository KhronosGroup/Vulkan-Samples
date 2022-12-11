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

#ifdef VK_VERSION_1_0
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

#endif

#ifdef VK_VERSION_1_1
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

#endif

#ifdef VK_VERSION_1_2
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

#endif

#ifdef VK_VERSION_1_3
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

#endif

#ifdef VK_KHR_swapchain
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

#endif

#ifdef VK_KHR_display
template <>
VkStructureType get_structure_type_name<VkDisplayModeCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_MODE_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkDisplaySurfaceCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
}

#endif

#ifdef VK_KHR_display_swapchain
template <>
VkStructureType get_structure_type_name<VkDisplayPresentInfoKHR>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_PRESENT_INFO_KHR;
}

#endif

#ifdef VK_KHR_xlib_surface
template <>
VkStructureType get_structure_type_name<VkXlibSurfaceCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
}

#endif

#ifdef VK_KHR_xcb_surface
template <>
VkStructureType get_structure_type_name<VkXcbSurfaceCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
}

#endif

#ifdef VK_KHR_wayland_surface
template <>
VkStructureType get_structure_type_name<VkWaylandSurfaceCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
}

#endif

#ifdef VK_KHR_android_surface
template <>
VkStructureType get_structure_type_name<VkAndroidSurfaceCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
}

#endif

#ifdef VK_KHR_win32_surface
template <>
VkStructureType get_structure_type_name<VkWin32SurfaceCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
}

#endif

#ifdef VK_KHR_video_queue
template <>
VkStructureType get_structure_type_name<VkQueueFamilyQueryResultStatusProperties2KHR>()
{
	return VK_STRUCTURE_TYPE_QUEUE_FAMILY_QUERY_RESULT_STATUS_PROPERTIES_2_KHR;
}
template <>
VkStructureType get_structure_type_name<VkVideoQueueFamilyProperties2KHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_QUEUE_FAMILY_PROPERTIES_2_KHR;
}
template <>
VkStructureType get_structure_type_name<VkVideoProfileKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_PROFILE_KHR;
}
template <>
VkStructureType get_structure_type_name<VkVideoProfilesKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_PROFILES_KHR;
}
template <>
VkStructureType get_structure_type_name<VkVideoCapabilitiesKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceVideoFormatInfoKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VIDEO_FORMAT_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkVideoFormatPropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_FORMAT_PROPERTIES_KHR;
}
template <>
VkStructureType get_structure_type_name<VkVideoPictureResourceKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_KHR;
}
template <>
VkStructureType get_structure_type_name<VkVideoReferenceSlotKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_KHR;
}
template <>
VkStructureType get_structure_type_name<VkVideoGetMemoryPropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_GET_MEMORY_PROPERTIES_KHR;
}
template <>
VkStructureType get_structure_type_name<VkVideoBindMemoryKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_BIND_MEMORY_KHR;
}
template <>
VkStructureType get_structure_type_name<VkVideoSessionCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_SESSION_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkVideoSessionParametersCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkVideoSessionParametersUpdateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_UPDATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkVideoBeginCodingInfoKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkVideoEndCodingInfoKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkVideoCodingControlInfoKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR;
}

#endif

#ifdef VK_KHR_video_decode_queue
template <>
VkStructureType get_structure_type_name<VkVideoDecodeCapabilitiesKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_CAPABILITIES_KHR;
}
template <>
VkStructureType get_structure_type_name<VkVideoDecodeInfoKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_INFO_KHR;
}

#endif

#ifdef VK_KHR_dynamic_rendering
template <>
VkStructureType get_structure_type_name<VkRenderingFragmentShadingRateAttachmentInfoKHR>()
{
	return VK_STRUCTURE_TYPE_RENDERING_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkRenderingFragmentDensityMapAttachmentInfoEXT>()
{
	return VK_STRUCTURE_TYPE_RENDERING_FRAGMENT_DENSITY_MAP_ATTACHMENT_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkAttachmentSampleCountInfoAMD>()
{
	return VK_STRUCTURE_TYPE_ATTACHMENT_SAMPLE_COUNT_INFO_AMD;
}
template <>
VkStructureType get_structure_type_name<VkMultiviewPerViewAttributesInfoNVX>()
{
	return VK_STRUCTURE_TYPE_MULTIVIEW_PER_VIEW_ATTRIBUTES_INFO_NVX;
}

#endif

#ifdef VK_KHR_external_memory_win32
template <>
VkStructureType get_structure_type_name<VkImportMemoryWin32HandleInfoKHR>()
{
	return VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkExportMemoryWin32HandleInfoKHR>()
{
	return VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkMemoryWin32HandlePropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHR;
}
template <>
VkStructureType get_structure_type_name<VkMemoryGetWin32HandleInfoKHR>()
{
	return VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
}

#endif

#ifdef VK_KHR_external_memory_fd
template <>
VkStructureType get_structure_type_name<VkImportMemoryFdInfoKHR>()
{
	return VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkMemoryFdPropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR;
}
template <>
VkStructureType get_structure_type_name<VkMemoryGetFdInfoKHR>()
{
	return VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
}

#endif

#ifdef VK_KHR_win32_keyed_mutex
template <>
VkStructureType get_structure_type_name<VkWin32KeyedMutexAcquireReleaseInfoKHR>()
{
	return VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_KHR;
}

#endif

#ifdef VK_KHR_external_semaphore_win32
template <>
VkStructureType get_structure_type_name<VkImportSemaphoreWin32HandleInfoKHR>()
{
	return VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkExportSemaphoreWin32HandleInfoKHR>()
{
	return VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkD3D12FenceSubmitInfoKHR>()
{
	return VK_STRUCTURE_TYPE_D3D12_FENCE_SUBMIT_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkSemaphoreGetWin32HandleInfoKHR>()
{
	return VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
}

#endif

#ifdef VK_KHR_external_semaphore_fd
template <>
VkStructureType get_structure_type_name<VkImportSemaphoreFdInfoKHR>()
{
	return VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkSemaphoreGetFdInfoKHR>()
{
	return VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
}

#endif

#ifdef VK_KHR_push_descriptor
template <>
VkStructureType get_structure_type_name<VkPhysicalDevicePushDescriptorPropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR;
}

#endif

#ifdef VK_KHR_incremental_present
template <>
VkStructureType get_structure_type_name<VkPresentRegionsKHR>()
{
	return VK_STRUCTURE_TYPE_PRESENT_REGIONS_KHR;
}

#endif

#ifdef VK_KHR_shared_presentable_image
template <>
VkStructureType get_structure_type_name<VkSharedPresentSurfaceCapabilitiesKHR>()
{
	return VK_STRUCTURE_TYPE_SHARED_PRESENT_SURFACE_CAPABILITIES_KHR;
}

#endif

#ifdef VK_KHR_external_fence_win32
template <>
VkStructureType get_structure_type_name<VkImportFenceWin32HandleInfoKHR>()
{
	return VK_STRUCTURE_TYPE_IMPORT_FENCE_WIN32_HANDLE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkExportFenceWin32HandleInfoKHR>()
{
	return VK_STRUCTURE_TYPE_EXPORT_FENCE_WIN32_HANDLE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkFenceGetWin32HandleInfoKHR>()
{
	return VK_STRUCTURE_TYPE_FENCE_GET_WIN32_HANDLE_INFO_KHR;
}

#endif

#ifdef VK_KHR_external_fence_fd
template <>
VkStructureType get_structure_type_name<VkImportFenceFdInfoKHR>()
{
	return VK_STRUCTURE_TYPE_IMPORT_FENCE_FD_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkFenceGetFdInfoKHR>()
{
	return VK_STRUCTURE_TYPE_FENCE_GET_FD_INFO_KHR;
}

#endif

#ifdef VK_KHR_performance_query
template <>
VkStructureType get_structure_type_name<VkPhysicalDevicePerformanceQueryFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDevicePerformanceQueryPropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_PROPERTIES_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPerformanceCounterKHR>()
{
	return VK_STRUCTURE_TYPE_PERFORMANCE_COUNTER_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPerformanceCounterDescriptionKHR>()
{
	return VK_STRUCTURE_TYPE_PERFORMANCE_COUNTER_DESCRIPTION_KHR;
}
template <>
VkStructureType get_structure_type_name<VkQueryPoolPerformanceCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_QUERY_POOL_PERFORMANCE_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkAcquireProfilingLockInfoKHR>()
{
	return VK_STRUCTURE_TYPE_ACQUIRE_PROFILING_LOCK_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPerformanceQuerySubmitInfoKHR>()
{
	return VK_STRUCTURE_TYPE_PERFORMANCE_QUERY_SUBMIT_INFO_KHR;
}

#endif

#ifdef VK_KHR_get_surface_capabilities2
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceSurfaceInfo2KHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
}
template <>
VkStructureType get_structure_type_name<VkSurfaceCapabilities2KHR>()
{
	return VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;
}
template <>
VkStructureType get_structure_type_name<VkSurfaceFormat2KHR>()
{
	return VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
}

#endif

#ifdef VK_KHR_get_display_properties2
template <>
VkStructureType get_structure_type_name<VkDisplayProperties2KHR>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_PROPERTIES_2_KHR;
}
template <>
VkStructureType get_structure_type_name<VkDisplayPlaneProperties2KHR>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_PLANE_PROPERTIES_2_KHR;
}
template <>
VkStructureType get_structure_type_name<VkDisplayModeProperties2KHR>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_MODE_PROPERTIES_2_KHR;
}
template <>
VkStructureType get_structure_type_name<VkDisplayPlaneInfo2KHR>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_PLANE_INFO_2_KHR;
}
template <>
VkStructureType get_structure_type_name<VkDisplayPlaneCapabilities2KHR>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_PLANE_CAPABILITIES_2_KHR;
}

#endif

#ifdef VK_KHR_portability_subset
template <>
VkStructureType get_structure_type_name<VkPhysicalDevicePortabilitySubsetFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDevicePortabilitySubsetPropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_PROPERTIES_KHR;
}

#endif

#ifdef VK_KHR_shader_clock
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShaderClockFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR;
}

#endif

#ifdef VK_KHR_global_priority
template <>
VkStructureType get_structure_type_name<VkDeviceQueueGlobalPriorityCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_DEVICE_QUEUE_GLOBAL_PRIORITY_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceGlobalPriorityQueryFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GLOBAL_PRIORITY_QUERY_FEATURES_KHR;
}
template <>
VkStructureType get_structure_type_name<VkQueueFamilyGlobalPriorityPropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_QUEUE_FAMILY_GLOBAL_PRIORITY_PROPERTIES_KHR;
}

#endif

#ifdef VK_KHR_fragment_shading_rate
template <>
VkStructureType get_structure_type_name<VkFragmentShadingRateAttachmentInfoKHR>()
{
	return VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPipelineFragmentShadingRateStateCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_STATE_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceFragmentShadingRateFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceFragmentShadingRatePropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceFragmentShadingRateKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_KHR;
}

#endif

#ifdef VK_KHR_surface_protected_capabilities
template <>
VkStructureType get_structure_type_name<VkSurfaceProtectedCapabilitiesKHR>()
{
	return VK_STRUCTURE_TYPE_SURFACE_PROTECTED_CAPABILITIES_KHR;
}

#endif

#ifdef VK_KHR_present_wait
template <>
VkStructureType get_structure_type_name<VkPhysicalDevicePresentWaitFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR;
}

#endif

#ifdef VK_KHR_pipeline_executable_properties
template <>
VkStructureType get_structure_type_name<VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPipelineInfoKHR>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPipelineExecutablePropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_PROPERTIES_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPipelineExecutableInfoKHR>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPipelineExecutableStatisticKHR>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_STATISTIC_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPipelineExecutableInternalRepresentationKHR>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INTERNAL_REPRESENTATION_KHR;
}

#endif

#ifdef VK_KHR_pipeline_library
template <>
VkStructureType get_structure_type_name<VkPipelineLibraryCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;
}

#endif

#ifdef VK_KHR_present_id
template <>
VkStructureType get_structure_type_name<VkPresentIdKHR>()
{
	return VK_STRUCTURE_TYPE_PRESENT_ID_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDevicePresentIdFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR;
}

#endif

#ifdef VK_KHR_video_encode_queue
template <>
VkStructureType get_structure_type_name<VkVideoEncodeInfoKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeCapabilitiesKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_CAPABILITIES_KHR;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeRateControlLayerInfoKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_LAYER_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeRateControlInfoKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_INFO_KHR;
}

#endif

#ifdef VK_KHR_synchronization2
template <>
VkStructureType get_structure_type_name<VkQueueFamilyCheckpointProperties2NV>()
{
	return VK_STRUCTURE_TYPE_QUEUE_FAMILY_CHECKPOINT_PROPERTIES_2_NV;
}
template <>
VkStructureType get_structure_type_name<VkCheckpointData2NV>()
{
	return VK_STRUCTURE_TYPE_CHECKPOINT_DATA_2_NV;
}

#endif

#ifdef VK_KHR_shader_subgroup_uniform_control_flow
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_UNIFORM_CONTROL_FLOW_FEATURES_KHR;
}

#endif

#ifdef VK_KHR_workgroup_memory_explicit_layout
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_FEATURES_KHR;
}

#endif

#ifdef VK_EXT_debug_report
template <>
VkStructureType get_structure_type_name<VkDebugReportCallbackCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
}

#endif

#ifdef VK_AMD_rasterization_order
template <>
VkStructureType get_structure_type_name<VkPipelineRasterizationStateRasterizationOrderAMD>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD;
}

#endif

#ifdef VK_EXT_debug_marker
template <>
VkStructureType get_structure_type_name<VkDebugMarkerObjectNameInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkDebugMarkerObjectTagInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkDebugMarkerMarkerInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
}

#endif

#ifdef VK_NV_dedicated_allocation
template <>
VkStructureType get_structure_type_name<VkDedicatedAllocationImageCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type_name<VkDedicatedAllocationBufferCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type_name<VkDedicatedAllocationMemoryAllocateInfoNV>()
{
	return VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV;
}

#endif

#ifdef VK_EXT_transform_feedback
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceTransformFeedbackFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceTransformFeedbackPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPipelineRasterizationStateStreamCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_STREAM_CREATE_INFO_EXT;
}

#endif

#ifdef VK_NVX_binary_import
template <>
VkStructureType get_structure_type_name<VkCuModuleCreateInfoNVX>()
{
	return VK_STRUCTURE_TYPE_CU_MODULE_CREATE_INFO_NVX;
}
template <>
VkStructureType get_structure_type_name<VkCuFunctionCreateInfoNVX>()
{
	return VK_STRUCTURE_TYPE_CU_FUNCTION_CREATE_INFO_NVX;
}
template <>
VkStructureType get_structure_type_name<VkCuLaunchInfoNVX>()
{
	return VK_STRUCTURE_TYPE_CU_LAUNCH_INFO_NVX;
}

#endif

#ifdef VK_NVX_image_view_handle
template <>
VkStructureType get_structure_type_name<VkImageViewHandleInfoNVX>()
{
	return VK_STRUCTURE_TYPE_IMAGE_VIEW_HANDLE_INFO_NVX;
}
template <>
VkStructureType get_structure_type_name<VkImageViewAddressPropertiesNVX>()
{
	return VK_STRUCTURE_TYPE_IMAGE_VIEW_ADDRESS_PROPERTIES_NVX;
}

#endif

#ifdef VK_EXT_video_encode_h264
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH264CapabilitiesEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_CAPABILITIES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH264SessionParametersAddInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_ADD_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH264SessionParametersCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH264DpbSlotInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_DPB_SLOT_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH264ReferenceListsEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_REFERENCE_LISTS_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH264NaluSliceEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_NALU_SLICE_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH264VclFrameInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_VCL_FRAME_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH264EmitPictureParametersEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_EMIT_PICTURE_PARAMETERS_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH264ProfileEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PROFILE_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH264RateControlInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_RATE_CONTROL_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH264RateControlLayerInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_RATE_CONTROL_LAYER_INFO_EXT;
}

#endif

#ifdef VK_EXT_video_encode_h265
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH265CapabilitiesEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_CAPABILITIES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH265SessionParametersAddInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_SESSION_PARAMETERS_ADD_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH265SessionParametersCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_SESSION_PARAMETERS_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH265DpbSlotInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_DPB_SLOT_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH265ReferenceListsEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_REFERENCE_LISTS_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH265NaluSliceSegmentEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_NALU_SLICE_SEGMENT_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH265VclFrameInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_VCL_FRAME_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH265EmitPictureParametersEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_EMIT_PICTURE_PARAMETERS_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH265ProfileEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_PROFILE_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH265RateControlInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_RATE_CONTROL_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoEncodeH265RateControlLayerInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_RATE_CONTROL_LAYER_INFO_EXT;
}

#endif

#ifdef VK_EXT_video_decode_h264
template <>
VkStructureType get_structure_type_name<VkVideoDecodeH264ProfileEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoDecodeH264CapabilitiesEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_CAPABILITIES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoDecodeH264SessionParametersAddInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_ADD_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoDecodeH264SessionParametersCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoDecodeH264PictureInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PICTURE_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoDecodeH264MvcEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_MVC_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoDecodeH264DpbSlotInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_DPB_SLOT_INFO_EXT;
}

#endif

#ifdef VK_AMD_texture_gather_bias_lod
template <>
VkStructureType get_structure_type_name<VkTextureLODGatherFormatPropertiesAMD>()
{
	return VK_STRUCTURE_TYPE_TEXTURE_LOD_GATHER_FORMAT_PROPERTIES_AMD;
}

#endif

#ifdef VK_GGP_stream_descriptor_surface
template <>
VkStructureType get_structure_type_name<VkStreamDescriptorSurfaceCreateInfoGGP>()
{
	return VK_STRUCTURE_TYPE_STREAM_DESCRIPTOR_SURFACE_CREATE_INFO_GGP;
}

#endif

#ifdef VK_NV_corner_sampled_image
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceCornerSampledImageFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CORNER_SAMPLED_IMAGE_FEATURES_NV;
}

#endif

#ifdef VK_NV_external_memory
template <>
VkStructureType get_structure_type_name<VkExternalMemoryImageCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type_name<VkExportMemoryAllocateInfoNV>()
{
	return VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_NV;
}

#endif

#ifdef VK_NV_external_memory_win32
template <>
VkStructureType get_structure_type_name<VkImportMemoryWin32HandleInfoNV>()
{
	return VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_NV;
}
template <>
VkStructureType get_structure_type_name<VkExportMemoryWin32HandleInfoNV>()
{
	return VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_NV;
}

#endif

#ifdef VK_NV_win32_keyed_mutex
template <>
VkStructureType get_structure_type_name<VkWin32KeyedMutexAcquireReleaseInfoNV>()
{
	return VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_NV;
}

#endif

#ifdef VK_EXT_validation_flags
template <>
VkStructureType get_structure_type_name<VkValidationFlagsEXT>()
{
	return VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT;
}

#endif

#ifdef VK_NN_vi_surface
template <>
VkStructureType get_structure_type_name<VkViSurfaceCreateInfoNN>()
{
	return VK_STRUCTURE_TYPE_VI_SURFACE_CREATE_INFO_NN;
}

#endif

#ifdef VK_EXT_astc_decode_mode
template <>
VkStructureType get_structure_type_name<VkImageViewASTCDecodeModeEXT>()
{
	return VK_STRUCTURE_TYPE_IMAGE_VIEW_ASTC_DECODE_MODE_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceASTCDecodeFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ASTC_DECODE_FEATURES_EXT;
}

#endif

#ifdef VK_EXT_conditional_rendering
template <>
VkStructureType get_structure_type_name<VkConditionalRenderingBeginInfoEXT>()
{
	return VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceConditionalRenderingFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkCommandBufferInheritanceConditionalRenderingInfoEXT>()
{
	return VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_CONDITIONAL_RENDERING_INFO_EXT;
}

#endif

#ifdef VK_NV_clip_space_w_scaling
template <>
VkStructureType get_structure_type_name<VkPipelineViewportWScalingStateCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_W_SCALING_STATE_CREATE_INFO_NV;
}

#endif

#ifdef VK_EXT_display_surface_counter
template <>
VkStructureType get_structure_type_name<VkSurfaceCapabilities2EXT>()
{
	return VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_EXT;
}

#endif

#ifdef VK_EXT_display_control
template <>
VkStructureType get_structure_type_name<VkDisplayPowerInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_POWER_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkDeviceEventInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DEVICE_EVENT_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkDisplayEventInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_EVENT_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkSwapchainCounterCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_SWAPCHAIN_COUNTER_CREATE_INFO_EXT;
}

#endif

#ifdef VK_GOOGLE_display_timing
template <>
VkStructureType get_structure_type_name<VkPresentTimesInfoGOOGLE>()
{
	return VK_STRUCTURE_TYPE_PRESENT_TIMES_INFO_GOOGLE;
}

#endif

#ifdef VK_NVX_multiview_per_view_attributes
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PER_VIEW_ATTRIBUTES_PROPERTIES_NVX;
}

#endif

#ifdef VK_NV_viewport_swizzle
template <>
VkStructureType get_structure_type_name<VkPipelineViewportSwizzleStateCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_SWIZZLE_STATE_CREATE_INFO_NV;
}

#endif

#ifdef VK_EXT_discard_rectangles
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceDiscardRectanglePropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DISCARD_RECTANGLE_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPipelineDiscardRectangleStateCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_DISCARD_RECTANGLE_STATE_CREATE_INFO_EXT;
}

#endif

#ifdef VK_EXT_conservative_rasterization
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceConservativeRasterizationPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPipelineRasterizationConservativeStateCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT;
}

#endif

#ifdef VK_EXT_depth_clip_enable
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceDepthClipEnableFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPipelineRasterizationDepthClipStateCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT;
}

#endif

#ifdef VK_EXT_hdr_metadata
template <>
VkStructureType get_structure_type_name<VkHdrMetadataEXT>()
{
	return VK_STRUCTURE_TYPE_HDR_METADATA_EXT;
}

#endif

#ifdef VK_MVK_ios_surface
template <>
VkStructureType get_structure_type_name<VkIOSSurfaceCreateInfoMVK>()
{
	return VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK;
}

#endif

#ifdef VK_MVK_macos_surface
template <>
VkStructureType get_structure_type_name<VkMacOSSurfaceCreateInfoMVK>()
{
	return VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
}

#endif

#ifdef VK_EXT_debug_utils
template <>
VkStructureType get_structure_type_name<VkDebugUtilsLabelEXT>()
{
	return VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
}
template <>
VkStructureType get_structure_type_name<VkDebugUtilsObjectNameInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkDebugUtilsMessengerCallbackDataEXT>()
{
	return VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT;
}
template <>
VkStructureType get_structure_type_name<VkDebugUtilsMessengerCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkDebugUtilsObjectTagInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_TAG_INFO_EXT;
}

#endif

#ifdef VK_ANDROID_external_memory_android_hardware_buffer
template <>
VkStructureType get_structure_type_name<VkAndroidHardwareBufferUsageANDROID>()
{
	return VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_USAGE_ANDROID;
}
template <>
VkStructureType get_structure_type_name<VkAndroidHardwareBufferPropertiesANDROID>()
{
	return VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID;
}
template <>
VkStructureType get_structure_type_name<VkAndroidHardwareBufferFormatPropertiesANDROID>()
{
	return VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_ANDROID;
}
template <>
VkStructureType get_structure_type_name<VkImportAndroidHardwareBufferInfoANDROID>()
{
	return VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID;
}
template <>
VkStructureType get_structure_type_name<VkMemoryGetAndroidHardwareBufferInfoANDROID>()
{
	return VK_STRUCTURE_TYPE_MEMORY_GET_ANDROID_HARDWARE_BUFFER_INFO_ANDROID;
}
template <>
VkStructureType get_structure_type_name<VkExternalFormatANDROID>()
{
	return VK_STRUCTURE_TYPE_EXTERNAL_FORMAT_ANDROID;
}
template <>
VkStructureType get_structure_type_name<VkAndroidHardwareBufferFormatProperties2ANDROID>()
{
	return VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_2_ANDROID;
}

#endif

#ifdef VK_EXT_sample_locations
template <>
VkStructureType get_structure_type_name<VkSampleLocationsInfoEXT>()
{
	return VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkRenderPassSampleLocationsBeginInfoEXT>()
{
	return VK_STRUCTURE_TYPE_RENDER_PASS_SAMPLE_LOCATIONS_BEGIN_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPipelineSampleLocationsStateCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_SAMPLE_LOCATIONS_STATE_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceSampleLocationsPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLE_LOCATIONS_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkMultisamplePropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_MULTISAMPLE_PROPERTIES_EXT;
}

#endif

#ifdef VK_EXT_blend_operation_advanced
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPipelineColorBlendAdvancedStateCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_ADVANCED_STATE_CREATE_INFO_EXT;
}

#endif

#ifdef VK_NV_fragment_coverage_to_color
template <>
VkStructureType get_structure_type_name<VkPipelineCoverageToColorStateCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_TO_COLOR_STATE_CREATE_INFO_NV;
}

#endif

#ifdef VK_NV_framebuffer_mixed_samples
template <>
VkStructureType get_structure_type_name<VkPipelineCoverageModulationStateCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_MODULATION_STATE_CREATE_INFO_NV;
}

#endif

#ifdef VK_NV_shader_sm_builtins
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShaderSMBuiltinsPropertiesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_PROPERTIES_NV;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShaderSMBuiltinsFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_FEATURES_NV;
}

#endif

#ifdef VK_EXT_image_drm_format_modifier
template <>
VkStructureType get_structure_type_name<VkDrmFormatModifierPropertiesListEXT>()
{
	return VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceImageDrmFormatModifierInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_DRM_FORMAT_MODIFIER_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkImageDrmFormatModifierListCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_LIST_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkImageDrmFormatModifierExplicitCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkImageDrmFormatModifierPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkDrmFormatModifierPropertiesList2EXT>()
{
	return VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_2_EXT;
}

#endif

#ifdef VK_EXT_validation_cache
template <>
VkStructureType get_structure_type_name<VkValidationCacheCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VALIDATION_CACHE_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkShaderModuleValidationCacheCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_SHADER_MODULE_VALIDATION_CACHE_CREATE_INFO_EXT;
}

#endif

#ifdef VK_NV_shading_rate_image
template <>
VkStructureType get_structure_type_name<VkPipelineViewportShadingRateImageStateCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_SHADING_RATE_IMAGE_STATE_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShadingRateImageFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShadingRateImagePropertiesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_PROPERTIES_NV;
}
template <>
VkStructureType get_structure_type_name<VkPipelineViewportCoarseSampleOrderStateCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_COARSE_SAMPLE_ORDER_STATE_CREATE_INFO_NV;
}

#endif

#ifdef VK_NV_ray_tracing
template <>
VkStructureType get_structure_type_name<VkRayTracingShaderGroupCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type_name<VkRayTracingPipelineCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type_name<VkGeometryTrianglesNV>()
{
	return VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
}
template <>
VkStructureType get_structure_type_name<VkGeometryAABBNV>()
{
	return VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;
}
template <>
VkStructureType get_structure_type_name<VkGeometryNV>()
{
	return VK_STRUCTURE_TYPE_GEOMETRY_NV;
}
template <>
VkStructureType get_structure_type_name<VkAccelerationStructureInfoNV>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
}
template <>
VkStructureType get_structure_type_name<VkAccelerationStructureCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type_name<VkBindAccelerationStructureMemoryInfoNV>()
{
	return VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
}
template <>
VkStructureType get_structure_type_name<VkWriteDescriptorSetAccelerationStructureNV>()
{
	return VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
}
template <>
VkStructureType get_structure_type_name<VkAccelerationStructureMemoryRequirementsInfoNV>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceRayTracingPropertiesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
}

#endif

#ifdef VK_NV_representative_fragment_test
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_REPRESENTATIVE_FRAGMENT_TEST_FEATURES_NV;
}
template <>
VkStructureType get_structure_type_name<VkPipelineRepresentativeFragmentTestStateCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_REPRESENTATIVE_FRAGMENT_TEST_STATE_CREATE_INFO_NV;
}

#endif

#ifdef VK_EXT_filter_cubic
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceImageViewImageFormatInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_IMAGE_FORMAT_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkFilterCubicImageViewImageFormatPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_FILTER_CUBIC_IMAGE_VIEW_IMAGE_FORMAT_PROPERTIES_EXT;
}

#endif

#ifdef VK_EXT_external_memory_host
template <>
VkStructureType get_structure_type_name<VkImportMemoryHostPointerInfoEXT>()
{
	return VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkMemoryHostPointerPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_MEMORY_HOST_POINTER_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceExternalMemoryHostPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT;
}

#endif

#ifdef VK_AMD_pipeline_compiler_control
template <>
VkStructureType get_structure_type_name<VkPipelineCompilerControlCreateInfoAMD>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_COMPILER_CONTROL_CREATE_INFO_AMD;
}

#endif

#ifdef VK_EXT_calibrated_timestamps
template <>
VkStructureType get_structure_type_name<VkCalibratedTimestampInfoEXT>()
{
	return VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_EXT;
}

#endif

#ifdef VK_AMD_shader_core_properties
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShaderCorePropertiesAMD>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_AMD;
}

#endif

#ifdef VK_EXT_video_decode_h265
template <>
VkStructureType get_structure_type_name<VkVideoDecodeH265ProfileEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PROFILE_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoDecodeH265CapabilitiesEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_CAPABILITIES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoDecodeH265SessionParametersAddInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_SESSION_PARAMETERS_ADD_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoDecodeH265SessionParametersCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_SESSION_PARAMETERS_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoDecodeH265PictureInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PICTURE_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVideoDecodeH265DpbSlotInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_DPB_SLOT_INFO_EXT;
}

#endif

#ifdef VK_AMD_memory_overallocation_behavior
template <>
VkStructureType get_structure_type_name<VkDeviceMemoryOverallocationCreateInfoAMD>()
{
	return VK_STRUCTURE_TYPE_DEVICE_MEMORY_OVERALLOCATION_CREATE_INFO_AMD;
}

#endif

#ifdef VK_EXT_vertex_attribute_divisor
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPipelineVertexInputDivisorStateCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT;
}

#endif

#ifdef VK_GGP_frame_token
template <>
VkStructureType get_structure_type_name<VkPresentFrameTokenGGP>()
{
	return VK_STRUCTURE_TYPE_PRESENT_FRAME_TOKEN_GGP;
}

#endif

#ifdef VK_NV_compute_shader_derivatives
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceComputeShaderDerivativesFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_NV;
}

#endif

#ifdef VK_NV_mesh_shader
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceMeshShaderFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceMeshShaderPropertiesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;
}

#endif

#ifdef VK_NV_fragment_shader_barycentric
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_NV;
}

#endif

#ifdef VK_NV_shader_image_footprint
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShaderImageFootprintFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_FOOTPRINT_FEATURES_NV;
}

#endif

#ifdef VK_NV_scissor_exclusive
template <>
VkStructureType get_structure_type_name<VkPipelineViewportExclusiveScissorStateCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_EXCLUSIVE_SCISSOR_STATE_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceExclusiveScissorFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXCLUSIVE_SCISSOR_FEATURES_NV;
}

#endif

#ifdef VK_NV_device_diagnostic_checkpoints
template <>
VkStructureType get_structure_type_name<VkQueueFamilyCheckpointPropertiesNV>()
{
	return VK_STRUCTURE_TYPE_QUEUE_FAMILY_CHECKPOINT_PROPERTIES_NV;
}
template <>
VkStructureType get_structure_type_name<VkCheckpointDataNV>()
{
	return VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV;
}

#endif

#ifdef VK_INTEL_shader_integer_functions2
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_FUNCTIONS_2_FEATURES_INTEL;
}

#endif

#ifdef VK_INTEL_performance_query
template <>
VkStructureType get_structure_type_name<VkInitializePerformanceApiInfoINTEL>()
{
	return VK_STRUCTURE_TYPE_INITIALIZE_PERFORMANCE_API_INFO_INTEL;
}
template <>
VkStructureType get_structure_type_name<VkQueryPoolPerformanceQueryCreateInfoINTEL>()
{
	return VK_STRUCTURE_TYPE_QUERY_POOL_PERFORMANCE_QUERY_CREATE_INFO_INTEL;
}
template <>
VkStructureType get_structure_type_name<VkPerformanceMarkerInfoINTEL>()
{
	return VK_STRUCTURE_TYPE_PERFORMANCE_MARKER_INFO_INTEL;
}
template <>
VkStructureType get_structure_type_name<VkPerformanceStreamMarkerInfoINTEL>()
{
	return VK_STRUCTURE_TYPE_PERFORMANCE_STREAM_MARKER_INFO_INTEL;
}
template <>
VkStructureType get_structure_type_name<VkPerformanceOverrideInfoINTEL>()
{
	return VK_STRUCTURE_TYPE_PERFORMANCE_OVERRIDE_INFO_INTEL;
}
template <>
VkStructureType get_structure_type_name<VkPerformanceConfigurationAcquireInfoINTEL>()
{
	return VK_STRUCTURE_TYPE_PERFORMANCE_CONFIGURATION_ACQUIRE_INFO_INTEL;
}

#endif

#ifdef VK_EXT_pci_bus_info
template <>
VkStructureType get_structure_type_name<VkPhysicalDevicePCIBusInfoPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PCI_BUS_INFO_PROPERTIES_EXT;
}

#endif

#ifdef VK_AMD_display_native_hdr
template <>
VkStructureType get_structure_type_name<VkDisplayNativeHdrSurfaceCapabilitiesAMD>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_NATIVE_HDR_SURFACE_CAPABILITIES_AMD;
}
template <>
VkStructureType get_structure_type_name<VkSwapchainDisplayNativeHdrCreateInfoAMD>()
{
	return VK_STRUCTURE_TYPE_SWAPCHAIN_DISPLAY_NATIVE_HDR_CREATE_INFO_AMD;
}

#endif

#ifdef VK_FUCHSIA_imagepipe_surface
template <>
VkStructureType get_structure_type_name<VkImagePipeSurfaceCreateInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_IMAGEPIPE_SURFACE_CREATE_INFO_FUCHSIA;
}

#endif

#ifdef VK_EXT_metal_surface
template <>
VkStructureType get_structure_type_name<VkMetalSurfaceCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
}

#endif

#ifdef VK_EXT_fragment_density_map
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceFragmentDensityMapFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceFragmentDensityMapPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkRenderPassFragmentDensityMapCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_CREATE_INFO_EXT;
}

#endif

#ifdef VK_AMD_shader_core_properties2
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShaderCoreProperties2AMD>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_2_AMD;
}

#endif

#ifdef VK_AMD_device_coherent_memory
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceCoherentMemoryFeaturesAMD>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD;
}

#endif

#ifdef VK_EXT_shader_image_atomic_int64
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT;
}

#endif

#ifdef VK_EXT_memory_budget
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceMemoryBudgetPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
}

#endif

#ifdef VK_EXT_memory_priority
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceMemoryPriorityFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkMemoryPriorityAllocateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
}

#endif

#ifdef VK_NV_dedicated_allocation_image_aliasing
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEDICATED_ALLOCATION_IMAGE_ALIASING_FEATURES_NV;
}

#endif

#ifdef VK_EXT_buffer_device_address
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceBufferDeviceAddressFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkBufferDeviceAddressCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_CREATE_INFO_EXT;
}

#endif

#ifdef VK_EXT_validation_features
template <>
VkStructureType get_structure_type_name<VkValidationFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
}

#endif

#ifdef VK_NV_cooperative_matrix
template <>
VkStructureType get_structure_type_name<VkCooperativeMatrixPropertiesNV>()
{
	return VK_STRUCTURE_TYPE_COOPERATIVE_MATRIX_PROPERTIES_NV;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceCooperativeMatrixFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_NV;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceCooperativeMatrixPropertiesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_PROPERTIES_NV;
}

#endif

#ifdef VK_NV_coverage_reduction_mode
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceCoverageReductionModeFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COVERAGE_REDUCTION_MODE_FEATURES_NV;
}
template <>
VkStructureType get_structure_type_name<VkPipelineCoverageReductionStateCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_REDUCTION_STATE_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type_name<VkFramebufferMixedSamplesCombinationNV>()
{
	return VK_STRUCTURE_TYPE_FRAMEBUFFER_MIXED_SAMPLES_COMBINATION_NV;
}

#endif

#ifdef VK_EXT_fragment_shader_interlock
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT;
}

#endif

#ifdef VK_EXT_ycbcr_image_arrays
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceYcbcrImageArraysFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_IMAGE_ARRAYS_FEATURES_EXT;
}

#endif

#ifdef VK_EXT_provoking_vertex
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceProvokingVertexFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROVOKING_VERTEX_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceProvokingVertexPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROVOKING_VERTEX_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPipelineRasterizationProvokingVertexStateCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_PROVOKING_VERTEX_STATE_CREATE_INFO_EXT;
}

#endif

#ifdef VK_EXT_full_screen_exclusive
template <>
VkStructureType get_structure_type_name<VkSurfaceFullScreenExclusiveInfoEXT>()
{
	return VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkSurfaceCapabilitiesFullScreenExclusiveEXT>()
{
	return VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_FULL_SCREEN_EXCLUSIVE_EXT;
}
template <>
VkStructureType get_structure_type_name<VkSurfaceFullScreenExclusiveWin32InfoEXT>()
{
	return VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT;
}

#endif

#ifdef VK_EXT_headless_surface
template <>
VkStructureType get_structure_type_name<VkHeadlessSurfaceCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT;
}

#endif

#ifdef VK_EXT_line_rasterization
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceLineRasterizationFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceLineRasterizationPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPipelineRasterizationLineStateCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_EXT;
}

#endif

#ifdef VK_EXT_shader_atomic_float
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShaderAtomicFloatFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT;
}

#endif

#ifdef VK_EXT_index_type_uint8
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceIndexTypeUint8FeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT;
}

#endif

#ifdef VK_EXT_extended_dynamic_state
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceExtendedDynamicStateFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
}

#endif

#ifdef VK_EXT_shader_atomic_float2
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT;
}

#endif

#ifdef VK_NV_device_generated_commands
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_PROPERTIES_NV;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_NV;
}
template <>
VkStructureType get_structure_type_name<VkGraphicsShaderGroupCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_GRAPHICS_SHADER_GROUP_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type_name<VkGraphicsPipelineShaderGroupsCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_SHADER_GROUPS_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type_name<VkIndirectCommandsLayoutTokenNV>()
{
	return VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_TOKEN_NV;
}
template <>
VkStructureType get_structure_type_name<VkIndirectCommandsLayoutCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type_name<VkGeneratedCommandsInfoNV>()
{
	return VK_STRUCTURE_TYPE_GENERATED_COMMANDS_INFO_NV;
}
template <>
VkStructureType get_structure_type_name<VkGeneratedCommandsMemoryRequirementsInfoNV>()
{
	return VK_STRUCTURE_TYPE_GENERATED_COMMANDS_MEMORY_REQUIREMENTS_INFO_NV;
}

#endif

#ifdef VK_NV_inherited_viewport_scissor
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceInheritedViewportScissorFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INHERITED_VIEWPORT_SCISSOR_FEATURES_NV;
}
template <>
VkStructureType get_structure_type_name<VkCommandBufferInheritanceViewportScissorInfoNV>()
{
	return VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_VIEWPORT_SCISSOR_INFO_NV;
}

#endif

#ifdef VK_EXT_texel_buffer_alignment
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_FEATURES_EXT;
}

#endif

#ifdef VK_QCOM_render_pass_transform
template <>
VkStructureType get_structure_type_name<VkRenderPassTransformBeginInfoQCOM>()
{
	return VK_STRUCTURE_TYPE_RENDER_PASS_TRANSFORM_BEGIN_INFO_QCOM;
}
template <>
VkStructureType get_structure_type_name<VkCommandBufferInheritanceRenderPassTransformInfoQCOM>()
{
	return VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDER_PASS_TRANSFORM_INFO_QCOM;
}

#endif

#ifdef VK_EXT_device_memory_report
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceDeviceMemoryReportFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_MEMORY_REPORT_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkDeviceMemoryReportCallbackDataEXT>()
{
	return VK_STRUCTURE_TYPE_DEVICE_MEMORY_REPORT_CALLBACK_DATA_EXT;
}
template <>
VkStructureType get_structure_type_name<VkDeviceDeviceMemoryReportCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DEVICE_DEVICE_MEMORY_REPORT_CREATE_INFO_EXT;
}

#endif

#ifdef VK_EXT_robustness2
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceRobustness2FeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceRobustness2PropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_PROPERTIES_EXT;
}

#endif

#ifdef VK_EXT_custom_border_color
template <>
VkStructureType get_structure_type_name<VkSamplerCustomBorderColorCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceCustomBorderColorPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceCustomBorderColorFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT;
}

#endif

#ifdef VK_NV_device_diagnostics_config
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceDiagnosticsConfigFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DIAGNOSTICS_CONFIG_FEATURES_NV;
}
template <>
VkStructureType get_structure_type_name<VkDeviceDiagnosticsConfigCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV;
}

#endif

#ifdef VK_EXT_graphics_pipeline_library
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceGraphicsPipelineLibraryPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkGraphicsPipelineLibraryCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT;
}

#endif

#ifdef VK_NV_fragment_shading_rate_enums
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceFragmentShadingRateEnumsFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_ENUMS_FEATURES_NV;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceFragmentShadingRateEnumsPropertiesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_ENUMS_PROPERTIES_NV;
}
template <>
VkStructureType get_structure_type_name<VkPipelineFragmentShadingRateEnumStateCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_ENUM_STATE_CREATE_INFO_NV;
}

#endif

#ifdef VK_NV_ray_tracing_motion_blur
template <>
VkStructureType get_structure_type_name<VkAccelerationStructureGeometryMotionTrianglesDataNV>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_MOTION_TRIANGLES_DATA_NV;
}
template <>
VkStructureType get_structure_type_name<VkAccelerationStructureMotionInfoNV>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MOTION_INFO_NV;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceRayTracingMotionBlurFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MOTION_BLUR_FEATURES_NV;
}

#endif

#ifdef VK_EXT_ycbcr_2plane_444_formats
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_2_PLANE_444_FORMATS_FEATURES_EXT;
}

#endif

#ifdef VK_EXT_fragment_density_map2
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceFragmentDensityMap2FeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceFragmentDensityMap2PropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_PROPERTIES_EXT;
}

#endif

#ifdef VK_QCOM_rotated_copy_commands
template <>
VkStructureType get_structure_type_name<VkCopyCommandTransformInfoQCOM>()
{
	return VK_STRUCTURE_TYPE_COPY_COMMAND_TRANSFORM_INFO_QCOM;
}

#endif

#ifdef VK_EXT_4444_formats
template <>
VkStructureType get_structure_type_name<VkPhysicalDevice4444FormatsFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_4444_FORMATS_FEATURES_EXT;
}

#endif

#ifdef VK_ARM_rasterization_order_attachment_access
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesARM>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_FEATURES_ARM;
}

#endif

#ifdef VK_EXT_rgba10x6_formats
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RGBA10X6_FORMATS_FEATURES_EXT;
}

#endif

#ifdef VK_EXT_directfb_surface
template <>
VkStructureType get_structure_type_name<VkDirectFBSurfaceCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DIRECTFB_SURFACE_CREATE_INFO_EXT;
}

#endif

#ifdef VK_VALVE_mutable_descriptor_type
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceMutableDescriptorTypeFeaturesVALVE>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_VALVE;
}
template <>
VkStructureType get_structure_type_name<VkMutableDescriptorTypeCreateInfoVALVE>()
{
	return VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_VALVE;
}

#endif

#ifdef VK_EXT_vertex_input_dynamic_state
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVertexInputBindingDescription2EXT>()
{
	return VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
}
template <>
VkStructureType get_structure_type_name<VkVertexInputAttributeDescription2EXT>()
{
	return VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
}

#endif

#ifdef VK_EXT_physical_device_drm
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceDrmPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRM_PROPERTIES_EXT;
}

#endif

#ifdef VK_EXT_depth_clip_control
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceDepthClipControlFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_CONTROL_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPipelineViewportDepthClipControlCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_DEPTH_CLIP_CONTROL_CREATE_INFO_EXT;
}

#endif

#ifdef VK_EXT_primitive_topology_list_restart
template <>
VkStructureType get_structure_type_name<VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVE_TOPOLOGY_LIST_RESTART_FEATURES_EXT;
}

#endif

#ifdef VK_FUCHSIA_external_memory
template <>
VkStructureType get_structure_type_name<VkImportMemoryZirconHandleInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_IMPORT_MEMORY_ZIRCON_HANDLE_INFO_FUCHSIA;
}
template <>
VkStructureType get_structure_type_name<VkMemoryZirconHandlePropertiesFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_MEMORY_ZIRCON_HANDLE_PROPERTIES_FUCHSIA;
}
template <>
VkStructureType get_structure_type_name<VkMemoryGetZirconHandleInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_MEMORY_GET_ZIRCON_HANDLE_INFO_FUCHSIA;
}

#endif

#ifdef VK_FUCHSIA_external_semaphore
template <>
VkStructureType get_structure_type_name<VkImportSemaphoreZirconHandleInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_ZIRCON_HANDLE_INFO_FUCHSIA;
}
template <>
VkStructureType get_structure_type_name<VkSemaphoreGetZirconHandleInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_SEMAPHORE_GET_ZIRCON_HANDLE_INFO_FUCHSIA;
}

#endif

#ifdef VK_FUCHSIA_buffer_collection
template <>
VkStructureType get_structure_type_name<VkBufferCollectionCreateInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_BUFFER_COLLECTION_CREATE_INFO_FUCHSIA;
}
template <>
VkStructureType get_structure_type_name<VkImportMemoryBufferCollectionFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_IMPORT_MEMORY_BUFFER_COLLECTION_FUCHSIA;
}
template <>
VkStructureType get_structure_type_name<VkBufferCollectionImageCreateInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_BUFFER_COLLECTION_IMAGE_CREATE_INFO_FUCHSIA;
}
template <>
VkStructureType get_structure_type_name<VkBufferCollectionConstraintsInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_BUFFER_COLLECTION_CONSTRAINTS_INFO_FUCHSIA;
}
template <>
VkStructureType get_structure_type_name<VkBufferConstraintsInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_BUFFER_CONSTRAINTS_INFO_FUCHSIA;
}
template <>
VkStructureType get_structure_type_name<VkBufferCollectionBufferCreateInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_BUFFER_COLLECTION_BUFFER_CREATE_INFO_FUCHSIA;
}
template <>
VkStructureType get_structure_type_name<VkSysmemColorSpaceFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_SYSMEM_COLOR_SPACE_FUCHSIA;
}
template <>
VkStructureType get_structure_type_name<VkBufferCollectionPropertiesFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_BUFFER_COLLECTION_PROPERTIES_FUCHSIA;
}
template <>
VkStructureType get_structure_type_name<VkImageFormatConstraintsInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_IMAGE_FORMAT_CONSTRAINTS_INFO_FUCHSIA;
}
template <>
VkStructureType get_structure_type_name<VkImageConstraintsInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_IMAGE_CONSTRAINTS_INFO_FUCHSIA;
}

#endif

#ifdef VK_HUAWEI_subpass_shading
template <>
VkStructureType get_structure_type_name<VkSubpassShadingPipelineCreateInfoHUAWEI>()
{
	return VK_STRUCTURE_TYPE_SUBPASS_SHADING_PIPELINE_CREATE_INFO_HUAWEI;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceSubpassShadingFeaturesHUAWEI>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_SHADING_FEATURES_HUAWEI;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceSubpassShadingPropertiesHUAWEI>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_SHADING_PROPERTIES_HUAWEI;
}

#endif

#ifdef VK_HUAWEI_invocation_mask
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceInvocationMaskFeaturesHUAWEI>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INVOCATION_MASK_FEATURES_HUAWEI;
}

#endif

#ifdef VK_NV_external_memory_rdma
template <>
VkStructureType get_structure_type_name<VkMemoryGetRemoteAddressInfoNV>()
{
	return VK_STRUCTURE_TYPE_MEMORY_GET_REMOTE_ADDRESS_INFO_NV;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceExternalMemoryRDMAFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_RDMA_FEATURES_NV;
}

#endif

#ifdef VK_EXT_extended_dynamic_state2
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceExtendedDynamicState2FeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT;
}

#endif

#ifdef VK_QNX_screen_surface
template <>
VkStructureType get_structure_type_name<VkScreenSurfaceCreateInfoQNX>()
{
	return VK_STRUCTURE_TYPE_SCREEN_SURFACE_CREATE_INFO_QNX;
}

#endif

#ifdef VK_EXT_color_write_enable
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceColorWriteEnableFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COLOR_WRITE_ENABLE_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPipelineColorWriteCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_COLOR_WRITE_CREATE_INFO_EXT;
}

#endif

#ifdef VK_EXT_primitives_generated_query
template <>
VkStructureType get_structure_type_name<VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVES_GENERATED_QUERY_FEATURES_EXT;
}

#endif

#ifdef VK_EXT_image_view_min_lod
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceImageViewMinLodFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_MIN_LOD_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkImageViewMinLodCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_IMAGE_VIEW_MIN_LOD_CREATE_INFO_EXT;
}

#endif

#ifdef VK_EXT_multi_draw
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceMultiDrawFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceMultiDrawPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_PROPERTIES_EXT;
}

#endif

#ifdef VK_EXT_image_2d_view_of_3d
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceImage2DViewOf3DFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_2D_VIEW_OF_3D_FEATURES_EXT;
}

#endif

#ifdef VK_EXT_border_color_swizzle
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceBorderColorSwizzleFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BORDER_COLOR_SWIZZLE_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type_name<VkSamplerBorderColorComponentMappingCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_SAMPLER_BORDER_COLOR_COMPONENT_MAPPING_CREATE_INFO_EXT;
}

#endif

#ifdef VK_EXT_pageable_device_local_memory
template <>
VkStructureType get_structure_type_name<VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PAGEABLE_DEVICE_LOCAL_MEMORY_FEATURES_EXT;
}

#endif

#ifdef VK_VALVE_descriptor_set_host_mapping
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceDescriptorSetHostMappingFeaturesVALVE>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_SET_HOST_MAPPING_FEATURES_VALVE;
}
template <>
VkStructureType get_structure_type_name<VkDescriptorSetBindingReferenceVALVE>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_SET_BINDING_REFERENCE_VALVE;
}
template <>
VkStructureType get_structure_type_name<VkDescriptorSetLayoutHostMappingInfoVALVE>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_HOST_MAPPING_INFO_VALVE;
}

#endif

#ifdef VK_QCOM_fragment_density_map_offset
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceFragmentDensityMapOffsetFeaturesQCOM>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_FEATURES_QCOM;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceFragmentDensityMapOffsetPropertiesQCOM>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_PROPERTIES_QCOM;
}
template <>
VkStructureType get_structure_type_name<VkSubpassFragmentDensityMapOffsetEndInfoQCOM>()
{
	return VK_STRUCTURE_TYPE_SUBPASS_FRAGMENT_DENSITY_MAP_OFFSET_END_INFO_QCOM;
}

#endif

#ifdef VK_NV_linear_color_attachment
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceLinearColorAttachmentFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINEAR_COLOR_ATTACHMENT_FEATURES_NV;
}

#endif

#ifdef VK_KHR_acceleration_structure
template <>
VkStructureType get_structure_type_name<VkAccelerationStructureGeometryTrianglesDataKHR>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
}
template <>
VkStructureType get_structure_type_name<VkAccelerationStructureGeometryAabbsDataKHR>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
}
template <>
VkStructureType get_structure_type_name<VkAccelerationStructureGeometryInstancesDataKHR>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
}
template <>
VkStructureType get_structure_type_name<VkAccelerationStructureGeometryKHR>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
}
template <>
VkStructureType get_structure_type_name<VkAccelerationStructureBuildGeometryInfoKHR>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkAccelerationStructureCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkWriteDescriptorSetAccelerationStructureKHR>()
{
	return VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceAccelerationStructureFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceAccelerationStructurePropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
}
template <>
VkStructureType get_structure_type_name<VkAccelerationStructureDeviceAddressInfoKHR>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkAccelerationStructureVersionInfoKHR>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_VERSION_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkCopyAccelerationStructureToMemoryInfoKHR>()
{
	return VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_TO_MEMORY_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkCopyMemoryToAccelerationStructureInfoKHR>()
{
	return VK_STRUCTURE_TYPE_COPY_MEMORY_TO_ACCELERATION_STRUCTURE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkCopyAccelerationStructureInfoKHR>()
{
	return VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkAccelerationStructureBuildSizesInfoKHR>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
}

#endif

#ifdef VK_KHR_ray_tracing_pipeline
template <>
VkStructureType get_structure_type_name<VkRayTracingShaderGroupCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkRayTracingPipelineInterfaceCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_INTERFACE_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkRayTracingPipelineCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceRayTracingPipelineFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
}
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceRayTracingPipelinePropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
}

#endif

#ifdef VK_KHR_ray_query
template <>
VkStructureType get_structure_type_name<VkPhysicalDeviceRayQueryFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
}

#endif
}        // namespace vulkan
}        // namespace components
