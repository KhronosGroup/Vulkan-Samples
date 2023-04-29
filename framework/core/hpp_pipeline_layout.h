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

#include "core/pipeline_layout.h"
#include <core/hpp_descriptor_set_layout.h>
#include <core/hpp_shader_module.h>

namespace vkb
{
namespace core
{
/**
 * @brief facade class around vkb::core::PipelineLayout, providing a vulkan.hpp-based interface
 *
 * See vkb::core::PipelineLayout for documentation
 */
class HPPPipelineLayout : private vkb::PipelineLayout
{
  public:
	using vkb::PipelineLayout::has_descriptor_set_layout;

  public:
	HPPPipelineLayout(vkb::core::HPPDevice &device, const std::vector<vkb::core::HPPShaderModule *> &shader_modules) :
	    vkb::PipelineLayout(reinterpret_cast<vkb::Device &>(device), reinterpret_cast<std::vector<vkb::ShaderModule *> const &>(shader_modules))
	{}

	vkb::core::HPPDescriptorSetLayout &get_descriptor_set_layout(const uint32_t set_index) const
	{
		return reinterpret_cast<vkb::core::HPPDescriptorSetLayout &>(vkb::PipelineLayout::get_descriptor_set_layout(set_index));
	}

	vk::PipelineLayout get_handle() const
	{
		return static_cast<vk::PipelineLayout>(vkb::PipelineLayout::get_handle());
	}

	vk::ShaderStageFlags get_push_constant_range_stage(uint32_t size, uint32_t offset = 0) const
	{
		return static_cast<vk::ShaderStageFlags>(vkb::PipelineLayout::get_push_constant_range_stage(size, offset));
	}

	const std::unordered_map<uint32_t, std::vector<vkb::core::HPPShaderResource>> &get_shader_sets() const
	{
		return reinterpret_cast<std::unordered_map<uint32_t, std::vector<vkb::core::HPPShaderResource>> const &>(vkb::PipelineLayout::get_shader_sets());
	}
};
}        // namespace core
}        // namespace vkb
