/* Copyright (c) 2021, Sascha Willems
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
/**
 * @brief A simplied buffer class for creating temporary device local scratch buffers, used in e.g. ray tracing
 */
class ScratchBuffer
{
  public:
	/**
	 * @brief Creates a scratch buffer using VMA with pre-defined usage flags
	 * @param device A valid Vulkan device
	 * @param size The size in bytes of the buffer
	 */
	ScratchBuffer(Device &     device,
	              VkDeviceSize size);

	~ScratchBuffer();

	VkBuffer get_handle() const;

	uint64_t get_device_address() const;

	/**
	 * @return The size of the buffer
	 */
	VkDeviceSize get_size() const
	{
		return size;
	}

  private:
	Device &device;

	uint64_t device_address{0};

	VkBuffer handle{VK_NULL_HANDLE};

	VmaAllocation allocation{VK_NULL_HANDLE};

	VkDeviceMemory memory{VK_NULL_HANDLE};

	VkDeviceSize size{0};
};
}        // namespace core
}        // namespace vkb