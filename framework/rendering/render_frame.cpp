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

#include "render_frame.h"

#include "common/logging.h"

namespace vkb
{
RenderFrame::RenderFrame(Device &device, RenderTarget &&render_target, uint16_t command_pool_count) :
    device{device},
    fence_pool{device},
    semaphore_pool{device},
    swapchain_render_target{std::move(render_target)},
    command_pool_count{command_pool_count}
{
	buffer_pools.emplace(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, std::make_pair(BufferPool{device, BUFFER_POOL_BLOCK_SIZE * 1024, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT}, nullptr));
	buffer_pools.emplace(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, std::make_pair(BufferPool{device, BUFFER_POOL_BLOCK_SIZE * 1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT}, nullptr));
	buffer_pools.emplace(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, std::make_pair(BufferPool{device, BUFFER_POOL_BLOCK_SIZE * 1024, VK_BUFFER_USAGE_INDEX_BUFFER_BIT}, nullptr));
}

void RenderFrame::update_render_target(RenderTarget &&render_target)
{
	swapchain_render_target = std::move(render_target);
}

void RenderFrame::reset()
{
	VK_CHECK(fence_pool.wait());

	fence_pool.reset();

	for (auto &command_pools_per_queue : command_pools)
	{
		for (auto &command_pool : command_pools_per_queue.second)
		{
			command_pool->reset_pool();
		}
	}

	for (auto &buffer_pool_it : buffer_pools)
	{
		auto &buffer_pool  = buffer_pool_it.second.first;
		auto *buffer_block = buffer_pool_it.second.second;

		buffer_pool.reset();

		buffer_block = nullptr;
	}

	semaphore_pool.reset();
}

std::vector<std::unique_ptr<CommandPool>> &RenderFrame::get_command_pools(const Queue &queue, CommandBuffer::ResetMode reset_mode)
{
	auto command_pool_it = command_pools.find(queue.get_family_index());

	if (command_pool_it != command_pools.end())
	{
		if (command_pool_it->second.at(0)->get_reset_mode() != reset_mode)
		{
			device.wait_idle();

			// Delete pools
			command_pools.erase(command_pool_it);
		}
		else
		{
			return command_pool_it->second;
		}
	}

	std::vector<std::unique_ptr<CommandPool>> queue_command_pools;
	for (int i = 0; i < command_pool_count; i++)
	{
		queue_command_pools.push_back(std::make_unique<CommandPool>(device, queue.get_family_index(), reset_mode));
	}

	auto res_ins_it = command_pools.emplace(queue.get_family_index(), std::move(queue_command_pools));

	if (!res_ins_it.second)
	{
		throw std::runtime_error("Failed to insert command pool");
	}

	command_pool_it = res_ins_it.first;

	return command_pool_it->second;
}

FencePool &RenderFrame::get_fence_pool()
{
	return fence_pool;
}

SemaphorePool &RenderFrame::get_semaphore_pool()
{
	return semaphore_pool;
}

RenderTarget &RenderFrame::get_render_target()
{
	return swapchain_render_target;
}

BufferAllocation RenderFrame::allocate_buffer(const VkBufferUsageFlags usage, const VkDeviceSize size)
{
	// Find a pool for this usage
	auto buffer_pool_it = buffer_pools.find(usage);
	if (buffer_pool_it == buffer_pools.end())
	{
		LOGE("No buffer pool for buffer usage {}", usage);
		return BufferAllocation{};
	}

	auto &buffer_pool  = buffer_pool_it->second.first;
	auto &buffer_block = buffer_pool_it->second.second;

	if (!buffer_block)
	{
		// If there is no block associated with the pool
		// Request one with that size
		buffer_block = &buffer_pool.request_buffer_block(to_u32(size));
	}

	auto data = buffer_block->allocate(to_u32(size));

	// Check if the buffer block can allocate the requested size
	if (data.empty())
	{
		buffer_block = &buffer_pool.request_buffer_block(to_u32(size));

		data = buffer_block->allocate(to_u32(size));
	}

	return data;
}
}        // namespace vkb
