/* Copyright (c) 2019-2023, Arm Limited and Contributors
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

#include "fence_pool.h"

#include "core/device.h"

namespace vkb
{
FencePool::FencePool(Device &device) :
    device{device}
{
}

FencePool::~FencePool()
{
	wait();
	reset();

	// Destroy all fences
	for (VkFence fence : fences)
	{
		vkDestroyFence(device.get_handle(), fence, nullptr);
	}

	fences.clear();
}

VkFence FencePool::request_fence()
{
	// Check if there is an available fence
	if (active_fence_count < fences.size())
	{
		return fences[active_fence_count++];
	}

	VkFence fence{VK_NULL_HANDLE};

	VkFenceCreateInfo create_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};

	VkResult result = vkCreateFence(device.get_handle(), &create_info, nullptr, &fence);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create fence.");
	}

	fences.push_back(fence);

	active_fence_count++;

	return fences.back();
}

VkResult FencePool::wait(uint32_t timeout) const
{
	if (active_fence_count < 1 || fences.empty())
	{
		return VK_SUCCESS;
	}

	return vkWaitForFences(device.get_handle(), active_fence_count, fences.data(), true, timeout);
}

VkResult FencePool::reset()
{
	if (active_fence_count < 1 || fences.empty())
	{
		return VK_SUCCESS;
	}

	VkResult result = vkResetFences(device.get_handle(), active_fence_count, fences.data());

	if (result != VK_SUCCESS)
	{
		return result;
	}

	active_fence_count = 0;

	return VK_SUCCESS;
}
}        // namespace vkb
