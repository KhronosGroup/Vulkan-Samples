/* Copyright (c) 2019-2024, Arm Limited and Contributors
 * Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "builder_base.h"
#include "common/vk_common.h"
#include "core/allocated.h"
#include "core/hpp_allocated.h"

namespace vkb
{
namespace core
{
template <vkb::BindingType bindingType>
class Buffer;
template <vkb::BindingType bindingType>
using BufferPtr = std::unique_ptr<Buffer<bindingType>>;

template <vkb::BindingType bindingType>
struct BufferBuilder
    : public vkb::BuilderBase<bindingType,
                              BufferBuilder<bindingType>,
                              typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::BufferCreateInfo, VkBufferCreateInfo>::type>
{
  public:
	using BufferCreateFlagsType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::BufferCreateFlags, VkBufferCreateFlags>::type;
	using BufferCreateInfoType  = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::BufferCreateInfo, VkBufferCreateInfo>::type;
	using BufferUsageFlagsType  = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::BufferUsageFlags, VkBufferUsageFlags>::type;
	using DeviceSizeType        = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::DeviceSize, VkDeviceSize>::type;
	using SharingModeType       = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::SharingMode, VkSharingMode>::type;

	using DeviceType  = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPDevice, vkb::Device>::type;

  private:
	using Parent = vkb::BuilderBase<bindingType, BufferBuilder<bindingType>, BufferCreateInfoType>;

  public:
	BufferBuilder(DeviceSizeType size);

	Buffer<bindingType>    build(DeviceType &device) const;
	BufferPtr<bindingType> build_unique(DeviceType &device) const;
	BufferBuilder         &with_flags(BufferCreateFlagsType flags);
	BufferBuilder         &with_implicit_sharing_mode();
	BufferBuilder         &with_sharing_mode(SharingModeType sharing_mode);
	BufferBuilder         &with_usage(BufferUsageFlagsType usage);
};

using BufferBuilderC   = BufferBuilder<vkb::BindingType::C>;
using BufferBuilderCpp = BufferBuilder<vkb::BindingType::Cpp>;

template <>
inline BufferBuilder<vkb::BindingType::Cpp>::BufferBuilder(vk::DeviceSize size) :
    Parent(BufferCreateInfoType{{}, size})
{
}

template <>
inline BufferBuilder<vkb::BindingType::C>::BufferBuilder(VkDeviceSize size) :
    Parent(VkBufferCreateInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, nullptr, 0, size})
{}

template <vkb::BindingType bindingType>
inline Buffer<bindingType> BufferBuilder<bindingType>::build(DeviceType &device) const
{
	return Buffer<bindingType>{device, *this};
}

template <vkb::BindingType bindingType>
inline BufferPtr<bindingType> BufferBuilder<bindingType>::build_unique(DeviceType &device) const
{
	return std::make_unique<Buffer<bindingType>>(device, *this);
}

template <vkb::BindingType bindingType>
inline BufferBuilder<bindingType> &BufferBuilder<bindingType>::with_flags(BufferCreateFlagsType flags)
{
	this->create_info.flags = flags;
	return *this;
}

template <vkb::BindingType bindingType>
inline BufferBuilder<bindingType> &BufferBuilder<bindingType>::with_implicit_sharing_mode()
{
	if (this->create_info.queueFamilyIndexCount != 0)
	{
		this->create_info.sharingMode = vk::SharingMode::eConcurrent;
	}
	return *this;
}

template <vkb::BindingType bindingType>
inline BufferBuilder<bindingType> &BufferBuilder<bindingType>::with_sharing_mode(SharingModeType sharing_mode)
{
	this->create_info.sharingMode = sharing_mode;
	return *this;
}

template <vkb::BindingType bindingType>
inline BufferBuilder<bindingType> &BufferBuilder<bindingType>::with_usage(BufferUsageFlagsType usage)
{
	this->get_create_info().usage = usage;
	return *this;
}

/*=========================================================*/

template <vkb::BindingType bindingType>
class Buffer
    : public std::conditional<bindingType == vkb::BindingType::Cpp, allocated::HPPAllocated<vk::Buffer>, allocated::Allocated<VkBuffer>>::type
{
  public:
	using BufferUsageFlagsType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::BufferUsageFlags, VkBufferUsageFlags>::type;
	using DeviceSizeType       = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::DeviceSize, VkDeviceSize>::type;

	using AllocatedType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, allocated::HPPAllocated<vk::Buffer>, allocated::Allocated<VkBuffer>>::type;
	using DeviceType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPDevice, vkb::Device>::type;

  private:
	using Parent = AllocatedType;

  public:
	static Buffer<bindingType> create_staging_buffer(DeviceType &device, DeviceSizeType size, const void *data);

	template <typename T>
	static Buffer create_staging_buffer(DeviceType &device, std::vector<T> const &data);

	template <typename T>
	static Buffer create_staging_buffer(DeviceType &device, const T &data);

	Buffer()                          = delete;
	Buffer(const Buffer &)            = delete;
	Buffer(Buffer &&other)            = default;
	Buffer &operator=(const Buffer &) = delete;
	Buffer &operator=(Buffer &&)      = default;

	/**
	 * @brief Creates a buffer using VMA
	 * @param device A valid Vulkan device
	 * @param size The size in bytes of the buffer
	 * @param buffer_usage The usage flags for the VkBuffer
	 * @param memory_usage The memory usage of the buffer
	 * @param flags The allocation create flags
	 * @param queue_family_indices optional queue family indices
	 */
	// [[deprecated("Use the BufferBuilder ctor instead")]]
	Buffer(DeviceType                  &device,
	       DeviceSizeType               size,
	       BufferUsageFlagsType         buffer_usage,
	       VmaMemoryUsage               memory_usage,
	       VmaAllocationCreateFlags     flags                = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
	       const std::vector<uint32_t> &queue_family_indices = {});

	Buffer(DeviceType &device, BufferBuilder<bindingType> const &builder);

	~Buffer();

	/**
	 * @return Return the buffer's device address (note: requires that the buffer has been created with the VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT usage fla)
	 */
	uint64_t get_device_address() const;

	/**
	 * @return The size of the buffer
	 */
	DeviceSizeType get_size() const;

  private:
	vk::DeviceSize size = 0;
};

using BufferC   = Buffer<vkb::BindingType::C>;
using BufferCpp = Buffer<vkb::BindingType::Cpp>;

template <vkb::BindingType bindingType>
template <typename T>
inline Buffer<bindingType> Buffer<bindingType>::create_staging_buffer(DeviceType &device, const T &data)
{
	return create_staging_buffer(device, sizeof(T), &data);
}

template <vkb::BindingType bindingType>
inline Buffer<bindingType> Buffer<bindingType>::create_staging_buffer(DeviceType &device, DeviceSizeType size, const void *data)
{
	BufferBuilder<bindingType> builder(size);
	builder.with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		builder.with_usage(vk::BufferUsageFlagBits::eTransferSrc);
	}
	else
	{
		builder.with_usage(static_cast<VkBufferUsageFlagBits>(vk::BufferUsageFlagBits::eTransferSrc));
	}
	Buffer result(device, builder);
	if (data != nullptr)
	{
		result.update(data, size);
	}
	return result;
}

template <vkb::BindingType bindingType>
template <typename T>
inline Buffer<bindingType> Buffer<bindingType>::create_staging_buffer(DeviceType &device, std::vector<T> const &data)
{
	return create_staging_buffer(device, data.size() * sizeof(T), data.data());
}

template <vkb::BindingType bindingType>
inline Buffer<bindingType>::Buffer(DeviceType                  &device,
                                   DeviceSizeType               size,
                                   BufferUsageFlagsType         buffer_usage,
                                   VmaMemoryUsage               memory_usage,
                                   VmaAllocationCreateFlags     flags,
                                   const std::vector<uint32_t> &queue_family_indices) :
    Buffer(device,
           BufferBuilder<bindingType>(size)
               .with_usage(buffer_usage)
               .with_vma_usage(memory_usage)
               .with_vma_flags(flags)
               .with_queue_families(queue_family_indices)
               .with_implicit_sharing_mode())
{}

template <vkb::BindingType bindingType>
inline Buffer<bindingType>::Buffer(DeviceType &device, const BufferBuilder<bindingType> &builder) :
    AllocatedType{builder.get_allocation_create_info(), nullptr, &device}, size(builder.get_create_info().size)
{
	this->set_handle(this->create_buffer(builder.get_create_info()));
	if (!builder.get_debug_name().empty())
	{
		this->set_debug_name(builder.get_debug_name());
	}
}

template <vkb::BindingType bindingType>
inline Buffer<bindingType>::~Buffer()
{
	this->destroy_buffer(this->get_handle());
}

template <vkb::BindingType bindingType>
inline uint64_t Buffer<bindingType>::get_device_address() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return this->get_device().get_handle().getBufferAddressKHR({this->get_handle()});
	}
	else
	{
		return static_cast<vk::Device>(this->get_device().get_handle()).getBufferAddressKHR({static_cast<vk::Buffer>(this->get_handle())});
	}
}

template <vkb::BindingType bindingType>
inline typename Buffer<bindingType>::DeviceSizeType Buffer<bindingType>::get_size() const
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

}        // namespace core
}        // namespace vkb
