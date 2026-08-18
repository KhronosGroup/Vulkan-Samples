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

#ifndef _VKCODECUTILS_VULKANBUFFERPOOL_H_
#define _VKCODECUTILS_VULKANBUFFERPOOL_H_

#include <assert.h>
#include <stdint.h>
#include <vector>
#include <atomic>
#include <mutex>

#include "VkCodecUtils/VkVideoRefCountBase.h"

class VulkanBufferPoolIf : public VkVideoRefCountBase
{
public:
    virtual bool ReleasePoolNodeToPool(uint32_t poolNodeIndex) = 0;
};

template <typename PoolNodeType>
class VulkanBufferPool : public VulkanBufferPoolIf
{
public:
    static constexpr size_t maxPoolNodes = 64;

    VulkanBufferPool()
        : m_refCount()
        , m_queueMutex()
        , m_poolSize(0)
        , m_nextNodeToUse(0)
        , m_availablePoolNodes(0UL)
        , m_poolNodes(maxPoolNodes)
    {
    }

    VulkanBufferPool(const VulkanBufferPool &other) = delete;
    VulkanBufferPool(VulkanBufferPool &&other) = delete;
    VulkanBufferPool& operator=(const VulkanBufferPool &other) = delete;
    VulkanBufferPool& operator=(VulkanBufferPool &&other) = delete;

    void Init(uint32_t numPoolNodes)
    {
        for (uint32_t poolNodeIdx = 0; poolNodeIdx < numPoolNodes; poolNodeIdx++) {
            m_poolNodes[poolNodeIdx].Init();
            m_availablePoolNodes |= (1ULL << poolNodeIdx);
        }

        m_poolSize = numPoolNodes;
    }

    void Deinit()
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        for (size_t ndx = 0; ndx < m_poolSize; ndx++) {
            m_poolNodes[ndx].Deinit();
        }
        m_poolSize = 0;
    }

    virtual ~VulkanBufferPool()
    {
        Deinit();
    }

    virtual int32_t AddRef()
    {
        return ++m_refCount;
    }

    virtual int32_t Release()
    {
        uint32_t ret = --m_refCount;
        // Destroy the device if ref-count reaches zero
        if (ret == 0) {
            delete this;
        }
        return ret;
    }

    PoolNodeType& operator[](unsigned int index)
    {
        assert(index < m_poolNodes.size());
        return m_poolNodes[index];
    }

    size_t size()
    {
        return m_poolNodes.size();
    }

    bool GetAvailablePoolNode(VkSharedBaseObj<PoolNodeType>&  poolNode)
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

    virtual bool ReleasePoolNodeToPool(uint32_t poolNodeIndex)
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);

        assert(!(m_availablePoolNodes & uint64_t(1ULL << poolNodeIndex)));
        m_availablePoolNodes |= uint64_t(1ULL << poolNodeIndex);

        return true;
    }

private:
    std::atomic<int32_t>       m_refCount;
    std::mutex                 m_queueMutex;
    uint32_t                   m_poolSize;
    uint32_t                   m_nextNodeToUse;
    uint64_t                   m_availablePoolNodes;
    std::vector<PoolNodeType>  m_poolNodes;
};

#endif /* _VKCODECUTILS_VULKANBUFFERPOOL_H_ */
