/* Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
 * Copyright (c) 2024, Bradley Austin Davis. All rights reserved.
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

#include "allocated.h"
#include "common/error.h"

namespace vkb
{

namespace allocated
{

VmaAllocator &get_memory_allocator()
{
	static VmaAllocator memory_allocator = VK_NULL_HANDLE;
	return memory_allocator;
}

void init(const VmaAllocatorCreateInfo &create_info)
{
	auto &allocator = get_memory_allocator();
	if (allocator == VK_NULL_HANDLE)
	{
		VkResult result = vmaCreateAllocator(&create_info, &allocator);
		if (result != VK_SUCCESS)
		{
			throw VulkanException{result, "Cannot create allocator"};
		}
	}
}

void shutdown()
{
	auto &allocator = get_memory_allocator();
	if (allocator != VK_NULL_HANDLE)
	{
		VmaTotalStatistics stats;
		vmaCalculateStatistics(allocator, &stats);
		LOGI("Total device memory leaked: {} bytes.", stats.total.statistics.allocationBytes);
		vmaDestroyAllocator(allocator);
		allocator = VK_NULL_HANDLE;
	}
}

}        // namespace allocated
}        // namespace vkb
