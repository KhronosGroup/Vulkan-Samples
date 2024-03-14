/* Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
 * Copyright (c) 2024, Bradley Austin Davis. All rights reserved.
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

#include "allocated.h"

namespace vkb
{

namespace allocated
{

VmaAllocator &get_memory_allocator()
{
	static VmaAllocator memory_allocator = VK_NULL_HANDLE;
	return memory_allocator;
}

void init(const VmaAllocatorCreateInfo &create_info)
{
	VkResult result = vmaCreateAllocator(&create_info, &get_memory_allocator());
	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create allocator"};
	}
}

void shutdown()
{
	auto &allocator = get_memory_allocator();
	if (allocator != VK_NULL_HANDLE)
	{
		VmaTotalStatistics stats;
		vmaCalculateStatistics(allocator, &stats);
		LOGI("Total device memory leaked: {} bytes.", stats.total.statistics.allocationBytes);
		vmaDestroyAllocator(allocator);
		allocator = VK_NULL_HANDLE;
	}
}

AllocatedBase::AllocatedBase(const VmaAllocationCreateInfo &alloc_create_info) :
    alloc_create_info(alloc_create_info)
{
}

AllocatedBase::AllocatedBase(AllocatedBase &&other) noexcept :
    alloc_create_info(std::exchange(other.alloc_create_info, {})),
    allocation(std::exchange(other.allocation, {})),
    mapped_data(std::exchange(other.mapped_data, {})),
    coherent(std::exchange(other.coherent, {})),
    persistent(std::exchange(other.persistent, {}))
{
}

const uint8_t *AllocatedBase::get_data() const
{
	return mapped_data;
}

VkDeviceMemory AllocatedBase::get_memory() const
{
	VmaAllocationInfo alloc_info;
	vmaGetAllocationInfo(get_memory_allocator(), allocation, &alloc_info);
	return alloc_info.deviceMemory;
}

void AllocatedBase::flush(VkDeviceSize offset, VkDeviceSize size)
{
	if (!coherent)
	{
		vmaFlushAllocation(get_memory_allocator(), allocation, offset, size);
	}
}

uint8_t *AllocatedBase::map()
{
	if (!persistent && !mapped())
	{
		VK_CHECK(vmaMapMemory(get_memory_allocator(), allocation, reinterpret_cast<void **>(&mapped_data)));
		assert(mapped_data);
	}
	return mapped_data;
}

void AllocatedBase::unmap()
{
	if (!persistent && mapped())
	{
		vmaUnmapMemory(get_memory_allocator(), allocation);
		mapped_data = nullptr;
	}
}

size_t AllocatedBase::update(const uint8_t *data, size_t size, size_t offset)
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
	return size;
}

size_t AllocatedBase::update(void const *data, size_t size, size_t offset)
{
	return update(reinterpret_cast<const uint8_t *>(data), size, offset);
}

void AllocatedBase::post_create(VmaAllocationInfo const &allocation_info)
{
	VkMemoryPropertyFlags memory_properties;
	vmaGetAllocationMemoryProperties(get_memory_allocator(), allocation, &memory_properties);
	coherent    = (memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	mapped_data = static_cast<uint8_t *>(allocation_info.pMappedData);
	persistent  = mapped();
}

[[nodiscard]] VkBuffer AllocatedBase::create_buffer(VkBufferCreateInfo const &create_info)
{
	VkBuffer          handleResult = VK_NULL_HANDLE;
	VmaAllocationInfo allocation_info{};

	auto result = vmaCreateBuffer(
	    get_memory_allocator(),
	    &create_info,
	    &alloc_create_info,
	    &handleResult,
	    &allocation,
	    &allocation_info);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create Buffer"};
	}
	post_create(allocation_info);
	return handleResult;
}

[[nodiscard]] VkImage AllocatedBase::create_image(VkImageCreateInfo const &create_info)
{
	assert(0 < create_info.mipLevels && "Images should have at least one level");
	assert(0 < create_info.arrayLayers && "Images should have at least one layer");
	assert(0 < create_info.usage && "Images should have at least one usage type");

	VkImage           handleResult = VK_NULL_HANDLE;
	VmaAllocationInfo allocation_info{};

#if 0
		// If the image is an attachment, prefer dedicated memory
		constexpr VkImageUsageFlags attachment_only_flags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
		if (create_info.usage & attachment_only_flags)
		{
			alloc_create_info.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		}

		if (create_info.usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
		{
			alloc_create_info.preferredFlags |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
		}
#endif

	auto result = vmaCreateImage(
	    get_memory_allocator(),
	    &create_info,
	    &alloc_create_info,
	    &handleResult,
	    &allocation,
	    &allocation_info);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create Image"};
	}

	post_create(allocation_info);
	return handleResult;
}

void AllocatedBase::destroy_buffer(VkBuffer handle)
{
	if (handle != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE)
	{
		unmap();
		vmaDestroyBuffer(get_memory_allocator(), handle, allocation);
		clear();
	}
}

void AllocatedBase::destroy_image(VkImage image)
{
	if (image != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE)
	{
		unmap();
		vmaDestroyImage(get_memory_allocator(), image, allocation);
		clear();
	}
}

bool AllocatedBase::mapped() const
{
	return mapped_data != nullptr;
}

void AllocatedBase::clear()
{
	mapped_data       = nullptr;
	persistent        = false;
	alloc_create_info = {};
}

}        // namespace allocated
}        // namespace vkb
