/* Copyright (c) 2019-2025, Arm Limited and Contributors
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
namespace core
{
template <vkb::BindingType bindingType>
class Device;
using DeviceC = Device<vkb::BindingType::C>;
}        // namespace core

class FencePool
{
  public:
	FencePool(vkb::core::DeviceC &device);

	FencePool(const FencePool &) = delete;

	FencePool(FencePool &&other) = delete;

	~FencePool();

	FencePool &operator=(const FencePool &) = delete;

	FencePool &operator=(FencePool &&) = delete;

	VkFence request_fence();

	VkResult wait(uint64_t timeout = std::numeric_limits<uint64_t>::max()) const;

	VkResult reset();

  private:
	vkb::core::DeviceC &device;

	std::vector<VkFence> fences;

	uint32_t active_fence_count{0};
};
}        // namespace vkb
