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

#include "core/descriptor_set_layout.h"
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace core
{
/**
 * @brief facade class around vkb::DescriptorSetLayout, providing a vulkan.hpp-based interface
 *
 * See vkb::DescriptorSetLayout for documentation
 */
class HPPDescriptorSetLayout : private vkb::DescriptorSetLayout
{
  public:
	vk::DescriptorSetLayout get_handle() const
	{
		return static_cast<vk::DescriptorSetLayout>(vkb::DescriptorSetLayout::get_handle());
	}

	std::unique_ptr<vk::DescriptorSetLayoutBinding> get_layout_binding(const uint32_t binding_index) const
	{
		return std::unique_ptr<vk::DescriptorSetLayoutBinding>(
		    reinterpret_cast<vk::DescriptorSetLayoutBinding *>(vkb::DescriptorSetLayout::get_layout_binding(binding_index).release()));
	}
};
}        // namespace core
}        // namespace vkb
