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
 * @brief Extended buffer class to simplify ray tracing shader binding table usage
 */
class ShaderBindingTable
{
  public:
	/**
	 * @brief Creates a shader binding table
	 * @param device A valid Vulkan device
	 * @param handle_count Shader group handle count
	 * @param handle_size_aligned Aligned shader group handle size
	 * @param memory_usage The memory usage of the shader binding table
	 */
	ShaderBindingTable(Device &       device,
	                   uint32_t       handle_count,
	                   VkDeviceSize   handle_size_aligned,
	                   VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU);

	~ShaderBindingTable();

	const VkStridedDeviceAddressRegionKHR *get_strided_device_address_region() const;

	uint8_t *get_data() const;

  private:
	Device &device;

	VkStridedDeviceAddressRegionKHR strided_device_address_region{};

	uint64_t device_address{0};

	VkBuffer handle{VK_NULL_HANDLE};

	VmaAllocation allocation{VK_NULL_HANDLE};

	VkDeviceMemory memory{VK_NULL_HANDLE};

	uint8_t *mapped_data{nullptr};
};
}        // namespace core
}        // namespace vkb