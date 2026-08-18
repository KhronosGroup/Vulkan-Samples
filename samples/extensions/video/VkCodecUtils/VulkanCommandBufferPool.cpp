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

#include "vulkan_interfaces.h"
#include "VkCodecUtils/VulkanDeviceContext.h"
#include "VkVideoCore/VkVideoCoreProfile.h"
#include "VkCodecUtils/VulkanCommandBufferPool.h"

int32_t VulkanCommandBufferPool::PoolNode::Release()
{
    uint32_t ret = --m_refCount;
    if (ret == 1) {
        m_parent->ReleasePoolNodeToPool(m_parentIndex);
        m_parentIndex = -1;
        m_parent = nullptr;
    } else if (ret == 0) {
        // Destroy the resources if ref-count reaches zero
    }
    return ret;
}


VkResult VulkanCommandBufferPool::PoolNode::Init(const VulkanDeviceContext* vkDevCtx)
{
    AddRef();
    m_vkDevCtx = vkDevCtx;
    return VK_SUCCESS;
}

VkResult VulkanCommandBufferPool::PoolNode::SetParent(VulkanCommandBufferPool* cmdBuffPool, int32_t parentIndex)
{
    assert(m_parent == nullptr);
    m_parent      = cmdBuffPool;
    assert(m_parentIndex == -1);
    m_parentIndex = parentIndex;

    return VK_SUCCESS;
}

void VulkanCommandBufferPool::PoolNode::Deinit()
{
    Release();
    m_vkDevCtx = nullptr;
}


bool VulkanCommandBufferPool::GetAvailablePoolNode(VkSharedBaseObj<PoolNode>& poolNode)
{
    int32_t availablePoolNodeIndx = -1;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (m_nextNodeToUse >= m_poolSize) {
            m_nextNodeToUse = 0;
        }
        bool retryFirstPoolPartition = false;
        do {
            for (uint32_t i = m_nextNodeToUse; i < m_poolSize; i++) {
                if (m_availablePoolNodes & (1ULL << i)) {
                    m_nextNodeToUse = i + 1;
                    m_availablePoolNodes &= ~(1ULL << i);
                    availablePoolNodeIndx = i;
                    break;
                }
            }

            if ((availablePoolNodeIndx == -1) && (m_nextNodeToUse > 0)) {
                // Search again the first part of the bit array
                m_nextNodeToUse = 0;
                retryFirstPoolPartition = true;
            } else {
                retryFirstPoolPartition = false;
                break;
            }

        } while (retryFirstPoolPartition);
    }
    if (availablePoolNodeIndx != -1) {
        m_poolNodes[availablePoolNodeIndx].SetParent(this, availablePoolNodeIndx);
        poolNode = &m_poolNodes[availablePoolNodeIndx];
        return true;
    }
    return false;
}

bool VulkanCommandBufferPool::ReleasePoolNodeToPool(uint32_t poolNodeIndex)
{
    std::lock_guard<std::mutex> lock(m_queueMutex);

    assert(!(m_availablePoolNodes & (1ULL << poolNodeIndex)));
    m_availablePoolNodes |= (1ULL << poolNodeIndex);

    return true;
}

VkResult VulkanCommandBufferPool::Create(const VulkanDeviceContext* vkDevCtx,
                                         VkSharedBaseObj<VulkanCommandBufferPool>& cmdBuffPool)
{
    VkSharedBaseObj<VulkanCommandBufferPool> _cmdBuffPool(new VulkanCommandBufferPool());

    if (_cmdBuffPool) {
        cmdBuffPool = _cmdBuffPool;
        return VK_SUCCESS;
    }
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}


VkResult VulkanCommandBufferPool::Configure(const VulkanDeviceContext*   vkDevCtx,
                                            uint32_t                     numPoolNodes,
                                            uint32_t                     queueFamilyIndex,
                                            bool                         createQueryPool,
                                            const void*                  pNext,
                                            bool                         createSemaphores,
                                            bool                         createFences)
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    if (numPoolNodes > m_poolNodes.size()) {
        assert(!"Number of requested number of pool nodes exceeds the max size of the m_poolNodes array");
        return VK_ERROR_TOO_MANY_OBJECTS;
    }

    VkResult result = m_commandBuffersSet.CreateCommandBufferPool(queueFamilyIndex, numPoolNodes);
    if (result != VK_SUCCESS) {
        assert(!"ERROR: CreateCommandBufferPool!");
        return result;
    }

    if (createSemaphores) {
        result = m_semaphoreSet.CreateSet( numPoolNodes);
        if (result != VK_SUCCESS) {
            assert(!"ERROR: m_filterWaitSemaphoreSet.CreateSet!");
            return result;
        }
    }

    if (createFences) {
        result = m_fenceSet.CreateSet(numPoolNodes);
        if (result != VK_SUCCESS) {
            assert(!"ERROR: CreateCommandBufferPool!");
            return result;
        }
    }

    if (createQueryPool) {
        m_queryPoolSet.CreateSet(vkDevCtx, numPoolNodes,
                                 VK_QUERY_TYPE_VIDEO_ENCODE_FEEDBACK_KHR,
                                 VkQueryPoolCreateFlags(),
                                 pNext);
    }

    for (uint32_t poolNodeIdx = 0; poolNodeIdx < numPoolNodes; poolNodeIdx++) {
        m_poolNodes[poolNodeIdx].Init(vkDevCtx);
        m_availablePoolNodes |= (1ULL << poolNodeIdx);
    }

    m_vkDevCtx         = vkDevCtx;
    m_poolSize         = numPoolNodes;
    m_queueFamilyIndex = queueFamilyIndex;
    return VK_SUCCESS;
}

void VulkanCommandBufferPool::Deinit()
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    for (size_t ndx = 0; ndx < m_poolSize; ndx++) {
        m_poolNodes[ndx].Deinit();
    }
}
