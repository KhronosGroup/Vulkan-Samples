/* Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
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

#include <core/hpp_buffer.h>

namespace vkb
{
namespace core
{
HPPBuffer::HPPBuffer(vkb::core::HPPDevice &   device,
                     vk::DeviceSize           size,
                     vk::BufferUsageFlags     buffer_usage,
                     VmaMemoryUsage           memory_usage,
                     VmaAllocationCreateFlags flags) :
    buffer_create_info({}, size, buffer_usage), vmaAllocator(device.get_memory_allocator())
{
#ifdef VK_USE_PLATFORM_METAL_EXT
	// Workaround for Mac (MoltenVK requires unmapping https://github.com/KhronosGroup/MoltenVK/issues/175)
	// Force cleares the flag VMA_ALLOCATION_CREATE_MAPPED_BIT
	flags &= ~VMA_ALLOCATION_CREATE_MAPPED_BIT;
#endif

	persistent = (flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) != 0;

	VmaAllocationCreateInfo memory_info{};
	memory_info.flags = flags;
	memory_info.usage = memory_usage;

	VmaAllocationInfo allocation_info{};

	VK_CHECK(vmaCreateBuffer(vmaAllocator,
	                         reinterpret_cast<VkBufferCreateInfo *>(&buffer_create_info),
	                         &memory_info,
	                         reinterpret_cast<VkBuffer *>(&handle),
	                         &vmaAllocation,
	                         &allocation_info));

	if (persistent)
	{
		mapped_data = static_cast<uint8_t *>(allocation_info.pMappedData);
	}
}

HPPBuffer::HPPBuffer(HPPBuffer &&rhs) :
    handle(std::exchange(rhs.handle, nullptr)),
    buffer_create_info(std::exchange(rhs.buffer_create_info, {})),
    mapped_data(std::exchange(rhs.mapped_data, nullptr)),
    persistent(std::exchange(rhs.persistent, {})),
    vmaAllocation(std::exchange(rhs.vmaAllocation, {})),
    vmaAllocator(std::exchange(rhs.vmaAllocator, {}))
{}

HPPBuffer::~HPPBuffer()
{
	destroy();
}

HPPBuffer &HPPBuffer::operator=(HPPBuffer &&rhs)
{
	if (&rhs != this)
	{
		destroy();

		handle             = std::exchange(rhs.handle, nullptr);
		buffer_create_info = std::exchange(rhs.buffer_create_info, {});
		mapped_data        = std::exchange(rhs.mapped_data, nullptr);
		persistent         = std::exchange(rhs.persistent, false);
		vmaAllocation      = std::exchange(rhs.vmaAllocation, {});
		vmaAllocator       = std::exchange(rhs.vmaAllocator, {});
	}
	return *this;
}

void HPPBuffer::flush() const
{
	vmaFlushAllocation(vmaAllocator, vmaAllocation, 0, buffer_create_info.size);
}

vk::Buffer HPPBuffer::get_handle() const
{
	return handle;
}

vk::DeviceSize HPPBuffer::get_size() const
{
	return buffer_create_info.size;
}

uint8_t *HPPBuffer::map()
{
	if (!mapped_data)
	{
		assert(!persistent);
		VK_CHECK(vmaMapMemory(vmaAllocator, vmaAllocation, reinterpret_cast<void **>(&mapped_data)));
	}
	return mapped_data;
}

void HPPBuffer::unmap()
{
	if (mapped_data && !persistent)
	{
		vmaUnmapMemory(vmaAllocator, vmaAllocation);
		mapped_data = nullptr;
	}
}

void HPPBuffer::update(uint8_t const *data, size_t size, size_t offset)
{
	if (persistent)
	{
		std::copy(data, data + size, mapped_data + offset);
		flush();
	}
	else
	{
		map();
		std::copy(data, data + size, mapped_data + offset);
		flush();
		unmap();
	}
}

void HPPBuffer::update(void *data, size_t size, size_t offset)
{
	update(reinterpret_cast<const uint8_t *>(data), size, offset);
}

void HPPBuffer::destroy()
{
	if (handle)
	{
		assert(vmaAllocation != VK_NULL_HANDLE);
		unmap();
		vmaDestroyBuffer(vmaAllocator, handle, vmaAllocation);
	}
}

}        // namespace core
}        // namespace vkb
