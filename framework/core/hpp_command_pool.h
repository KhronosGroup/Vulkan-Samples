/* Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
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

#include <core/command_pool.h>

namespace vkb
{
namespace rendering
{
class HPPRenderFrame;
}

namespace core
{
/**
 * @brief facade class around vkb::CommandPool, providing a vulkan.hpp-based interface
 *
 * See vkb::CommandPool for documentation
 */
class HPPCommandPool : private vkb::CommandPool
{
  public:
	HPPCommandPool(vkb::core::HPPDevice &                 device,
	               uint32_t                               queue_family_index,
	               vkb::rendering::HPPRenderFrame *       render_frame = nullptr,
	               size_t                                 thread_index = 0,
	               vkb::core::HPPCommandBuffer::ResetMode reset_mode   = vkb::core::HPPCommandBuffer::ResetMode::ResetPool) :
	    vkb::CommandPool(reinterpret_cast<vkb::Device &>(device),
	                     queue_family_index,
	                     reinterpret_cast<vkb::RenderFrame *>(render_frame),
	                     thread_index,
	                     static_cast<vkb::CommandBuffer::ResetMode>(reset_mode))
	{}

	vk::CommandPool get_handle() const
	{
		return static_cast<vk::CommandPool>(vkb::CommandPool::get_handle());
	}

	vkb::core::HPPCommandBuffer &request_command_buffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary)
	{
		return reinterpret_cast<vkb::core::HPPCommandBuffer &>(vkb::CommandPool::request_command_buffer(static_cast<VkCommandBufferLevel>(level)));
	}
};
}        // namespace core
}        // namespace vkb
