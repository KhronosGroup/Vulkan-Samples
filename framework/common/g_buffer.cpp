/* Copyright (c) 2026, NVIDIA CORPORATION. All rights reserved.
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

#include "common/g_buffer.h"

namespace vkb
{
namespace gbuffer
{
std::vector<vkb::common::LoadStoreInfoC> get_load_all_store_swapchain()
{
	// Load every attachment and store only swapchain
	std::vector<vkb::common::LoadStoreInfoC> load_store{4};

	// Swapchain
	load_store[0].load_op  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	load_store[0].store_op = VK_ATTACHMENT_STORE_OP_STORE;

	// Depth
	load_store[1].load_op  = VK_ATTACHMENT_LOAD_OP_LOAD;
	load_store[1].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Albedo
	load_store[2].load_op  = VK_ATTACHMENT_LOAD_OP_LOAD;
	load_store[2].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Normal
	load_store[3].load_op  = VK_ATTACHMENT_LOAD_OP_LOAD;
	load_store[3].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	return load_store;
}

std::vector<vkb::common::LoadStoreInfoC> get_clear_all_store_swapchain()
{
	// Clear every attachment and store only swapchain
	std::vector<vkb::common::LoadStoreInfoC> load_store{4};

	// Swapchain
	load_store[0].load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	load_store[0].store_op = VK_ATTACHMENT_STORE_OP_STORE;

	// Depth
	load_store[1].load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	load_store[1].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Albedo
	load_store[2].load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	load_store[2].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Normal
	load_store[3].load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	load_store[3].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	return load_store;
}

std::vector<vkb::common::LoadStoreInfoC> get_clear_store_all()
{
	// Clear and store every attachment
	std::vector<vkb::common::LoadStoreInfoC> load_store{4};

	// Swapchain
	load_store[0].load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	load_store[0].store_op = VK_ATTACHMENT_STORE_OP_STORE;

	// Depth
	load_store[1].load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	load_store[1].store_op = VK_ATTACHMENT_STORE_OP_STORE;

	// Albedo
	load_store[2].load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	load_store[2].store_op = VK_ATTACHMENT_STORE_OP_STORE;

	// Normal
	load_store[3].load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	load_store[3].store_op = VK_ATTACHMENT_STORE_OP_STORE;

	return load_store;
}

std::vector<VkClearValue> get_clear_value()
{
	// Clear values
	std::vector<VkClearValue> clear_value{4};
	clear_value[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clear_value[1].depthStencil = {0.0f, ~0U};
	clear_value[2].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clear_value[3].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};

	return clear_value;
}
}        // namespace gbuffer
}        // namespace vkb
