/* Copyright (c) 2019-2021, Arm Limited and Contributors
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
#ifdef VK_USE_PLATFORM_METAL_EXT
	// Workaround for Mac (MoltenVK requires unmapping https://github.com/KhronosGroup/MoltenVK/issues/175)
	// Force cleares the flag VMA_ALLOCATION_CREATE_MAPPED_BIT
	flags &= ~VMA_ALLOCATION_CREATE_MAPPED_BIT;
#endif

	persistent = (flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) != 0;

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

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create Buffer"};
	}

	memory = allocation_info.deviceMemory;

	if (persistent)
	{
		mapped_data = static_cast<uint8_t *>(allocation_info.pMappedData);
	}
}

Buffer::Buffer(Buffer &&other)  noexcept :
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

uint64_t Buffer::get_device_address()
{
	VkBufferDeviceAddressInfoKHR buffer_device_address_info{};
	buffer_device_address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	buffer_device_address_info.buffer = handle;
	return vkGetBufferDeviceAddressKHR(device.get_handle(), &buffer_device_address_info);
}

void Buffer::update(void *data, size_t _size, size_t offset)
{
	update(reinterpret_cast<const uint8_t *>(data), _size, offset);
}

void Buffer::update(const uint8_t *data, const size_t _size, const size_t offset)
{
	if (persistent)
	{
		std::copy(data, data + _size, mapped_data + offset);
		flush();
	}
	else
	{
		map();
		std::copy(data, data + _size, mapped_data + offset);
		flush();
		unmap();
	}
}

}        // namespace core
}        // namespace vkb
