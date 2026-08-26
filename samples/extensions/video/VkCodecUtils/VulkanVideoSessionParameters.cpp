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

#include "VulkanVideoSessionParameters.h"

int32_t VulkanVideoSessionParameters::AddRef()
{
    return ++m_refCount;
}

int32_t VulkanVideoSessionParameters::Release()
{
    uint32_t ret;
    ret = --m_refCount;
    // Destroy the device if refcount reaches zero
    if (ret == 0) {
        delete this;
    }
    return ret;
}

VulkanVideoSessionParameters::VulkanVideoSessionParameters(const VulkanDeviceContext* vkDevCtx,
                                                           VkVideoSessionParametersKHR sessionParameters)
    :  m_refCount(0)
    ,  m_vkDevCtx(vkDevCtx)
    ,  m_videoSession()
    ,  m_sessionParameters(sessionParameters)
{

}

VulkanVideoSessionParameters::~VulkanVideoSessionParameters()
{
    if (m_sessionParameters) {
        m_vkDevCtx->DestroyVideoSessionParametersKHR(*m_vkDevCtx, m_sessionParameters, nullptr);
        m_sessionParameters = VkVideoSessionParametersKHR();
    }
    m_videoSession = nullptr;
}

VkResult
VulkanVideoSessionParameters::Create(const VulkanDeviceContext* vkDevCtx,
                                     VkSharedBaseObj<VulkanVideoSession>& videoSession,
                                     VkVideoSessionParametersKHR& sessionParameters,
                                     VkSharedBaseObj<VulkanVideoSessionParameters>& videoPictureParameters)
{
    VkSharedBaseObj<VulkanVideoSessionParameters> newVideoPictureParameters(
            new VulkanVideoSessionParameters(vkDevCtx, sessionParameters));
    if (!newVideoPictureParameters) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    videoPictureParameters = newVideoPictureParameters;
    return VK_SUCCESS;
}

