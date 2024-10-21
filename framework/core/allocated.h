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

namespace vkb
{

class Device;

namespace allocated
{
VmaAllocator &get_memory_allocator();

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

void shutdown();

template <vkb::BindingType bindingType>
class AllocatedBase
{
  public:
	using BufferType           = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Buffer, VkBuffer>::type;
	using BufferCreateInfoType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::BufferCreateInfo, VkBufferCreateInfo>::type;
	using DeviceMemoryType     = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::DeviceMemory, VkDeviceMemory>::type;
	using DeviceSizeType       = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::DeviceSize, VkDeviceSize>::type;
	using ImageCreateInfoType  = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::ImageCreateInfo, VkImageCreateInfo>::type;
	using ImageType            = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Image, VkImage>::type;

  public:
	AllocatedBase() = default;
	AllocatedBase(const VmaAllocationCreateInfo &allocation_create_info);
	AllocatedBase(AllocatedBase const &) = delete;
	AllocatedBase(AllocatedBase &&other) noexcept;

	AllocatedBase &operator=(const AllocatedBase &) = delete;
	AllocatedBase &operator=(AllocatedBase &&)      = default;

	/**
	 * @brief Flushes memory if it is HOST_VISIBLE and not HOST_COHERENT
	 */
	void flush(DeviceSizeType offset = 0, DeviceSizeType size = VK_WHOLE_SIZE);

	const uint8_t   *get_data() const;
	DeviceMemoryType get_memory() const;

	/**
	 * @brief Maps vulkan memory if it isn't already mapped to an host visible address
	 * @return Pointer to host visible memory
	 */
	uint8_t *map();

	/**
	 * @brief Returns true if the memory is mapped, false otherwise
	 * @return mapping status
	 */
	bool mapped() const;

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
	virtual void             post_create(VmaAllocationInfo const &allocation_info);
	void                     clear();
	[[nodiscard]] BufferType create_buffer(BufferCreateInfoType const &create_info);
	[[nodiscard]] ImageType  create_image(ImageCreateInfoType const &create_info);
	void                     destroy_buffer(BufferType buffer);
	void                     destroy_image(ImageType image);

  private:
	vk::Buffer create_buffer_impl(vk::BufferCreateInfo const &create_info);
	vk::Image  create_image_impl(vk::ImageCreateInfo const &create_info);

	VmaAllocationCreateInfo allocation_create_info = {};
	VmaAllocation           allocation             = VK_NULL_HANDLE;
	uint8_t                *mapped_data            = nullptr;
	bool                    coherent               = false;
	bool                    persistent             = false;        // Whether the buffer is persistently mapped or not
};

using AllocatedBaseC   = AllocatedBase<vkb::BindingType::C>;
using AllocatedBaseCpp = AllocatedBase<vkb::BindingType::Cpp>;

template <vkb::BindingType bindingType>
inline AllocatedBase<bindingType>::AllocatedBase(const VmaAllocationCreateInfo &allocation_create_info) :
    allocation_create_info(allocation_create_info)
{
}

template <vkb::BindingType bindingType>
inline AllocatedBase<bindingType>::AllocatedBase(AllocatedBase<bindingType> &&other) noexcept :
    allocation_create_info(std::exchange(other.allocation_create_info, {})),
    allocation(std::exchange(other.allocation, {})),
    mapped_data(std::exchange(other.mapped_data, {})),
    coherent(std::exchange(other.coherent, {})),
    persistent(std::exchange(other.persistent, {}))
{
}

template <vkb::BindingType bindingType>
inline void AllocatedBase<bindingType>::clear()
{
	mapped_data            = nullptr;
	persistent             = false;
	allocation_create_info = {};
}

template <vkb::BindingType bindingType>
inline typename AllocatedBase<bindingType>::BufferType AllocatedBase<bindingType>::create_buffer(BufferCreateInfoType const &create_info)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return create_buffer_impl(create_info);
	}
	else
	{
		return static_cast<VkBuffer>(create_buffer_impl(reinterpret_cast<vk::BufferCreateInfo const &>(create_info)));
	}
}

template <vkb::BindingType bindingType>
inline vk::Buffer AllocatedBase<bindingType>::create_buffer_impl(vk::BufferCreateInfo const &create_info)
{
	vk::Buffer        buffer = VK_NULL_HANDLE;
	VmaAllocationInfo allocation_info{};

	auto result = vmaCreateBuffer(
	    get_memory_allocator(),
	    reinterpret_cast<VkBufferCreateInfo const *>(&create_info),
	    &allocation_create_info,
	    reinterpret_cast<VkBuffer *>(&buffer),
	    &allocation,
	    &allocation_info);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create Buffer"};
	}
	post_create(allocation_info);
	return buffer;
}

template <vkb::BindingType bindingType>
inline typename AllocatedBase<bindingType>::ImageType AllocatedBase<bindingType>::create_image(ImageCreateInfoType const &create_info)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return create_image_impl(create_info);
	}
	else
	{
		return static_cast<VkImage>(create_image_impl(reinterpret_cast<vk::ImageCreateInfo const &>(create_info)));
	}
}

template <vkb::BindingType bindingType>
inline vk::Image AllocatedBase<bindingType>::create_image_impl(vk::ImageCreateInfo const &create_info)
{
	assert(0 < create_info.mipLevels && "Images should have at least one level");
	assert(0 < create_info.arrayLayers && "Images should have at least one layer");
	assert(create_info.usage && "Images should have at least one usage type");

	vk::Image         image = VK_NULL_HANDLE;
	VmaAllocationInfo allocation_info{};

#if 0
	// If the image is an attachment, prefer dedicated memory
	constexpr vk::ImageUsageFlags attachment_only_flags = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment;
	if (create_info.usage & attachment_only_flags)
	{
		allocation_create_info.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	}

	if (create_info.usage & vk::ImageUsageFlagBits::eTransientAttachment)
	{
		allocation_create_info.preferredFlags |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
	}
#endif

	VkResult result = vmaCreateImage(get_memory_allocator(),
	                                 reinterpret_cast<VkImageCreateInfo const *>(&create_info),
	                                 &allocation_create_info,
	                                 reinterpret_cast<VkImage *>(&image),
	                                 &allocation,
	                                 &allocation_info);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create Image"};
	}

	post_create(allocation_info);
	return image;
}

template <vkb::BindingType bindingType>
inline void AllocatedBase<bindingType>::destroy_buffer(BufferType handle)
{
	if (handle != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE)
	{
		unmap();
		if constexpr (bindingType == vkb::BindingType::Cpp)
		{
			vmaDestroyBuffer(get_memory_allocator(), static_cast<VkBuffer>(handle), allocation);
		}
		else
		{
			vmaDestroyBuffer(get_memory_allocator(), handle, allocation);
		}
		clear();
	}
}

template <vkb::BindingType bindingType>
inline void AllocatedBase<bindingType>::destroy_image(ImageType image)
{
	if (image != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE)
	{
		unmap();
		if constexpr (bindingType == vkb::BindingType::Cpp)
		{
			vmaDestroyImage(get_memory_allocator(), static_cast<VkImage>(image), allocation);
		}
		else
		{
			vmaDestroyImage(get_memory_allocator(), image, allocation);
		}
		clear();
	}
}

template <vkb::BindingType bindingType>
inline void AllocatedBase<bindingType>::flush(DeviceSizeType offset, DeviceSizeType size)
{
	if (!coherent)
	{
		if constexpr (bindingType == vkb::BindingType::Cpp)
		{
			vmaFlushAllocation(get_memory_allocator(), allocation, static_cast<VkDeviceSize>(offset), static_cast<VkDeviceSize>(size));
		}
		else
		{
			vmaFlushAllocation(get_memory_allocator(), allocation, offset, size);
		}
	}
}

template <vkb::BindingType bindingType>
inline const uint8_t *AllocatedBase<bindingType>::get_data() const
{
	return mapped_data;
}

template <vkb::BindingType bindingType>
inline typename AllocatedBase<bindingType>::DeviceMemoryType AllocatedBase<bindingType>::get_memory() const
{
	VmaAllocationInfo alloc_info;
	vmaGetAllocationInfo(get_memory_allocator(), allocation, &alloc_info);
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return static_cast<vk::DeviceMemory>(alloc_info.deviceMemory);
	}
	else
	{
		return alloc_info.deviceMemory;
	}
}

template <vkb::BindingType bindingType>
inline uint8_t *AllocatedBase<bindingType>::map()
{
	if (!persistent && !mapped())
	{
		VK_CHECK(vmaMapMemory(get_memory_allocator(), allocation, reinterpret_cast<void **>(&mapped_data)));
		assert(mapped_data);
	}
	return mapped_data;
}

template <vkb::BindingType bindingType>
inline bool AllocatedBase<bindingType>::mapped() const
{
	return mapped_data != nullptr;
}

template <vkb::BindingType bindingType>
inline void AllocatedBase<bindingType>::post_create(VmaAllocationInfo const &allocation_info)
{
	VkMemoryPropertyFlags memory_properties;
	vmaGetAllocationMemoryProperties(get_memory_allocator(), allocation, &memory_properties);
	coherent    = (memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	mapped_data = static_cast<uint8_t *>(allocation_info.pMappedData);
	persistent  = mapped();
}

template <vkb::BindingType bindingType>
inline void AllocatedBase<bindingType>::unmap()
{
	if (!persistent && mapped())
	{
		vmaUnmapMemory(get_memory_allocator(), allocation);
		mapped_data = nullptr;
	}
}

template <vkb::BindingType bindingType>
inline size_t AllocatedBase<bindingType>::update(const uint8_t *data, size_t size, size_t offset)
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

template <vkb::BindingType bindingType>
inline size_t AllocatedBase<bindingType>::update(void const *data, size_t size, size_t offset)
{
	return update(reinterpret_cast<const uint8_t *>(data), size, offset);
}

template <vkb::BindingType bindingType, typename HandleType>
class Allocated : public vkb::core::VulkanResource<bindingType, HandleType>, public AllocatedBase<bindingType>
{
  public:
	using DeviceType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPDevice, vkb::Device>::type;

	using ParentType = vkb::core::VulkanResource<bindingType, HandleType>;

  public:
	Allocated()                                  = delete;
	Allocated(const Allocated &)                 = delete;
	Allocated &operator=(Allocated const &other) = delete;
	Allocated &operator=(Allocated &&other)      = default;

	// Import the base class constructors
	template <typename... Args>
	Allocated(const VmaAllocationCreateInfo &allocation_create_info, Args &&...args);

	Allocated(HandleType handle, DeviceType *device_ = nullptr);

	Allocated(Allocated &&other) noexcept;

	const HandleType *get() const;
};

template <typename HandleType>
using AllocatedC = Allocated<vkb::BindingType::C, HandleType>;
template <typename HandleType>
using AllocatedCpp = Allocated<vkb::BindingType::Cpp, HandleType>;

template <vkb::BindingType bindingType, typename HandleType>
template <typename... Args>
inline Allocated<bindingType, HandleType>::Allocated(const VmaAllocationCreateInfo &allocation_create_info, Args &&...args) :
    ParentType{std::forward<Args>(args)...},
    AllocatedBase<bindingType>{allocation_create_info}
{}

template <vkb::BindingType bindingType, typename HandleType>
inline Allocated<bindingType, HandleType>::Allocated(HandleType handle, DeviceType *device_) :
    ParentType(handle, device_)
{}

template <vkb::BindingType bindingType, typename HandleType>
inline Allocated<bindingType, HandleType>::Allocated(Allocated &&other) noexcept :
    ParentType{static_cast<ParentType &&>(other)},
    AllocatedBase<bindingType>{static_cast<AllocatedBase<bindingType> &&>(other)}
{}

template <vkb::BindingType bindingType, typename HandleType>
inline const HandleType *Allocated<bindingType, HandleType>::get() const
{
	return &ParentType::get_handle();
}
}        // namespace allocated
}        // namespace vkb
