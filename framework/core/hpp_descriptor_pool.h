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

#include "descriptor_pool.h"
#include <core/hpp_descriptor_set_layout.h>

namespace vkb
{
namespace core
{
class HPPDevice;

/**
 * @brief facade class around vkb::DescriptorPool, providing a vulkan.hpp-based interface
 *
 * See vkb::DescriptorPool for documentation
 */
class HPPDescriptorPool : private vkb::DescriptorPool
{
  public:
	HPPDescriptorPool(vkb::core::HPPDevice &device, const vkb::core::HPPDescriptorSetLayout &descriptor_set_layout, uint32_t pool_size = MAX_SETS_PER_POOL) :
	    vkb::DescriptorPool(reinterpret_cast<vkb::Device &>(device), reinterpret_cast<vkb::DescriptorSetLayout const &>(descriptor_set_layout), pool_size)
	{}
};
}        // namespace core
}        // namespace vkb
