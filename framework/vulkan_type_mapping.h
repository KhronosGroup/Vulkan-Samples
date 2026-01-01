/* Copyright (c) 2025, Arm Limited and Contributors
 * Copyright (c) 2024-2025, NVIDIA CORPORATION. All rights reserved.
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

#include "common/vk_common.h"
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace detail
{
// Mapping from VkType to vk::Type
template <typename VkType>
struct HPPType
{
};

template <>
struct HPPType<VkBuffer>
{
	using Type = vk::Buffer;
};

template <>
struct HPPType<VkBufferCreateInfo>
{
	using Type = vk::BufferCreateInfo;
};

template <>
struct HPPType<VkCommandBuffer>
{
	using Type = vk::CommandBuffer;
};

template <>
struct HPPType<VkDataGraphPipelineSessionARM>
{
	using Type = vk::DataGraphPipelineSessionARM;
};

template <>
struct HPPType<VkDevice>
{
	using Type = vk::Device;
};

template <>
struct HPPType<VkImage>
{
	using Type = vk::Image;
};

template <>
struct HPPType<VkImageCreateInfo>
{
	using Type = vk::ImageCreateInfo;
};

template <>
struct HPPType<VkImageView>
{
	using Type = vk::ImageView;
};

template <>
struct HPPType<VkPhysicalDeviceAccelerationStructureFeaturesKHR>
{
	using Type = vk::PhysicalDeviceAccelerationStructureFeaturesKHR;
};

template <>
struct HPPType<VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT>
{
	using Type = vk::PhysicalDeviceBlendOperationAdvancedFeaturesEXT;
};

template <>
struct HPPType<VkPhysicalDeviceBufferDeviceAddressFeatures>
{
	using Type = vk::PhysicalDeviceBufferDeviceAddressFeatures;
};

template <>
struct HPPType<VkPhysicalDeviceColorWriteEnableFeaturesEXT>
{
	using Type = vk::PhysicalDeviceColorWriteEnableFeaturesEXT;
};

template <>
struct HPPType<VkPhysicalDeviceDepthClipEnableFeaturesEXT>
{
	using Type = vk::PhysicalDeviceDepthClipEnableFeaturesEXT;
};

template <>
struct HPPType<VkPhysicalDeviceConditionalRenderingFeaturesEXT>
{
	using Type = vk::PhysicalDeviceConditionalRenderingFeaturesEXT;
};

template <>
struct HPPType<VkPhysicalDeviceDescriptorBufferFeaturesEXT>
{
	using Type = vk::PhysicalDeviceDescriptorBufferFeaturesEXT;
};

template <>
struct HPPType<VkPhysicalDeviceDescriptorIndexingFeaturesEXT>
{
	using Type = vk::PhysicalDeviceDescriptorIndexingFeaturesEXT;
};

template <>
struct HPPType<VkPhysicalDeviceDynamicRenderingFeaturesKHR>
{
	using Type = vk::PhysicalDeviceDynamicRenderingFeaturesKHR;
};

template <>
struct HPPType<VkPhysicalDeviceDynamicRenderingLocalReadFeaturesKHR>
{
	using Type = vk::PhysicalDeviceDynamicRenderingLocalReadFeaturesKHR;
};

template <>
struct HPPType<VkPhysicalDeviceExtendedDynamicStateFeaturesEXT>
{
	using Type = vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT;
};

template <>
struct HPPType<VkPhysicalDeviceExtendedDynamicState2FeaturesEXT>
{
	using Type = vk::PhysicalDeviceExtendedDynamicState2FeaturesEXT;
};

template <>
struct HPPType<VkPhysicalDeviceExtendedDynamicState3FeaturesEXT>
{
	using Type = vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT;
};

template <>
struct HPPType<VkPhysicalDeviceFloat16Int8FeaturesKHR>
{
	using Type = vk::PhysicalDeviceFloat16Int8FeaturesKHR;
};

template <>
struct HPPType<VkPhysicalDeviceFragmentDensityMapFeaturesEXT>
{
	using Type = vk::PhysicalDeviceFragmentDensityMapFeaturesEXT;
};

template <>
struct HPPType<VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR>
{
	using Type = vk::PhysicalDeviceFragmentShaderBarycentricFeaturesKHR;
};

template <>
struct HPPType<VkPhysicalDeviceFragmentShadingRateFeaturesKHR>
{
	using Type = vk::PhysicalDeviceFragmentShadingRateFeaturesKHR;
};

template <>
struct HPPType<VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT>
{
	using Type = vk::PhysicalDeviceGraphicsPipelineLibraryFeaturesEXT;
};

template <>
struct HPPType<VkPhysicalDeviceHostImageCopyFeaturesEXT>
{
	using Type = vk::PhysicalDeviceHostImageCopyFeaturesEXT;
};

template <>
struct HPPType<VkPhysicalDeviceImageCompressionControlFeaturesEXT>
{
	using Type = vk::PhysicalDeviceImageCompressionControlFeaturesEXT;
};

template <>
struct HPPType<VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT>
{
	using Type = vk::PhysicalDeviceImageCompressionControlSwapchainFeaturesEXT;
};

template <>
struct HPPType<VkPhysicalDeviceLineRasterizationFeaturesEXT>
{
	using Type = vk::PhysicalDeviceLineRasterizationFeaturesEXT;
};

template <>
struct HPPType<VkPhysicalDeviceMeshShaderFeaturesEXT>
{
	using Type = vk::PhysicalDeviceMeshShaderFeaturesEXT;
};

#ifdef VKB_ENABLE_PORTABILITY
template <>
struct HPPType<VkPhysicalDevicePortabilitySubsetFeaturesKHR>
{
	using Type = vk::PhysicalDevicePortabilitySubsetFeaturesKHR;
};
#endif

template <>
struct HPPType<VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT>
{
	using Type = vk::PhysicalDevicePrimitiveTopologyListRestartFeaturesEXT;
};

template <>
struct HPPType<VkPhysicalDeviceRayQueryFeaturesKHR>
{
	using Type = vk::PhysicalDeviceRayQueryFeaturesKHR;
};

template <>
struct HPPType<VkPhysicalDeviceRayTracingPipelineFeaturesKHR>
{
	using Type = vk::PhysicalDeviceRayTracingPipelineFeaturesKHR;
};

template <>
struct HPPType<VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR>
{
	using Type = vk::PhysicalDeviceRayTracingPositionFetchFeaturesKHR;
};

template <>
struct HPPType<VkPhysicalDeviceScalarBlockLayoutFeaturesEXT>
{
	using Type = vk::PhysicalDeviceScalarBlockLayoutFeaturesEXT;
};

template <>
struct HPPType<VkPhysicalDeviceShaderObjectFeaturesEXT>
{
	using Type = vk::PhysicalDeviceShaderObjectFeaturesEXT;
};

template <>
struct HPPType<VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT>
{
	using Type = vk::PhysicalDeviceSwapchainMaintenance1FeaturesEXT;
};

template <>
struct HPPType<VkPhysicalDeviceSynchronization2FeaturesKHR>
{
	using Type = vk::PhysicalDeviceSynchronization2FeaturesKHR;
};

template <>
struct HPPType<VkPhysicalDeviceTensorFeaturesARM>
{
	using Type = vk::PhysicalDeviceTensorFeaturesARM;
};

template <>
struct HPPType<VkPhysicalDeviceTimelineSemaphoreFeaturesKHR>
{
	using Type = vk::PhysicalDeviceTimelineSemaphoreFeaturesKHR;
};

template <>
struct HPPType<VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT>
{
	using Type = vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT;
};

template <>
struct HPPType<VkPhysicalDeviceVulkan12Features>
{
	using Type = vk::PhysicalDeviceVulkan12Features;
};

template <>
struct HPPType<VkPhysicalDeviceVulkan13Features>
{
	using Type = vk::PhysicalDeviceVulkan13Features;
};

template <>
struct HPPType<VkPhysicalDevice16BitStorageFeatures>
{
	using Type = vk::PhysicalDevice16BitStorageFeatures;
};

template <>
struct HPPType<VkPipeline>
{
	using Type = vk::Pipeline;
};

template <>
struct HPPType<VkPipelineLayout>
{
	using Type = vk::PipelineLayout;
};

template <>
struct HPPType<VkRenderPass>
{
	using Type = vk::RenderPass;
};

template <>
struct HPPType<VkSampler>
{
	using Type = vk::Sampler;
};

template <>
struct HPPType<VkTensorARM>
{
	using Type = vk::TensorARM;
};

template <>
struct HPPType<VkTensorCreateInfoARM>
{
	using Type = vk::TensorCreateInfoARM;
};

template <>
struct HPPType<VkTensorViewARM>
{
	using Type = vk::TensorViewARM;
};
}        // namespace detail

template <vkb::BindingType bindingType, typename T>
struct VulkanTypeMapping
{
};
template <typename T>
struct VulkanTypeMapping<vkb::BindingType::Cpp, T>
{
	using Type = T;
};
template <typename T>
struct VulkanTypeMapping<vkb::BindingType::C, T>
{
	using Type = typename detail::HPPType<T>::Type;
};

}        // namespace vkb