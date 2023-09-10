/* Copyright (c) 2019-2023, Arm Limited and Contributors
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

#include "buffer_pool.h"

#include <cstddef>

#include "common/logging.h"
#include "core/device.h"

namespace vkb
{
BufferBlock::BufferBlock(Device &device, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage) :
    buffer{device, size, usage, memory_usage}
{
	if (usage == VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
	{
		alignment = device.get_gpu().get_properties().limits.minUniformBufferOffsetAlignment;
	}
	else if (usage == VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
	{
		alignment = device.get_gpu().get_properties().limits.minStorageBufferOffsetAlignment;
	}
	else if (usage == VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)
	{
		alignment = device.get_gpu().get_properties().limits.minTexelBufferOffsetAlignment;
	}
	else if (usage == VK_BUFFER_USAGE_INDEX_BUFFER_BIT || usage == VK_BUFFER_USAGE_VERTEX_BUFFER_BIT || usage == VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
	{
		// Used to calculate the offset, required when allocating memory (its value should be power of 2)
		alignment = 16;
	}
	else
	{
		throw std::runtime_error("Usage not recognised");
	}
}

VkDeviceSize BufferBlock::aligned_offset() const
{
	return (offset + alignment - 1) & ~(alignment - 1);
}

bool BufferBlock::can_allocate(VkDeviceSize size) const
{
	assert(size > 0 && "Allocation size must be greater than zero");
	return (aligned_offset() + size <= buffer.get_size());
}

BufferAllocation BufferBlock::allocate(VkDeviceSize size)
{
	if (can_allocate(size))
	{
		// Move the current offset and return an allocation
		auto aligned = aligned_offset();
		offset       = aligned + size;
		return BufferAllocation{buffer, size, aligned};
	}

	// No more space available from the underlying buffer, return empty allocation
	return BufferAllocation{};
}

VkDeviceSize BufferBlock::get_size() const
{
	return buffer.get_size();
}

void BufferBlock::reset()
{
	offset = 0;
}

BufferPool::BufferPool(Device &device, VkDeviceSize block_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage) :
    device{device},
    block_size{block_size},
    usage{usage},
    memory_usage{memory_usage}
{
}

BufferBlock &BufferPool::request_buffer_block(const VkDeviceSize minimum_size, bool minimal)
{
	// Find a block in the range of the blocks which can fit the minimum size
	auto it = minimal ? std::find_if(buffer_blocks.begin(),
	                                 buffer_blocks.end(),
	                                 [&minimum_size](const std::unique_ptr<BufferBlock> &buffer_block) { return (buffer_block->get_size() == minimum_size) && buffer_block->can_allocate(minimum_size); }) :
	                    std::find_if(buffer_blocks.begin(),
	                                 buffer_blocks.end(),
	                                 [&minimum_size](const std::unique_ptr<BufferBlock> &buffer_block) { return buffer_block->can_allocate(minimum_size); });

	if (it == buffer_blocks.end())
	{
		LOGD("Building #{} buffer block ({})", buffer_blocks.size(), usage);

		VkDeviceSize new_block_size = minimal ? minimum_size : std::max(block_size, minimum_size);

		// Create a new block and get the iterator on it
		it = buffer_blocks.emplace(buffer_blocks.end(), std::make_unique<BufferBlock>(device, new_block_size, usage, memory_usage));
	}

	return *it->get();
}

void BufferPool::reset()
{
	for (auto &buffer_block : buffer_blocks)
	{
		buffer_block->reset();
	}
}

BufferAllocation::BufferAllocation(core::Buffer &buffer, VkDeviceSize size, VkDeviceSize offset) :
    buffer{&buffer},
    size{size},
    base_offset{offset}
{
}

void BufferAllocation::update(const std::vector<uint8_t> &data, uint32_t offset)
{
	assert(buffer && "Invalid buffer pointer");

	if (offset + data.size() <= size)
	{
		buffer->update(data, to_u32(base_offset) + offset);
	}
	else
	{
		LOGE("Ignore buffer allocation update");
	}
}

bool BufferAllocation::empty() const
{
	return size == 0 || buffer == nullptr;
}

VkDeviceSize BufferAllocation::get_size() const
{
	return size;
}

VkDeviceSize BufferAllocation::get_offset() const
{
	return base_offset;
}

core::Buffer &BufferAllocation::get_buffer()
{
	assert(buffer && "Invalid buffer pointer");
	return *buffer;
}

}        // namespace vkb
