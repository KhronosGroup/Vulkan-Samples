/* Copyright (c) 2021-2025, NVIDIA CORPORATION. All rights reserved.
 * Copyright (c) 2024-2025, Bradley Austin Davis. All rights reserved.
 * Copyright (c) 2025, Arm Limited and Contributors
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
#include "core/physical_device.h"
#include "core/vulkan_resource.h"

namespace vkb
{
namespace allocated
{
/**
 * @brief Retrieves a reference to the VMA allocator singleton.  It will hold an opaque handle to the VMA
 * allocator between calls to `init` and `shutdown`.  Otherwise it contains a null pointer.
 * @return A reference to the VMA allocator singleton handle.
 */
VmaAllocator &get_memory_allocator();

/**
 * @brief The non-templatized VMA initializer function, referenced by the template version to smooth
 * over the differences between the `vkb::Device` and `vkb::core::HPPDevice` classes.
 * Idempotent, but should be paired with `shutdown`.
 * @param create_info The VMA allocator create info.
 */
void init(const VmaAllocatorCreateInfo &create_info);

/**
 * @brief Initializes the VMA allocator with the specified device, expressed
 * as the `vkb` wrapper class, which might be `vkb::Device` or `vkb::core::HPPDevice`.
 * @tparam DeviceType The type of the device.
 * @param device The Vulkan device.
 */
template <typename DeviceType = vkb::core::DeviceC>
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

	bool can_get_memory_requirements = device.get_gpu().is_extension_supported(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
	bool has_dedicated_allocation    = device.get_gpu().is_extension_supported(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
	if (can_get_memory_requirements && has_dedicated_allocation && device.is_extension_enabled(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME))
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
	}

	if (device.get_gpu().is_extension_supported(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) &&
	    device.is_extension_enabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	}

	if (device.get_gpu().is_extension_supported(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) && device.is_extension_enabled(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
	}

	if (device.get_gpu().is_extension_supported(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME) &&
	    device.is_extension_enabled(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME))
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
	}

	if (device.get_gpu().is_extension_supported(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME) && device.is_extension_enabled(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME))
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;
	}

	if (device.get_gpu().is_extension_supported(VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME) &&
	    device.is_extension_enabled(VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME))
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_AMD_DEVICE_COHERENT_MEMORY_BIT;
	}

	init(allocator_info);
}

/**
 * @brief Shuts down the VMA allocator and releases all resources.  Should be preceeded with a call to `init`.
 */
void shutdown();

/**
 * @brief The `Allocated` class serves as a base class for wrappers around Vulkan that require memory allocation
 * (`VkImage` and `VkBuffer`).  This class mostly ensures proper behavior for a RAII pattern, preventing double-release by
 * preventing copy assignment and copy construction in favor of move semantics, as well as preventing default construction
 * in favor of explicit construction with a pre-existing handle or a populated create info struct.
 *
 * This project uses the [VMA](https://gpuopen.com/vulkan-memory-allocator/) to handle the low
 * level details of memory allocation and management, as it hides away many of the messyy details of
 * memory allocation when a user is first learning Vulkan, but still allows for fine grained control
 * when a user becomes more experienced and the situation calls for it.
 *
 * @note Constants used in this documentation in the form of `HOST_COHERENT` are shorthand for
 * `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT` used for the sake of brevity.
 *
 * @tparam bindingType A flag indicating whether this is being used with the C or C++ API
 */
template <vkb::BindingType bindingType, typename HandleType>
class Allocated : public vkb::core::VulkanResource<bindingType, HandleType>
{
  public:
	using ParentType = vkb::core::VulkanResource<bindingType, HandleType>;

	using BufferType           = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Buffer, VkBuffer>::type;
	using BufferCreateInfoType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::BufferCreateInfo, VkBufferCreateInfo>::type;
	using DeviceMemoryType     = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::DeviceMemory, VkDeviceMemory>::type;
	using DeviceSizeType       = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::DeviceSize, VkDeviceSize>::type;
	using ImageCreateInfoType  = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::ImageCreateInfo, VkImageCreateInfo>::type;
	using ImageType            = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Image, VkImage>::type;

  public:
	Allocated()                  = delete;
	Allocated(const Allocated &) = delete;
	Allocated(Allocated &&other) noexcept;
	Allocated &operator=(Allocated const &other) = delete;
	Allocated &operator=(Allocated &&other)      = default;

  protected:
	/**
	 * @brief The VMA-specific constructor for new objects. This should only be visible to derived classes.
	 * @param allocation_create_info All of the non-resource-specific information needed by the VMA to allocate the memory.
	 * @param args Additional constructor arguments needed for the derived class. Typically a `VkImageCreateInfo` or `VkBufferCreateInfo` struct.
	 */
	template <typename... Args>
	Allocated(const VmaAllocationCreateInfo &allocation_create_info, Args &&...args);

	/**
	 * @brief This constructor is used when the handle is already created, and the user wants to wrap it in an `Allocated` object.
	 * @note This constructor is used when the API provides us a pre-existing handle to something we didn't actually allocate, for instance
	 * when we allocate a swapchain and access the images in it.  In these cases the `allocation` member variable will remain null for the
	 * lifetime of the wrapper object (which is NOT necessarily the lifetime of the handle) and the wrapper will make no attempt to apply
	 * RAII semantics.
	 */
	Allocated(HandleType handle, vkb::core::Device<bindingType> *device_ = nullptr);

  public:
	const HandleType *get() const;

	/**
	 * @brief Flushes memory if it is NOT `HOST_COHERENT` (which also implies `HOST_VISIBLE`).
	 * This is a no-op for `HOST_COHERENT` memory.
	 *
	 * @param offset The offset into the memory to flush.  Defaults to 0.
	 * @param size The size of the memory to flush.  Defaults to the entire block of memory.
	 */
	void flush(DeviceSizeType offset = 0, DeviceSizeType size = VK_WHOLE_SIZE);

	/**
	 * @brief Retrieves a pointer to the host visible memory as an unsigned byte array.
	 * @return The pointer to the host visible memory.
	 * @note This performs no checking that the memory is actually mapped, so it's possible to get a nullptr
	 */
	const uint8_t *get_data() const;
	/**
	 * @brief Retrieves the raw Vulkan memory object.
	 * @return The Vulkan memory object.
	 */
	DeviceMemoryType get_memory() const;

	/**
	 * @brief Retrieves the offset into the raw Vulkan memory object (which can be retrieved from get_memory()).
	 */
	DeviceSizeType get_memory_offset() const;

	/**
	 * @brief Maps Vulkan memory if it isn't already mapped to a host visible address. Does nothing if the
	 * allocation is already mapped (including persistently mapped allocations).
	 * @return Pointer to host visible memory.
	 */
	uint8_t *map();

	/**
	 * @brief Returns true if the memory is mapped (i.e. the object contains a pointer for the mapping).
	 * This is true for both objects where `map` has been called as well as objects created with persistent
	 * mapping, where no call to `map` is necessary.
	 * @return mapping status.
	 */
	bool mapped() const;

	/**
	 * @brief Unmaps Vulkan memory from the host visible address.  Does nothing if the memory is not mapped or
	 * if the allocation is persistently mapped.
	 */
	void unmap();

	/**
	 * @brief Copies the specified unsigned byte data into the mapped memory region.
	 * @note For non-persistently mapped memory, this function will call the `map` and `unmap` methods and SHOULD NOT
	 * be used if the user intends to make multiple updates to the memory region.  In that case, the user should call
	 * `map` once, make all the updates against the pointer returned by `get_data`, and then call `unmap`.  This may
	 * be a poor design choice as it creates a side effect of using the method (that mapped memory will
	 * unexpectedly be unmapped), but it is the current design of the method and changing it would be burdensome.
	 * Refactoring could be eased by creating a new method with a more explicit name, and then removing this method
	 * entirely.
	 *
	 * @param data The data to copy from.
	 * @param size The amount of bytes to copy.
	 * @param offset The offset to start the copying into the mapped data. Defaults to 0.
	 */
	size_t update(const uint8_t *data, size_t size, size_t offset = 0);

	/**
	 * @brief Converts any non-byte data into bytes and then updates the buffer.  This allows the user to pass
	 * arbitrary structure pointers to the update method, which will then be copied into the buffer as bytes.
	 * @param data The data to copy from.
	 * @param size The amount of bytes to copy.
	 * @param offset The offset to start the copying into the mapped data. Defaults to 0.
	 */
	size_t update(void const *data, size_t size, size_t offset = 0);

	/**
	 * @brief Copies a vector of items into the buffer.  This is a convenience method that allows the user to
	 * pass a vector of items to the update method, which will then be copied into the buffer as bytes.
	 *
	 * This function DOES NOT automatically manage adhering to the alignment requirements of the items being copied,
	 * for instance the `minUniformBufferOffsetAlignment` property of the [device](https://vulkan.gpuinfo.org/displaydevicelimit.php?name=minUniformBufferOffsetAlignment&platform=all).
	 * If the data needs to be aligned on something other than `sizeof(T)`, the user must manage that themselves.
	 * @param data The data vector to upload
	 * @param offset The offset to start the copying into the mapped data
	 * @deprecated Use the `updateTyped` method that uses the `vk::ArrayProxy` class instead.
	 */
	template <typename T>
	size_t update(std::vector<T> const &data, size_t offset = 0)
	{
		return update(data.data(), data.size() * sizeof(T), offset);
	}

	/**
	 * @brief Another convenience method, similar to the vector update method, but for std::array. The same caveats apply.
	 * @param data The data vector to upload
	 * @param offset The offset to start the copying into the mapped data
	 * @see update(std::vector<T> const &data, size_t offset = 0)
	 * @deprecated Use the `updateTyped` method that uses the `vk::ArrayProxy` class instead.
	 */
	template <typename T, size_t N>
	size_t update(std::array<T, N> const &data, size_t offset = 0)
	{
		return update(data.data(), data.size() * sizeof(T), offset);
	}

	/**
	 * @brief Copies an object as byte data into the buffer.  This is a convenience method that allows the user to
	 * pass an object to the update method, which will then be copied into the buffer as bytes.  The name difference
	 * is to avoid amibuity with the `update` method signatures (including the non-templated version)
	 * @param object The object to convert into byte data
	 * @param offset The offset to start the copying into the mapped data
	 * @deprecated Use the `updateTyped` method that uses the `vk::ArrayProxy` class instead.
	 */
	template <class T>
	size_t convert_and_update(const T &object, size_t offset = 0)
	{
		return update(reinterpret_cast<const uint8_t *>(&object), sizeof(T), offset);
	}
	/**
	 * @brief Copies an object as byte data into the buffer.  This is a convenience method that allows the user to
	 * pass an object to the update method, which will then be copied into the buffer as bytes.  The use of the `vk::ArrayProxy`
	 * type here to wrap the passed data means you can use any type related to T that can be used as a constructor to `vk::ArrayProxy`.
	 * This includes `T`, `std::vector<T>`, `std::array<T, N>`, and `vk::ArrayProxy<T>`.
	 *
	 * @remark This was previously not feasible as it would have been undesirable to create a strong coupling with the
	 * C++ Vulkan bindings where the `vk::ArrayProxy` type is defined.  However, structural changes have ensured that this
	 * coupling is always present, so the `vk::ArrayProxy` may as well be used to our advantage here.
	 *
	 * @note This function DOES NOT automatically manage adhering to the alignment requirements of the items being copied,
	 * for instance the `minUniformBufferOffsetAlignment` property of the [device](https://vulkan.gpuinfo.org/displaydevicelimit.php?name=minUniformBufferOffsetAlignment&platform=all).
	 * If the data needs to be aligned on something other than `sizeof(T)`, the user must manage that themselves.
	 *
	 * @todo create `updateTypedAligned` which has an additional argument specifying the required GPU alignment of the elements of the array.
	 */
	template <class T>
	size_t updateTyped(const vk::ArrayProxy<T> &object, size_t offset = 0)
	{
		return update(reinterpret_cast<const uint8_t *>(object.data()), object.size() * sizeof(T), offset);
	}

  protected:
	/**
	 * @brief Internal method to actually create the buffer, allocate the memory and bind them.
	 * Should only be called from the `Buffer` derived class.
	 *
	 * Present in this common base class in order to allow the internal state members to remain `private`
	 * instead of `protected`, and because it (mostly) isolates interaction with the VMA to a single class
	 */
	[[nodiscard]] BufferType create_buffer(BufferCreateInfoType const &create_info, DeviceSizeType alignment);
	/**
	 * @brief Internal method to actually create the image, allocate the memory and bind them.
	 * Should only be called from the `Image` derived class.
	 *
	 * Present in this common base class in order to allow the internal state members to remain `private`
	 * instead of `protected`, and because it (mostly) isolates interaction with the VMA to a single class
	 */
	[[nodiscard]] ImageType create_image(ImageCreateInfoType const &create_info);

	/**
	 * @brief Internal method to retrieve the VMA allocation owned by this object.
	 *
	 * This is needed for derived classes to handle some of the VMA allocation code themselves,
	 * in particular for Tensor objects to be allocated, in tensor_and_data_graph_common.cpp.
	 * Once Tensor objects are integrated into VMA, this code can be refactored and this function removed.
	 */
	VmaAllocation get_allocation() const;

	/**
	 * @brief Internal method to set the VMA allocation owned by this object.
	 *
	 * This is needed for derived classes to handle some of the VMA allocation code themselves,
	 * in particular for Tensor objects to be allocated, in tensor_and_data_graph_common.cpp.
	 * Once Tensor objects are integrated into VMA, this code can be refactored and this function removed.
	 */
	void set_allocation(VmaAllocation alloc);

	/**
	 * @brief The post_create method is called after the creation of a buffer or image to store the allocation info internally.  Derived classes
	 * could in theory override this to ensure any post-allocation operations are performed, but the base class should always be called to ensure
	 * the allocation info is stored.
	 * Should only be called in the corresponding `create_xxx` methods.
	 */
	virtual void post_create(VmaAllocationInfo const &allocation_info);

	/**
	 * @brief Internal method to actually destroy the buffer and release the allocated memory.  Should
	 * only be called from the `Buffer` derived class.
	 * Present in this common base class in order to allow the internal state members to remain `private`
	 * instead of `protected`, and because it (mostly) isolates interaction with the VMA to a single class
	 */
	void destroy_buffer(BufferType buffer);
	/**
	 * @brief Internal method to actually destroy the image and release the allocated memory.  Should
	 * only be called from the `Image` derived class.
	 * Present in this common base class in order to allow the internal state members to remain `private`
	 * instead of `protected`, and because it (mostly) isolates interaction with the VMA to a single class
	 */
	void destroy_image(ImageType image);
	/**
	 * @brief Clears the internal state.  Can be overridden by derived classes to perform additional cleanup of members.
	 * Should only be called in the corresping `destroy_xxx` methods.
	 */
	void clear();

  private:
	vk::Buffer create_buffer_impl(vk::BufferCreateInfo const &create_info, DeviceSizeType alignment);
	vk::Image  create_image_impl(vk::ImageCreateInfo const &create_info);

	VmaAllocationCreateInfo allocation_create_info = {};
	VmaAllocation           allocation             = VK_NULL_HANDLE;
	/**
	 * @brief A pointer to the allocation memory, if the memory is HOST_VISIBLE and is currently (or persistently) mapped.
	 * Contains null otherwise.
	 */
	uint8_t *mapped_data = nullptr;
	/**
	 * @brief This flag is set to true if the memory is coherent and doesn't need to be flushed after writes.
	 *
	 * @note This is initialized at allocation time to avoid subsequent need to call a function to fetch the
	 * allocation information from the VMA, since this property won't change for the lifetime of the allocation.
	 */
	bool coherent = false;
	/**
	 * @brief This flag is set to true if the memory is persistently mapped (i.e. not just HOST_VISIBLE, but available
	 * as a pointer to the application for the lifetime of the allocation).
	 *
	 * @note This is initialized at allocation time to avoid subsequent need to call a function to fetch the
	 * allocation information from the VMA, since this property won't change for the lifetime of the allocation.
	 */
	bool persistent = false;
};

template <vkb::BindingType bindingType, typename HandleType>
inline Allocated<bindingType, HandleType>::Allocated(Allocated &&other) noexcept :
    ParentType{static_cast<ParentType &&>(other)},
    allocation_create_info(std::exchange(other.allocation_create_info, {})),
    allocation(std::exchange(other.allocation, {})),
    mapped_data(std::exchange(other.mapped_data, {})),
    coherent(std::exchange(other.coherent, {})),
    persistent(std::exchange(other.persistent, {}))
{
}

template <vkb::BindingType bindingType, typename HandleType>
template <typename... Args>
inline Allocated<bindingType, HandleType>::Allocated(const VmaAllocationCreateInfo &allocation_create_info, Args &&...args) :
    ParentType{std::forward<Args>(args)...},
    allocation_create_info(allocation_create_info)
{}

template <vkb::BindingType bindingType, typename HandleType>
inline Allocated<bindingType, HandleType>::Allocated(HandleType handle, vkb::core::Device<bindingType> *device_) :
    ParentType(handle, device_)
{}

template <vkb::BindingType bindingType, typename HandleType>
inline const HandleType *Allocated<bindingType, HandleType>::get() const
{
	return &ParentType::get_handle();
}

template <vkb::BindingType bindingType, typename HandleType>
inline void Allocated<bindingType, HandleType>::clear()
{
	mapped_data            = nullptr;
	persistent             = false;
	allocation_create_info = {};
}

template <vkb::BindingType bindingType, typename HandleType>
inline typename Allocated<bindingType, HandleType>::BufferType Allocated<bindingType, HandleType>::create_buffer(BufferCreateInfoType const &create_info, DeviceSizeType alignment)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return create_buffer_impl(create_info, alignment);
	}
	else
	{
		return static_cast<VkBuffer>(create_buffer_impl(reinterpret_cast<vk::BufferCreateInfo const &>(create_info), alignment));
	}
}

template <vkb::BindingType bindingType, typename HandleType>
inline vk::Buffer Allocated<bindingType, HandleType>::create_buffer_impl(vk::BufferCreateInfo const &create_info, DeviceSizeType alignment)
{
	vk::Buffer        buffer = VK_NULL_HANDLE;
	VmaAllocationInfo allocation_info{};

	auto result = VK_SUCCESS;
	if (alignment == 0)
	{
		result = vmaCreateBuffer(
		    get_memory_allocator(),
		    reinterpret_cast<VkBufferCreateInfo const *>(&create_info),
		    &allocation_create_info,
		    reinterpret_cast<VkBuffer *>(&buffer),
		    &allocation,
		    &allocation_info);
	}
	else
	{
		result = vmaCreateBufferWithAlignment(
		    get_memory_allocator(),
		    reinterpret_cast<VkBufferCreateInfo const *>(&create_info),
		    &allocation_create_info,
		    alignment,
		    reinterpret_cast<VkBuffer *>(&buffer),
		    &allocation,
		    &allocation_info);
	}

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create Buffer"};
	}
	post_create(allocation_info);
	return buffer;
}

template <vkb::BindingType bindingType, typename HandleType>
inline typename Allocated<bindingType, HandleType>::ImageType Allocated<bindingType, HandleType>::create_image(ImageCreateInfoType const &create_info)
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

template <vkb::BindingType bindingType, typename HandleType>
inline vk::Image Allocated<bindingType, HandleType>::create_image_impl(vk::ImageCreateInfo const &create_info)
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

template <vkb::BindingType bindingType, typename HandleType>
inline void Allocated<bindingType, HandleType>::destroy_buffer(BufferType handle)
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

template <vkb::BindingType bindingType, typename HandleType>
inline void Allocated<bindingType, HandleType>::destroy_image(ImageType image)
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

template <vkb::BindingType bindingType, typename HandleType>
inline void Allocated<bindingType, HandleType>::flush(DeviceSizeType offset, DeviceSizeType size)
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

template <vkb::BindingType bindingType, typename HandleType>
inline const uint8_t *Allocated<bindingType, HandleType>::get_data() const
{
	return mapped_data;
}

template <vkb::BindingType bindingType, typename HandleType>
inline typename Allocated<bindingType, HandleType>::DeviceMemoryType Allocated<bindingType, HandleType>::get_memory() const
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

template <vkb::BindingType bindingType, typename HandleType>
inline typename Allocated<bindingType, HandleType>::DeviceSizeType Allocated<bindingType, HandleType>::get_memory_offset() const
{
	VmaAllocationInfo alloc_info;
	vmaGetAllocationInfo(get_memory_allocator(), allocation, &alloc_info);
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return static_cast<vk::DeviceSize>(alloc_info.offset);
	}
	else
	{
		return alloc_info.offset;
	}
}

template <vkb::BindingType bindingType, typename HandleType>
inline uint8_t *Allocated<bindingType, HandleType>::map()
{
	if (!persistent && !mapped())
	{
		VK_CHECK(vmaMapMemory(get_memory_allocator(), allocation, reinterpret_cast<void **>(&mapped_data)));
		assert(mapped_data);
	}
	return mapped_data;
}

template <vkb::BindingType bindingType, typename HandleType>
inline bool Allocated<bindingType, HandleType>::mapped() const
{
	return mapped_data != nullptr;
}

template <vkb::BindingType bindingType, typename HandleType>
inline VmaAllocation Allocated<bindingType, HandleType>::get_allocation() const
{
	return allocation;
}

template <vkb::BindingType bindingType, typename HandleType>
inline void Allocated<bindingType, HandleType>::set_allocation(VmaAllocation alloc)
{
	allocation = alloc;
}

template <vkb::BindingType bindingType, typename HandleType>
inline void Allocated<bindingType, HandleType>::post_create(VmaAllocationInfo const &allocation_info)
{
	VkMemoryPropertyFlags memory_properties;
	vmaGetAllocationMemoryProperties(get_memory_allocator(), allocation, &memory_properties);
	coherent    = (memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	mapped_data = static_cast<uint8_t *>(allocation_info.pMappedData);
	persistent  = mapped();
}

template <vkb::BindingType bindingType, typename HandleType>
inline void Allocated<bindingType, HandleType>::unmap()
{
	if (!persistent && mapped())
	{
		vmaUnmapMemory(get_memory_allocator(), allocation);
		mapped_data = nullptr;
	}
}

template <vkb::BindingType bindingType, typename HandleType>
inline size_t Allocated<bindingType, HandleType>::update(const uint8_t *data, size_t size, size_t offset)
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

template <vkb::BindingType bindingType, typename HandleType>
inline size_t Allocated<bindingType, HandleType>::update(void const *data, size_t size, size_t offset)
{
	return update(reinterpret_cast<const uint8_t *>(data), size, offset);
}

template <typename HandleType>
using AllocatedC = Allocated<vkb::BindingType::C, HandleType>;
template <typename HandleType>
using AllocatedCpp = Allocated<vkb::BindingType::Cpp, HandleType>;

}        // namespace allocated
}        // namespace vkb
