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

#include "core/command_pool_base.h"
#include "core/command_buffer.h"

namespace vkb
{
namespace core
{
vkb::core::CommandPoolBase::CommandPoolBase(vkb::core::DeviceCpp           &device_,
                                            uint32_t                        queue_family_index,
                                            vkb::rendering::RenderFrameCpp *render_frame_,
                                            size_t                          thread_index,
                                            vkb::CommandBufferResetMode     reset_mode) :
    device{device_}, render_frame{render_frame_}, thread_index{thread_index}, reset_mode{reset_mode}
{
	vk::CommandPoolCreateFlags flags;
	switch (reset_mode)
	{
		case vkb::CommandBufferResetMode::ResetIndividually:
		case vkb::CommandBufferResetMode::AlwaysAllocate:
			flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
			break;
		case vkb::CommandBufferResetMode::ResetPool:
		default:
			flags = vk::CommandPoolCreateFlagBits::eTransient;
			break;
	}

	vk::CommandPoolCreateInfo command_pool_create_info{.flags = flags, .queueFamilyIndex = queue_family_index};

	handle = device.get_handle().createCommandPool(command_pool_create_info);
}

CommandPoolBase::CommandPoolBase(CommandPoolBase &&other) :
    device{other.device},
    handle{other.handle},
    render_frame{other.render_frame},
    thread_index{other.thread_index},
    queue_family_index{other.queue_family_index},
    primary_command_buffers{std::move(other.primary_command_buffers)},
    active_primary_command_buffer_count{other.active_primary_command_buffer_count},
    secondary_command_buffers{std::move(other.secondary_command_buffers)},
    active_secondary_command_buffer_count{other.active_secondary_command_buffer_count},
    reset_mode{other.reset_mode}
{
	other.handle = nullptr;
}

CommandPoolBase::~CommandPoolBase()
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

vkb::core::DeviceCpp &CommandPoolBase::get_device()
{
	return device;
}

vk::CommandPool CommandPoolBase::get_handle() const
{
	return handle;
}

uint32_t CommandPoolBase::get_queue_family_index() const
{
	return queue_family_index;
}

vkb::rendering::RenderFrameCpp *CommandPoolBase::get_render_frame()
{
	return render_frame;
}

vkb::CommandBufferResetMode CommandPoolBase::get_reset_mode() const
{
	return reset_mode;
}

size_t CommandPoolBase::get_thread_index() const
{
	return thread_index;
}

std::shared_ptr<vkb::core::CommandBufferCpp> CommandPoolBase::request_command_buffer(vkb::core::CommandPoolCpp &commandPool, vk::CommandBufferLevel level)
{
	if (static_cast<vk::CommandBufferLevel>(level) == vk::CommandBufferLevel::ePrimary)
	{
		if (active_primary_command_buffer_count < primary_command_buffers.size())
		{
			return primary_command_buffers[active_primary_command_buffer_count++];
		}

		primary_command_buffers.emplace_back(std::make_shared<vkb::core::CommandBufferCpp>(commandPool, level));

		active_primary_command_buffer_count++;

		return primary_command_buffers.back();
	}
	else
	{
		if (active_secondary_command_buffer_count < secondary_command_buffers.size())
		{
			return secondary_command_buffers[active_secondary_command_buffer_count++];
		}

		secondary_command_buffers.emplace_back(std::make_shared<vkb::core::CommandBufferCpp>(commandPool, level));

		active_secondary_command_buffer_count++;

		return secondary_command_buffers.back();
	}
}

void CommandPoolBase::reset_pool()
{
	switch (reset_mode)
	{
		case vkb::CommandBufferResetMode::ResetIndividually:
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
			break;

		case vkb::CommandBufferResetMode::ResetPool:
			device.get_handle().resetCommandPool(handle);
			active_primary_command_buffer_count   = 0;
			active_secondary_command_buffer_count = 0;
			break;

		case vkb::CommandBufferResetMode::AlwaysAllocate:
			primary_command_buffers.clear();
			active_primary_command_buffer_count = 0;
			secondary_command_buffers.clear();
			active_secondary_command_buffer_count = 0;
			break;

		default:
			throw std::runtime_error("Unknown reset mode for command pools");
	}
}

}        // namespace core
}        // namespace vkb