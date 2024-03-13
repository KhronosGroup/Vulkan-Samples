/* Copyright (c) 2019-2024, Arm Limited and Contributors
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

#include "common/helpers.h"
#include "common/vk_common.h"
#include "core/allocated.h"
#include "core/vulkan_resource.h"

namespace vkb
{
class Device;

namespace core
{

class Buffer;
using BufferPtr = std::unique_ptr<Buffer>;

struct BufferBuilder : public allocated::Builder<BufferBuilder, VkBufferCreateInfo>
{
	BufferBuilder(VkDeviceSize size) :
	    Builder(VkBufferCreateInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, nullptr, 0, size})
	{
	}

	BufferBuilder &with_flags(VkBufferCreateFlags flags)
	{
		create_info.flags = flags;
		return *this;
	}

	BufferBuilder &with_usage(VkBufferUsageFlags usage)
	{
		create_info.usage = usage;
		return *this;
	}

	BufferBuilder &with_sharing_mode(VkSharingMode sharing_mode)
	{
		create_info.sharingMode = sharing_mode;
		return *this;
	}

	BufferBuilder &with_implicit_sharing_mode()
	{
		if (create_info.queueFamilyIndexCount != 0)
		{
			create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
		}
		return *this;
	}

	Buffer    build(Device const &device) const;
	BufferPtr build_unique(Device const &device) const;
};

class Buffer : public allocated::Allocated<VkBuffer>
{
  public:
	static Buffer create_staging_buffer(Device const &device, VkDeviceSize size, const void *data);

	template <typename T>
	static Buffer create_staging_buffer(Device const &device, const std::vector<T> &data)
	{
		return create_staging_buffer(device, data.size() * sizeof(T), data.data());
	}

	template <typename T>
	static Buffer create_staging_buffer(Device const &device, const T &data)
	{
		return create_staging_buffer(device, sizeof(T), &data);
	}

	/**
	 * @brief Creates a buffer using VMA
	 * @param device A valid Vulkan device
	 * @param size The size in bytes of the buffer
	 * @param buffer_usage The usage flags for the VkBuffer
	 * @param memory_usage The memory usage of the buffer
	 * @param flags The allocation create flags
	 * @param queue_family_indices optional queue family indices
	 */
	// [[deprecated]]
	Buffer(Device const                &device,
	       VkDeviceSize                 size,
	       VkBufferUsageFlags           buffer_usage,
	       VmaMemoryUsage               memory_usage,
	       VmaAllocationCreateFlags     flags                = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
	       const std::vector<uint32_t> &queue_family_indices = {});

	Buffer(Device const &device, const BufferBuilder &builder);

	Buffer(const Buffer &) = delete;

	Buffer(Buffer &&other) noexcept;

	~Buffer();

	Buffer &operator=(const Buffer &) = delete;

	Buffer &operator=(Buffer &&) = delete;

	// FIXME should include a stride parameter, because if you want to copy out of a
	// uniform buffer that's dynamically indexed, you need to account for the aligned
	// offset of the T values
	template <typename T>
	static std::vector<T> copy(std::unordered_map<std::string, vkb::core::Buffer> &buffers, const char *buffer_name)
	{
		auto iter = buffers.find(buffer_name);
		if (iter == buffers.cend())
		{
			return {};
		}
		auto          &buffer = iter->second;
		std::vector<T> out;

		const size_t sz = buffer.get_size();
		out.resize(sz / sizeof(T));
		const bool already_mapped = buffer.get_data() != nullptr;
		if (!already_mapped)
		{
			buffer.map();
		}
		memcpy(&out[0], buffer.get_data(), sz);
		if (!already_mapped)
		{
			buffer.unmap();
		}
		return out;
	}

	/**
	 * @return The size of the buffer
	 */
	VkDeviceSize get_size() const;

	/**
	 * @return Return the buffer's device address (note: requires that the buffer has been created with the VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT usage fla)
	 */
	uint64_t get_device_address();

  private:
	VkDeviceSize size{0};
};
}        // namespace core
}        // namespace vkb
