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

#pragma once

#include "common/error.h"
#include "core/vulkan_resource.h"
#include <vector>
#include <vk_mem_alloc.h>

namespace vkb
{

class Device;

namespace allocated
{

template <
    typename BuilderType,
    typename CreateInfoType,
    typename SharingModeType = VkSharingMode>
struct Builder
{
	VmaAllocationCreateInfo alloc_create_info{};
	std::string             debug_name;
	CreateInfoType          create_info;

  protected:
	Builder(const Builder &other) = delete;
	Builder(const CreateInfoType &create_info) :
	    create_info(create_info)
	{
		alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	};

  public:
	BuilderType &with_debug_name(const std::string &name)
	{
		debug_name = name;
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_vma_usage(VmaMemoryUsage usage)
	{
		alloc_create_info.usage = usage;
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_vma_flags(VmaAllocationCreateFlags flags)
	{
		alloc_create_info.flags = flags;
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_vma_required_flags(VkMemoryPropertyFlags flags)
	{
		alloc_create_info.requiredFlags = flags;
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_vma_preferred_flags(VkMemoryPropertyFlags flags)
	{
		alloc_create_info.preferredFlags = flags;
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_memory_type_bits(uint32_t type_bits)
	{
		alloc_create_info.memoryTypeBits = type_bits;
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_vma_pool(VmaPool pool)
	{
		alloc_create_info.pool = pool;
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_queue_families(uint32_t count, const uint32_t *family_indices)
	{
		create_info.queueFamilyIndexCount = count;
		create_info.pQueueFamilyIndices   = family_indices;
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_sharing(VkSharingMode sharing)
	{
		create_info.sharingMode = sharing;
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_implicit_sharing_mode()
	{
		if (create_info.queueFamilyIndexCount != 0)
		{
			create_info.sharingMode = static_cast<SharingModeType>(VK_SHARING_MODE_CONCURRENT);
		}
		else
		{
			create_info.sharingMode = static_cast<SharingModeType>(VK_SHARING_MODE_EXCLUSIVE);
		}
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_queue_families(const std::vector<uint32_t> &queue_families)
	{
		return with_queue_families(static_cast<uint32_t>(queue_families.size()), queue_families.data());
	}
};

void init(const VmaAllocatorCreateInfo &create_info);

template <typename DeviceType = vkb::Device>
void init(const DeviceType &device)
{
	VmaVulkanFunctions vma_vulkan_func{};
	vma_vulkan_func.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vma_vulkan_func.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo allocator_info{};
	allocator_info.pVulkanFunctions = &vma_vulkan_func;
	allocator_info.physicalDevice   = static_cast<VkPhysicalDevice>(device.get_gpu().get_handle());
	allocator_info.device           = static_cast<VkDevice>(device.get_handle());
	allocator_info.instance         = static_cast<VkInstance>(device.get_gpu().get_instance().get_handle());

	bool can_get_memory_requirements = device.is_extension_supported(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
	bool has_dedicated_allocation    = device.is_extension_supported(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
	if (can_get_memory_requirements && has_dedicated_allocation && device.is_enabled(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME))
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
	}

	if (device.is_extension_supported(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) && device.is_enabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	}

	if (device.is_extension_supported(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) && device.is_enabled(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
	}

	if (device.is_extension_supported(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME) && device.is_enabled(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME))
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
	}

	if (device.is_extension_supported(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME) && device.is_enabled(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME))
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;
	}

	if (device.is_extension_supported(VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME) && device.is_enabled(VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME))
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_AMD_DEVICE_COHERENT_MEMORY_BIT;
	}

	init(allocator_info);
}

VmaAllocator &get_memory_allocator();

void shutdown();

class AllocatedBase
{
  public:
	AllocatedBase() = default;
	AllocatedBase(const VmaAllocationCreateInfo &alloc_create_info);
	AllocatedBase(AllocatedBase &&other) noexcept;

	AllocatedBase &operator=(const AllocatedBase &) = delete;
	AllocatedBase &operator=(AllocatedBase &&)      = delete;

	const uint8_t *get_data() const;
	VkDeviceMemory get_memory() const;

	/**
	 * @brief Flushes memory if it is HOST_VISIBLE and not HOST_COHERENT
	 */
	void flush(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

	/**
	 * @brief Returns true if the memory is mapped, false otherwise
	 * @return mapping status
	 */
	bool mapped() const;

	/**
	 * @brief Maps vulkan memory if it isn't already mapped to an host visible address
	 * @return Pointer to host visible memory
	 */
	uint8_t *map();

	/**
	 * @brief Unmaps vulkan memory from the host visible address
	 */
	void unmap();

	/**
	 * @brief Copies byte data into the buffer
	 * @param data The data to copy from
	 * @param size The amount of bytes to copy
	 * @param offset The offset to start the copying into the mapped data
	 */
	size_t update(const uint8_t *data, size_t size, size_t offset = 0);
	/**
	 * @brief Converts any non byte data into bytes and then updates the buffer
	 * @param data The data to copy from
	 * @param size The amount of bytes to copy
	 * @param offset The offset to start the copying into the mapped data
	 */
	size_t update(void const *data, size_t size, size_t offset = 0);

	/**
	 * @todo Use the vk::ArrayBuffer class to collapse some of these templates
	 * @brief Copies a vector of items into the buffer
	 * @param data The data vector to upload
	 * @param offset The offset to start the copying into the mapped data
	 */
	template <typename T>
	size_t update(std::vector<T> const &data, size_t offset = 0)
	{
		return update(data.data(), data.size() * sizeof(T), offset);
	}

	template <typename T, size_t N>
	size_t update(std::array<T, N> const &data, size_t offset = 0)
	{
		return update(data.data(), data.size() * sizeof(T), offset);
	}

	/**
	 * @brief Copies an object as byte data into the buffer
	 * @param object The object to convert into byte data
	 * @param offset The offset to start the copying into the mapped data
	 */
	template <class T>
	size_t convert_and_update(const T &object, size_t offset = 0)
	{
		return update(reinterpret_cast<const uint8_t *>(&object), sizeof(T), offset);
	}

  protected:
	virtual void           post_create(VmaAllocationInfo const &allocation_info);
	[[nodiscard]] VkBuffer create_buffer(VkBufferCreateInfo const &create_info);
	[[nodiscard]] VkImage  create_image(VkImageCreateInfo const &create_info);
	void                   destroy_buffer(VkBuffer buffer);
	void                   destroy_image(VkImage image);
	void                   clear();

	VmaAllocationCreateInfo alloc_create_info{};
	VmaAllocation           allocation  = VK_NULL_HANDLE;
	uint8_t                *mapped_data = nullptr;
	bool                    coherent    = false;
	bool                    persistent  = false;        // Whether the buffer is persistently mapped or not
};

template <
    typename HandleType,
    typename MemoryType = VkDeviceMemory,
    typename ParentType = vkb::core::VulkanResourceC<HandleType>>
class Allocated : public ParentType, public AllocatedBase
{
  public:
	using ParentType::ParentType;

	Allocated()                  = delete;
	Allocated(const Allocated &) = delete;

	// Import the base class constructors
	template <typename... Args>
	Allocated(const VmaAllocationCreateInfo &alloc_create_info, Args &&...args) :
	    ParentType(std::forward<Args>(args)...),
	    AllocatedBase(alloc_create_info)
	{
	}

	Allocated(Allocated &&other) noexcept
	    :
	    ParentType{static_cast<ParentType &&>(other)},
	    AllocatedBase{static_cast<AllocatedBase &&>(other)}
	{
	}

	const HandleType *get() const
	{
		return &ParentType::get_handle();
	}
};

}        // namespace allocated
}        // namespace vkb
