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

#include "core/query_pool.h"

namespace vkb
{
namespace core
{
/**
 * @brief facade class around vkb::QueryPool, providing a vulkan.hpp-based interface
 *
 * See vkb::QueryPool for documentation
 */
class HPPQueryPool : private vkb::QueryPool
{
  public:
	vk::QueryPool get_handle() const
	{
		return static_cast<vk::QueryPool>(vkb::QueryPool::get_handle());
	}
};
}        // namespace core
}        // namespace vkb
