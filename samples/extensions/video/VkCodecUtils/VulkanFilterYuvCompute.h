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


#ifndef _VULKANFILTERYUVCOMPUTE_H_
#define _VULKANFILTERYUVCOMPUTE_H_

#include "VkCodecUtils/Helpers.h"
#include "VkCodecUtils/VulkanCommandBuffersSet.h"
#include "VkCodecUtils/VulkanSemaphoreSet.h"
#include "VkCodecUtils/VulkanFenceSet.h"
#include "VkCodecUtils/VulkanDescriptorSetLayout.h"
#include "VkCodecUtils/VulkanComputePipeline.h"
#include "VkCodecUtils/VulkanFilter.h"
#include "nvidia_utils/vulkan/ycbcr_utils.h"

class VulkanFilterYuvCompute : public VulkanFilter
{
public:

    enum FilterType { YCBCRCOPY, YCBCRCLEAR, YCBCR2RGBA, RGBA2YCBCR };

    static VkResult Create(const VulkanDeviceContext* vkDevCtx,
                           uint32_t queueFamilyIndex,
                           uint32_t queueIndex,
                           FilterType flterType,
                           uint32_t maxNumFrames,
                           VkFormat inputFormat,
                           VkFormat outputFormat,
                           const VkSamplerYcbcrConversionCreateInfo* pYcbcrConversionCreateInfo,
                           const YcbcrPrimariesConstants* pYcbcrPrimariesConstants,
                           const VkSamplerCreateInfo* pSamplerCreateInfo,
                           VkSharedBaseObj<VulkanFilter>& vulkanFilter);

    VulkanFilterYuvCompute(const VulkanDeviceContext* vkDevCtx,
                           uint32_t queueFamilyIndex,
                           uint32_t queueIndex,
                           FilterType filterType,
                           uint32_t maxNumFrames,
                           VkFormat inputFormat,
                           VkFormat outputFormat,
                           const YcbcrPrimariesConstants* pYcbcrPrimariesConstants)
        : VulkanFilter(vkDevCtx, queueFamilyIndex, queueIndex)
        , m_filterType(filterType)
        , m_inputFormat(inputFormat)
        , m_outputFormat(outputFormat)
        , m_workgroupSizeX(16)
        , m_workgroupSizeY(16)
        , m_maxNumFrames(maxNumFrames)
        , m_ycbcrPrimariesConstants (*pYcbcrPrimariesConstants)
        , m_inputImageAspects(  VK_IMAGE_ASPECT_COLOR_BIT |
                                VK_IMAGE_ASPECT_PLANE_0_BIT |
                                VK_IMAGE_ASPECT_PLANE_1_BIT |
                                VK_IMAGE_ASPECT_PLANE_2_BIT)
        , m_outputImageAspects( VK_IMAGE_ASPECT_COLOR_BIT |
                                VK_IMAGE_ASPECT_PLANE_0_BIT |
                                VK_IMAGE_ASPECT_PLANE_1_BIT |
                                VK_IMAGE_ASPECT_PLANE_2_BIT)
    {
    }

    VkResult Init(const VkSamplerYcbcrConversionCreateInfo* pYcbcrConversionCreateInfo,
                  const VkSamplerCreateInfo* pSamplerCreateInfo);

    virtual ~VulkanFilterYuvCompute() {

    }

    virtual VkSemaphore GetFilterWaitSemaphore(uint32_t frameIdx) const
    {
        return m_filterWaitSemaphoreSet.GetSemaphore(frameIdx);
    }

    virtual VkFence GetFilterSignalFence(uint32_t frameIdx) const {

        VkFence filterCompleteFence = m_filterCompleteFenceSet.GetFence(frameIdx);

        vk::WaitAndResetFence(VulkanDeviceContext::GetThe()->getDevice(), filterCompleteFence, true,
                              "filterCompleteFence");

        return filterCompleteFence;
    }

    virtual VkResult RecordCommandBuffer(uint32_t frameIdx,
                                         const VkImageResourceView* inputImageView,
                                         const VkVideoPictureResourceInfoKHR * inputImageResourceInfo,
                                         const VkImageResourceView* outputImageView,
                                         const VkVideoPictureResourceInfoKHR * outputImageResourceInfo,
                                         VkFence frameCompleteFence = VK_NULL_HANDLE)
    {

        if (frameCompleteFence != VK_NULL_HANDLE) {

            // if frameCompleteFence is a valid handle, then it must to be in a non-signaled state
            // i.e. if this frameIdx was previously submitted, then it had to be waited for and
            // the fence reset.
            assert(VK_NOT_READY == VulkanDeviceContext::GetThe()->GetFenceStatus(VulkanDeviceContext::GetThe()->getDevice(), frameCompleteFence));

        } else {

            // Get our own fence for this frame
            frameCompleteFence = GetFilterSignalFence(frameIdx);
            assert(frameCompleteFence != VK_NULL_HANDLE);
        }

        VkCommandBufferBeginInfo cmdBufferBeginInfo {};
        cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkCommandBuffer cmdBuf = *m_commandBuffersSet.GetCommandBuffer((uint32_t)frameIdx);

        VkResult result = VulkanDeviceContext::GetThe()->BeginCommandBuffer(cmdBuf, &cmdBufferBeginInfo);
        if (result != VK_SUCCESS) {
            return result;
        }

        VulkanDeviceContext::GetThe()->CmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline.getPipeline());

        VkDescriptorSetLayoutCreateFlags layoutMode = m_descriptorSetLayout.GetDescriptorSetLayoutInfo().GetDescriptorLayoutMode();

        switch (layoutMode) {
            case VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR:
            case VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT:
            {

                const uint32_t maxNumComputeDescr = 8;
                VkDescriptorImageInfo imageDescriptors[8]{};
                std::array<VkWriteDescriptorSet, maxNumComputeDescr> writeDescriptorSets{};

                // Images
                uint32_t set = 0;
                uint32_t descrIndex = 0;
                uint32_t dstBinding = 0;
                // RGBA color converted by an YCbCr sample
                if (m_inputImageAspects & VK_IMAGE_ASPECT_COLOR_BIT) {
                    writeDescriptorSets[descrIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writeDescriptorSets[descrIndex].dstSet = VK_NULL_HANDLE;
                    writeDescriptorSets[descrIndex].dstBinding = dstBinding;
                    writeDescriptorSets[descrIndex].descriptorCount = 1;
                    writeDescriptorSets[descrIndex].descriptorType = (m_samplerYcbcrConversion.GetSampler() != VK_NULL_HANDLE) ?
                                                                        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER :
                                                                        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

                    imageDescriptors[descrIndex].sampler = m_samplerYcbcrConversion.GetSampler();
                    imageDescriptors[descrIndex].imageView = inputImageView->GetImageView();
                    assert(imageDescriptors[descrIndex].imageView);
                    imageDescriptors[descrIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    writeDescriptorSets[descrIndex].pImageInfo = &imageDescriptors[descrIndex]; // RGBA or Sampled YCbCr
                    descrIndex++;
                }
                dstBinding++;

                uint32_t planeNum = 0;
                // y plane - G -> R8
                if ((m_inputImageAspects & (VK_IMAGE_ASPECT_PLANE_0_BIT << planeNum)) &&
                        (planeNum < inputImageView->GetNumberOfPlanes())) {
                    writeDescriptorSets[descrIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writeDescriptorSets[descrIndex].dstSet = VK_NULL_HANDLE;
                    writeDescriptorSets[descrIndex].dstBinding = dstBinding;
                    writeDescriptorSets[descrIndex].descriptorCount = 1;
                    writeDescriptorSets[descrIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    imageDescriptors[descrIndex].sampler = VK_NULL_HANDLE;
                    imageDescriptors[descrIndex].imageView = inputImageView->GetPlaneImageView(planeNum++);
                    assert(imageDescriptors[descrIndex].imageView);
                    imageDescriptors[descrIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    writeDescriptorSets[descrIndex].pImageInfo = &imageDescriptors[descrIndex]; // Y (0) plane
                    descrIndex++;
                }
                dstBinding++;

                // CbCr plane - BR -> R8B8
                if ((m_inputImageAspects & (VK_IMAGE_ASPECT_PLANE_0_BIT << planeNum)) &&
                        (planeNum < inputImageView->GetNumberOfPlanes())) {
                    writeDescriptorSets[descrIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writeDescriptorSets[descrIndex].dstSet = VK_NULL_HANDLE;
                    writeDescriptorSets[descrIndex].dstBinding = dstBinding;
                    writeDescriptorSets[descrIndex].descriptorCount = 1;
                    writeDescriptorSets[descrIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    imageDescriptors[descrIndex].sampler = VK_NULL_HANDLE;
                    imageDescriptors[descrIndex].imageView = inputImageView->GetPlaneImageView(planeNum++);
                    assert(imageDescriptors[descrIndex].imageView);
                    imageDescriptors[descrIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    writeDescriptorSets[descrIndex].pImageInfo = &imageDescriptors[descrIndex]; // CbCr (1) plane
                    descrIndex++;
                }
                dstBinding++;

                // Cr plane - R -> R8
                if ((m_inputImageAspects & (VK_IMAGE_ASPECT_PLANE_0_BIT << planeNum)) &&
                        (planeNum < inputImageView->GetNumberOfPlanes())) {
                    writeDescriptorSets[descrIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writeDescriptorSets[descrIndex].dstSet = VK_NULL_HANDLE;
                    writeDescriptorSets[descrIndex].dstBinding = dstBinding;
                    writeDescriptorSets[descrIndex].descriptorCount = 1;
                    writeDescriptorSets[descrIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    imageDescriptors[descrIndex].sampler = VK_NULL_HANDLE;
                    imageDescriptors[descrIndex].imageView = inputImageView->GetPlaneImageView(planeNum++);
                    assert(imageDescriptors[descrIndex].imageView);
                    imageDescriptors[descrIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    writeDescriptorSets[descrIndex].pImageInfo = &imageDescriptors[descrIndex]; // CbCr (1) plane
                    descrIndex++;
                }
                dstBinding++;

                // Out RGBA or single planar YCbCr image
                if (m_outputImageAspects & VK_IMAGE_ASPECT_COLOR_BIT) {
                    writeDescriptorSets[descrIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writeDescriptorSets[descrIndex].dstSet = VK_NULL_HANDLE;
                    writeDescriptorSets[descrIndex].dstBinding = dstBinding;
                    writeDescriptorSets[descrIndex].descriptorCount = 1;
                    writeDescriptorSets[descrIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    imageDescriptors[descrIndex].sampler = VK_NULL_HANDLE;
                    imageDescriptors[descrIndex].imageView = outputImageView->GetImageView();
                    imageDescriptors[descrIndex].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                    writeDescriptorSets[descrIndex].pImageInfo = &imageDescriptors[descrIndex];
                    descrIndex++;
                }
                dstBinding++;

                planeNum = 0;
                // y plane out - G -> R8
                if ((m_outputImageAspects & (VK_IMAGE_ASPECT_PLANE_0_BIT << planeNum)) &&
                        (planeNum < outputImageView->GetNumberOfPlanes())) {
                    writeDescriptorSets[descrIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writeDescriptorSets[descrIndex].dstSet = VK_NULL_HANDLE;
                    writeDescriptorSets[descrIndex].dstBinding = dstBinding;
                    writeDescriptorSets[descrIndex].descriptorCount = 1;
                    writeDescriptorSets[descrIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    imageDescriptors[descrIndex].sampler = VK_NULL_HANDLE;
                    imageDescriptors[descrIndex].imageView = outputImageView->GetPlaneImageView(planeNum++);
                    assert(imageDescriptors[descrIndex].imageView);
                    imageDescriptors[descrIndex].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                    writeDescriptorSets[descrIndex].pImageInfo = &imageDescriptors[descrIndex];
                    descrIndex++;
                }
                dstBinding++;

                // CbCr plane out - BR -> R8B8
                if ((m_outputImageAspects & (VK_IMAGE_ASPECT_PLANE_0_BIT << planeNum)) &&
                        (planeNum < outputImageView->GetNumberOfPlanes())) {
                    writeDescriptorSets[descrIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writeDescriptorSets[descrIndex].dstSet = VK_NULL_HANDLE;
                    writeDescriptorSets[descrIndex].dstBinding = dstBinding;
                    writeDescriptorSets[descrIndex].descriptorCount = 1;
                    writeDescriptorSets[descrIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    imageDescriptors[descrIndex].sampler = VK_NULL_HANDLE;
                    imageDescriptors[descrIndex].imageView = outputImageView->GetPlaneImageView(planeNum++);
                    assert(imageDescriptors[descrIndex].imageView);
                    imageDescriptors[descrIndex].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                    writeDescriptorSets[descrIndex].pImageInfo = &imageDescriptors[descrIndex];
                    descrIndex++;
                }
                dstBinding++;

                // Cr plane out - R -> R8
                if ((m_outputImageAspects & (VK_IMAGE_ASPECT_PLANE_0_BIT << planeNum)) &&
                        (planeNum < outputImageView->GetNumberOfPlanes())) {
                    writeDescriptorSets[descrIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writeDescriptorSets[descrIndex].dstSet = VK_NULL_HANDLE;
                    writeDescriptorSets[descrIndex].dstBinding = dstBinding;
                    writeDescriptorSets[descrIndex].descriptorCount = 1;
                    writeDescriptorSets[descrIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    imageDescriptors[descrIndex].sampler = VK_NULL_HANDLE;
                    imageDescriptors[descrIndex].imageView = outputImageView->GetPlaneImageView(planeNum++);
                    assert(imageDescriptors[descrIndex].imageView);
                    imageDescriptors[descrIndex].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                    writeDescriptorSets[descrIndex].pImageInfo = &imageDescriptors[descrIndex];
                    descrIndex++;
                }
                dstBinding++;

                assert(descrIndex <= maxNumComputeDescr);
                assert(descrIndex >= 2);

                if (layoutMode ==  VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR) {
                    VulkanDeviceContext::GetThe()->CmdPushDescriptorSetKHR(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE,
                                                        m_descriptorSetLayout.GetPipelineLayout(),
                                                        set, descrIndex, writeDescriptorSets.data());
                } else {

                    VkDeviceOrHostAddressConstKHR imageDescriptorBufferDeviceAddress =
                          m_descriptorSetLayout.UpdateDescriptorBuffer(frameIdx, set, descrIndex, writeDescriptorSets.data());


                    // Descriptor buffer bindings
                    // Set 0 = Image
                    VkDescriptorBufferBindingInfoEXT bindingInfo{};
                    bindingInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
                    bindingInfo.pNext = nullptr;
                    bindingInfo.address = imageDescriptorBufferDeviceAddress.deviceAddress;
                    bindingInfo.usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
                                        VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
                    VulkanDeviceContext::GetThe()->CmdBindDescriptorBuffersEXT(cmdBuf, 1, &bindingInfo);

                    // Image (set 0)
                    uint32_t bufferIndexImage = 0;
                    VkDeviceSize bufferOffset = 0;
                    VulkanDeviceContext::GetThe()->CmdSetDescriptorBufferOffsetsEXT(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE,
                                                               m_descriptorSetLayout.GetPipelineLayout(),
                                                               set, 1, &bufferIndexImage, &bufferOffset);
                }
            }
            break;

            default:
            VulkanDeviceContext::GetThe()->CmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE,
                                              m_descriptorSetLayout.GetPipelineLayout(),
                                              0, 1, m_descriptorSetLayout.GetDescriptorSet(), 0, 0);
        }

        struct PushConstants {
            uint32_t srcLayer;
            uint32_t dstLayer;
        };

        PushConstants pushConstants = {
                inputImageResourceInfo  ? inputImageResourceInfo->baseArrayLayer : 0, // Set the source layer index
                outputImageResourceInfo ? outputImageResourceInfo->baseArrayLayer : 0 // Set the destination layer index
        };

        VulkanDeviceContext::GetThe()->CmdPushConstants(cmdBuf,
                                     m_descriptorSetLayout.GetPipelineLayout(),
                                     VK_SHADER_STAGE_COMPUTE_BIT,
                                     0, // offset
                                     sizeof(PushConstants),
                                     &pushConstants);

        const VkImageCreateInfo& imageCreateInfo = outputImageView->GetImageResource()->GetImageCreateInfo();
        uint32_t  width  = imageCreateInfo.extent.width  + (m_workgroupSizeX - 1);
        uint32_t  height = imageCreateInfo.extent.height + (m_workgroupSizeY - 1);

        VulkanDeviceContext::GetThe()->CmdDispatch(cmdBuf, width / m_workgroupSizeX, height / m_workgroupSizeY, 1);

        return VulkanDeviceContext::GetThe()->EndCommandBuffer(cmdBuf);
    }

    virtual uint32_t GetSubmitCommandBuffers(uint32_t frameIdx, const VkCommandBuffer** ppCommandBuffers) const {
        *ppCommandBuffers = m_commandBuffersSet.GetCommandBuffer(frameIdx);
        return 1;
    }

private:
    VkResult InitDescriptorSetLayout(uint32_t maxNumFrames);
    size_t InitYCBCRCOPY(std::string& computeShader);
    size_t InitYCBCRCLEAR(std::string& computeShader);
    size_t InitYCBCR2RGBA(std::string& computeShader);

	const FilterType                         m_filterType;
    VkFormat                                 m_inputFormat;
    VkFormat                                 m_outputFormat;
    uint32_t                                 m_workgroupSizeX; // usually 16
    uint32_t                                 m_workgroupSizeY; // usually 16
    uint32_t                                 m_maxNumFrames;
    const YcbcrPrimariesConstants            m_ycbcrPrimariesConstants;
    VulkanSamplerYcbcrConversion             m_samplerYcbcrConversion;
    VulkanDescriptorSetLayout                m_descriptorSetLayout;
    VulkanComputePipeline                    m_computePipeline;
    VulkanCommandBuffersSet                  m_commandBuffersSet;
    VulkanSemaphoreSet                       m_filterWaitSemaphoreSet;
    VulkanFenceSet                           m_filterCompleteFenceSet;
    VkImageAspectFlags                       m_inputImageAspects;
    VkImageAspectFlags                       m_outputImageAspects;

};

#endif /* _VULKANFILTERYUVCOMPUTE_H_ */
