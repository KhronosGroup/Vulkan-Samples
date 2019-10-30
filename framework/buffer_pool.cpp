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

#include "buffer_pool.h"

#include <cstddef>

#include "common/error.h"
#include "common/logging.h"
#include "core/device.h"

namespace vkb
{
BufferBlock::BufferBlock(Device &device, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage) :
    buffer{device, size, usage, memory_usage}
{
	if (usage == VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
	{
		alignment = device.get_properties().limits.minUniformBufferOffsetAlignment;
	}
	else if (usage == VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
	{
		alignment = device.get_properties().limits.minStorageBufferOffsetAlignment;
	}
	else if (usage == VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)
	{
		alignment = device.get_properties().limits.minTexelBufferOffsetAlignment;
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

BufferAllocation BufferBlock::allocate(const uint32_t allocate_size)
{
	assert(allocate_size > 0 && "Allocation size must be greater than zero");

	auto aligned_offset = (offset + alignment - 1) & ~(alignment - 1);

	if (aligned_offset + allocate_size > buffer.get_size())
	{
		// No more space available from the underlying buffer, return empty allocation
		return BufferAllocation{};
	}

	// Move the current offset and return an allocation
	offset = aligned_offset + allocate_size;
	return BufferAllocation{buffer, allocate_size, aligned_offset};
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

BufferBlock &BufferPool::request_buffer_block(const VkDeviceSize minimum_size)
{
	// Find the first block in the range of the inactive blocks
	// which size is greater than the minimum size
	auto it = std::upper_bound(buffer_blocks.begin() + active_buffer_block_count, buffer_blocks.end(), minimum_size,
	                           [](const VkDeviceSize &a, const BufferBlock &b) -> bool { return a < b.get_size(); });

	if (it != buffer_blocks.end())
	{
		// Recycle inactive block
		active_buffer_block_count++;
		return *it;
	}

	LOGI("Building #{} buffer block ({})", buffer_blocks.size(), usage);

	// Create a new block, store and return it
	buffer_blocks.emplace_back(device, std::max(block_size, minimum_size), usage, memory_usage);

	auto &block = buffer_blocks[active_buffer_block_count++];

	return block;
}

void BufferPool::reset()
{
	for (auto &buffer_block : buffer_blocks)
	{
		buffer_block.reset();
	}

	active_buffer_block_count = 0;
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
		buffer->update(data, static_cast<size_t>(base_offset) + offset);
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
