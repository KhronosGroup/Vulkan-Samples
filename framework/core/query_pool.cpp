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

#include "query_pool.h"

#include "device.h"

namespace vkb
{
QueryPool::QueryPool(Device &d, const VkQueryPoolCreateInfo &info) :
    device{d}
{
	VK_CHECK(vkCreateQueryPool(device.get_handle(), &info, nullptr, &handle));
}

QueryPool::QueryPool(QueryPool &&other) :
    device{other.device},
    handle{other.handle}
{
	other.handle = VK_NULL_HANDLE;
}

QueryPool::~QueryPool()
{
	if (handle != VK_NULL_HANDLE)
	{
		vkDestroyQueryPool(device.get_handle(), handle, nullptr);
	}
}

VkQueryPool QueryPool::get_handle() const
{
	assert(handle != VK_NULL_HANDLE && "QueryPool handle is invalid");
	return handle;
}

void QueryPool::host_reset(uint32_t firstQuery, uint32_t queryCount)
{
	assert(device.is_enabled("VK_EXT_host_query_reset") &&
	       "VK_EXT_host_query_reset needs to be enabled to call QueryPool::host_reset");

	vkResetQueryPoolEXT(device.get_handle(), get_handle(), firstQuery, queryCount);
}

VkResult QueryPool::get_results(uint32_t first_query, uint32_t num_queries,
                                size_t result_bytes, void *results, VkDeviceSize stride,
                                VkQueryResultFlags flags)
{
	return vkGetQueryPoolResults(device.get_handle(), get_handle(), first_query, num_queries,
	                             result_bytes, results, stride, flags);
}

}        // namespace vkb
