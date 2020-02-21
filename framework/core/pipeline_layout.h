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
#include "common/vk_common.h"
#include "core/descriptor_set_layout.h"
#include "core/shader_module.h"

namespace vkb
{
class Device;
class ShaderModule;
class DescriptorSetLayout;

class PipelineLayout
{
  public:
	PipelineLayout(Device &device, const std::vector<ShaderModule *> &shader_modules);

	PipelineLayout(const PipelineLayout &) = delete;

	PipelineLayout(PipelineLayout &&other);

	~PipelineLayout();

	PipelineLayout &operator=(const PipelineLayout &) = delete;

	PipelineLayout &operator=(PipelineLayout &&) = delete;

	VkPipelineLayout get_handle() const;

	const std::vector<ShaderModule *> &get_shader_modules() const;

	const std::vector<ShaderResource> get_resources(const ShaderResourceType &type = ShaderResourceType::All, VkShaderStageFlagBits stage = VK_SHADER_STAGE_ALL) const;

	const std::unordered_map<uint32_t, std::vector<ShaderResource>> &get_shader_sets() const;

	bool has_descriptor_set_layout(uint32_t set_index) const;

	DescriptorSetLayout &get_descriptor_set_layout(uint32_t set_index) const;

	VkShaderStageFlags get_push_constant_range_stage(uint32_t offset, uint32_t size) const;

  private:
	Device &device;

	VkPipelineLayout handle{VK_NULL_HANDLE};

	// The shader modules that this pipeline layout uses
	std::vector<ShaderModule *> shader_modules;

	// The shader resources that this pipeline layout uses, indexed by their name
	std::unordered_map<std::string, ShaderResource> shader_resources;

	// A map of each set and the resources it owns used by the pipeline layout
	std::unordered_map<uint32_t, std::vector<ShaderResource>> shader_sets;

	// The different descriptor set layouts for this pipeline layout
	std::unordered_map<uint32_t, DescriptorSetLayout *> descriptor_set_layouts;
};
}        // namespace vkb
