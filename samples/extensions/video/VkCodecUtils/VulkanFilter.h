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

#ifndef _VKCODECUTILS_VULKANFILTER_H_

#include <atomic>
#include <string>

#include "VkCodecUtils/VkVideoRefCountBase.h"
#include "VkCodecUtils/VulkanDeviceContext.h"
#include "VkCodecUtils/VulkanShaderCompiler.h"
#include "VkCodecUtils/VkImageResource.h"

struct VulkanShaderInput {
    const std::string     shader;
    VkShaderStageFlagBits type;
    uint32_t              shaderIsFsPath : 1;
};

class VulkanFilter : public VkVideoRefCountBase
{
public:

    VulkanFilter(const VulkanDeviceContext* vkDevCtx,
                 uint32_t queueFamilyIndex,
                 uint32_t queueIndex)
        : m_refCount(0),
          m_vulkanShaderCompiler(),
          m_queueFamilyIndex(queueFamilyIndex),
          m_queueIndex(queueIndex),
          m_queue()
    {
        VulkanDeviceContext::GetThe()->GetDeviceQueue(VulkanDeviceContext::GetThe()->getDevice(), queueFamilyIndex, queueIndex, &m_queue);
    }

    virtual ~VulkanFilter()
    {
    }

    virtual int32_t AddRef()
    {
        return ++m_refCount;
    }

    virtual int32_t Release()
    {
        uint32_t ret = --m_refCount;
        // Destroy the object if ref-count reaches zero
        if (ret == 0) {
            delete this;
        }
        return ret;
    }

    VkShaderModule CreateShaderModule(const char *shaderCode, size_t shaderSize,
                                      VkShaderStageFlagBits type)
    {
        return m_vulkanShaderCompiler.BuildGlslShader(shaderCode, shaderSize,
                                                      type);
    }

    void DestroyShaderModule(VkShaderModule shaderModule)
    {
        if (shaderModule != VK_NULL_HANDLE) {
            VulkanDeviceContext::GetThe()->DestroyShaderModule(VulkanDeviceContext::GetThe()->getDevice(), shaderModule, nullptr);
        }
    }

    virtual VkSemaphore GetFilterWaitSemaphore(uint32_t frameIdx) const = 0;

    virtual VkResult RecordCommandBuffer(uint32_t frameIdx,
                                         const VkImageResourceView* inputImageView,
                                         const VkVideoPictureResourceInfoKHR * inputImageResourceInfo,
                                         const VkImageResourceView* outputImageView,
                                         const VkVideoPictureResourceInfoKHR * outputImageResourceInfo,
                                         VkFence frameCompleteFence = VK_NULL_HANDLE) = 0;

    virtual uint32_t GetSubmitCommandBuffers(uint32_t frameIdx, const VkCommandBuffer** ppCommandBuffers) const = 0;

    virtual VkFence GetFilterSignalFence(uint32_t frameIdx) const = 0;

    virtual VkResult SubmitCommandBuffer(uint32_t frameIdx,
                                         uint32_t waitSemaphoreCount,
                                         const VkSemaphore* pWaitSemaphores,
                                         uint32_t signalSemaphoreCount,
                                         const VkSemaphore* pSignalSemaphores,
                                         VkFence filterCompleteFence) const
    {

        assert(m_queue != VK_NULL_HANDLE);

        // Wait for rendering finished
        VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

        // Submit compute commands
        VkSubmitInfo submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = GetSubmitCommandBuffers(frameIdx, &submitInfo.pCommandBuffers);
        submitInfo.waitSemaphoreCount = waitSemaphoreCount;
        submitInfo.pWaitSemaphores = pWaitSemaphores;
        submitInfo.pWaitDstStageMask = &waitStageMask;
        submitInfo.signalSemaphoreCount = signalSemaphoreCount;
        submitInfo.pSignalSemaphores = pSignalSemaphores;
        return VulkanDeviceContext::GetThe()->QueueSubmit(m_queue, 1, &submitInfo, filterCompleteFence);
    }

private:
    std::atomic<int32_t>               m_refCount;
protected:
    VulkanShaderCompiler               m_vulkanShaderCompiler;
    uint32_t                           m_queueFamilyIndex;
    uint32_t                           m_queueIndex;
    VkQueue                            m_queue;
};
#endif /* _VKCODECUTILS_VULKANFILTER_H_ */
