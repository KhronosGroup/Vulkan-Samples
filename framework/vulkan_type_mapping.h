/* Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.
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
struct HPPType<VkRenderPass>
{
	using Type = vk::RenderPass;
};

template <>
struct HPPType<VkSampler>
{
	using Type = vk::Sampler;
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
