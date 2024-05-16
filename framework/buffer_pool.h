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

#pragma once

#include "common/helpers.h"
#include "core/buffer.h"

namespace vkb
{
class Device;

/**
 * @brief An allocation of vulkan memory; different buffer allocations,
 *        with different offset and size, may come from the same Vulkan buffer
 */
class BufferAllocation
{
  public:
	BufferAllocation() = default;

	BufferAllocation(core::Buffer &buffer, VkDeviceSize size, VkDeviceSize offset);

	BufferAllocation(const BufferAllocation &) = delete;

	BufferAllocation(BufferAllocation &&) = default;

	BufferAllocation &operator=(const BufferAllocation &) = delete;

	BufferAllocation &operator=(BufferAllocation &&) = default;

	void update(const std::vector<uint8_t> &data, uint32_t offset = 0);

	template <class T>
	void update(const T &value, uint32_t offset = 0)
	{
		update(to_bytes(value), offset);
	}

	bool empty() const;

	VkDeviceSize get_size() const;

	VkDeviceSize get_offset() const;

	core::Buffer &get_buffer();

  private:
	core::Buffer *buffer{nullptr};

	VkDeviceSize base_offset{0};

	VkDeviceSize size{0};
};

/**
 * @brief Helper class which handles multiple allocation from the same underlying Vulkan buffer.
 */
class BufferBlock
{
  public:
	BufferBlock(Device &device, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage);

	/**
	 * @brief check if this BufferBlock can allocate a given amount of memory
	 * @param size the number of bytes to check
	 * @return \c true if \a size bytes can be allocated from this \c BufferBlock, otherwise \c false.
	 */
	bool can_allocate(VkDeviceSize size) const;

	/**
	 * @return An usable view on a portion of the underlying buffer
	 */
	BufferAllocation allocate(VkDeviceSize size);

	VkDeviceSize get_size() const;

	void reset();

  private:
	/**
	 * @ brief Determine the current aligned offset.
	 * @return The current aligned offset.
	 */
	VkDeviceSize aligned_offset() const;

  private:
	core::Buffer buffer;

	// Memory alignment, it may change according to the usage
	VkDeviceSize alignment{0};

	// Current offset, it increases on every allocation
	VkDeviceSize offset{0};
};

/**
 * @brief A pool of buffer blocks for a specific usage.
 * It may contain inactive blocks that can be recycled.
 *
 * BufferPool is a linear allocator for buffer chunks, it gives you a view of the size you want.
 * A BufferBlock is the corresponding VkBuffer and you can get smaller offsets inside it.
 * Since a shader cannot specify dynamic UBOs, it has to be done from the code
 * (set_resource_dynamic).
 *
 * When a new frame starts, buffer blocks are returned: the offset is reset and contents are
 * overwritten. The minimum allocation size is 256 kb, if you ask for more you get a dedicated
 * buffer allocation.
 *
 * We re-use descriptor sets: we only need one for the corresponding buffer infos (and we only
 * have one VkBuffer per BufferBlock), then it is bound and we use dynamic offsets.
 */
class BufferPool
{
  public:
	BufferPool(Device &device, VkDeviceSize block_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU);

	BufferBlock &request_buffer_block(VkDeviceSize minimum_size, bool minimal = false);

	void reset();

  private:
	Device &device;

	/// List of blocks requested
	std::vector<std::unique_ptr<BufferBlock>> buffer_blocks;

	/// Minimum size of the blocks
	VkDeviceSize block_size{0};

	VkBufferUsageFlags usage{};

	VmaMemoryUsage memory_usage{};
};
}        // namespace vkb
