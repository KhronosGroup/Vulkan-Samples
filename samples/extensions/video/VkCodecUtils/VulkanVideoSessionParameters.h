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

#ifndef _VULKANVIDEOSESSIONPARAMETERS_H_
#define _VULKANVIDEOSESSIONPARAMETERS_H_

#include <assert.h>
#include <atomic>
#include "VkCodecUtils/VkVideoRefCountBase.h"
#include "VkCodecUtils/VulkanDeviceContext.h"
#include "VkVideoCore/VkVideoCoreProfile.h"
#include "VkCodecUtils/VulkanVideoSession.h"

class VulkanVideoSessionParameters
{
public:
    virtual ~VulkanVideoSessionParameters();

    //! Increment the reference count by 1.
    virtual int32_t AddRef();

    //! Decrement the reference count by 1. When the reference count
    //! goes to 0 the object is automatically destroyed.
    virtual int32_t Release();

    static VkResult Create(const VulkanDeviceContext* vkDevCtx,
                           VkSharedBaseObj<VulkanVideoSession>& videoSession,
                           VkVideoSessionParametersKHR& sessionParameters,
                           VkSharedBaseObj<VulkanVideoSessionParameters>& videoPictureParameters);

    operator VkVideoSessionParametersKHR() const {
        assert(m_sessionParameters != VK_NULL_HANDLE);
        return m_sessionParameters;
    }

private:
    VulkanVideoSessionParameters(const VulkanDeviceContext* vkDevCtx,
                                 VkVideoSessionParametersKHR sessionParameters);
private:
    std::atomic<int32_t>                m_refCount;
    const VulkanDeviceContext*          m_vkDevCtx;
    VkSharedBaseObj<VulkanVideoSession> m_videoSession;
    VkVideoSessionParametersKHR         m_sessionParameters;
};

#endif /* _VULKANVIDEOSESSIONPARAMETERS_H_ */
