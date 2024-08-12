/* Copyright (c) 2019-2024, Arm Limited and Contributors
 * Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.
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

#include "core/buffer.h"
#include "core/device.h"
#include "core/hpp_buffer.h"
#include "core/hpp_device.h"

namespace vkb
{
/**
 * @brief An allocation of vulkan memory; different buffer allocations,
 *        with different offset and size, may come from the same Vulkan buffer
 */
template <vkb::BindingType bindingType>
class BufferAllocation
{
  public:
	using DeviceSizeType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::DeviceSize, VkDeviceSize>::type;

	using BufferType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPBuffer, vkb::core::Buffer>::type;

  public:
	BufferAllocation()                                    = default;
	BufferAllocation(const BufferAllocation &)            = delete;
	BufferAllocation(BufferAllocation &&)                 = default;
	BufferAllocation &operator=(const BufferAllocation &) = delete;
	BufferAllocation &operator=(BufferAllocation &&)      = default;

	BufferAllocation(BufferType &buffer, DeviceSizeType size, DeviceSizeType offset);

	bool           empty() const;
	BufferType    &get_buffer();
	DeviceSizeType get_offset() const;
	DeviceSizeType get_size() const;
	void           update(const std::vector<uint8_t> &data, uint32_t offset = 0);
	template <typename T>
	void update(const T &value, uint32_t offset = 0);

  private:
	vkb::core::HPPBuffer *buffer = nullptr;
	vk::DeviceSize        offset = 0;
	vk::DeviceSize        size   = 0;
};

using BufferAllocationC   = BufferAllocation<vkb::BindingType::C>;
using BufferAllocationCpp = BufferAllocation<vkb::BindingType::Cpp>;

template <>
inline BufferAllocation<vkb::BindingType::Cpp>::BufferAllocation(vkb::core::HPPBuffer &buffer, vk::DeviceSize size, vk::DeviceSize offset) :
    buffer(&buffer),
    offset(offset),
    size(size)
{}

template <>
inline BufferAllocation<vkb::BindingType::C>::BufferAllocation(vkb::core::Buffer &buffer, VkDeviceSize size, VkDeviceSize offset) :
    buffer(reinterpret_cast<vkb::core::HPPBuffer *>(&buffer)),
    offset(static_cast<vk::DeviceSize>(offset)),
    size(static_cast<vk::DeviceSize>(size))
{}

template <vkb::BindingType bindingType>
bool BufferAllocation<bindingType>::empty() const
{
	return size == 0 || buffer == nullptr;
}

template <vkb::BindingType bindingType>
typename BufferAllocation<bindingType>::BufferType &BufferAllocation<bindingType>::get_buffer()
{
	assert(buffer && "Invalid buffer pointer");
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return *buffer;
	}
	else
	{
		return reinterpret_cast<vkb::core::Buffer &>(*buffer);
	}
}

template <vkb::BindingType bindingType>
typename BufferAllocation<bindingType>::DeviceSizeType BufferAllocation<bindingType>::get_offset() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return offset;
	}
	else
	{
		return static_cast<VkDeviceSize>(offset);
	}
}

template <vkb::BindingType bindingType>
typename BufferAllocation<bindingType>::DeviceSizeType BufferAllocation<bindingType>::get_size() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return size;
	}
	else
	{
		return static_cast<VkDeviceSize>(size);
	}
}

template <vkb::BindingType bindingType>
void BufferAllocation<bindingType>::update(const std::vector<uint8_t> &data, uint32_t offset)
{
	assert(buffer && "Invalid buffer pointer");

	if (offset + data.size() <= size)
	{
		buffer->update(data, to_u32(this->offset) + offset);
	}
	else
	{
		LOGE("Ignore buffer allocation update");
	}
}

template <vkb::BindingType bindingType>
template <typename T>
void BufferAllocation<bindingType>::update(const T &value, uint32_t offset)
{
	update(to_bytes(value), offset);
}

/**
 * @brief Helper class which handles multiple allocation from the same underlying Vulkan buffer.
 */
template <vkb::BindingType bindingType>
class BufferBlock
{
  public:
	using BufferUsageFlagsType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::BufferUsageFlags, VkBufferUsageFlags>::type;
	using DeviceSizeType       = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::DeviceSize, VkDeviceSize>::type;

	using DeviceType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPDevice, vkb::Device>::type;

  public:
	BufferBlock()                                  = delete;
	BufferBlock(BufferBlock const &rhs)            = delete;
	BufferBlock(BufferBlock &&rhs)                 = default;
	BufferBlock &operator=(BufferBlock const &rhs) = delete;
	BufferBlock &operator=(BufferBlock &&rhs)      = default;

	BufferBlock(DeviceType &device, DeviceSizeType size, BufferUsageFlagsType usage, VmaMemoryUsage memory_usage);

	/**
	 * @return An usable view on a portion of the underlying buffer
	 */
	BufferAllocation<bindingType> allocate(DeviceSizeType size);

	/**
	 * @brief check if this BufferBlock can allocate a given amount of memory
	 * @param size the number of bytes to check
	 * @return \c true if \a size bytes can be allocated from this \c BufferBlock, otherwise \c false.
	 */
	bool can_allocate(DeviceSizeType size) const;

	DeviceSizeType get_size() const;
	void           reset();

  private:
	/**
	 * @ brief Determine the current aligned offset.
	 * @return The current aligned offset.
	 */
	vk::DeviceSize aligned_offset() const;
	vk::DeviceSize determine_alignment(vk::BufferUsageFlags usage, vk::PhysicalDeviceLimits const &limits) const;

  private:
	vkb::core::HPPBuffer buffer;
	vk::DeviceSize       alignment = 0;        // Memory alignment, it may change according to the usage
	vk::DeviceSize       offset    = 0;        // Current offset, it increases on every allocation
};

using BufferBlockC   = BufferBlock<vkb::BindingType::C>;
using BufferBlockCpp = BufferBlock<vkb::BindingType::Cpp>;

template <vkb::BindingType bindingType>
BufferBlock<bindingType>::BufferBlock(DeviceType &device, DeviceSizeType size, BufferUsageFlagsType usage, VmaMemoryUsage memory_usage) :
    buffer{device, size, usage, memory_usage}
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		alignment = determine_alignment(usage, device.get_gpu().get_properties().limits);
	}
	else
	{
		alignment =
		    determine_alignment(static_cast<vk::BufferUsageFlags>(usage), static_cast<vk::PhysicalDeviceLimits>(device.get_gpu().get_properties().limits));
	}
}

template <vkb::BindingType bindingType>
BufferAllocation<bindingType> BufferBlock<bindingType>::allocate(DeviceSizeType size)
{
	if (can_allocate(size))
	{
		// Move the current offset and return an allocation
		auto aligned = aligned_offset();
		offset       = aligned + size;
		if constexpr (bindingType == vkb::BindingType::Cpp)
		{
			return BufferAllocationCpp{buffer, size, aligned};
		}
		else
		{
			return BufferAllocationC{reinterpret_cast<vkb::core::Buffer &>(buffer), static_cast<VkDeviceSize>(size), static_cast<VkDeviceSize>(aligned)};
		}
	}

	// No more space available from the underlying buffer, return empty allocation
	return BufferAllocation<bindingType>{};
}

template <vkb::BindingType bindingType>
bool BufferBlock<bindingType>::can_allocate(DeviceSizeType size) const
{
	assert(size > 0 && "Allocation size must be greater than zero");
	return (aligned_offset() + size <= buffer.get_size());
}

template <vkb::BindingType bindingType>
typename BufferBlock<bindingType>::DeviceSizeType BufferBlock<bindingType>::get_size() const
{
	return buffer.get_size();
}

template <vkb::BindingType bindingType>
void BufferBlock<bindingType>::reset()
{
	offset = 0;
}

template <vkb::BindingType bindingType>
vk::DeviceSize BufferBlock<bindingType>::aligned_offset() const
{
	return (offset + alignment - 1) & ~(alignment - 1);
}

template <vkb::BindingType bindingType>
vk::DeviceSize BufferBlock<bindingType>::determine_alignment(vk::BufferUsageFlags usage, vk::PhysicalDeviceLimits const &limits) const
{
	if (usage == vk::BufferUsageFlagBits::eUniformBuffer)
	{
		return limits.minUniformBufferOffsetAlignment;
	}
	else if (usage == vk::BufferUsageFlagBits::eStorageBuffer)
	{
		return limits.minStorageBufferOffsetAlignment;
	}
	else if (usage == vk::BufferUsageFlagBits::eUniformTexelBuffer)
	{
		return limits.minTexelBufferOffsetAlignment;
	}
	else if (usage == vk::BufferUsageFlagBits::eIndexBuffer || usage == vk::BufferUsageFlagBits::eVertexBuffer ||
	         usage == vk::BufferUsageFlagBits::eIndirectBuffer)
	{
		// Used to calculate the offset, required when allocating memory (its value should be power of 2)
		return 16;
	}
	else
	{
		throw std::runtime_error("Usage not recognised");
	}
}

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
template <vkb::BindingType bindingType>
class BufferPool
{
  public:
	using BufferUsageFlagsType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::BufferUsageFlags, VkBufferUsageFlags>::type;
	using DeviceSizeType       = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::DeviceSize, VkDeviceSize>::type;

	using DeviceType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPDevice, vkb::Device>::type;

  public:
	BufferPool(DeviceType &device, DeviceSizeType block_size, BufferUsageFlagsType usage, VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU);

	BufferBlock<bindingType> &request_buffer_block(DeviceSizeType minimum_size, bool minimal = false);

	void reset();

  private:
	vkb::core::HPPDevice                        &device;
	std::vector<std::unique_ptr<BufferBlockCpp>> buffer_blocks;         /// List of blocks requested (need to be pointers in order to keep their address constant on vector resizing)
	vk::DeviceSize                               block_size = 0;        /// Minimum size of the blocks
	vk::BufferUsageFlags                         usage;
	VmaMemoryUsage                               memory_usage{};
};

using BufferPoolC   = BufferPool<vkb::BindingType::C>;
using BufferPoolCpp = BufferPool<vkb::BindingType::Cpp>;

template <vkb::BindingType bindingType>
BufferPool<bindingType>::BufferPool(DeviceType &device, DeviceSizeType block_size, BufferUsageFlagsType usage, VmaMemoryUsage memory_usage) :
    device{reinterpret_cast<vkb::core::HPPDevice &>(device)}, block_size{block_size}, usage{usage}, memory_usage{memory_usage}
{
}

template <vkb::BindingType bindingType>
BufferBlock<bindingType> &BufferPool<bindingType>::request_buffer_block(DeviceSizeType minimum_size, bool minimal)
{
	// Find a block in the range of the blocks which can fit the minimum size
	auto it = minimal ? std::find_if(buffer_blocks.begin(),
	                                 buffer_blocks.end(),
	                                 [&minimum_size](auto const &buffer_block) { return (buffer_block->get_size() == minimum_size) && buffer_block->can_allocate(minimum_size); }) :
	                    std::find_if(buffer_blocks.begin(),
	                                 buffer_blocks.end(),
	                                 [&minimum_size](auto const &buffer_block) { return buffer_block->can_allocate(minimum_size); });

	if (it == buffer_blocks.end())
	{
		LOGD("Building #{} buffer block ({})", buffer_blocks.size(), vk::to_string(usage));

		vk::DeviceSize new_block_size = minimal ? minimum_size : std::max(block_size, minimum_size);

		// Create a new block and get the iterator on it
		it = buffer_blocks.emplace(buffer_blocks.end(), std::make_unique<BufferBlockCpp>(device, new_block_size, usage, memory_usage));
	}

	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return *it->get();
	}
	else
	{
		return reinterpret_cast<BufferBlockC &>(*it->get());
	}
}

template <vkb::BindingType bindingType>
void BufferPool<bindingType>::reset()
{
	// Attention: Resetting the BufferPool is not supposed to clear the BufferBlocks, but just reset them!
	//						The actual VkBuffers are used to hash the DescriptorSet in RenderFrame::request_descriptor_set.
	//						Don't know (for now) how that works with resetted buffers!
	for (auto &buffer_block : buffer_blocks)
	{
		buffer_block->reset();
	}
}

}        // namespace vkb
