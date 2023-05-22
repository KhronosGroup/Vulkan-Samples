/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#include "hpp_buffer.h"

#include "hpp_device.h"

namespace vkb
{
namespace core
{
HPPBuffer::HPPBuffer(vkb::core::HPPDevice        &device,
                     vk::DeviceSize               size_,
                     vk::BufferUsageFlags         buffer_usage,
                     VmaMemoryUsage               memory_usage,
                     VmaAllocationCreateFlags     flags,
                     const std::vector<uint32_t> &queue_family_indices) :
    HPPVulkanResource(nullptr, &device), size(size_)
{
#ifdef VK_USE_PLATFORM_METAL_EXT
	// Workaround for Mac (MoltenVK requires unmapping https://github.com/KhronosGroup/MoltenVK/issues/175)
	// Force cleares the flag VMA_ALLOCATION_CREATE_MAPPED_BIT
	flags &= ~VMA_ALLOCATION_CREATE_MAPPED_BIT;
#endif

	persistent = (flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) != 0;

	vk::BufferCreateInfo buffer_create_info({}, size, buffer_usage);
	if (queue_family_indices.size() >= 2)
	{
		buffer_create_info.sharingMode           = vk::SharingMode::eConcurrent;
		buffer_create_info.queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size());
		buffer_create_info.pQueueFamilyIndices   = queue_family_indices.data();
	}

	VmaAllocationCreateInfo memory_info{};
	memory_info.flags = flags;
	memory_info.usage = memory_usage;

	VmaAllocationInfo allocation_info{};
	auto              result = vmaCreateBuffer(device.get_memory_allocator(),
	                                           reinterpret_cast<VkBufferCreateInfo *>(&buffer_create_info), &memory_info,
	                                           reinterpret_cast<VkBuffer *>(&get_handle()), &allocation,
	                                           &allocation_info);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create HPPBuffer"};
	}

	memory = static_cast<vk::DeviceMemory>(allocation_info.deviceMemory);

	if (persistent)
	{
		mapped_data = static_cast<uint8_t *>(allocation_info.pMappedData);
	}
}

HPPBuffer::HPPBuffer(HPPBuffer &&other) :
    HPPVulkanResource{other.get_handle(), &other.get_device()},
    allocation(std::exchange(other.allocation, {})),
    memory(std::exchange(other.memory, {})),
    size(std::exchange(other.size, {})),
    mapped_data(std::exchange(other.mapped_data, {})),
    mapped(std::exchange(other.mapped, {}))
{}

HPPBuffer::~HPPBuffer()
{
	if (get_handle() && (allocation != VK_NULL_HANDLE))
	{
		unmap();
		vmaDestroyBuffer(get_device().get_memory_allocator(), static_cast<VkBuffer>(get_handle()), allocation);
	}
}

VmaAllocation HPPBuffer::get_allocation() const
{
	return allocation;
}

vk::DeviceMemory HPPBuffer::get_memory() const
{
	return memory;
}

vk::DeviceSize HPPBuffer::get_size() const
{
	return size;
}

const uint8_t *HPPBuffer::get_data() const
{
	return mapped_data;
}

uint8_t *HPPBuffer::map()
{
	if (!mapped && !mapped_data)
	{
		VK_CHECK(vmaMapMemory(get_device().get_memory_allocator(), allocation, reinterpret_cast<void **>(&mapped_data)));
		mapped = true;
	}
	return mapped_data;
}

void HPPBuffer::unmap()
{
	if (mapped)
	{
		vmaUnmapMemory(get_device().get_memory_allocator(), allocation);
		mapped_data = nullptr;
		mapped      = false;
	}
}

void HPPBuffer::flush()
{
	vmaFlushAllocation(get_device().get_memory_allocator(), allocation, 0, size);
}

void HPPBuffer::update(const std::vector<uint8_t> &data, size_t offset)
{
	update(data.data(), data.size(), offset);
}

uint64_t HPPBuffer::get_device_address() const
{
	return get_device().get_handle().getBufferAddressKHR({get_handle()});
}

void HPPBuffer::update(void *data, size_t size, size_t offset)
{
	update(reinterpret_cast<const uint8_t *>(data), size, offset);
}

void HPPBuffer::update(const uint8_t *data, const size_t size, const size_t offset)
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

}        // namespace core
}        // namespace vkb
