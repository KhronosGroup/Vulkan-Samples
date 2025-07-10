/* Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
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
#include <memory>
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace rendering
{
template <vkb::BindingType bindingType>
class RenderFrame;
using RenderFrameCpp = RenderFrame<vkb::BindingType::Cpp>;
}        // namespace rendering

namespace core
{
template <vkb::BindingType bindingType>
class CommandBuffer;
using CommandBufferCpp = CommandBuffer<vkb::BindingType::Cpp>;

template <vkb::BindingType bindingType>
class CommandPool;
using CommandPoolCpp = CommandPool<vkb::BindingType::Cpp>;

template <vkb::BindingType bindingType>
class Device;
using DeviceCpp = Device<vkb::BindingType::Cpp>;

class CommandPoolBase
{
  public:
	CommandPoolBase(vkb::core::DeviceCpp           &device,
	                uint32_t                        queue_family_index,
	                vkb::rendering::RenderFrameCpp *render_frame = nullptr,
	                size_t                          thread_index = 0,
	                vkb::CommandBufferResetMode     reset_mode   = vkb::CommandBufferResetMode::ResetPool);
	CommandPoolBase(CommandPoolBase const &) = delete;
	CommandPoolBase(CommandPoolBase &&other);
	CommandPoolBase &operator=(CommandPoolBase const &) = delete;
	CommandPoolBase &operator=(CommandPoolBase &&other) = delete;
	~CommandPoolBase();

  protected:
	vkb::core::DeviceCpp                        &get_device();
	vk::CommandPool                              get_handle() const;
	uint32_t                                     get_queue_family_index() const;
	vkb::rendering::RenderFrameCpp              *get_render_frame();
	vkb::CommandBufferResetMode                  get_reset_mode() const;
	size_t                                       get_thread_index() const;
	std::shared_ptr<vkb::core::CommandBufferCpp> request_command_buffer(vkb::core::CommandPoolCpp &commandPool, vk::CommandBufferLevel level);
	void                                         reset_pool();

  private:
	vkb::core::DeviceCpp                                     &device;
	vk::CommandPool                                           handle             = nullptr;
	vkb::rendering::RenderFrameCpp                           *render_frame       = nullptr;
	size_t                                                    thread_index       = 0;
	uint32_t                                                  queue_family_index = 0;
	std::vector<std::shared_ptr<vkb::core::CommandBufferCpp>> primary_command_buffers;
	uint32_t                                                  active_primary_command_buffer_count = 0;
	std::vector<std::shared_ptr<vkb::core::CommandBufferCpp>> secondary_command_buffers;
	uint32_t                                                  active_secondary_command_buffer_count = 0;
	vkb::CommandBufferResetMode                               reset_mode                            = vkb::CommandBufferResetMode::ResetPool;
};
}        // namespace core
}        // namespace vkb
