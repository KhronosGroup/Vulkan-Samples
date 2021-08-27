/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

namespace vkb
{
class Device;

class FencePool
{
  public:
	explicit FencePool(Device &device);

	FencePool(const FencePool &) = delete;

	FencePool(FencePool &&other) = delete;

	~FencePool();

	FencePool &operator=(const FencePool &) = delete;

	FencePool &operator=(FencePool &&) = delete;

	VkFence request_fence();

	VkResult wait(uint32_t timeout = std::numeric_limits<uint32_t>::max()) const;

	VkResult reset();

  private:
	Device &device;

	std::vector<VkFence> fences;

	uint32_t active_fence_count{0};
};
}        // namespace vkb
