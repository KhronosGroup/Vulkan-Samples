/* Copyright (c) 2020, Broadcom Inc. and Contributors
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
class Device;

/**
 * @brief Represents a Vulkan Query Pool
 */
class QueryPool
{
  public:
	/**
	 * @brief Creates a Vulkan Query Pool
	 * @param d The device to use
	 * @param info Creation details
	 */
	QueryPool(Device &d, const VkQueryPoolCreateInfo &info);

	QueryPool(const QueryPool &) = delete;

	QueryPool(QueryPool &&pool);

	~QueryPool();

	QueryPool &operator=(const QueryPool &) = delete;

	QueryPool &operator=(QueryPool &&) = delete;

	/**
	 * @return The vulkan query pool handle
	 */
	VkQueryPool get_handle() const;

	/**
	 * @brief Reset a range of queries in the query pool. Only call if VK_EXT_host_query_reset is enabled.
	 * @param pool The query pool
	 * @param firstQuery The first query to reset
	 * @param queryCount The number of queries to reset
	 */
	void host_reset(uint32_t firstQuery, uint32_t queryCount);

	/**
	 * @brief Get query pool results
	 * @param first_query The initial query index
	 * @param num_queries The number of queries to read
	 * @param results Result vector, must be large enough to hold results
	 * @param stride The stride in bytes between results for individual queries
	 * @param flags A bitmask of VkQueryResultFlagBits
	 */
	VkResult get_results(uint32_t first_query, uint32_t num_queries,
	                     size_t result_bytes, void *results, VkDeviceSize stride,
	                     VkQueryResultFlags flags);

  private:
	Device &device;

	VkQueryPool handle{VK_NULL_HANDLE};
};
}        // namespace vkb
