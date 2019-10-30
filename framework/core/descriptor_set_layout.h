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

namespace vkb
{
class DescriptorPool;
class Device;

struct ShaderResource;

// Caches DescriptorSet objects for the shader's set index.
// Creates a DescriptorPool to allocate the DescriptorSet objects
class DescriptorSetLayout : public NonCopyable
{
  public:
	DescriptorSetLayout(Device &device, const std::vector<ShaderResource> &set_resources);

	DescriptorSetLayout(DescriptorSetLayout &&other);

	~DescriptorSetLayout();

	VkDescriptorSetLayout get_handle() const;

	DescriptorPool &get_descriptor_pool();

	const std::vector<VkDescriptorSetLayoutBinding> &get_bindings() const;

	bool get_layout_binding(uint32_t binding_index, VkDescriptorSetLayoutBinding &binding) const;

	bool has_layout_binding(const std::string &name, VkDescriptorSetLayoutBinding &binding) const;

  private:
	Device &device;

	std::unique_ptr<DescriptorPool> descriptor_pool;

	VkDescriptorSetLayout handle{VK_NULL_HANDLE};

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings_lookup;

	std::unordered_map<std::string, uint32_t> resources_lookup;
};
}        // namespace vkb
