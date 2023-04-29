/* Copyright (c) 2021-2023, NVIDIA CORPORATION. All rights reserved.
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

#include "hpp_vulkan_resource.h"

#include <unordered_map>
#include <vk_mem_alloc.h>

namespace vkb
{
namespace core
{
class HPPBuffer : public vkb::core::HPPVulkanResource<vk::Buffer>
{
  public:
	/**
	 * @brief Creates a buffer using VMA
	 * @param device A valid Vulkan device
	 * @param size The size in bytes of the buffer
	 * @param buffer_usage The usage flags for the VkBuffer
	 * @param memory_usage The memory usage of the buffer
	 * @param flags The allocation create flags
	 * @param queue_family_indices optional queue family indices
	 */
	HPPBuffer(vkb::core::HPPDevice        &device,
	          vk::DeviceSize               size,
	          vk::BufferUsageFlags         buffer_usage,
	          VmaMemoryUsage               memory_usage,
	          VmaAllocationCreateFlags     flags                = VMA_ALLOCATION_CREATE_MAPPED_BIT,
	          const std::vector<uint32_t> &queue_family_indices = {});

	HPPBuffer(const HPPBuffer &) = delete;
	HPPBuffer(HPPBuffer &&other);

	~HPPBuffer();

	HPPBuffer &operator=(const HPPBuffer &) = delete;
	HPPBuffer &operator=(HPPBuffer &&)      = delete;

	VmaAllocation  get_allocation() const;
	const uint8_t *get_data() const;
	VkDeviceMemory get_memory() const;

	/**
	 * @return Return the buffer's device address (note: requires that the buffer has been created with the VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT usage fla)
	 */
	uint64_t get_device_address() const;

	/**
	 * @return The size of the buffer
	 */
	vk::DeviceSize get_size() const;

	/**
	 * @brief Flushes memory if it is HOST_VISIBLE and not HOST_COHERENT
	 */
	void flush();

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

  private:
	VmaAllocation    allocation  = VK_NULL_HANDLE;
	vk::DeviceMemory memory      = nullptr;
	vk::DeviceSize   size        = 0;
	uint8_t         *mapped_data = nullptr;
	bool             persistent  = false;        // Whether the buffer is persistently mapped or not
	bool             mapped      = false;        // Whether the buffer has been mapped with vmaMapMemory
};
}        // namespace core
}        // namespace vkb
