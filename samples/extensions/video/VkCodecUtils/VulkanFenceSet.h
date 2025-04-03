/*
 * Copyright 2023 NVIDIA Corporation.
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

#ifndef _VULKANFENCESET_H_
#define _VULKANFENCESET_H_

#include "VkCodecUtils/VulkanDeviceContext.h"
#include <vector>

class VulkanFenceSet
{
public:
    VulkanFenceSet() = default;

    VkResult CreateSet(uint32_t numFences, VkFenceCreateFlags flags = VkFenceCreateFlags(),
                       const void* pNext = nullptr) {

        DestroySet();

        m_fences.resize(numFences);
        const VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, pNext, flags };
        for (uint32_t fenceIdx = 0; fenceIdx < numFences; fenceIdx++ ) {
            VkResult result = VulkanDeviceContext::GetThe()->CreateFence(VulkanDeviceContext::GetThe()->getDevice(), &fenceInfo, nullptr, &m_fences[fenceIdx]);
            if (result != VK_SUCCESS) {
                return result;
            }
        }
        return VK_SUCCESS;
    }

    void DestroySet() {
        if (!m_fences.empty()) {
            for (auto & m_fence : m_fences) {
                if (m_fence != VK_NULL_HANDLE) {
                    VulkanDeviceContext::GetThe()->DestroyFence(VulkanDeviceContext::GetThe()->getDevice(), m_fence, nullptr);
                    m_fence = VK_NULL_HANDLE;
                }
            }
        }
    }

    VkFence GetFence(uint32_t fenceIdx = 0) const {
        if (fenceIdx < m_fences.size()) {
            return m_fences[fenceIdx];
        }
        return VK_NULL_HANDLE;
    }

    virtual ~VulkanFenceSet() {
        DestroySet();
    }

private:
    std::vector<VkFence>       m_fences;
};

#endif /* _VULKANFENCESET_H_ */
