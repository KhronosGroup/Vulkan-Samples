/*
* Copyright 2021 NVIDIA Corporation.
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

#pragma once

#include <atomic>
#include "VkCodecUtils/VkVideoRefCountBase.h"
#include "VkCodecUtils/VulkanDeviceContext.h"
#include "VkVideoCore/VkVideoCoreProfile.h"

class VulkanVideoSession : public VkVideoRefCountBase
{
    enum { MAX_BOUND_MEMORY = 8 };
public:
    static VkResult Create(VkVideoSessionCreateFlagsKHR sessionCreateFlags,
                           uint32_t            videoQueueFamily,
                           VkVideoCoreProfile* pVideoProfile,
                           VkFormat            pictureFormat,
                           const VkExtent2D&   maxCodedExtent,
                           VkFormat            referencePicturesFormat,
                           uint32_t            maxDpbSlots,
                           uint32_t            maxActiveReferencePictures,
                           VkSharedBaseObj<VulkanVideoSession>& videoSession);

    bool IsCompatible ( const VulkanDeviceContext* vkDevCtx,
                        VkVideoSessionCreateFlagsKHR sessionCreateFlags,
                        uint32_t            videoQueueFamily,
                        VkVideoCoreProfile* pVideoProfile,
                        VkFormat            pictureFormat,
                        const VkExtent2D&   maxCodedExtent,
                        VkFormat            referencePicturesFormat,
                        uint32_t            maxDpbSlots,
                        uint32_t            maxActiveReferencePictures)
    {
        if (*pVideoProfile != m_profile) {
            return false;
        }

        if (sessionCreateFlags != m_flags) {
            return false;
        }

        if (maxCodedExtent.width > m_createInfo.maxCodedExtent.width) {
            return false;
        }

        if (maxCodedExtent.height > m_createInfo.maxCodedExtent.height) {
            return false;
        }

        if (maxDpbSlots > m_createInfo.maxDpbSlots) {
            return false;
        }

        if (maxActiveReferencePictures > m_createInfo.maxActiveReferencePictures) {
            return false;
        }

        if (m_createInfo.referencePictureFormat != referencePicturesFormat) {
            return false;
        }

        if (m_createInfo.pictureFormat != pictureFormat) {
            return false;
        }

        if (VulkanDeviceContext::GetThe()->getDevice() != vkDevCtx->getDevice()) {
            return false;
        }

        if (m_createInfo.queueFamilyIndex != videoQueueFamily) {
            return false;
        }

        return true;
    }


    int32_t AddRef() override
    {
        return ++m_refCount;
    }

    int32_t Release() override
    {
        uint32_t ret = --m_refCount;
        // Destroy the device if refcount reaches zero
        if (ret == 0) {
            delete this;
        }
        return static_cast<int32_t>(ret);
    }

    VkVideoSessionKHR GetVideoSession() const { return m_videoSession; }

    explicit operator VkVideoSessionKHR() const {
        assert(m_videoSession != VK_NULL_HANDLE);
        return m_videoSession;
    }

private:

    explicit VulkanVideoSession(VkVideoCoreProfile* pVideoProfile)
       : m_refCount(0), m_flags(), m_profile(*pVideoProfile),
         m_createInfo{ VK_STRUCTURE_TYPE_VIDEO_SESSION_CREATE_INFO_KHR, nullptr },
         m_videoSession(VkVideoSessionKHR()), m_memoryBound{}
    {

    }

    ~VulkanVideoSession() override
    {
        if (m_videoSession) {
            VulkanDeviceContext::GetThe()->DestroyVideoSessionKHR(*VulkanDeviceContext::GetThe(), m_videoSession, nullptr);
            m_videoSession = VkVideoSessionKHR();
        }

        for (auto & memIdx : m_memoryBound) {
            if (memIdx != VK_NULL_HANDLE) {
                VulkanDeviceContext::GetThe()->FreeMemory(*VulkanDeviceContext::GetThe(), memIdx, 0);
                memIdx = VK_NULL_HANDLE;
            }
        }
    }

	std::atomic<int32_t>                   m_refCount;
    VkVideoSessionCreateFlagsKHR           m_flags;
    VkVideoCoreProfile                     m_profile;
    VkVideoSessionCreateInfoKHR            m_createInfo;
    VkVideoSessionKHR                      m_videoSession;
    VkDeviceMemory                         m_memoryBound[MAX_BOUND_MEMORY];
};
