/*
* Copyright 2024 NVIDIA Corporation.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef _VKCODECUTILS_VULKANQUERYPOOLSET_H_
#define _VKCODECUTILS_VULKANQUERYPOOLSET_H_

#include <vector>
#include <atomic>
#include <iostream>
#include "VkCodecUtils/VulkanDeviceContext.h"

class VulkanQueryPoolSet
{
public:
    VulkanQueryPoolSet() : m_vkDevCtx(), m_queryPool(VK_NULL_HANDLE), m_queryCount(0) {}

    VkResult CreateSet(const VulkanDeviceContext* vkDevCtx, uint32_t queryCount,
                       VkQueryType queryType,
                       VkQueryPoolCreateFlags flags = VkQueryPoolCreateFlags(),
                       const void* pNext = nullptr) {

        DestroySet();

        VkQueryPoolCreateInfo queryPoolCreateInfo = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
        queryPoolCreateInfo.queryType = queryType;
        queryPoolCreateInfo.queryCount = queryCount;
        queryPoolCreateInfo.pNext = pNext;

        VkResult result = vkDevCtx->CreateQueryPool(*vkDevCtx, &queryPoolCreateInfo, NULL, &m_queryPool);
        if (result != VK_SUCCESS) {
            assert(!"Failed to create query pool!");
            return result;
        }

        m_queryCount = queryCount;
        m_vkDevCtx   = vkDevCtx;

        return VK_SUCCESS;
    }

    void DestroySet() {

        if (m_vkDevCtx && (m_queryPool != VK_NULL_HANDLE)) {
            m_vkDevCtx->DestroyQueryPool(*m_vkDevCtx, m_queryPool, NULL);
            m_queryPool = VK_NULL_HANDLE;
            m_queryCount = 0;
        }
    }

    VkQueryPool GetQueryPool(uint32_t queryIdx = 0) const {
        if (queryIdx < m_queryCount) {
            return m_queryPool;
        }
        return VK_NULL_HANDLE;
    }

    virtual ~VulkanQueryPoolSet() {
        DestroySet();
    }

private:
    const VulkanDeviceContext* m_vkDevCtx;
    VkQueryPool                m_queryPool;
    uint32_t                   m_queryCount;
};

#endif /* _VKCODECUTILS_VULKANQUERYPOOLSET_H_ */
