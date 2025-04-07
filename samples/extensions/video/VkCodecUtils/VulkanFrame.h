/*
* Copyright 2020 NVIDIA Corporation.
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

#ifndef _VKCODECUTILS_VULKANFRAME_H_
#define _VKCODECUTILS_VULKANFRAME_H_

#include <memory>
#include <string>
#include <vector>

#include "VkCodecUtils/FrameProcessor.h"
#include "VkCodecUtils/VkVideoQueue.h"

template<class FrameDataType>
class VulkanFrame : public FrameProcessor {
public:

    static VkResult Create(
                           VkSharedBaseObj<VkVideoQueue<FrameDataType>>& videoQueue,
                           VkSharedBaseObj<VulkanFrame>& frameProcessor);

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

    virtual int AttachShell(int numBackBuffers);
    virtual void DetachShell();

    virtual int AttachSwapchain();
    virtual void DetachSwapchain();

    virtual int CreateFrameData(int count);
    virtual void DestroyFrameData();

    virtual bool OnKey(Key key);
    virtual bool OnFrame(  int32_t           renderIndex,
                          uint32_t           waitSemaphoreCount = 0,
                          const VkSemaphore* pWaitSemaphores  = nullptr,
                          uint32_t           signalSemaphoreCount = 0,
                          const VkSemaphore* pSignalSemaphores = nullptr);


    VkResult DrawFrame( int32_t           renderIndex,
                       uint32_t           waitSemaphoreCount,
                       const VkSemaphore* pWaitSemaphores,
                       uint32_t           signalSemaphoreCount,
                       const VkSemaphore* pSignalSemaphores,
                       FrameDataType*     inFrame);

    // called by attach_swapchain
    void PrepareViewport(const VkExtent2D& extent);

private:
    VulkanFrame(VkSharedBaseObj<VkVideoQueue<FrameDataType>>& videoProcessor);
    virtual ~VulkanFrame();

private:
    std::atomic<int32_t>                  m_refCount;
    // Decoder specific members
    VkSharedBaseObj<VkVideoQueue<FrameDataType>> m_videoQueue;
public:

    VkSamplerYcbcrModelConversion         m_samplerYcbcrModelConversion;
    VkSamplerYcbcrRange                   m_samplerYcbcrRange;
    vulkanVideoUtils::VkVideoAppCtx*      m_videoRenderer;
    bool                                  m_codecPaused;
    VkQueue                               m_gfxQueue;
    VkFormat                              m_vkFormat;

    VkPhysicalDeviceProperties            m_physicalDevProps;
    std::vector<VkMemoryPropertyFlags>    m_memFlags;

    std::vector<FrameDataType>            m_frameData;
    int                                   m_frameDataIndex;

    VkExtent2D                            m_extent;
    VkViewport                            m_viewport;
    VkRect2D                              m_scissor;
};

#endif // _VKCODECUTILS_VULKANFRAME_H_
