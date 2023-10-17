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

#include "semaphore_pool.h"

namespace vkb
{
/**
 * @brief facade class around vkb::SemaphorePool, providing a vulkan.hpp-based interface
 *
 * See vkb::SemaphorePool for documentation
 */
class HPPSemaphorePool : private vkb::SemaphorePool
{
  public:
	using vkb::SemaphorePool::reset;

	HPPSemaphorePool(vkb::core::HPPDevice &device) :
	    vkb::SemaphorePool(reinterpret_cast<vkb::Device &>(device))
	{}

	void release_owned_semaphore(vk::Semaphore semaphore)
	{
		vkb::SemaphorePool::release_owned_semaphore(static_cast<VkSemaphore>(semaphore));
	}

	vk::Semaphore request_semaphore()
	{
		return static_cast<vk::Semaphore>(vkb::SemaphorePool::request_semaphore());
	}

	vk::Semaphore request_semaphore_with_ownership()
	{
		return static_cast<vk::Semaphore>(vkb::SemaphorePool::request_semaphore_with_ownership());
	}
};
}        // namespace vkb
