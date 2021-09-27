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

#pragma once

#include "common/helpers.h"
#include "common/vk_common.h"

namespace vkb
{
class Device;

namespace core
{
class Buffer
{
  public:
	/**
	 * @brief Creates a buffer using VMA
	 * @param device A valid Vulkan device
	 * @param size The size in bytes of the buffer
	 * @param buffer_usage The usage flags for the VkBuffer
	 * @param memory_usage The memory usage of the buffer
	 * @param flags The allocation create flags
	 */
	Buffer(Device &                 device,
	       VkDeviceSize             size,
	       VkBufferUsageFlags       buffer_usage,
	       VmaMemoryUsage           memory_usage,
	       VmaAllocationCreateFlags flags = VMA_ALLOCATION_CREATE_MAPPED_BIT);

	Buffer(const Buffer &) = delete;

	Buffer(Buffer &&other);

	~Buffer();

	Buffer &operator=(const Buffer &) = delete;

	Buffer &operator=(Buffer &&) = delete;

	template <typename T>
	static std::vector<T> copy(std::unordered_map<std::string, vkb::core::Buffer> &buffers, const char *buffer_name)
	{
		auto iter = buffers.find(buffer_name);
		if (iter == buffers.cend())
		{
			return {};
		}
		auto &         buffer = iter->second;
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

	const Device &get_device() const;

	VkBuffer get_handle() const;

	const VkBuffer *get() const;

	VmaAllocation get_allocation() const;

	VkDeviceMemory get_memory() const;

	/**
	 * @brief Flushes memory if it is HOST_VISIBLE and not HOST_COHERENT
	 */
	void flush() const;

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
	 * @return The size of the buffer
	 */
	VkDeviceSize get_size() const;

	const uint8_t *get_data() const
	{
		return mapped_data;
	}

	/**
	 * @brief Copies byte data into the buffer
	 * @param data The data to copy from
	 * @param size The amount of bytes to copy
	 * @param offset The offset to start the copying into the mapped data
	 */
	void update(const uint8_t *data, size_t size, size_t offset = 0);

	/**
	 * @brief Converts any non byte data into bytes and then updates the buffer
	 * @param data The data to copy from
	 * @param size The amount of bytes to copy
	 * @param offset The offset to start the copying into the mapped data
	 */
	void update(void *data, size_t size, size_t offset = 0);

	/**
	 * @brief Copies a vector of bytes into the buffer
	 * @param data The data vector to upload
	 * @param offset The offset to start the copying into the mapped data
	 */
	void update(const std::vector<uint8_t> &data, size_t offset = 0);

	/**
	 * @brief Copies an object as byte data into the buffer
	 * @param object The object to convert into byte data
	 * @param offset The offset to start the copying into the mapped data
	 */
	template <class T>
	void convert_and_update(const T &object, size_t offset = 0)
	{
		update(reinterpret_cast<const uint8_t *>(&object), sizeof(T), offset);
	}

	/**
	 * @return Return the buffer's device address (note: requires that the buffer has been created with the VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT usage fla)
	 */
	uint64_t get_device_address();

  private:
	Device &device;

	VkBuffer handle{VK_NULL_HANDLE};

	VmaAllocation allocation{VK_NULL_HANDLE};

	VkDeviceMemory memory{VK_NULL_HANDLE};

	VkDeviceSize size{0};

	uint8_t *mapped_data{nullptr};

	/// Whether the buffer is persistently mapped or not
	bool persistent{false};

	/// Whether the buffer has been mapped with vmaMapMemory
	bool mapped{false};
};
}        // namespace core
}        // namespace vkb
