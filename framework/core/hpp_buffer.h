/* Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "allocated.h"
#include "buffer.h"
#include "common/hpp_error.h"
#include "hpp_allocated.h"
#include "vulkan_resource.h"
#include <unordered_map>

namespace vkb
{
namespace core
{

class HPPBuffer;
using HPPBufferPtr = std::unique_ptr<HPPBuffer>;

struct HPPBufferBuilder : public allocated::HPPBuilder<HPPBufferBuilder, vk::BufferCreateInfo>
{
  private:
	using Parent = HPPBuilder<HPPBufferBuilder, vk::BufferCreateInfo>;

  public:
	HPPBufferBuilder(vk::DeviceSize size) :
	    HPPBuilder(vk::BufferCreateInfo{{}, size})
	{
	}

	HPPBufferBuilder &with_usage(vk::BufferUsageFlags usage)
	{
		create_info.usage = usage;
		return *this;
	}

	HPPBufferBuilder &with_flags(vk::BufferCreateFlags flags)
	{
		create_info.flags = flags;
		return *this;
	}

	HPPBuffer    build(HPPDevice &device) const;
	HPPBufferPtr build_unique(HPPDevice &device) const;
};

class HPPBuffer : public allocated::HPPAllocated<vk::Buffer>
{
	using Parent = allocated::HPPAllocated<vk::Buffer>;

  public:
	static HPPBuffer create_staging_buffer(HPPDevice &device, vk::DeviceSize size, const void *data);

	template <typename T>
	static HPPBuffer create_staging_buffer(HPPDevice &device, std::vector<T> const &data)
	{
		return create_staging_buffer(device, data.size() * sizeof(T), data.data());
	}

	HPPBuffer()                             = delete;
	HPPBuffer(const HPPBuffer &)            = delete;
	HPPBuffer(HPPBuffer &&other)            = default;
	HPPBuffer &operator=(const HPPBuffer &) = delete;
	HPPBuffer &operator=(HPPBuffer &&)      = default;

	/**
	 * @brief Creates a buffer using VMA
	 * @param device A valid Vulkan device
	 * @param size The size in bytes of the buffer
	 * @param buffer_usage The usage flags for the VkBuffer
	 * @param memory_usage The memory usage of the buffer
	 * @param flags The allocation create flags
	 * @param queue_family_indices optional queue family indices
	 */
	// [[deprecated("Use the HPPBufferBuilder ctor instead")]]
	HPPBuffer(HPPDevice                   &device,
	          vk::DeviceSize               size,
	          vk::BufferUsageFlags         buffer_usage,
	          VmaMemoryUsage               memory_usage,
	          VmaAllocationCreateFlags     flags                = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
	          const std::vector<uint32_t> &queue_family_indices = {});

	HPPBuffer(vkb::core::HPPDevice &device, HPPBufferBuilder const &builder);

	~HPPBuffer();

	/**
	 * @return Return the buffer's device address (note: requires that the buffer has been created with the VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT usage fla)
	 */
	uint64_t get_device_address() const;

	/**
	 * @return The size of the buffer
	 */
	vk::DeviceSize get_size() const;

  private:
	vk::DeviceSize size = 0;
};

}        // namespace core
}        // namespace vkb
