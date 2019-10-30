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

class CommandPool : public NonCopyable
{
  public:
	CommandPool(Device &device, uint32_t queue_family_index, CommandBuffer::ResetMode reset_mode = CommandBuffer::ResetMode::ResetPool);

	~CommandPool();

	/// @brief Move construct
	CommandPool(CommandPool &&other);

	Device &get_device();

	uint32_t get_queue_family_index() const;

	VkCommandPool get_handle() const;

	VkResult reset_pool();

	CommandBuffer &request_command_buffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	const CommandBuffer::ResetMode get_reset_mode() const;

  private:
	Device &device;

	VkCommandPool handle{VK_NULL_HANDLE};

	uint32_t queue_family_index{0};

	std::vector<std::unique_ptr<CommandBuffer>> command_buffers;

	uint32_t active_command_buffer_count{0};

	CommandBuffer::ResetMode reset_mode{CommandBuffer::ResetMode::ResetPool};

	VkResult reset_command_buffers();
};
}        // namespace vkb
