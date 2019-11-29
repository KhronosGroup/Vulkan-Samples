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

#include <unordered_map>

#include "common/helpers.h"
#include "common/vk_common.h"

namespace vkb
{
class Device;
class DescriptorSetLayout;

/**
 * @brief Manages an array of fixed size VkDescriptorPool and is able to allocate descriptor sets
 */
class DescriptorPool
{
  public:
	static const uint32_t MAX_SETS_PER_POOL = 16;

	DescriptorPool(Device &                   device,
	               const DescriptorSetLayout &descriptor_set_layout,
	               uint32_t                   pool_size = MAX_SETS_PER_POOL);

	DescriptorPool(const DescriptorPool &) = delete;

	DescriptorPool(DescriptorPool &&) = default;

	~DescriptorPool();

	DescriptorPool &operator=(const DescriptorPool &) = delete;

	DescriptorPool &operator=(DescriptorPool &&) = delete;

	void reset();

	const DescriptorSetLayout &get_descriptor_set_layout() const;

	void set_descriptor_set_layout(const DescriptorSetLayout &set_layout);

	VkDescriptorSet allocate();

	VkResult free(VkDescriptorSet descriptor_set);

  private:
	Device &device;

	const DescriptorSetLayout *descriptor_set_layout{nullptr};

	// Descriptor pool size
	std::vector<VkDescriptorPoolSize> pool_sizes;

	// Number of sets to allocate for each pool
	uint32_t pool_max_sets{0};

	// Total descriptor pools created
	std::vector<VkDescriptorPool> pools;

	// Count sets for each pool
	std::vector<uint32_t> pool_sets_count;

	// Current pool index to allocate descriptor set
	uint32_t pool_index{0};

	// Map between descriptor set and pool index
	std::unordered_map<VkDescriptorSet, uint32_t> set_pool_mapping;

	// Find next pool index or create new pool
	uint32_t find_available_pool(uint32_t pool_index);
};
}        // namespace vkb
