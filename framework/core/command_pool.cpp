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

#include "command_pool.h"

#include "device.h"

namespace vkb
{
CommandPool::CommandPool(Device &d, uint32_t queue_family_index, CommandBuffer::ResetMode reset_mode) :
    device{d},
    reset_mode{reset_mode}
{
	VkCommandPoolCreateFlags flags;
	switch (reset_mode)
	{
		case CommandBuffer::ResetMode::ResetIndividually:
		case CommandBuffer::ResetMode::AlwaysAllocate:
			flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			break;
		case CommandBuffer::ResetMode::ResetPool:
		default:
			flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			break;
	}

	VkCommandPoolCreateInfo create_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};

	create_info.queueFamilyIndex = queue_family_index;
	create_info.flags            = flags;

	auto result = vkCreateCommandPool(device.get_handle(), &create_info, nullptr, &handle);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Failed to create command pool"};
	}
}

CommandPool::~CommandPool()
{
	command_buffers.clear();

	// Destroy command pool
	if (handle != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(device.get_handle(), handle, nullptr);
	}
}

CommandPool::CommandPool(CommandPool &&other) :
    device{other.device},
    handle{other.handle},
    queue_family_index{other.queue_family_index},
    command_buffers{std::move(other.command_buffers)},
    active_command_buffer_count{other.active_command_buffer_count},
    reset_mode{other.reset_mode}
{
	other.handle = VK_NULL_HANDLE;

	other.queue_family_index = 0;

	other.active_command_buffer_count = 0;
}

Device &CommandPool::get_device()
{
	return device;
}

uint32_t CommandPool::get_queue_family_index() const
{
	return queue_family_index;
}

VkCommandPool CommandPool::get_handle() const
{
	return handle;
}

VkResult CommandPool::reset_pool()
{
	VkResult result = VK_SUCCESS;

	switch (reset_mode)
	{
		case CommandBuffer::ResetMode::ResetIndividually:
		{
			result = reset_command_buffers();

			break;
		}
		case CommandBuffer::ResetMode::ResetPool:
		{
			result = vkResetCommandPool(device.get_handle(), handle, 0);

			if (result != VK_SUCCESS)
			{
				return result;
			}

			result = reset_command_buffers();

			break;
		}
		case CommandBuffer::ResetMode::AlwaysAllocate:
		{
			command_buffers.clear();

			active_command_buffer_count = 0;

			break;
		}
		default:
			throw std::runtime_error("Unknown reset mode for command pools");
	}

	return result;
}

VkResult CommandPool::reset_command_buffers()
{
	VkResult result = VK_SUCCESS;

	for (auto &cmd_buf : command_buffers)
	{
		result = cmd_buf->reset(reset_mode);

		if (result != VK_SUCCESS)
		{
			return result;
		}
	}

	active_command_buffer_count = 0;

	return result;
}

CommandBuffer &CommandPool::request_command_buffer(VkCommandBufferLevel level)
{
	if (active_command_buffer_count < command_buffers.size())
	{
		return *command_buffers.at(active_command_buffer_count++);
	}

	command_buffers.emplace_back(std::make_unique<CommandBuffer>(*this, level));

	active_command_buffer_count++;

	return *command_buffers.back();
}

CommandBuffer::ResetMode const CommandPool::get_reset_mode() const
{
	return reset_mode;
}
}        // namespace vkb
