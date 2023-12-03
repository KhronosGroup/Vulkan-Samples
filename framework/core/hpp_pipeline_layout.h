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

#include <core/hpp_shader_module.h>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace core
{
class HPPDevice;
class HPPDescriptorSetLayout;

/**
 * @brief A wrapper class for vk::HPPPipelineLayout
 *
 */
class HPPPipelineLayout
{
  public:
	HPPPipelineLayout(vkb::core::HPPDevice &device, const std::vector<vkb::core::HPPShaderModule *> &shader_modules);
	HPPPipelineLayout(const HPPPipelineLayout &) = delete;
	HPPPipelineLayout(HPPPipelineLayout &&other);
	~HPPPipelineLayout();

	HPPPipelineLayout &operator=(const HPPPipelineLayout &) = delete;
	HPPPipelineLayout &operator=(HPPPipelineLayout &&)      = delete;

	vkb::core::HPPDescriptorSetLayout const                                       &get_descriptor_set_layout(const uint32_t set_index) const;
	vk::PipelineLayout                                                             get_handle() const;
	vk::ShaderStageFlags                                                           get_push_constant_range_stage(uint32_t size, uint32_t offset = 0) const;
	std::vector<vkb::core::HPPShaderResource>                                      get_resources(const vkb::core::HPPShaderResourceType &type  = vkb::core::HPPShaderResourceType::All,
	                                                                                             vk::ShaderStageFlagBits                 stage = vk::ShaderStageFlagBits::eAll) const;
	const std::vector<vkb::core::HPPShaderModule *>                               &get_shader_modules() const;
	const std::unordered_map<uint32_t, std::vector<vkb::core::HPPShaderResource>> &get_shader_sets() const;
	bool                                                                           has_descriptor_set_layout(const uint32_t set_index) const;

  private:
	vkb::core::HPPDevice                                                   &device;
	vk::PipelineLayout                                                      handle;
	std::vector<vkb::core::HPPShaderModule *>                               shader_modules;                // The shader modules that this pipeline layout uses
	std::unordered_map<std::string, vkb::core::HPPShaderResource>           shader_resources;              // The shader resources that this pipeline layout uses, indexed by their name
	std::unordered_map<uint32_t, std::vector<vkb::core::HPPShaderResource>> shader_sets;                   // A map of each set and the resources it owns used by the pipeline layout
	std::vector<vkb::core::HPPDescriptorSetLayout *>                        descriptor_set_layouts;        // The different descriptor set layouts for this pipeline layout
};
}        // namespace core
}        // namespace vkb
