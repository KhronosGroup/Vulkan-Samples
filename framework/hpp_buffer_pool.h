/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#include "buffer_pool.h"
#include <core/hpp_buffer.h>

namespace vkb
{
/**
 * @brief facade class around vkb::BufferAllocation, providing a vulkan.hpp-based interface
 *
 * See vkb::BufferAllocation for documentation
 */
class HPPBufferAllocation : private vkb::BufferAllocation
{
  public:
	using vkb::BufferAllocation::update;

  public:
	vkb::core::HPPBuffer &get_buffer()
	{
		return reinterpret_cast<vkb::core::HPPBuffer &>(vkb::BufferAllocation::get_buffer());
	}

	vk::DeviceSize get_offset() const
	{
		return static_cast<vk::DeviceSize>(vkb::BufferAllocation::get_offset());
	}

	vk::DeviceSize get_size() const
	{
		return static_cast<vk::DeviceSize>(vkb::BufferAllocation::get_size());
	}
};

/**
 * @brief facade class around vkb::BufferBlock, providing a vulkan.hpp-based interface
 *
 * See vkb::BufferBlock for documentation
 */
class HPPBufferBlock : private vkb::BufferBlock
{
  public:
	vkb::HPPBufferAllocation allocate(vk::DeviceSize size)
	{
		vkb::BufferAllocation ba = vkb::BufferBlock::allocate(static_cast<VkDeviceSize>(size));
		return std::move(*(reinterpret_cast<vkb::HPPBufferAllocation *>(&ba)));
	}

	bool can_allocate(vk::DeviceSize size) const
	{
		return vkb::BufferBlock::can_allocate(static_cast<VkDeviceSize>(size));
	}
};

/**
 * @brief facade class around vkb::BufferPool, providing a vulkan.hpp-based interface
 *
 * See vkb::BufferPool for documentation
 */
class HPPBufferPool : private vkb::BufferPool
{
  public:
	using vkb::BufferPool::reset;

	HPPBufferPool(
	    vkb::core::HPPDevice &device, vk::DeviceSize block_size, vk::BufferUsageFlags usage, VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU) :
	    vkb::BufferPool(
	        reinterpret_cast<vkb::Device &>(device), static_cast<VkDeviceSize>(block_size), static_cast<VkBufferUsageFlags>(usage), memory_usage)
	{
	}

	vkb::HPPBufferBlock &request_buffer_block(vk::DeviceSize minimum_size, bool minimal = false)
	{
		return reinterpret_cast<vkb::HPPBufferBlock &>(vkb::BufferPool::request_buffer_block(static_cast<VkDeviceSize>(minimum_size), minimal));
	}
};
}        // namespace vkb
