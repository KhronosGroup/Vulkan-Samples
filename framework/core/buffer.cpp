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

#include "buffer.h"

#include "device.h"

namespace vkb
{
namespace core
{
Buffer::Buffer(Device &device, VkDeviceSize size, VkBufferUsageFlags buffer_usage, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags flags) :
    device{device},
    size{size}
{
	assert(((flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) == 0) && "Buffer memory should be mapped explicitly outside the constructor");

	VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	buffer_info.usage = buffer_usage;
	buffer_info.size  = size;

	VmaAllocationCreateInfo memory_info{};
	memory_info.flags = flags;
	memory_info.usage = memory_usage;

	VmaAllocationInfo allocation_info{};
	auto              result = vmaCreateBuffer(device.get_memory_allocator(),
                                  &buffer_info, &memory_info,
                                  &handle, &allocation,
                                  &allocation_info);

	memory = allocation_info.deviceMemory;

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create Buffer"};
	}
}

Buffer::Buffer(Buffer &&other) :
    device{other.device},
    handle{other.handle},
    allocation{other.allocation},
    memory{other.memory},
    size{other.size},
    mapped_data{other.mapped_data},
    mapped{other.mapped}
{
	// Reset other handles to avoid releasing on destruction
	other.handle      = VK_NULL_HANDLE;
	other.allocation  = VK_NULL_HANDLE;
	other.memory      = VK_NULL_HANDLE;
	other.mapped_data = nullptr;
	other.mapped      = false;
}

Buffer::~Buffer()
{
	if (handle != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE)
	{
		unmap();
		vmaDestroyBuffer(device.get_memory_allocator(), handle, allocation);
	}
}

const Device &Buffer::get_device() const
{
	return device;
}

VkBuffer Buffer::get_handle() const
{
	return handle;
}

const VkBuffer *Buffer::get() const
{
	return &handle;
}

VmaAllocation Buffer::get_allocation() const
{
	return allocation;
}

VkDeviceMemory Buffer::get_memory() const
{
	return memory;
}

VkDeviceSize Buffer::get_size() const
{
	return size;
}

uint8_t *Buffer::map()
{
	if (!mapped && !mapped_data)
	{
		VK_CHECK(vmaMapMemory(device.get_memory_allocator(), allocation, reinterpret_cast<void **>(&mapped_data)));
		mapped = true;
	}
	return mapped_data;
}

void Buffer::unmap()
{
	if (mapped)
	{
		vmaUnmapMemory(device.get_memory_allocator(), allocation);
		mapped_data = nullptr;
		mapped      = false;
	}
}

void Buffer::flush() const
{
	vmaFlushAllocation(device.get_memory_allocator(), allocation, 0, size);
}

void Buffer::update(const std::vector<uint8_t> &data, size_t offset)
{
	update(data.data(), data.size(), offset);
}

void Buffer::update(void *data, size_t size, size_t offset)
{
	update(reinterpret_cast<const uint8_t *>(data), size, offset);
}

void Buffer::update(const uint8_t *data, const size_t size, const size_t offset)
{
	map();
	std::copy(data, data + size, mapped_data + offset);
	flush();
	unmap();        // Workaround for Mac MoltenVK requiring unmapping (https://github.com/KhronosGroup/MoltenVK/issues/175)
}

}        // namespace core
}        // namespace vkb
