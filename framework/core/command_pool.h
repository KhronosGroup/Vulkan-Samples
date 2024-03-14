/* Copyright (c) 2019, Arm Limited and Contributors
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

#include "common/helpers.h"
#include "common/vk_common.h"
#include "core/command_buffer.h"

namespace vkb
{
class Device;
class RenderFrame;

class CommandPool
{
  public:
	CommandPool(vkb::Device                                             &device,
	            uint32_t                                                 queue_family_index,
	            vkb::RenderFrame                                        *render_frame = nullptr,
	            size_t                                                   thread_index = 0,
	            vkb::core::CommandBuffer<vkb::BindingType::C>::ResetMode reset_mode   = vkb::core::CommandBuffer<vkb::BindingType::C>::ResetMode::ResetPool);

	CommandPool(const CommandPool &) = delete;

	CommandPool(CommandPool &&other);

	~CommandPool();

	CommandPool &operator=(const CommandPool &) = delete;

	CommandPool &operator=(CommandPool &&) = delete;

	Device &get_device();

	uint32_t get_queue_family_index() const;

	VkCommandPool get_handle() const;

	RenderFrame *get_render_frame();

	size_t get_thread_index() const;

	VkResult reset_pool();

	vkb::core::CommandBuffer<vkb::BindingType::C> &request_command_buffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	const vkb::core::CommandBuffer<vkb::BindingType::C>::ResetMode get_reset_mode() const;

  private:
	Device &device;

	VkCommandPool handle{VK_NULL_HANDLE};

	RenderFrame *render_frame{nullptr};

	size_t thread_index{0};

	uint32_t queue_family_index{0};

	std::vector<std::unique_ptr<vkb::core::CommandBuffer<vkb::BindingType::C>>> primary_command_buffers;

	uint32_t active_primary_command_buffer_count{0};

	std::vector<std::unique_ptr<vkb::core::CommandBuffer<vkb::BindingType::C>>> secondary_command_buffers;

	uint32_t active_secondary_command_buffer_count{0};

	vkb::core::CommandBuffer<vkb::BindingType::C>::ResetMode reset_mode{vkb::core::CommandBuffer<vkb::BindingType::C>::ResetMode::ResetPool};

	VkResult reset_command_buffers();
};
}        // namespace vkb
