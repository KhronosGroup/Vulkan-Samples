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

#include <core/hpp_command_pool.h>
#include <core/hpp_device.h>

namespace vkb
{
namespace core
{
HPPCommandPool::HPPCommandPool(HPPDevice const &                     d,
                               uint32_t                              queue_family_index,
                               vkb::rendering::HPPRenderFrame const *render_frame,
                               size_t                                thread_index,
                               HPPCommandBuffer::ResetMode           reset_mode) :
    device{d}, render_frame{render_frame}, thread_index{thread_index}, reset_mode{reset_mode}
{
	vk::CommandPoolCreateFlags flags;
	switch (reset_mode)
	{
		case HPPCommandBuffer::ResetMode::ResetIndividually:
		case HPPCommandBuffer::ResetMode::AlwaysAllocate:
			flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
			break;
		case HPPCommandBuffer::ResetMode::ResetPool:
		default:
			flags = vk::CommandPoolCreateFlagBits::eTransient;
			break;
	}

	vk::CommandPoolCreateInfo command_pool_create_info(flags, queue_family_index);

	handle = device.get_handle().createCommandPool(command_pool_create_info);
}

HPPCommandPool::HPPCommandPool(HPPCommandPool &&other) :
    device(other.device),
    handle(std::exchange(other.handle, {})),
    queue_family_index(std::exchange(other.queue_family_index, {})),
    primary_command_buffers{std::move(other.primary_command_buffers)},
    active_primary_command_buffer_count(std::exchange(other.active_primary_command_buffer_count, {})),
    secondary_command_buffers{std::move(other.secondary_command_buffers)},
    active_secondary_command_buffer_count(std::exchange(other.active_secondary_command_buffer_count, {})),
    render_frame(std::exchange(other.render_frame, {})),
    thread_index(std::exchange(other.thread_index, {})),
    reset_mode(std::exchange(other.reset_mode, {}))
{
}

HPPCommandPool::~HPPCommandPool()
{
	// clear command buffers before destroying the command pool
	primary_command_buffers.clear();
	secondary_command_buffers.clear();

	// Destroy command pool
	if (handle)
	{
		device.get_handle().destroyCommandPool(handle);
	}
}

HPPDevice const &HPPCommandPool::get_device() const
{
	return device;
}

vk::CommandPool HPPCommandPool::get_handle() const
{
	return handle;
}

uint32_t HPPCommandPool::get_queue_family_index() const
{
	return queue_family_index;
}

vkb::rendering::HPPRenderFrame const *HPPCommandPool::get_render_frame() const
{
	return render_frame;
}

size_t HPPCommandPool::get_thread_index() const
{
	return thread_index;
}

void HPPCommandPool::reset_pool()
{
	switch (reset_mode)
	{
		case HPPCommandBuffer::ResetMode::ResetIndividually:
			reset_command_buffers();
			break;

		case HPPCommandBuffer::ResetMode::ResetPool:
			device.get_handle().resetCommandPool(handle);
			reset_command_buffers();
			break;

		case HPPCommandBuffer::ResetMode::AlwaysAllocate:
			primary_command_buffers.clear();
			active_primary_command_buffer_count = 0;
			secondary_command_buffers.clear();
			active_secondary_command_buffer_count = 0;
			break;

		default:
			throw std::runtime_error("Unknown reset mode for command pools");
	}
}

HPPCommandBuffer const &HPPCommandPool::request_command_buffer(vk::CommandBufferLevel level)
{
	if (level == vk::CommandBufferLevel::ePrimary)
	{
		if (active_primary_command_buffer_count < primary_command_buffers.size())
		{
			return *primary_command_buffers[active_primary_command_buffer_count++];
		}

		primary_command_buffers.emplace_back(std::make_unique<HPPCommandBuffer>(*this, level));

		active_primary_command_buffer_count++;

		return *primary_command_buffers.back();
	}
	else
	{
		if (active_secondary_command_buffer_count < secondary_command_buffers.size())
		{
			return *secondary_command_buffers[active_secondary_command_buffer_count++];
		}

		secondary_command_buffers.emplace_back(std::make_unique<HPPCommandBuffer>(*this, level));

		active_secondary_command_buffer_count++;

		return *secondary_command_buffers.back();
	}
}

HPPCommandBuffer::ResetMode HPPCommandPool::get_reset_mode() const
{
	return reset_mode;
}

void HPPCommandPool::reset_command_buffers()
{
	for (auto &cmd_buf : primary_command_buffers)
	{
		cmd_buf->reset(reset_mode);
	}
	active_primary_command_buffer_count = 0;

	for (auto &cmd_buf : secondary_command_buffers)
	{
		cmd_buf->reset(reset_mode);
	}
	active_secondary_command_buffer_count = 0;
}
}        // namespace core
}        // namespace vkb
