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
VkStructureType get_structure_type()
{
	throw "function not implemented";
}
#ifdef VK_VERSION_1_0
template <>
VkStructureType get_structure_type<VkBufferMemoryBarrier>()
{
	return VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
}
template <>
VkStructureType get_structure_type<VkImageMemoryBarrier>()
{
	return VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
}
template <>
VkStructureType get_structure_type<VkMemoryBarrier>()
{
	return VK_STRUCTURE_TYPE_MEMORY_BARRIER;
}
template <>
VkStructureType get_structure_type<VkApplicationInfo>()
{
	return VK_STRUCTURE_TYPE_APPLICATION_INFO;
}
template <>
VkStructureType get_structure_type<VkInstanceCreateInfo>()
{
	return VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkDeviceQueueCreateInfo>()
{
	return VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkDeviceCreateInfo>()
{
	return VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkSubmitInfo>()
{
	return VK_STRUCTURE_TYPE_SUBMIT_INFO;
}
template <>
VkStructureType get_structure_type<VkMappedMemoryRange>()
{
	return VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
}
template <>
VkStructureType get_structure_type<VkMemoryAllocateInfo>()
{
	return VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
}
template <>
VkStructureType get_structure_type<VkBindSparseInfo>()
{
	return VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
}
template <>
VkStructureType get_structure_type<VkFenceCreateInfo>()
{
	return VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkSemaphoreCreateInfo>()
{
	return VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkEventCreateInfo>()
{
	return VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkQueryPoolCreateInfo>()
{
	return VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkBufferCreateInfo>()
{
	return VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkBufferViewCreateInfo>()
{
	return VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkImageCreateInfo>()
{
	return VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkImageViewCreateInfo>()
{
	return VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkShaderModuleCreateInfo>()
{
	return VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPipelineCacheCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPipelineShaderStageCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkComputePipelineCreateInfo>()
{
	return VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPipelineVertexInputStateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPipelineInputAssemblyStateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPipelineTessellationStateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPipelineViewportStateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPipelineRasterizationStateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPipelineMultisampleStateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPipelineDepthStencilStateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPipelineColorBlendStateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPipelineDynamicStateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkGraphicsPipelineCreateInfo>()
{
	return VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPipelineLayoutCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkSamplerCreateInfo>()
{
	return VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkCopyDescriptorSet>()
{
	return VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
}
template <>
VkStructureType get_structure_type<VkDescriptorPoolCreateInfo>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkDescriptorSetAllocateInfo>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
}
template <>
VkStructureType get_structure_type<VkDescriptorSetLayoutCreateInfo>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkWriteDescriptorSet>()
{
	return VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
}
template <>
VkStructureType get_structure_type<VkFramebufferCreateInfo>()
{
	return VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkRenderPassCreateInfo>()
{
	return VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkCommandPoolCreateInfo>()
{
	return VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkCommandBufferAllocateInfo>()
{
	return VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
}
template <>
VkStructureType get_structure_type<VkCommandBufferInheritanceInfo>()
{
	return VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
}
template <>
VkStructureType get_structure_type<VkCommandBufferBeginInfo>()
{
	return VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
}
template <>
VkStructureType get_structure_type<VkRenderPassBeginInfo>()
{
	return VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
}
#endif
#ifdef VK_VERSION_1_1
template <>
VkStructureType get_structure_type<VkPhysicalDeviceSubgroupProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkBindBufferMemoryInfo>()
{
	return VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO;
}
template <>
VkStructureType get_structure_type<VkBindImageMemoryInfo>()
{
	return VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
}
template <>
VkStructureType get_structure_type<VkPhysicalDevice16BitStorageFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
}
template <>
VkStructureType get_structure_type<VkMemoryDedicatedRequirements>()
{
	return VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS;
}
template <>
VkStructureType get_structure_type<VkMemoryDedicatedAllocateInfo>()
{
	return VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
}
template <>
VkStructureType get_structure_type<VkMemoryAllocateFlagsInfo>()
{
	return VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
}
template <>
VkStructureType get_structure_type<VkDeviceGroupRenderPassBeginInfo>()
{
	return VK_STRUCTURE_TYPE_DEVICE_GROUP_RENDER_PASS_BEGIN_INFO;
}
template <>
VkStructureType get_structure_type<VkDeviceGroupCommandBufferBeginInfo>()
{
	return VK_STRUCTURE_TYPE_DEVICE_GROUP_COMMAND_BUFFER_BEGIN_INFO;
}
template <>
VkStructureType get_structure_type<VkDeviceGroupSubmitInfo>()
{
	return VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO;
}
template <>
VkStructureType get_structure_type<VkDeviceGroupBindSparseInfo>()
{
	return VK_STRUCTURE_TYPE_DEVICE_GROUP_BIND_SPARSE_INFO;
}
template <>
VkStructureType get_structure_type<VkBindBufferMemoryDeviceGroupInfo>()
{
	return VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_DEVICE_GROUP_INFO;
}
template <>
VkStructureType get_structure_type<VkBindImageMemoryDeviceGroupInfo>()
{
	return VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_DEVICE_GROUP_INFO;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceGroupProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkDeviceGroupDeviceCreateInfo>()
{
	return VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkBufferMemoryRequirementsInfo2>()
{
	return VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;
}
template <>
VkStructureType get_structure_type<VkImageMemoryRequirementsInfo2>()
{
	return VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
}
template <>
VkStructureType get_structure_type<VkImageSparseMemoryRequirementsInfo2>()
{
	return VK_STRUCTURE_TYPE_IMAGE_SPARSE_MEMORY_REQUIREMENTS_INFO_2;
}
template <>
VkStructureType get_structure_type<VkMemoryRequirements2>()
{
	return VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
}
template <>
VkStructureType get_structure_type<VkSparseImageMemoryRequirements2>()
{
	return VK_STRUCTURE_TYPE_SPARSE_IMAGE_MEMORY_REQUIREMENTS_2;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceFeatures2>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceProperties2>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
}
template <>
VkStructureType get_structure_type<VkFormatProperties2>()
{
	return VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
}
template <>
VkStructureType get_structure_type<VkImageFormatProperties2>()
{
	return VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceImageFormatInfo2>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
}
template <>
VkStructureType get_structure_type<VkQueueFamilyProperties2>()
{
	return VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceMemoryProperties2>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
}
template <>
VkStructureType get_structure_type<VkSparseImageFormatProperties2>()
{
	return VK_STRUCTURE_TYPE_SPARSE_IMAGE_FORMAT_PROPERTIES_2;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceSparseImageFormatInfo2>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SPARSE_IMAGE_FORMAT_INFO_2;
}
template <>
VkStructureType get_structure_type<VkPhysicalDevicePointClippingProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_POINT_CLIPPING_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkRenderPassInputAttachmentAspectCreateInfo>()
{
	return VK_STRUCTURE_TYPE_RENDER_PASS_INPUT_ATTACHMENT_ASPECT_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkImageViewUsageCreateInfo>()
{
	return VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPipelineTessellationDomainOriginStateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkRenderPassMultiviewCreateInfo>()
{
	return VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceMultiviewFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceMultiviewProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceVariablePointersFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceProtectedMemoryFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceProtectedMemoryProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkDeviceQueueInfo2>()
{
	return VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
}
template <>
VkStructureType get_structure_type<VkProtectedSubmitInfo>()
{
	return VK_STRUCTURE_TYPE_PROTECTED_SUBMIT_INFO;
}
template <>
VkStructureType get_structure_type<VkSamplerYcbcrConversionCreateInfo>()
{
	return VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkSamplerYcbcrConversionInfo>()
{
	return VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO;
}
template <>
VkStructureType get_structure_type<VkBindImagePlaneMemoryInfo>()
{
	return VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO;
}
template <>
VkStructureType get_structure_type<VkImagePlaneMemoryRequirementsInfo>()
{
	return VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceSamplerYcbcrConversionFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES;
}
template <>
VkStructureType get_structure_type<VkSamplerYcbcrConversionImageFormatProperties>()
{
	return VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_IMAGE_FORMAT_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkDescriptorUpdateTemplateCreateInfo>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceExternalImageFormatInfo>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO;
}
template <>
VkStructureType get_structure_type<VkExternalImageFormatProperties>()
{
	return VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceExternalBufferInfo>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_BUFFER_INFO;
}
template <>
VkStructureType get_structure_type<VkExternalBufferProperties>()
{
	return VK_STRUCTURE_TYPE_EXTERNAL_BUFFER_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceIDProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkExternalMemoryImageCreateInfo>()
{
	return VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkExternalMemoryBufferCreateInfo>()
{
	return VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkExportMemoryAllocateInfo>()
{
	return VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceExternalFenceInfo>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_FENCE_INFO;
}
template <>
VkStructureType get_structure_type<VkExternalFenceProperties>()
{
	return VK_STRUCTURE_TYPE_EXTERNAL_FENCE_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkExportFenceCreateInfo>()
{
	return VK_STRUCTURE_TYPE_EXPORT_FENCE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkExportSemaphoreCreateInfo>()
{
	return VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceExternalSemaphoreInfo>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO;
}
template <>
VkStructureType get_structure_type<VkExternalSemaphoreProperties>()
{
	return VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceMaintenance3Properties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkDescriptorSetLayoutSupport>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_SUPPORT;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShaderDrawParametersFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
}
#endif
#ifdef VK_VERSION_1_2
template <>
VkStructureType get_structure_type<VkPhysicalDeviceVulkan11Features>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceVulkan11Properties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceVulkan12Features>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceVulkan12Properties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkImageFormatListCreateInfo>()
{
	return VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkAttachmentDescription2>()
{
	return VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
}
template <>
VkStructureType get_structure_type<VkAttachmentReference2>()
{
	return VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
}
template <>
VkStructureType get_structure_type<VkSubpassDescription2>()
{
	return VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
}
template <>
VkStructureType get_structure_type<VkSubpassDependency2>()
{
	return VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
}
template <>
VkStructureType get_structure_type<VkRenderPassCreateInfo2>()
{
	return VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
}
template <>
VkStructureType get_structure_type<VkSubpassBeginInfo>()
{
	return VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO;
}
template <>
VkStructureType get_structure_type<VkSubpassEndInfo>()
{
	return VK_STRUCTURE_TYPE_SUBPASS_END_INFO;
}
template <>
VkStructureType get_structure_type<VkPhysicalDevice8BitStorageFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceDriverProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShaderAtomicInt64Features>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShaderFloat16Int8Features>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceFloatControlsProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkDescriptorSetLayoutBindingFlagsCreateInfo>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceDescriptorIndexingFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceDescriptorIndexingProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkDescriptorSetVariableDescriptorCountAllocateInfo>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
}
template <>
VkStructureType get_structure_type<VkDescriptorSetVariableDescriptorCountLayoutSupport>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_LAYOUT_SUPPORT;
}
template <>
VkStructureType get_structure_type<VkSubpassDescriptionDepthStencilResolve>()
{
	return VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceDepthStencilResolveProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceScalarBlockLayoutFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES;
}
template <>
VkStructureType get_structure_type<VkImageStencilUsageCreateInfo>()
{
	return VK_STRUCTURE_TYPE_IMAGE_STENCIL_USAGE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkSamplerReductionModeCreateInfo>()
{
	return VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceSamplerFilterMinmaxProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_FILTER_MINMAX_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceVulkanMemoryModelFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceImagelessFramebufferFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES;
}
template <>
VkStructureType get_structure_type<VkFramebufferAttachmentImageInfo>()
{
	return VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO;
}
template <>
VkStructureType get_structure_type<VkFramebufferAttachmentsCreateInfo>()
{
	return VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkRenderPassAttachmentBeginInfo>()
{
	return VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceUniformBufferStandardLayoutFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES;
}
template <>
VkStructureType get_structure_type<VkAttachmentReferenceStencilLayout>()
{
	return VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_STENCIL_LAYOUT;
}
template <>
VkStructureType get_structure_type<VkAttachmentDescriptionStencilLayout>()
{
	return VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_STENCIL_LAYOUT;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceHostQueryResetFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceTimelineSemaphoreFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceTimelineSemaphoreProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkSemaphoreTypeCreateInfo>()
{
	return VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkTimelineSemaphoreSubmitInfo>()
{
	return VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
}
template <>
VkStructureType get_structure_type<VkSemaphoreWaitInfo>()
{
	return VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
}
template <>
VkStructureType get_structure_type<VkSemaphoreSignalInfo>()
{
	return VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceBufferDeviceAddressFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
}
template <>
VkStructureType get_structure_type<VkBufferDeviceAddressInfo>()
{
	return VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
}
template <>
VkStructureType get_structure_type<VkBufferOpaqueCaptureAddressCreateInfo>()
{
	return VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkMemoryOpaqueCaptureAddressAllocateInfo>()
{
	return VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO;
}
template <>
VkStructureType get_structure_type<VkDeviceMemoryOpaqueCaptureAddressInfo>()
{
	return VK_STRUCTURE_TYPE_DEVICE_MEMORY_OPAQUE_CAPTURE_ADDRESS_INFO;
}
#endif
#ifdef VK_VERSION_1_3
template <>
VkStructureType get_structure_type<VkPhysicalDeviceVulkan13Features>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceVulkan13Properties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkPipelineCreationFeedbackCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShaderTerminateInvocationFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TERMINATE_INVOCATION_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceToolProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TOOL_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDevicePrivateDataFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIVATE_DATA_FEATURES;
}
template <>
VkStructureType get_structure_type<VkDevicePrivateDataCreateInfo>()
{
	return VK_STRUCTURE_TYPE_DEVICE_PRIVATE_DATA_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPrivateDataSlotCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PRIVATE_DATA_SLOT_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPhysicalDevicePipelineCreationCacheControlFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES;
}
template <>
VkStructureType get_structure_type<VkMemoryBarrier2>()
{
	return VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
}
template <>
VkStructureType get_structure_type<VkBufferMemoryBarrier2>()
{
	return VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
}
template <>
VkStructureType get_structure_type<VkImageMemoryBarrier2>()
{
	return VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
}
template <>
VkStructureType get_structure_type<VkDependencyInfo>()
{
	return VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
}
template <>
VkStructureType get_structure_type<VkSemaphoreSubmitInfo>()
{
	return VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
}
template <>
VkStructureType get_structure_type<VkCommandBufferSubmitInfo>()
{
	return VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
}
template <>
VkStructureType get_structure_type<VkSubmitInfo2>()
{
	return VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceSynchronization2Features>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ZERO_INITIALIZE_WORKGROUP_MEMORY_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceImageRobustnessFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ROBUSTNESS_FEATURES;
}
template <>
VkStructureType get_structure_type<VkBufferCopy2>()
{
	return VK_STRUCTURE_TYPE_BUFFER_COPY_2;
}
template <>
VkStructureType get_structure_type<VkCopyBufferInfo2>()
{
	return VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
}
template <>
VkStructureType get_structure_type<VkImageCopy2>()
{
	return VK_STRUCTURE_TYPE_IMAGE_COPY_2;
}
template <>
VkStructureType get_structure_type<VkCopyImageInfo2>()
{
	return VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2;
}
template <>
VkStructureType get_structure_type<VkBufferImageCopy2>()
{
	return VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
}
template <>
VkStructureType get_structure_type<VkCopyBufferToImageInfo2>()
{
	return VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
}
template <>
VkStructureType get_structure_type<VkCopyImageToBufferInfo2>()
{
	return VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2;
}
template <>
VkStructureType get_structure_type<VkImageBlit2>()
{
	return VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
}
template <>
VkStructureType get_structure_type<VkBlitImageInfo2>()
{
	return VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
}
template <>
VkStructureType get_structure_type<VkImageResolve2>()
{
	return VK_STRUCTURE_TYPE_IMAGE_RESOLVE_2;
}
template <>
VkStructureType get_structure_type<VkResolveImageInfo2>()
{
	return VK_STRUCTURE_TYPE_RESOLVE_IMAGE_INFO_2;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceSubgroupSizeControlFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceSubgroupSizeControlProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkPipelineShaderStageRequiredSubgroupSizeCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceInlineUniformBlockFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceInlineUniformBlockProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkWriteDescriptorSetInlineUniformBlock>()
{
	return VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK;
}
template <>
VkStructureType get_structure_type<VkDescriptorPoolInlineUniformBlockCreateInfo>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_INLINE_UNIFORM_BLOCK_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceTextureCompressionASTCHDRFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXTURE_COMPRESSION_ASTC_HDR_FEATURES;
}
template <>
VkStructureType get_structure_type<VkRenderingAttachmentInfo>()
{
	return VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
}
template <>
VkStructureType get_structure_type<VkRenderingInfo>()
{
	return VK_STRUCTURE_TYPE_RENDERING_INFO;
}
template <>
VkStructureType get_structure_type<VkPipelineRenderingCreateInfo>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceDynamicRenderingFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
}
template <>
VkStructureType get_structure_type<VkCommandBufferInheritanceRenderingInfo>()
{
	return VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShaderIntegerDotProductFeatures>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShaderIntegerDotProductProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceTexelBufferAlignmentProperties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkFormatProperties3>()
{
	return VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceMaintenance4Features>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceMaintenance4Properties>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES;
}
template <>
VkStructureType get_structure_type<VkDeviceBufferMemoryRequirements>()
{
	return VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS;
}
template <>
VkStructureType get_structure_type<VkDeviceImageMemoryRequirements>()
{
	return VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS;
}
#endif
#ifdef VK_KHR_swapchain
template <>
VkStructureType get_structure_type<VkSwapchainCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkPresentInfoKHR>()
{
	return VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkImageSwapchainCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_IMAGE_SWAPCHAIN_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkBindImageMemorySwapchainInfoKHR>()
{
	return VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_SWAPCHAIN_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkAcquireNextImageInfoKHR>()
{
	return VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkDeviceGroupPresentCapabilitiesKHR>()
{
	return VK_STRUCTURE_TYPE_DEVICE_GROUP_PRESENT_CAPABILITIES_KHR;
}
template <>
VkStructureType get_structure_type<VkDeviceGroupPresentInfoKHR>()
{
	return VK_STRUCTURE_TYPE_DEVICE_GROUP_PRESENT_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkDeviceGroupSwapchainCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_DEVICE_GROUP_SWAPCHAIN_CREATE_INFO_KHR;
}
#endif
#ifdef VK_KHR_display
template <>
VkStructureType get_structure_type<VkDisplayModeCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_MODE_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkDisplaySurfaceCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
}
#endif
#ifdef VK_KHR_display_swapchain
template <>
VkStructureType get_structure_type<VkDisplayPresentInfoKHR>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_PRESENT_INFO_KHR;
}
#endif
#ifdef VK_KHR_xlib_surface
template <>
VkStructureType get_structure_type<VkXlibSurfaceCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
}
#endif
#ifdef VK_KHR_xcb_surface
template <>
VkStructureType get_structure_type<VkXcbSurfaceCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
}
#endif
#ifdef VK_KHR_wayland_surface
template <>
VkStructureType get_structure_type<VkWaylandSurfaceCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
}
#endif
#ifdef VK_KHR_android_surface
template <>
VkStructureType get_structure_type<VkAndroidSurfaceCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
}
#endif
#ifdef VK_KHR_win32_surface
template <>
VkStructureType get_structure_type<VkWin32SurfaceCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
}
#endif
#ifdef VK_KHR_video_queue
template <>
VkStructureType get_structure_type<VkQueueFamilyQueryResultStatusProperties2KHR>()
{
	return VK_STRUCTURE_TYPE_QUEUE_FAMILY_QUERY_RESULT_STATUS_PROPERTIES_2_KHR;
}
template <>
VkStructureType get_structure_type<VkVideoQueueFamilyProperties2KHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_QUEUE_FAMILY_PROPERTIES_2_KHR;
}
template <>
VkStructureType get_structure_type<VkVideoProfileKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_PROFILE_KHR;
}
template <>
VkStructureType get_structure_type<VkVideoProfilesKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_PROFILES_KHR;
}
template <>
VkStructureType get_structure_type<VkVideoCapabilitiesKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceVideoFormatInfoKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VIDEO_FORMAT_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkVideoFormatPropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_FORMAT_PROPERTIES_KHR;
}
template <>
VkStructureType get_structure_type<VkVideoPictureResourceKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_KHR;
}
template <>
VkStructureType get_structure_type<VkVideoReferenceSlotKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_KHR;
}
template <>
VkStructureType get_structure_type<VkVideoGetMemoryPropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_GET_MEMORY_PROPERTIES_KHR;
}
template <>
VkStructureType get_structure_type<VkVideoBindMemoryKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_BIND_MEMORY_KHR;
}
template <>
VkStructureType get_structure_type<VkVideoSessionCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_SESSION_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkVideoSessionParametersCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkVideoSessionParametersUpdateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_UPDATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkVideoBeginCodingInfoKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkVideoEndCodingInfoKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkVideoCodingControlInfoKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR;
}
#endif
#ifdef VK_KHR_video_decode_queue
template <>
VkStructureType get_structure_type<VkVideoDecodeCapabilitiesKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_CAPABILITIES_KHR;
}
template <>
VkStructureType get_structure_type<VkVideoDecodeInfoKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_INFO_KHR;
}
#endif
#ifdef VK_KHR_dynamic_rendering
template <>
VkStructureType get_structure_type<VkRenderingFragmentShadingRateAttachmentInfoKHR>()
{
	return VK_STRUCTURE_TYPE_RENDERING_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkRenderingFragmentDensityMapAttachmentInfoEXT>()
{
	return VK_STRUCTURE_TYPE_RENDERING_FRAGMENT_DENSITY_MAP_ATTACHMENT_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkAttachmentSampleCountInfoAMD>()
{
	return VK_STRUCTURE_TYPE_ATTACHMENT_SAMPLE_COUNT_INFO_AMD;
}
template <>
VkStructureType get_structure_type<VkMultiviewPerViewAttributesInfoNVX>()
{
	return VK_STRUCTURE_TYPE_MULTIVIEW_PER_VIEW_ATTRIBUTES_INFO_NVX;
}
#endif
#ifdef VK_KHR_external_memory_win32
template <>
VkStructureType get_structure_type<VkImportMemoryWin32HandleInfoKHR>()
{
	return VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkExportMemoryWin32HandleInfoKHR>()
{
	return VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkMemoryWin32HandlePropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHR;
}
template <>
VkStructureType get_structure_type<VkMemoryGetWin32HandleInfoKHR>()
{
	return VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
}
#endif
#ifdef VK_KHR_external_memory_fd
template <>
VkStructureType get_structure_type<VkImportMemoryFdInfoKHR>()
{
	return VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkMemoryFdPropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR;
}
template <>
VkStructureType get_structure_type<VkMemoryGetFdInfoKHR>()
{
	return VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
}
#endif
#ifdef VK_KHR_win32_keyed_mutex
template <>
VkStructureType get_structure_type<VkWin32KeyedMutexAcquireReleaseInfoKHR>()
{
	return VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_KHR;
}
#endif
#ifdef VK_KHR_external_semaphore_win32
template <>
VkStructureType get_structure_type<VkImportSemaphoreWin32HandleInfoKHR>()
{
	return VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkExportSemaphoreWin32HandleInfoKHR>()
{
	return VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkD3D12FenceSubmitInfoKHR>()
{
	return VK_STRUCTURE_TYPE_D3D12_FENCE_SUBMIT_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkSemaphoreGetWin32HandleInfoKHR>()
{
	return VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
}
#endif
#ifdef VK_KHR_external_semaphore_fd
template <>
VkStructureType get_structure_type<VkImportSemaphoreFdInfoKHR>()
{
	return VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkSemaphoreGetFdInfoKHR>()
{
	return VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
}
#endif
#ifdef VK_KHR_push_descriptor
template <>
VkStructureType get_structure_type<VkPhysicalDevicePushDescriptorPropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR;
}
#endif
#ifdef VK_KHR_incremental_present
template <>
VkStructureType get_structure_type<VkPresentRegionsKHR>()
{
	return VK_STRUCTURE_TYPE_PRESENT_REGIONS_KHR;
}
#endif
#ifdef VK_KHR_shared_presentable_image
template <>
VkStructureType get_structure_type<VkSharedPresentSurfaceCapabilitiesKHR>()
{
	return VK_STRUCTURE_TYPE_SHARED_PRESENT_SURFACE_CAPABILITIES_KHR;
}
#endif
#ifdef VK_KHR_external_fence_win32
template <>
VkStructureType get_structure_type<VkImportFenceWin32HandleInfoKHR>()
{
	return VK_STRUCTURE_TYPE_IMPORT_FENCE_WIN32_HANDLE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkExportFenceWin32HandleInfoKHR>()
{
	return VK_STRUCTURE_TYPE_EXPORT_FENCE_WIN32_HANDLE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkFenceGetWin32HandleInfoKHR>()
{
	return VK_STRUCTURE_TYPE_FENCE_GET_WIN32_HANDLE_INFO_KHR;
}
#endif
#ifdef VK_KHR_external_fence_fd
template <>
VkStructureType get_structure_type<VkImportFenceFdInfoKHR>()
{
	return VK_STRUCTURE_TYPE_IMPORT_FENCE_FD_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkFenceGetFdInfoKHR>()
{
	return VK_STRUCTURE_TYPE_FENCE_GET_FD_INFO_KHR;
}
#endif
#ifdef VK_KHR_performance_query
template <>
VkStructureType get_structure_type<VkPhysicalDevicePerformanceQueryFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR;
}
template <>
VkStructureType get_structure_type<VkPhysicalDevicePerformanceQueryPropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_PROPERTIES_KHR;
}
template <>
VkStructureType get_structure_type<VkPerformanceCounterKHR>()
{
	return VK_STRUCTURE_TYPE_PERFORMANCE_COUNTER_KHR;
}
template <>
VkStructureType get_structure_type<VkPerformanceCounterDescriptionKHR>()
{
	return VK_STRUCTURE_TYPE_PERFORMANCE_COUNTER_DESCRIPTION_KHR;
}
template <>
VkStructureType get_structure_type<VkQueryPoolPerformanceCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_QUERY_POOL_PERFORMANCE_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkAcquireProfilingLockInfoKHR>()
{
	return VK_STRUCTURE_TYPE_ACQUIRE_PROFILING_LOCK_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkPerformanceQuerySubmitInfoKHR>()
{
	return VK_STRUCTURE_TYPE_PERFORMANCE_QUERY_SUBMIT_INFO_KHR;
}
#endif
#ifdef VK_KHR_get_surface_capabilities2
template <>
VkStructureType get_structure_type<VkPhysicalDeviceSurfaceInfo2KHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
}
template <>
VkStructureType get_structure_type<VkSurfaceCapabilities2KHR>()
{
	return VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;
}
template <>
VkStructureType get_structure_type<VkSurfaceFormat2KHR>()
{
	return VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
}
#endif
#ifdef VK_KHR_get_display_properties2
template <>
VkStructureType get_structure_type<VkDisplayProperties2KHR>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_PROPERTIES_2_KHR;
}
template <>
VkStructureType get_structure_type<VkDisplayPlaneProperties2KHR>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_PLANE_PROPERTIES_2_KHR;
}
template <>
VkStructureType get_structure_type<VkDisplayModeProperties2KHR>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_MODE_PROPERTIES_2_KHR;
}
template <>
VkStructureType get_structure_type<VkDisplayPlaneInfo2KHR>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_PLANE_INFO_2_KHR;
}
template <>
VkStructureType get_structure_type<VkDisplayPlaneCapabilities2KHR>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_PLANE_CAPABILITIES_2_KHR;
}
#endif
#ifdef VK_KHR_portability_subset
template <>
VkStructureType get_structure_type<VkPhysicalDevicePortabilitySubsetFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR;
}
template <>
VkStructureType get_structure_type<VkPhysicalDevicePortabilitySubsetPropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_PROPERTIES_KHR;
}
#endif
#ifdef VK_KHR_shader_clock
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShaderClockFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR;
}
#endif
#ifdef VK_KHR_global_priority
template <>
VkStructureType get_structure_type<VkDeviceQueueGlobalPriorityCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_DEVICE_QUEUE_GLOBAL_PRIORITY_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceGlobalPriorityQueryFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GLOBAL_PRIORITY_QUERY_FEATURES_KHR;
}
template <>
VkStructureType get_structure_type<VkQueueFamilyGlobalPriorityPropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_QUEUE_FAMILY_GLOBAL_PRIORITY_PROPERTIES_KHR;
}
#endif
#ifdef VK_KHR_fragment_shading_rate
template <>
VkStructureType get_structure_type<VkFragmentShadingRateAttachmentInfoKHR>()
{
	return VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkPipelineFragmentShadingRateStateCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_STATE_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceFragmentShadingRateFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceFragmentShadingRatePropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceFragmentShadingRateKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_KHR;
}
#endif
#ifdef VK_KHR_surface_protected_capabilities
template <>
VkStructureType get_structure_type<VkSurfaceProtectedCapabilitiesKHR>()
{
	return VK_STRUCTURE_TYPE_SURFACE_PROTECTED_CAPABILITIES_KHR;
}
#endif
#ifdef VK_KHR_present_wait
template <>
VkStructureType get_structure_type<VkPhysicalDevicePresentWaitFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR;
}
#endif
#ifdef VK_KHR_pipeline_executable_properties
template <>
VkStructureType get_structure_type<VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR;
}
template <>
VkStructureType get_structure_type<VkPipelineInfoKHR>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkPipelineExecutablePropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_PROPERTIES_KHR;
}
template <>
VkStructureType get_structure_type<VkPipelineExecutableInfoKHR>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkPipelineExecutableStatisticKHR>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_STATISTIC_KHR;
}
template <>
VkStructureType get_structure_type<VkPipelineExecutableInternalRepresentationKHR>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INTERNAL_REPRESENTATION_KHR;
}
#endif
#ifdef VK_KHR_pipeline_library
template <>
VkStructureType get_structure_type<VkPipelineLibraryCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;
}
#endif
#ifdef VK_KHR_present_id
template <>
VkStructureType get_structure_type<VkPresentIdKHR>()
{
	return VK_STRUCTURE_TYPE_PRESENT_ID_KHR;
}
template <>
VkStructureType get_structure_type<VkPhysicalDevicePresentIdFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR;
}
#endif
#ifdef VK_KHR_video_encode_queue
template <>
VkStructureType get_structure_type<VkVideoEncodeInfoKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeCapabilitiesKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_CAPABILITIES_KHR;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeRateControlLayerInfoKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_LAYER_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeRateControlInfoKHR>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_INFO_KHR;
}
#endif
#ifdef VK_KHR_synchronization2
template <>
VkStructureType get_structure_type<VkQueueFamilyCheckpointProperties2NV>()
{
	return VK_STRUCTURE_TYPE_QUEUE_FAMILY_CHECKPOINT_PROPERTIES_2_NV;
}
template <>
VkStructureType get_structure_type<VkCheckpointData2NV>()
{
	return VK_STRUCTURE_TYPE_CHECKPOINT_DATA_2_NV;
}
#endif
#ifdef VK_KHR_shader_subgroup_uniform_control_flow
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_UNIFORM_CONTROL_FLOW_FEATURES_KHR;
}
#endif
#ifdef VK_KHR_workgroup_memory_explicit_layout
template <>
VkStructureType get_structure_type<VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_FEATURES_KHR;
}
#endif
#ifdef VK_EXT_debug_report
template <>
VkStructureType get_structure_type<VkDebugReportCallbackCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
}
#endif
#ifdef VK_AMD_rasterization_order
template <>
VkStructureType get_structure_type<VkPipelineRasterizationStateRasterizationOrderAMD>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD;
}
#endif
#ifdef VK_EXT_debug_marker
template <>
VkStructureType get_structure_type<VkDebugMarkerObjectNameInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkDebugMarkerObjectTagInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkDebugMarkerMarkerInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
}
#endif
#ifdef VK_NV_dedicated_allocation
template <>
VkStructureType get_structure_type<VkDedicatedAllocationImageCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type<VkDedicatedAllocationBufferCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type<VkDedicatedAllocationMemoryAllocateInfoNV>()
{
	return VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV;
}
#endif
#ifdef VK_EXT_transform_feedback
template <>
VkStructureType get_structure_type<VkPhysicalDeviceTransformFeedbackFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceTransformFeedbackPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type<VkPipelineRasterizationStateStreamCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_STREAM_CREATE_INFO_EXT;
}
#endif
#ifdef VK_NVX_binary_import
template <>
VkStructureType get_structure_type<VkCuModuleCreateInfoNVX>()
{
	return VK_STRUCTURE_TYPE_CU_MODULE_CREATE_INFO_NVX;
}
template <>
VkStructureType get_structure_type<VkCuFunctionCreateInfoNVX>()
{
	return VK_STRUCTURE_TYPE_CU_FUNCTION_CREATE_INFO_NVX;
}
template <>
VkStructureType get_structure_type<VkCuLaunchInfoNVX>()
{
	return VK_STRUCTURE_TYPE_CU_LAUNCH_INFO_NVX;
}
#endif
#ifdef VK_NVX_image_view_handle
template <>
VkStructureType get_structure_type<VkImageViewHandleInfoNVX>()
{
	return VK_STRUCTURE_TYPE_IMAGE_VIEW_HANDLE_INFO_NVX;
}
template <>
VkStructureType get_structure_type<VkImageViewAddressPropertiesNVX>()
{
	return VK_STRUCTURE_TYPE_IMAGE_VIEW_ADDRESS_PROPERTIES_NVX;
}
#endif
#ifdef VK_EXT_video_encode_h264
template <>
VkStructureType get_structure_type<VkVideoEncodeH264CapabilitiesEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_CAPABILITIES_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeH264SessionParametersAddInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_ADD_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeH264SessionParametersCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeH264DpbSlotInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_DPB_SLOT_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeH264ReferenceListsEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_REFERENCE_LISTS_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeH264NaluSliceEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_NALU_SLICE_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeH264VclFrameInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_VCL_FRAME_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeH264EmitPictureParametersEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_EMIT_PICTURE_PARAMETERS_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeH264ProfileEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PROFILE_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeH264RateControlInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_RATE_CONTROL_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeH264RateControlLayerInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_RATE_CONTROL_LAYER_INFO_EXT;
}
#endif
#ifdef VK_EXT_video_encode_h265
template <>
VkStructureType get_structure_type<VkVideoEncodeH265CapabilitiesEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_CAPABILITIES_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeH265SessionParametersAddInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_SESSION_PARAMETERS_ADD_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeH265SessionParametersCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_SESSION_PARAMETERS_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeH265DpbSlotInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_DPB_SLOT_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeH265ReferenceListsEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_REFERENCE_LISTS_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeH265NaluSliceSegmentEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_NALU_SLICE_SEGMENT_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeH265VclFrameInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_VCL_FRAME_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeH265EmitPictureParametersEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_EMIT_PICTURE_PARAMETERS_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeH265ProfileEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_PROFILE_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeH265RateControlInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_RATE_CONTROL_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoEncodeH265RateControlLayerInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_RATE_CONTROL_LAYER_INFO_EXT;
}
#endif
#ifdef VK_EXT_video_decode_h264
template <>
VkStructureType get_structure_type<VkVideoDecodeH264ProfileEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoDecodeH264CapabilitiesEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_CAPABILITIES_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoDecodeH264SessionParametersAddInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_ADD_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoDecodeH264SessionParametersCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoDecodeH264PictureInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PICTURE_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoDecodeH264MvcEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_MVC_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoDecodeH264DpbSlotInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_DPB_SLOT_INFO_EXT;
}
#endif
#ifdef VK_AMD_texture_gather_bias_lod
template <>
VkStructureType get_structure_type<VkTextureLODGatherFormatPropertiesAMD>()
{
	return VK_STRUCTURE_TYPE_TEXTURE_LOD_GATHER_FORMAT_PROPERTIES_AMD;
}
#endif
#ifdef VK_GGP_stream_descriptor_surface
template <>
VkStructureType get_structure_type<VkStreamDescriptorSurfaceCreateInfoGGP>()
{
	return VK_STRUCTURE_TYPE_STREAM_DESCRIPTOR_SURFACE_CREATE_INFO_GGP;
}
#endif
#ifdef VK_NV_corner_sampled_image
template <>
VkStructureType get_structure_type<VkPhysicalDeviceCornerSampledImageFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CORNER_SAMPLED_IMAGE_FEATURES_NV;
}
#endif
#ifdef VK_NV_external_memory
template <>
VkStructureType get_structure_type<VkExternalMemoryImageCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type<VkExportMemoryAllocateInfoNV>()
{
	return VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_NV;
}
#endif
#ifdef VK_NV_external_memory_win32
template <>
VkStructureType get_structure_type<VkImportMemoryWin32HandleInfoNV>()
{
	return VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_NV;
}
template <>
VkStructureType get_structure_type<VkExportMemoryWin32HandleInfoNV>()
{
	return VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_NV;
}
#endif
#ifdef VK_NV_win32_keyed_mutex
template <>
VkStructureType get_structure_type<VkWin32KeyedMutexAcquireReleaseInfoNV>()
{
	return VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_NV;
}
#endif
#ifdef VK_EXT_validation_flags
template <>
VkStructureType get_structure_type<VkValidationFlagsEXT>()
{
	return VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT;
}
#endif
#ifdef VK_NN_vi_surface
template <>
VkStructureType get_structure_type<VkViSurfaceCreateInfoNN>()
{
	return VK_STRUCTURE_TYPE_VI_SURFACE_CREATE_INFO_NN;
}
#endif
#ifdef VK_EXT_astc_decode_mode
template <>
VkStructureType get_structure_type<VkImageViewASTCDecodeModeEXT>()
{
	return VK_STRUCTURE_TYPE_IMAGE_VIEW_ASTC_DECODE_MODE_EXT;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceASTCDecodeFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ASTC_DECODE_FEATURES_EXT;
}
#endif
#ifdef VK_EXT_conditional_rendering
template <>
VkStructureType get_structure_type<VkConditionalRenderingBeginInfoEXT>()
{
	return VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceConditionalRenderingFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type<VkCommandBufferInheritanceConditionalRenderingInfoEXT>()
{
	return VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_CONDITIONAL_RENDERING_INFO_EXT;
}
#endif
#ifdef VK_NV_clip_space_w_scaling
template <>
VkStructureType get_structure_type<VkPipelineViewportWScalingStateCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_W_SCALING_STATE_CREATE_INFO_NV;
}
#endif
#ifdef VK_EXT_display_surface_counter
template <>
VkStructureType get_structure_type<VkSurfaceCapabilities2EXT>()
{
	return VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_EXT;
}
#endif
#ifdef VK_EXT_display_control
template <>
VkStructureType get_structure_type<VkDisplayPowerInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_POWER_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkDeviceEventInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DEVICE_EVENT_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkDisplayEventInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_EVENT_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkSwapchainCounterCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_SWAPCHAIN_COUNTER_CREATE_INFO_EXT;
}
#endif
#ifdef VK_GOOGLE_display_timing
template <>
VkStructureType get_structure_type<VkPresentTimesInfoGOOGLE>()
{
	return VK_STRUCTURE_TYPE_PRESENT_TIMES_INFO_GOOGLE;
}
#endif
#ifdef VK_NVX_multiview_per_view_attributes
template <>
VkStructureType get_structure_type<VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PER_VIEW_ATTRIBUTES_PROPERTIES_NVX;
}
#endif
#ifdef VK_NV_viewport_swizzle
template <>
VkStructureType get_structure_type<VkPipelineViewportSwizzleStateCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_SWIZZLE_STATE_CREATE_INFO_NV;
}
#endif
#ifdef VK_EXT_discard_rectangles
template <>
VkStructureType get_structure_type<VkPhysicalDeviceDiscardRectanglePropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DISCARD_RECTANGLE_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type<VkPipelineDiscardRectangleStateCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_DISCARD_RECTANGLE_STATE_CREATE_INFO_EXT;
}
#endif
#ifdef VK_EXT_conservative_rasterization
template <>
VkStructureType get_structure_type<VkPhysicalDeviceConservativeRasterizationPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type<VkPipelineRasterizationConservativeStateCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT;
}
#endif
#ifdef VK_EXT_depth_clip_enable
template <>
VkStructureType get_structure_type<VkPhysicalDeviceDepthClipEnableFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type<VkPipelineRasterizationDepthClipStateCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT;
}
#endif
#ifdef VK_EXT_hdr_metadata
template <>
VkStructureType get_structure_type<VkHdrMetadataEXT>()
{
	return VK_STRUCTURE_TYPE_HDR_METADATA_EXT;
}
#endif
#ifdef VK_MVK_ios_surface
template <>
VkStructureType get_structure_type<VkIOSSurfaceCreateInfoMVK>()
{
	return VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK;
}
#endif
#ifdef VK_MVK_macos_surface
template <>
VkStructureType get_structure_type<VkMacOSSurfaceCreateInfoMVK>()
{
	return VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
}
#endif
#ifdef VK_EXT_debug_utils
template <>
VkStructureType get_structure_type<VkDebugUtilsLabelEXT>()
{
	return VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
}
template <>
VkStructureType get_structure_type<VkDebugUtilsObjectNameInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkDebugUtilsMessengerCallbackDataEXT>()
{
	return VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT;
}
template <>
VkStructureType get_structure_type<VkDebugUtilsMessengerCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkDebugUtilsObjectTagInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_TAG_INFO_EXT;
}
#endif
#ifdef VK_ANDROID_external_memory_android_hardware_buffer
template <>
VkStructureType get_structure_type<VkAndroidHardwareBufferUsageANDROID>()
{
	return VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_USAGE_ANDROID;
}
template <>
VkStructureType get_structure_type<VkAndroidHardwareBufferPropertiesANDROID>()
{
	return VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID;
}
template <>
VkStructureType get_structure_type<VkAndroidHardwareBufferFormatPropertiesANDROID>()
{
	return VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_ANDROID;
}
template <>
VkStructureType get_structure_type<VkImportAndroidHardwareBufferInfoANDROID>()
{
	return VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID;
}
template <>
VkStructureType get_structure_type<VkMemoryGetAndroidHardwareBufferInfoANDROID>()
{
	return VK_STRUCTURE_TYPE_MEMORY_GET_ANDROID_HARDWARE_BUFFER_INFO_ANDROID;
}
template <>
VkStructureType get_structure_type<VkExternalFormatANDROID>()
{
	return VK_STRUCTURE_TYPE_EXTERNAL_FORMAT_ANDROID;
}
template <>
VkStructureType get_structure_type<VkAndroidHardwareBufferFormatProperties2ANDROID>()
{
	return VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_2_ANDROID;
}
#endif
#ifdef VK_EXT_sample_locations
template <>
VkStructureType get_structure_type<VkSampleLocationsInfoEXT>()
{
	return VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkRenderPassSampleLocationsBeginInfoEXT>()
{
	return VK_STRUCTURE_TYPE_RENDER_PASS_SAMPLE_LOCATIONS_BEGIN_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkPipelineSampleLocationsStateCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_SAMPLE_LOCATIONS_STATE_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceSampleLocationsPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLE_LOCATIONS_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type<VkMultisamplePropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_MULTISAMPLE_PROPERTIES_EXT;
}
#endif
#ifdef VK_EXT_blend_operation_advanced
template <>
VkStructureType get_structure_type<VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type<VkPipelineColorBlendAdvancedStateCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_ADVANCED_STATE_CREATE_INFO_EXT;
}
#endif
#ifdef VK_NV_fragment_coverage_to_color
template <>
VkStructureType get_structure_type<VkPipelineCoverageToColorStateCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_TO_COLOR_STATE_CREATE_INFO_NV;
}
#endif
#ifdef VK_NV_framebuffer_mixed_samples
template <>
VkStructureType get_structure_type<VkPipelineCoverageModulationStateCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_MODULATION_STATE_CREATE_INFO_NV;
}
#endif
#ifdef VK_NV_shader_sm_builtins
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShaderSMBuiltinsPropertiesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_PROPERTIES_NV;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShaderSMBuiltinsFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_FEATURES_NV;
}
#endif
#ifdef VK_EXT_image_drm_format_modifier
template <>
VkStructureType get_structure_type<VkDrmFormatModifierPropertiesListEXT>()
{
	return VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceImageDrmFormatModifierInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_DRM_FORMAT_MODIFIER_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkImageDrmFormatModifierListCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_LIST_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkImageDrmFormatModifierExplicitCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkImageDrmFormatModifierPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type<VkDrmFormatModifierPropertiesList2EXT>()
{
	return VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_2_EXT;
}
#endif
#ifdef VK_EXT_validation_cache
template <>
VkStructureType get_structure_type<VkValidationCacheCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VALIDATION_CACHE_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkShaderModuleValidationCacheCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_SHADER_MODULE_VALIDATION_CACHE_CREATE_INFO_EXT;
}
#endif
#ifdef VK_NV_shading_rate_image
template <>
VkStructureType get_structure_type<VkPipelineViewportShadingRateImageStateCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_SHADING_RATE_IMAGE_STATE_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShadingRateImageFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShadingRateImagePropertiesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_PROPERTIES_NV;
}
template <>
VkStructureType get_structure_type<VkPipelineViewportCoarseSampleOrderStateCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_COARSE_SAMPLE_ORDER_STATE_CREATE_INFO_NV;
}
#endif
#ifdef VK_NV_ray_tracing
template <>
VkStructureType get_structure_type<VkRayTracingShaderGroupCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type<VkRayTracingPipelineCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type<VkGeometryTrianglesNV>()
{
	return VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
}
template <>
VkStructureType get_structure_type<VkGeometryAABBNV>()
{
	return VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;
}
template <>
VkStructureType get_structure_type<VkGeometryNV>()
{
	return VK_STRUCTURE_TYPE_GEOMETRY_NV;
}
template <>
VkStructureType get_structure_type<VkAccelerationStructureInfoNV>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
}
template <>
VkStructureType get_structure_type<VkAccelerationStructureCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type<VkBindAccelerationStructureMemoryInfoNV>()
{
	return VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
}
template <>
VkStructureType get_structure_type<VkWriteDescriptorSetAccelerationStructureNV>()
{
	return VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
}
template <>
VkStructureType get_structure_type<VkAccelerationStructureMemoryRequirementsInfoNV>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceRayTracingPropertiesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
}
#endif
#ifdef VK_NV_representative_fragment_test
template <>
VkStructureType get_structure_type<VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_REPRESENTATIVE_FRAGMENT_TEST_FEATURES_NV;
}
template <>
VkStructureType get_structure_type<VkPipelineRepresentativeFragmentTestStateCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_REPRESENTATIVE_FRAGMENT_TEST_STATE_CREATE_INFO_NV;
}
#endif
#ifdef VK_EXT_filter_cubic
template <>
VkStructureType get_structure_type<VkPhysicalDeviceImageViewImageFormatInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_IMAGE_FORMAT_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkFilterCubicImageViewImageFormatPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_FILTER_CUBIC_IMAGE_VIEW_IMAGE_FORMAT_PROPERTIES_EXT;
}
#endif
#ifdef VK_EXT_external_memory_host
template <>
VkStructureType get_structure_type<VkImportMemoryHostPointerInfoEXT>()
{
	return VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkMemoryHostPointerPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_MEMORY_HOST_POINTER_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceExternalMemoryHostPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT;
}
#endif
#ifdef VK_AMD_pipeline_compiler_control
template <>
VkStructureType get_structure_type<VkPipelineCompilerControlCreateInfoAMD>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_COMPILER_CONTROL_CREATE_INFO_AMD;
}
#endif
#ifdef VK_EXT_calibrated_timestamps
template <>
VkStructureType get_structure_type<VkCalibratedTimestampInfoEXT>()
{
	return VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_EXT;
}
#endif
#ifdef VK_AMD_shader_core_properties
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShaderCorePropertiesAMD>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_AMD;
}
#endif
#ifdef VK_EXT_video_decode_h265
template <>
VkStructureType get_structure_type<VkVideoDecodeH265ProfileEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PROFILE_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoDecodeH265CapabilitiesEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_CAPABILITIES_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoDecodeH265SessionParametersAddInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_SESSION_PARAMETERS_ADD_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoDecodeH265SessionParametersCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_SESSION_PARAMETERS_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoDecodeH265PictureInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PICTURE_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkVideoDecodeH265DpbSlotInfoEXT>()
{
	return VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_DPB_SLOT_INFO_EXT;
}
#endif
#ifdef VK_AMD_memory_overallocation_behavior
template <>
VkStructureType get_structure_type<VkDeviceMemoryOverallocationCreateInfoAMD>()
{
	return VK_STRUCTURE_TYPE_DEVICE_MEMORY_OVERALLOCATION_CREATE_INFO_AMD;
}
#endif
#ifdef VK_EXT_vertex_attribute_divisor
template <>
VkStructureType get_structure_type<VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type<VkPipelineVertexInputDivisorStateCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT;
}
#endif
#ifdef VK_GGP_frame_token
template <>
VkStructureType get_structure_type<VkPresentFrameTokenGGP>()
{
	return VK_STRUCTURE_TYPE_PRESENT_FRAME_TOKEN_GGP;
}
#endif
#ifdef VK_NV_compute_shader_derivatives
template <>
VkStructureType get_structure_type<VkPhysicalDeviceComputeShaderDerivativesFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_NV;
}
#endif
#ifdef VK_NV_mesh_shader
template <>
VkStructureType get_structure_type<VkPhysicalDeviceMeshShaderFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceMeshShaderPropertiesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;
}
#endif
#ifdef VK_NV_fragment_shader_barycentric
template <>
VkStructureType get_structure_type<VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_NV;
}
#endif
#ifdef VK_NV_shader_image_footprint
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShaderImageFootprintFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_FOOTPRINT_FEATURES_NV;
}
#endif
#ifdef VK_NV_scissor_exclusive
template <>
VkStructureType get_structure_type<VkPipelineViewportExclusiveScissorStateCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_EXCLUSIVE_SCISSOR_STATE_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceExclusiveScissorFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXCLUSIVE_SCISSOR_FEATURES_NV;
}
#endif
#ifdef VK_NV_device_diagnostic_checkpoints
template <>
VkStructureType get_structure_type<VkQueueFamilyCheckpointPropertiesNV>()
{
	return VK_STRUCTURE_TYPE_QUEUE_FAMILY_CHECKPOINT_PROPERTIES_NV;
}
template <>
VkStructureType get_structure_type<VkCheckpointDataNV>()
{
	return VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV;
}
#endif
#ifdef VK_INTEL_shader_integer_functions2
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_FUNCTIONS_2_FEATURES_INTEL;
}
#endif
#ifdef VK_INTEL_performance_query
template <>
VkStructureType get_structure_type<VkInitializePerformanceApiInfoINTEL>()
{
	return VK_STRUCTURE_TYPE_INITIALIZE_PERFORMANCE_API_INFO_INTEL;
}
template <>
VkStructureType get_structure_type<VkQueryPoolPerformanceQueryCreateInfoINTEL>()
{
	return VK_STRUCTURE_TYPE_QUERY_POOL_PERFORMANCE_QUERY_CREATE_INFO_INTEL;
}
template <>
VkStructureType get_structure_type<VkPerformanceMarkerInfoINTEL>()
{
	return VK_STRUCTURE_TYPE_PERFORMANCE_MARKER_INFO_INTEL;
}
template <>
VkStructureType get_structure_type<VkPerformanceStreamMarkerInfoINTEL>()
{
	return VK_STRUCTURE_TYPE_PERFORMANCE_STREAM_MARKER_INFO_INTEL;
}
template <>
VkStructureType get_structure_type<VkPerformanceOverrideInfoINTEL>()
{
	return VK_STRUCTURE_TYPE_PERFORMANCE_OVERRIDE_INFO_INTEL;
}
template <>
VkStructureType get_structure_type<VkPerformanceConfigurationAcquireInfoINTEL>()
{
	return VK_STRUCTURE_TYPE_PERFORMANCE_CONFIGURATION_ACQUIRE_INFO_INTEL;
}
#endif
#ifdef VK_EXT_pci_bus_info
template <>
VkStructureType get_structure_type<VkPhysicalDevicePCIBusInfoPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PCI_BUS_INFO_PROPERTIES_EXT;
}
#endif
#ifdef VK_AMD_display_native_hdr
template <>
VkStructureType get_structure_type<VkDisplayNativeHdrSurfaceCapabilitiesAMD>()
{
	return VK_STRUCTURE_TYPE_DISPLAY_NATIVE_HDR_SURFACE_CAPABILITIES_AMD;
}
template <>
VkStructureType get_structure_type<VkSwapchainDisplayNativeHdrCreateInfoAMD>()
{
	return VK_STRUCTURE_TYPE_SWAPCHAIN_DISPLAY_NATIVE_HDR_CREATE_INFO_AMD;
}
#endif
#ifdef VK_FUCHSIA_imagepipe_surface
template <>
VkStructureType get_structure_type<VkImagePipeSurfaceCreateInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_IMAGEPIPE_SURFACE_CREATE_INFO_FUCHSIA;
}
#endif
#ifdef VK_EXT_metal_surface
template <>
VkStructureType get_structure_type<VkMetalSurfaceCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
}
#endif
#ifdef VK_EXT_fragment_density_map
template <>
VkStructureType get_structure_type<VkPhysicalDeviceFragmentDensityMapFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceFragmentDensityMapPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type<VkRenderPassFragmentDensityMapCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_CREATE_INFO_EXT;
}
#endif
#ifdef VK_AMD_shader_core_properties2
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShaderCoreProperties2AMD>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_2_AMD;
}
#endif
#ifdef VK_AMD_device_coherent_memory
template <>
VkStructureType get_structure_type<VkPhysicalDeviceCoherentMemoryFeaturesAMD>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD;
}
#endif
#ifdef VK_EXT_shader_image_atomic_int64
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT;
}
#endif
#ifdef VK_EXT_memory_budget
template <>
VkStructureType get_structure_type<VkPhysicalDeviceMemoryBudgetPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
}
#endif
#ifdef VK_EXT_memory_priority
template <>
VkStructureType get_structure_type<VkPhysicalDeviceMemoryPriorityFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type<VkMemoryPriorityAllocateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
}
#endif
#ifdef VK_NV_dedicated_allocation_image_aliasing
template <>
VkStructureType get_structure_type<VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEDICATED_ALLOCATION_IMAGE_ALIASING_FEATURES_NV;
}
#endif
#ifdef VK_EXT_buffer_device_address
template <>
VkStructureType get_structure_type<VkPhysicalDeviceBufferDeviceAddressFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type<VkBufferDeviceAddressCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_CREATE_INFO_EXT;
}
#endif
#ifdef VK_EXT_validation_features
template <>
VkStructureType get_structure_type<VkValidationFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
}
#endif
#ifdef VK_NV_cooperative_matrix
template <>
VkStructureType get_structure_type<VkCooperativeMatrixPropertiesNV>()
{
	return VK_STRUCTURE_TYPE_COOPERATIVE_MATRIX_PROPERTIES_NV;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceCooperativeMatrixFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_NV;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceCooperativeMatrixPropertiesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_PROPERTIES_NV;
}
#endif
#ifdef VK_NV_coverage_reduction_mode
template <>
VkStructureType get_structure_type<VkPhysicalDeviceCoverageReductionModeFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COVERAGE_REDUCTION_MODE_FEATURES_NV;
}
template <>
VkStructureType get_structure_type<VkPipelineCoverageReductionStateCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_REDUCTION_STATE_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type<VkFramebufferMixedSamplesCombinationNV>()
{
	return VK_STRUCTURE_TYPE_FRAMEBUFFER_MIXED_SAMPLES_COMBINATION_NV;
}
#endif
#ifdef VK_EXT_fragment_shader_interlock
template <>
VkStructureType get_structure_type<VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT;
}
#endif
#ifdef VK_EXT_ycbcr_image_arrays
template <>
VkStructureType get_structure_type<VkPhysicalDeviceYcbcrImageArraysFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_IMAGE_ARRAYS_FEATURES_EXT;
}
#endif
#ifdef VK_EXT_provoking_vertex
template <>
VkStructureType get_structure_type<VkPhysicalDeviceProvokingVertexFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROVOKING_VERTEX_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceProvokingVertexPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROVOKING_VERTEX_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type<VkPipelineRasterizationProvokingVertexStateCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_PROVOKING_VERTEX_STATE_CREATE_INFO_EXT;
}
#endif
#ifdef VK_EXT_full_screen_exclusive
template <>
VkStructureType get_structure_type<VkSurfaceFullScreenExclusiveInfoEXT>()
{
	return VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkSurfaceCapabilitiesFullScreenExclusiveEXT>()
{
	return VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_FULL_SCREEN_EXCLUSIVE_EXT;
}
template <>
VkStructureType get_structure_type<VkSurfaceFullScreenExclusiveWin32InfoEXT>()
{
	return VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT;
}
#endif
#ifdef VK_EXT_headless_surface
template <>
VkStructureType get_structure_type<VkHeadlessSurfaceCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT;
}
#endif
#ifdef VK_EXT_line_rasterization
template <>
VkStructureType get_structure_type<VkPhysicalDeviceLineRasterizationFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceLineRasterizationPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type<VkPipelineRasterizationLineStateCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_EXT;
}
#endif
#ifdef VK_EXT_shader_atomic_float
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShaderAtomicFloatFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT;
}
#endif
#ifdef VK_EXT_index_type_uint8
template <>
VkStructureType get_structure_type<VkPhysicalDeviceIndexTypeUint8FeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT;
}
#endif
#ifdef VK_EXT_extended_dynamic_state
template <>
VkStructureType get_structure_type<VkPhysicalDeviceExtendedDynamicStateFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
}
#endif
#ifdef VK_EXT_shader_atomic_float2
template <>
VkStructureType get_structure_type<VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT;
}
#endif
#ifdef VK_NV_device_generated_commands
template <>
VkStructureType get_structure_type<VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_PROPERTIES_NV;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_NV;
}
template <>
VkStructureType get_structure_type<VkGraphicsShaderGroupCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_GRAPHICS_SHADER_GROUP_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type<VkGraphicsPipelineShaderGroupsCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_SHADER_GROUPS_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type<VkIndirectCommandsLayoutTokenNV>()
{
	return VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_TOKEN_NV;
}
template <>
VkStructureType get_structure_type<VkIndirectCommandsLayoutCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_CREATE_INFO_NV;
}
template <>
VkStructureType get_structure_type<VkGeneratedCommandsInfoNV>()
{
	return VK_STRUCTURE_TYPE_GENERATED_COMMANDS_INFO_NV;
}
template <>
VkStructureType get_structure_type<VkGeneratedCommandsMemoryRequirementsInfoNV>()
{
	return VK_STRUCTURE_TYPE_GENERATED_COMMANDS_MEMORY_REQUIREMENTS_INFO_NV;
}
#endif
#ifdef VK_NV_inherited_viewport_scissor
template <>
VkStructureType get_structure_type<VkPhysicalDeviceInheritedViewportScissorFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INHERITED_VIEWPORT_SCISSOR_FEATURES_NV;
}
template <>
VkStructureType get_structure_type<VkCommandBufferInheritanceViewportScissorInfoNV>()
{
	return VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_VIEWPORT_SCISSOR_INFO_NV;
}
#endif
#ifdef VK_EXT_texel_buffer_alignment
template <>
VkStructureType get_structure_type<VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_FEATURES_EXT;
}
#endif
#ifdef VK_QCOM_render_pass_transform
template <>
VkStructureType get_structure_type<VkRenderPassTransformBeginInfoQCOM>()
{
	return VK_STRUCTURE_TYPE_RENDER_PASS_TRANSFORM_BEGIN_INFO_QCOM;
}
template <>
VkStructureType get_structure_type<VkCommandBufferInheritanceRenderPassTransformInfoQCOM>()
{
	return VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDER_PASS_TRANSFORM_INFO_QCOM;
}
#endif
#ifdef VK_EXT_device_memory_report
template <>
VkStructureType get_structure_type<VkPhysicalDeviceDeviceMemoryReportFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_MEMORY_REPORT_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type<VkDeviceMemoryReportCallbackDataEXT>()
{
	return VK_STRUCTURE_TYPE_DEVICE_MEMORY_REPORT_CALLBACK_DATA_EXT;
}
template <>
VkStructureType get_structure_type<VkDeviceDeviceMemoryReportCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DEVICE_DEVICE_MEMORY_REPORT_CREATE_INFO_EXT;
}
#endif
#ifdef VK_EXT_robustness2
template <>
VkStructureType get_structure_type<VkPhysicalDeviceRobustness2FeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceRobustness2PropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_PROPERTIES_EXT;
}
#endif
#ifdef VK_EXT_custom_border_color
template <>
VkStructureType get_structure_type<VkSamplerCustomBorderColorCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceCustomBorderColorPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceCustomBorderColorFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT;
}
#endif
#ifdef VK_NV_device_diagnostics_config
template <>
VkStructureType get_structure_type<VkPhysicalDeviceDiagnosticsConfigFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DIAGNOSTICS_CONFIG_FEATURES_NV;
}
template <>
VkStructureType get_structure_type<VkDeviceDiagnosticsConfigCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV;
}
#endif
#ifdef VK_EXT_graphics_pipeline_library
template <>
VkStructureType get_structure_type<VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceGraphicsPipelineLibraryPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_PROPERTIES_EXT;
}
template <>
VkStructureType get_structure_type<VkGraphicsPipelineLibraryCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT;
}
#endif
#ifdef VK_NV_fragment_shading_rate_enums
template <>
VkStructureType get_structure_type<VkPhysicalDeviceFragmentShadingRateEnumsFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_ENUMS_FEATURES_NV;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceFragmentShadingRateEnumsPropertiesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_ENUMS_PROPERTIES_NV;
}
template <>
VkStructureType get_structure_type<VkPipelineFragmentShadingRateEnumStateCreateInfoNV>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_ENUM_STATE_CREATE_INFO_NV;
}
#endif
#ifdef VK_NV_ray_tracing_motion_blur
template <>
VkStructureType get_structure_type<VkAccelerationStructureGeometryMotionTrianglesDataNV>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_MOTION_TRIANGLES_DATA_NV;
}
template <>
VkStructureType get_structure_type<VkAccelerationStructureMotionInfoNV>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MOTION_INFO_NV;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceRayTracingMotionBlurFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MOTION_BLUR_FEATURES_NV;
}
#endif
#ifdef VK_EXT_ycbcr_2plane_444_formats
template <>
VkStructureType get_structure_type<VkPhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_2_PLANE_444_FORMATS_FEATURES_EXT;
}
#endif
#ifdef VK_EXT_fragment_density_map2
template <>
VkStructureType get_structure_type<VkPhysicalDeviceFragmentDensityMap2FeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceFragmentDensityMap2PropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_PROPERTIES_EXT;
}
#endif
#ifdef VK_QCOM_rotated_copy_commands
template <>
VkStructureType get_structure_type<VkCopyCommandTransformInfoQCOM>()
{
	return VK_STRUCTURE_TYPE_COPY_COMMAND_TRANSFORM_INFO_QCOM;
}
#endif
#ifdef VK_EXT_4444_formats
template <>
VkStructureType get_structure_type<VkPhysicalDevice4444FormatsFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_4444_FORMATS_FEATURES_EXT;
}
#endif
#ifdef VK_ARM_rasterization_order_attachment_access
template <>
VkStructureType get_structure_type<VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesARM>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_FEATURES_ARM;
}
#endif
#ifdef VK_EXT_rgba10x6_formats
template <>
VkStructureType get_structure_type<VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RGBA10X6_FORMATS_FEATURES_EXT;
}
#endif
#ifdef VK_EXT_directfb_surface
template <>
VkStructureType get_structure_type<VkDirectFBSurfaceCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_DIRECTFB_SURFACE_CREATE_INFO_EXT;
}
#endif
#ifdef VK_VALVE_mutable_descriptor_type
template <>
VkStructureType get_structure_type<VkPhysicalDeviceMutableDescriptorTypeFeaturesVALVE>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_VALVE;
}
template <>
VkStructureType get_structure_type<VkMutableDescriptorTypeCreateInfoVALVE>()
{
	return VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_VALVE;
}
#endif
#ifdef VK_EXT_vertex_input_dynamic_state
template <>
VkStructureType get_structure_type<VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type<VkVertexInputBindingDescription2EXT>()
{
	return VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
}
template <>
VkStructureType get_structure_type<VkVertexInputAttributeDescription2EXT>()
{
	return VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
}
#endif
#ifdef VK_EXT_physical_device_drm
template <>
VkStructureType get_structure_type<VkPhysicalDeviceDrmPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRM_PROPERTIES_EXT;
}
#endif
#ifdef VK_EXT_depth_clip_control
template <>
VkStructureType get_structure_type<VkPhysicalDeviceDepthClipControlFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_CONTROL_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type<VkPipelineViewportDepthClipControlCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_DEPTH_CLIP_CONTROL_CREATE_INFO_EXT;
}
#endif
#ifdef VK_EXT_primitive_topology_list_restart
template <>
VkStructureType get_structure_type<VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVE_TOPOLOGY_LIST_RESTART_FEATURES_EXT;
}
#endif
#ifdef VK_FUCHSIA_external_memory
template <>
VkStructureType get_structure_type<VkImportMemoryZirconHandleInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_IMPORT_MEMORY_ZIRCON_HANDLE_INFO_FUCHSIA;
}
template <>
VkStructureType get_structure_type<VkMemoryZirconHandlePropertiesFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_MEMORY_ZIRCON_HANDLE_PROPERTIES_FUCHSIA;
}
template <>
VkStructureType get_structure_type<VkMemoryGetZirconHandleInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_MEMORY_GET_ZIRCON_HANDLE_INFO_FUCHSIA;
}
#endif
#ifdef VK_FUCHSIA_external_semaphore
template <>
VkStructureType get_structure_type<VkImportSemaphoreZirconHandleInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_ZIRCON_HANDLE_INFO_FUCHSIA;
}
template <>
VkStructureType get_structure_type<VkSemaphoreGetZirconHandleInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_SEMAPHORE_GET_ZIRCON_HANDLE_INFO_FUCHSIA;
}
#endif
#ifdef VK_FUCHSIA_buffer_collection
template <>
VkStructureType get_structure_type<VkBufferCollectionCreateInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_BUFFER_COLLECTION_CREATE_INFO_FUCHSIA;
}
template <>
VkStructureType get_structure_type<VkImportMemoryBufferCollectionFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_IMPORT_MEMORY_BUFFER_COLLECTION_FUCHSIA;
}
template <>
VkStructureType get_structure_type<VkBufferCollectionImageCreateInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_BUFFER_COLLECTION_IMAGE_CREATE_INFO_FUCHSIA;
}
template <>
VkStructureType get_structure_type<VkBufferCollectionConstraintsInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_BUFFER_COLLECTION_CONSTRAINTS_INFO_FUCHSIA;
}
template <>
VkStructureType get_structure_type<VkBufferConstraintsInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_BUFFER_CONSTRAINTS_INFO_FUCHSIA;
}
template <>
VkStructureType get_structure_type<VkBufferCollectionBufferCreateInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_BUFFER_COLLECTION_BUFFER_CREATE_INFO_FUCHSIA;
}
template <>
VkStructureType get_structure_type<VkSysmemColorSpaceFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_SYSMEM_COLOR_SPACE_FUCHSIA;
}
template <>
VkStructureType get_structure_type<VkBufferCollectionPropertiesFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_BUFFER_COLLECTION_PROPERTIES_FUCHSIA;
}
template <>
VkStructureType get_structure_type<VkImageFormatConstraintsInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_IMAGE_FORMAT_CONSTRAINTS_INFO_FUCHSIA;
}
template <>
VkStructureType get_structure_type<VkImageConstraintsInfoFUCHSIA>()
{
	return VK_STRUCTURE_TYPE_IMAGE_CONSTRAINTS_INFO_FUCHSIA;
}
#endif
#ifdef VK_HUAWEI_subpass_shading
template <>
VkStructureType get_structure_type<VkSubpassShadingPipelineCreateInfoHUAWEI>()
{
	return VK_STRUCTURE_TYPE_SUBPASS_SHADING_PIPELINE_CREATE_INFO_HUAWEI;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceSubpassShadingFeaturesHUAWEI>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_SHADING_FEATURES_HUAWEI;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceSubpassShadingPropertiesHUAWEI>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_SHADING_PROPERTIES_HUAWEI;
}
#endif
#ifdef VK_HUAWEI_invocation_mask
template <>
VkStructureType get_structure_type<VkPhysicalDeviceInvocationMaskFeaturesHUAWEI>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INVOCATION_MASK_FEATURES_HUAWEI;
}
#endif
#ifdef VK_NV_external_memory_rdma
template <>
VkStructureType get_structure_type<VkMemoryGetRemoteAddressInfoNV>()
{
	return VK_STRUCTURE_TYPE_MEMORY_GET_REMOTE_ADDRESS_INFO_NV;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceExternalMemoryRDMAFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_RDMA_FEATURES_NV;
}
#endif
#ifdef VK_EXT_extended_dynamic_state2
template <>
VkStructureType get_structure_type<VkPhysicalDeviceExtendedDynamicState2FeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT;
}
#endif
#ifdef VK_QNX_screen_surface
template <>
VkStructureType get_structure_type<VkScreenSurfaceCreateInfoQNX>()
{
	return VK_STRUCTURE_TYPE_SCREEN_SURFACE_CREATE_INFO_QNX;
}
#endif
#ifdef VK_EXT_color_write_enable
template <>
VkStructureType get_structure_type<VkPhysicalDeviceColorWriteEnableFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COLOR_WRITE_ENABLE_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type<VkPipelineColorWriteCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_PIPELINE_COLOR_WRITE_CREATE_INFO_EXT;
}
#endif
#ifdef VK_EXT_primitives_generated_query
template <>
VkStructureType get_structure_type<VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVES_GENERATED_QUERY_FEATURES_EXT;
}
#endif
#ifdef VK_EXT_image_view_min_lod
template <>
VkStructureType get_structure_type<VkPhysicalDeviceImageViewMinLodFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_MIN_LOD_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type<VkImageViewMinLodCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_IMAGE_VIEW_MIN_LOD_CREATE_INFO_EXT;
}
#endif
#ifdef VK_EXT_multi_draw
template <>
VkStructureType get_structure_type<VkPhysicalDeviceMultiDrawFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceMultiDrawPropertiesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_PROPERTIES_EXT;
}
#endif
#ifdef VK_EXT_image_2d_view_of_3d
template <>
VkStructureType get_structure_type<VkPhysicalDeviceImage2DViewOf3DFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_2D_VIEW_OF_3D_FEATURES_EXT;
}
#endif
#ifdef VK_EXT_border_color_swizzle
template <>
VkStructureType get_structure_type<VkPhysicalDeviceBorderColorSwizzleFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BORDER_COLOR_SWIZZLE_FEATURES_EXT;
}
template <>
VkStructureType get_structure_type<VkSamplerBorderColorComponentMappingCreateInfoEXT>()
{
	return VK_STRUCTURE_TYPE_SAMPLER_BORDER_COLOR_COMPONENT_MAPPING_CREATE_INFO_EXT;
}
#endif
#ifdef VK_EXT_pageable_device_local_memory
template <>
VkStructureType get_structure_type<VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PAGEABLE_DEVICE_LOCAL_MEMORY_FEATURES_EXT;
}
#endif
#ifdef VK_VALVE_descriptor_set_host_mapping
template <>
VkStructureType get_structure_type<VkPhysicalDeviceDescriptorSetHostMappingFeaturesVALVE>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_SET_HOST_MAPPING_FEATURES_VALVE;
}
template <>
VkStructureType get_structure_type<VkDescriptorSetBindingReferenceVALVE>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_SET_BINDING_REFERENCE_VALVE;
}
template <>
VkStructureType get_structure_type<VkDescriptorSetLayoutHostMappingInfoVALVE>()
{
	return VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_HOST_MAPPING_INFO_VALVE;
}
#endif
#ifdef VK_QCOM_fragment_density_map_offset
template <>
VkStructureType get_structure_type<VkPhysicalDeviceFragmentDensityMapOffsetFeaturesQCOM>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_FEATURES_QCOM;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceFragmentDensityMapOffsetPropertiesQCOM>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_PROPERTIES_QCOM;
}
template <>
VkStructureType get_structure_type<VkSubpassFragmentDensityMapOffsetEndInfoQCOM>()
{
	return VK_STRUCTURE_TYPE_SUBPASS_FRAGMENT_DENSITY_MAP_OFFSET_END_INFO_QCOM;
}
#endif
#ifdef VK_NV_linear_color_attachment
template <>
VkStructureType get_structure_type<VkPhysicalDeviceLinearColorAttachmentFeaturesNV>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINEAR_COLOR_ATTACHMENT_FEATURES_NV;
}
#endif
#ifdef VK_KHR_acceleration_structure
template <>
VkStructureType get_structure_type<VkAccelerationStructureGeometryTrianglesDataKHR>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
}
template <>
VkStructureType get_structure_type<VkAccelerationStructureGeometryAabbsDataKHR>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
}
template <>
VkStructureType get_structure_type<VkAccelerationStructureGeometryInstancesDataKHR>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
}
template <>
VkStructureType get_structure_type<VkAccelerationStructureGeometryKHR>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
}
template <>
VkStructureType get_structure_type<VkAccelerationStructureBuildGeometryInfoKHR>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkAccelerationStructureCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkWriteDescriptorSetAccelerationStructureKHR>()
{
	return VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceAccelerationStructureFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceAccelerationStructurePropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
}
template <>
VkStructureType get_structure_type<VkAccelerationStructureDeviceAddressInfoKHR>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkAccelerationStructureVersionInfoKHR>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_VERSION_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkCopyAccelerationStructureToMemoryInfoKHR>()
{
	return VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_TO_MEMORY_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkCopyMemoryToAccelerationStructureInfoKHR>()
{
	return VK_STRUCTURE_TYPE_COPY_MEMORY_TO_ACCELERATION_STRUCTURE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkCopyAccelerationStructureInfoKHR>()
{
	return VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkAccelerationStructureBuildSizesInfoKHR>()
{
	return VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
}
#endif
#ifdef VK_KHR_ray_tracing_pipeline
template <>
VkStructureType get_structure_type<VkRayTracingShaderGroupCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkRayTracingPipelineInterfaceCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_INTERFACE_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkRayTracingPipelineCreateInfoKHR>()
{
	return VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceRayTracingPipelineFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
}
template <>
VkStructureType get_structure_type<VkPhysicalDeviceRayTracingPipelinePropertiesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
}
#endif
#ifdef VK_KHR_ray_query
template <>
VkStructureType get_structure_type<VkPhysicalDeviceRayQueryFeaturesKHR>()
{
	return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
}
#endif
}        // namespace vulkan
}        // namespace components
