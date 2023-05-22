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
class HPPDevice;
class HPPShaderModule;
struct HPPShaderResource;

/**
 * @brief facade class around vkb::DescriptorSetLayout, providing a vulkan.hpp-based interface
 *
 * See vkb::DescriptorSetLayout for documentation
 */
class HPPDescriptorSetLayout : private vkb::DescriptorSetLayout
{
  public:
	using vkb::DescriptorSetLayout::get_index;

  public:
	HPPDescriptorSetLayout(vkb::core::HPPDevice                            &device,
	                       const uint32_t                                   set_index,
	                       const std::vector<vkb::core::HPPShaderModule *> &shader_modules,
	                       const std::vector<vkb::core::HPPShaderResource> &resource_set) :
	    vkb::DescriptorSetLayout(reinterpret_cast<vkb::Device &>(device),
	                             set_index,
	                             reinterpret_cast<std::vector<vkb::ShaderModule *> const &>(shader_modules),
	                             reinterpret_cast<std::vector<vkb::ShaderResource> const &>(resource_set))
	{}

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
