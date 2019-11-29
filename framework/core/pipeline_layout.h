/* Copyright (c) 2019, Arm Limited and Contributors
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

	const std::vector<ShaderModule *> &get_stages() const;

	const std::unordered_map<uint32_t, std::vector<ShaderResource>> &get_bindings() const;

	const std::vector<ShaderResource> &get_set_bindings(uint32_t set_index) const;

	bool has_set_layout(uint32_t set_index) const;

	DescriptorSetLayout &get_set_layout(uint32_t set_index);

	std::vector<ShaderResource> get_vertex_input_attributes() const;

	std::vector<ShaderResource> get_fragment_output_attachments() const;

	std::vector<ShaderResource> get_fragment_input_attachments() const;

	VkShaderStageFlags get_push_constant_range_stage(uint32_t offset, uint32_t size) const;

  private:
	Device &device;

	std::vector<ShaderModule *> shader_modules;

	VkPipelineLayout handle{VK_NULL_HANDLE};

	std::map<std::string, ShaderResource> resources;

	std::unordered_map<uint32_t, std::vector<ShaderResource>> set_bindings;

	std::unordered_map<uint32_t, DescriptorSetLayout *> set_layouts;
};
}        // namespace vkb
