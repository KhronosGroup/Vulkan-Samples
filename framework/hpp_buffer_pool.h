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
};

/**
 * @brief facade class around vkb::BufferBlock, providing a vulkan.hpp-based interface
 *
 * See vkb::BufferBlock for documentation
 */
class HPPBufferBlock : private vkb::BufferBlock
{
};

/**
 * @brief facade class around vkb::BufferPool, providing a vulkan.hpp-based interface
 *
 * See vkb::BufferPool for documentation
 */
class HPPBufferPool : private vkb::BufferPool
{
};
}        // namespace vkb
