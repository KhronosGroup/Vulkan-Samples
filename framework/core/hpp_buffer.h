/* Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
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

#include <core/buffer.h>
#include <core/hpp_device.h>
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace core
{
/**
 * @brief wrapper class for use with vulkan.hpp in the samples of this framework
 *
 * See vkb::VulkanSample for documentation
 */
class HPPBuffer : protected vkb::core::Buffer
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
	HPPBuffer(vkb::core::HPPDevice &   device,
	          vk::DeviceSize           size,
	          vk::BufferUsageFlags     buffer_usage,
	          VmaMemoryUsage           memory_usage,
	          VmaAllocationCreateFlags flags = VMA_ALLOCATION_CREATE_MAPPED_BIT);

	/**
	 * @brief Flushes memory if it is HOST_VISIBLE and not HOST_COHERENT
	 */
	void flush() const;

	vk::Buffer get_handle() const;

	/**
	 * @return The size of the buffer
	 */
	vk::DeviceSize get_size() const;

	/**
	 * @brief Copies byte data into the buffer
	 * @param data The data to copy from
	 * @param size The amount of bytes to copy
	 * @param offset The offset to start the copying into the mapped data
	 */
	void update(uint8_t const *data, size_t size, size_t offset = 0);

	/**
	 * @brief Converts any non byte data into bytes and then updates the buffer
	 * @param data The data to copy from
	 * @param size The amount of bytes to copy
	 * @param offset The offset to start the copying into the mapped data
	 */
	void update(void *data, size_t size, size_t offset = 0);

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
};
}        // namespace core
}        // namespace vkb
