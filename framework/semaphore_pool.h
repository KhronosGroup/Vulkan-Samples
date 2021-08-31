/* Copyright (c) 2019-2021, Arm Limited and Contributors
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

class SemaphorePool
{
  public:
	explicit SemaphorePool(Device &device);

	SemaphorePool(const SemaphorePool &) = delete;

	SemaphorePool(SemaphorePool &&other) = delete;

	~SemaphorePool();

	SemaphorePool &operator=(const SemaphorePool &) = delete;

	SemaphorePool &operator=(SemaphorePool &&) = delete;

	VkSemaphore request_semaphore();
	VkSemaphore request_semaphore_with_ownership();
	void        release_owned_semaphore(VkSemaphore semaphore);

	void reset();

	uint32_t get_active_semaphore_count() const;

  private:
	Device &device;

	std::vector<VkSemaphore> semaphores;
	std::vector<VkSemaphore> released_semaphores;

	uint32_t active_semaphore_count{0};
};
}        // namespace vkb
