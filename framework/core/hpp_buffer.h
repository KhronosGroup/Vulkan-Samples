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

#include <core/buffer.h>
#include <core/hpp_device.h>
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace core
{
class HPPDevice;

/**
 * @brief facade class around vkb::core::Buffer, providing a vulkan.hpp-based interface
 *
 * See vkb::core::Buffer for documentation
 */
class HPPBuffer : private vkb::core::Buffer
{
  public:
	using vkb::core::Buffer::convert_and_update;
	using vkb::core::Buffer::flush;
	using vkb::core::Buffer::map;
	using vkb::core::Buffer::set_debug_name;
	using vkb::core::Buffer::unmap;
	using vkb::core::Buffer::update;

	HPPBuffer(vkb::core::HPPDevice const &device,
	          vk::DeviceSize              size,
	          vk::BufferUsageFlags        buffer_usage,
	          VmaMemoryUsage              memory_usage,
	          VmaAllocationCreateFlags    flags = VMA_ALLOCATION_CREATE_MAPPED_BIT) :
	    vkb::core::Buffer(reinterpret_cast<vkb::Device const &>(device),
	                      static_cast<VkDeviceSize>(size),
	                      static_cast<VkBufferUsageFlags>(buffer_usage),
	                      memory_usage,
	                      flags)
	{}

	vk::Buffer get_handle() const
	{
		return static_cast<vk::Buffer>(vkb::core::Buffer::get_handle());
	}

	vk::DeviceSize get_size() const
	{
		return static_cast<vk::DeviceSize>(vkb::core::Buffer::get_size());
	}
};
}        // namespace core
}        // namespace vkb
