/* Copyright (c) 2021-2025, NVIDIA CORPORATION. All rights reserved.
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
namespace core
{
template <vkb::BindingType bindingType>
class Device;
using DeviceCpp = Device<vkb::BindingType::Cpp>;

template <vkb::BindingType bindingType>
class CommandBuffer;
using CommandBufferCpp = CommandBuffer<vkb::BindingType::Cpp>;

/**
 * @brief A wrapper class for vk::Queue
 *
 */
class HPPQueue
{
  public:
	HPPQueue(vkb::core::DeviceCpp &device, uint32_t family_index, vk::QueueFamilyProperties const &properties, vk::Bool32 can_present, uint32_t index);

	HPPQueue(const HPPQueue &) = default;

	HPPQueue(HPPQueue &&other);

	HPPQueue &operator=(const HPPQueue &) = delete;

	HPPQueue &operator=(HPPQueue &&) = delete;

	const vkb::core::DeviceCpp &get_device() const;

	vk::Queue get_handle() const;

	uint32_t get_family_index() const;

	uint32_t get_index() const;

	const vk::QueueFamilyProperties &get_properties() const;

	vk::Bool32 support_present() const;

	void submit(const vkb::core::CommandBufferCpp &command_buffer, vk::Fence fence) const;

	vk::Result present(const vk::PresentInfoKHR &present_infos) const;

  private:
	vkb::core::DeviceCpp &device;

	vk::Queue handle;

	uint32_t family_index{0};

	uint32_t index{0};

	vk::Bool32 can_present = false;

	vk::QueueFamilyProperties properties{};
};
}        // namespace core
}        // namespace vkb
