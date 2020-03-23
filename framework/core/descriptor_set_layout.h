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

namespace vkb
{
class DescriptorPool;
class Device;

struct ShaderResource;

/**
 * @brief Caches DescriptorSet objects for the shader's set index.
 *        Creates a DescriptorPool to allocate the DescriptorSet objects
 */
class DescriptorSetLayout
{
  public:
	/**
	 * @brief Creates a descriptor set layout from a set of resources
	 * @param device A valid Vulkan device
	 * @param resource_set A grouping of shader resources belonging to the same set
	 */
	DescriptorSetLayout(Device &device, const std::vector<ShaderResource> &resource_set);

	DescriptorSetLayout(const DescriptorSetLayout &) = delete;

	DescriptorSetLayout(DescriptorSetLayout &&other);

	~DescriptorSetLayout();

	DescriptorSetLayout &operator=(const DescriptorSetLayout &) = delete;

	DescriptorSetLayout &operator=(DescriptorSetLayout &&) = delete;

	VkDescriptorSetLayout get_handle() const;

	const std::vector<VkDescriptorSetLayoutBinding> &get_bindings() const;

	std::unique_ptr<VkDescriptorSetLayoutBinding> get_layout_binding(const uint32_t binding_index) const;

	std::unique_ptr<VkDescriptorSetLayoutBinding> get_layout_binding(const std::string &name) const;

	const std::vector<VkDescriptorBindingFlagsEXT> &get_binding_flags() const;

	VkDescriptorBindingFlagsEXT get_layout_binding_flag(const uint32_t binding_index) const;

  private:
	Device &device;

	VkDescriptorSetLayout handle{VK_NULL_HANDLE};

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	std::vector<VkDescriptorBindingFlagsEXT> binding_flags;

	std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings_lookup;

	std::unordered_map<uint32_t, VkDescriptorBindingFlagsEXT> binding_flags_lookup;

	std::unordered_map<std::string, uint32_t> resources_lookup;
};
}        // namespace vkb
