/* Copyright (c) 2021-2022, NVIDIA CORPORATION. All rights reserved.
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

#include <core/queue.h>

namespace vkb
{
namespace core
{
/**
 * @brief facade class around vkb::Queue, providing a vulkan.hpp-based interface
 *
 * See vkb::Queue for documentation
 */
class HPPQueue : protected vkb::Queue
{
  public:
	using vkb::Queue::get_family_index;

	vk::Queue get_handle() const
	{
		return vkb::Queue::get_handle();
	}

	vk::Result present(const vk::PresentInfoKHR &present_infos) const
	{
		return static_cast<vk::Result>(vkb::Queue::present(reinterpret_cast<VkPresentInfoKHR const &>(present_infos)));
	}

	vk::Result wait_idle() const
	{
		return static_cast<vk::Result>(vkb::Queue::wait_idle());
	}
};
}        // namespace core
}        // namespace vkb
