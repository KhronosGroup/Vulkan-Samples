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

#ifndef _VULKANCOMMANDBUFFERPOOL_H_
#define _VULKANCOMMANDBUFFERPOOL_H_

#include <assert.h>
#include <stdint.h>

#include "VkCodecUtils/VkVideoRefCountBase.h"
#include "VkCodecUtils/Helpers.h"
#include "VkCodecUtils/VulkanDeviceContext.h"
#include "VkCodecUtils/VulkanCommandBuffersSet.h"
#include "VkCodecUtils/VulkanSemaphoreSet.h"
#include "VkCodecUtils/VulkanFenceSet.h"
#include "VkCodecUtils/VulkanQueryPoolSet.h"

class VulkanCommandBufferPool : public VkVideoRefCountBase {
public:

    class PoolNode : public VkVideoRefCountBase {
    public:

        // VulkanCommandBufferPool is a friend class to be able to call SetParent()
        friend class VulkanCommandBufferPool;

        virtual int32_t AddRef()
        {
            return ++m_refCount;
        }

        virtual int32_t Release();

        VkResult Init(const VulkanDeviceContext* vkDevCtx);

        enum CmdBufState { CmdBufStateReset = 0,
                           CmdBufStateRecording,
                           CmdBufStateRecorded,
                           CmdBufStateSubmitted};

        PoolNode()
            : m_vkDevCtx()
            , m_refCount()
            , m_parent()
            , m_parentIndex(-1)
            , m_cmdBufState(CmdBufStateReset)
        {
        }

        // Copy Constructor
        PoolNode (const PoolNode &srcObj) = delete;
        // Move Constructor
        PoolNode (PoolNode &&srcObj) = delete;
        // Copy Assignment Operator
        PoolNode& operator=(const PoolNode& other) = delete;
        // Move Assignment Operator
        PoolNode& operator=(PoolNode&& other) noexcept = delete;

        void Deinit();

        virtual ~PoolNode()
        {
            Deinit();
        }

        const VkCommandBuffer* GetCommandBuffer() const {
            if ((m_parent == nullptr) || (m_parentIndex < 0)) {
                assert(!"Invalid PoolNode state!");
                return nullptr;
            }
            return m_parent->m_commandBuffersSet.GetCommandBuffer(m_parentIndex);
        }

        VkCommandBuffer BeginCommandBufferRecording(const VkCommandBufferBeginInfo& beginInfo) {
            if ((m_parent == nullptr) || (m_parentIndex < 0)) {
                assert(!"Invalid PoolNode state!");
                return VK_NULL_HANDLE;
            }
            if (m_cmdBufState != CmdBufStateReset) {
                assert(!"Command buffer is not in a reset state!");
                return VK_NULL_HANDLE;
            }

            const VkCommandBuffer cmdBuf = *m_parent->m_commandBuffersSet.GetCommandBuffer(m_parentIndex);
            m_vkDevCtx->BeginCommandBuffer(cmdBuf, &beginInfo);
            m_cmdBufState = CmdBufStateRecording;
            return cmdBuf;
        }

        VkResult EndCommandBufferRecording(VkCommandBuffer cmdBuf) {
            if ((m_parent == nullptr) || (m_parentIndex < 0)) {
                assert(!"Invalid PoolNode state!");
                return VK_ERROR_INITIALIZATION_FAILED;
            }
            if (m_cmdBufState != CmdBufStateRecording) {
                assert(!"Command buffer is not in recording state!");
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            assert(cmdBuf == *m_parent->m_commandBuffersSet.GetCommandBuffer(m_parentIndex));

            VkResult result = m_vkDevCtx->EndCommandBuffer(cmdBuf);
            if (result == VK_SUCCESS) {
                m_cmdBufState = CmdBufStateRecorded;
            } else {
                assert(!"Error in command buffer recording!");
            }
            return result;
        }

        bool SetCommandBufferSubmitted() {
            if ((m_parent == nullptr) || (m_parentIndex < 0)) {
                assert(!"Invalid PoolNode state!");
                return false;
            }
            if (m_cmdBufState != CmdBufStateRecorded) {
                assert(!"Command Buffer is not in recorded state!");
                return false;
            }
            m_cmdBufState = CmdBufStateSubmitted;
            return true;
        }

        VkFence GetFence() const {
            if ((m_parent == nullptr) || (m_parentIndex < 0)) {
                assert(!"Invalid PoolNode state!");
                return VK_NULL_HANDLE;
            }
            return m_parent->m_fenceSet.GetFence(m_parentIndex);
        }

        VkResult SyncHostOnCmdBuffComplete(bool resetAfterWait = true,
                                           const char* fenceName = "unknown",
                                           const uint64_t fenceWaitTimeoutNsec = 100ULL * 1000ULL * 1000ULL /* 200 mSec */,
                                           const uint64_t fenceTotalWaitTimeoutNsec = 5ULL * 1000ULL * 1000ULL * 1000ULL /* 5 sec */)
        {
            if(m_cmdBufState != CmdBufStateSubmitted) {
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            VkFence cmdBufferCompleteFence = GetFence();

            if ((m_vkDevCtx == nullptr) || (cmdBufferCompleteFence == VK_NULL_HANDLE)) {
                assert(!"Invalid PoolNode state!");
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            VkResult result = vk::WaitAndResetFence(*m_vkDevCtx, cmdBufferCompleteFence, resetAfterWait,
                                                    fenceName, fenceWaitTimeoutNsec, fenceTotalWaitTimeoutNsec);
            if (result != VK_SUCCESS) {
                fprintf(stderr, "\nERROR: WaitAndResetFence() for %s with result: 0x%x\n", fenceName, result);
                assert(result == VK_SUCCESS);
            }

            return result;
        }

        bool ResetCommandBuffer(bool syncWithHost = true,
                                const char* fenceName = "unknown")
        {
           if (m_cmdBufState == CmdBufStateReset) {
               return false;
           }

           if (syncWithHost) {
               SyncHostOnCmdBuffComplete(true, fenceName);
           }

           m_cmdBufState = CmdBufStateReset;

           return true;
        }

        VkSemaphore GetSemaphore() const {
            if ((m_parent == nullptr) || (m_parentIndex < 0)) {
                assert(!"Invalid PoolNode state!");
                return VK_NULL_HANDLE;
            }
            return m_parent->m_semaphoreSet.GetSemaphore(m_parentIndex);
        }

        VkQueryPool GetQueryPool(uint32_t& queryIdx) const {
            if ((m_parent == nullptr) || (m_parentIndex < 0)) {
                assert(!"Invalid PoolNode state!");
                queryIdx = (uint32_t)-1;
                return VK_NULL_HANDLE;
            }
            queryIdx = m_parentIndex;
            return m_parent->m_queryPoolSet.GetQueryPool(queryIdx);
        }

        const VulkanDeviceContext* GetDeviceContext() const { return m_vkDevCtx; }

    private:
        VkResult SetParent(VulkanCommandBufferPool* cmdBuffPool, int32_t parentIndex);

    private:
        const VulkanDeviceContext*               m_vkDevCtx;
        std::atomic<int32_t>                     m_refCount;
        VkSharedBaseObj<VulkanCommandBufferPool> m_parent;
        int32_t                                  m_parentIndex;
        CmdBufState                              m_cmdBufState;
    };

    static constexpr size_t maxPoolNodes = 64;

    VulkanCommandBufferPool()
        : m_vkDevCtx()
        , m_refCount()
        , m_queueMutex()
        , m_poolSize(0)
        , m_nextNodeToUse(0)
        , m_availablePoolNodes(0UL)
        , m_queueFamilyIndex((uint32_t)-1)
        , m_commandBuffersSet()
        , m_semaphoreSet()
        , m_fenceSet()
        , m_queryPoolSet()
        , m_poolNodes(maxPoolNodes)
    {
    }

    static VkResult Create(const VulkanDeviceContext* vkDevCtx,
                           VkSharedBaseObj<VulkanCommandBufferPool>& cmdBuffPool);

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

    VkResult Configure(const VulkanDeviceContext*   vkDevCtx,
                       uint32_t                     numPoolNodes,
                       uint32_t                     queueFamilyIndex,
                       bool                         createQueryPool = false, // must have a valid pVideoProfile
                       const void*                  pNext           = nullptr,
                       bool                         createSemaphores = false,
                       bool                         createFences = true);

    void Deinit();

    ~VulkanCommandBufferPool()
    {
        Deinit();
    }

    PoolNode& operator[](unsigned int index)
    {
        assert(index < m_poolNodes.size());
        return m_poolNodes[index];
    }

    size_t size()
    {
        return m_poolNodes.size();
    }

    bool GetAvailablePoolNode(VkSharedBaseObj<PoolNode>&  poolNode);

    bool ReleasePoolNodeToPool(uint32_t poolNodeIndex);

private:
    const VulkanDeviceContext* m_vkDevCtx;
    std::atomic<int32_t>       m_refCount;
    std::mutex                 m_queueMutex;
    uint32_t                   m_poolSize;
    uint32_t                   m_nextNodeToUse;
    uint64_t                   m_availablePoolNodes;
    uint32_t                   m_queueFamilyIndex;
    VulkanCommandBuffersSet    m_commandBuffersSet;
    VulkanSemaphoreSet         m_semaphoreSet;
    VulkanFenceSet             m_fenceSet;
    VulkanQueryPoolSet         m_queryPoolSet;
    std::vector<PoolNode>      m_poolNodes;
};

#endif /* _VULKANCOMMANDBUFFERPOOL_H_ */
