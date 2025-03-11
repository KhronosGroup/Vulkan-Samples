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

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <vector>
#include <queue>
#include <sstream>
#include <string.h>
#include <string>

#include "vulkan_interfaces.h"
#include "VkCodecUtils/HelpersDispatchTable.h"
#include "VkCodecUtils/VulkanDeviceContext.h"
#include "VkVideoCore/VkVideoCoreProfile.h"
#include "VkCodecUtils/VkImageResource.h"
#include "VulkanVideoImagePool.h"

int32_t VulkanVideoImagePoolNode::Release()
{
    uint32_t ret = --m_refCount;
    if (ret == 1) {
        m_parent->ReleaseImageToPool(m_parentIndex);
        m_parentIndex = -1;
        m_parent = nullptr;
    } else if (ret == 0) {
        // Destroy the image if ref-count reaches zero
        m_currentImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        m_pictureResourceInfo = VkVideoPictureResourceInfoKHR();
        m_imageResourceView  = nullptr;
    }
    return ret;
}

VkResult VulkanVideoImagePoolNode::CreateImage(const VkImageCreateInfo              *pImageCreateInfo,
                                               VkMemoryPropertyFlags                 requiredMemProps,
                                               uint32_t                              imageIndex,
                                               VkSharedBaseObj<VkImageResource>     &imageArrayParent,
                                               VkSharedBaseObj<VkImageResourceView> &imageViewArrayParent)
{
    VkResult result = VK_SUCCESS;

    if (!ImageExist() || m_recreateImage) {

        m_currentImageLayout = pImageCreateInfo->initialLayout;

        VkSharedBaseObj<VkImageResource> imageResource;
        if (!imageArrayParent) {
            result = VkImageResource::Create(pImageCreateInfo,
                                             requiredMemProps,
                                             imageResource);
            if (result != VK_SUCCESS) {
                return result;
            }
        } else {
            // We are using a parent array image
            imageResource = imageArrayParent;
        }

        uint32_t baseArrayLayer = imageArrayParent ? imageIndex : 0;
        if (!imageViewArrayParent) {
            VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, baseArrayLayer, 1 };
            result = VkImageResourceView::Create(imageResource,
                                                 subresourceRange,
                                                 m_imageResourceView);

            if (result != VK_SUCCESS) {
                return result;
            }

            // The picture resource's baseArrayLayer is an index into the image view's layers
            m_pictureResourceInfo.baseArrayLayer = 0;
        } else {
            m_pictureResourceInfo.baseArrayLayer = baseArrayLayer;
            m_imageResourceView = imageViewArrayParent;
        }
    }

    m_currentImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_recreateImage      = false;
    m_pictureResourceInfo.sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR;
    m_pictureResourceInfo.codedOffset.x = 0;
    m_pictureResourceInfo.codedOffset.y = 0;
    m_pictureResourceInfo.codedExtent.width = pImageCreateInfo->extent.width;
    m_pictureResourceInfo.codedExtent.height = pImageCreateInfo->extent.height;
    m_pictureResourceInfo.imageViewBinding = m_imageResourceView->GetImageView();

    return result;
}

VkResult VulkanVideoImagePoolNode::Init()
{
    AddRef();
    return VK_SUCCESS;
}

VkResult VulkanVideoImagePoolNode::SetParent(VulkanVideoImagePool* imagePool, int32_t parentIndex)
{
    assert(m_parent == nullptr);
    m_parent      = imagePool;
    assert(m_parentIndex == -1);
    m_parentIndex = parentIndex;

    return VK_SUCCESS;
}

void VulkanVideoImagePoolNode::Deinit()
{
    Release();
    m_imageResourceView = nullptr;
}


VkResult VulkanVideoImagePool::GetImageSetNewLayout(uint32_t imageIndex,
                                                    VkImageLayout newImageLayout) {

    VkResult result = VK_SUCCESS;
    bool recreateImage = !m_imageResources[imageIndex].RecreateImage();

    if (recreateImage) {
        result = m_imageResources[imageIndex].CreateImage(
                           &m_imageCreateInfo,
                           m_requiredMemProps,
                           imageIndex,
                           m_imageArray,
                           m_imageViewArray);

        if (result != VK_SUCCESS) {
            return result;
        }
    }

    bool validImage = m_imageResources[imageIndex].SetNewLayout(newImageLayout);
    assert(validImage);
    if (!validImage) {
	return VK_ERROR_INITIALIZATION_FAILED;
    }

    return result;
}

bool VulkanVideoImagePool::GetAvailableImage(VkSharedBaseObj<VulkanVideoImagePoolNode>& imageResource,
                                             VkImageLayout newImageLayout)
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
        VkResult result = GetImageSetNewLayout(availablePoolNodeIndx, newImageLayout);
        if (result == VK_SUCCESS) {
            m_imageResources[availablePoolNodeIndx].SetParent(this, availablePoolNodeIndx);
            imageResource = &m_imageResources[availablePoolNodeIndx];
            return true;
        }
    }
    return false;
}

bool VulkanVideoImagePool::ReleaseImageToPool(uint32_t imageIndex)
{
    std::lock_guard<std::mutex> lock(m_queueMutex);

    assert(!(m_availablePoolNodes & (1ULL << imageIndex)));
    m_availablePoolNodes |= (1ULL << imageIndex);

    return true;
}

VkResult VulkanVideoImagePool::Create(const VulkanDeviceContext* vkDevCtx,
                                      VkSharedBaseObj<VulkanVideoImagePool>& imagePool)
{
    VkSharedBaseObj<VulkanVideoImagePool> _imagePool(new VulkanVideoImagePool());

    if (_imagePool) {
        imagePool = _imagePool;
        return VK_SUCCESS;
    }
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}


VkResult VulkanVideoImagePool::Configure(uint32_t                     numImages,
                                         VkFormat                     imageFormat,
                                         const VkExtent2D&            maxImageExtent,
                                         VkImageUsageFlags            imageUsage,
                                         uint32_t                     queueFamilyIndex,
                                         VkMemoryPropertyFlags        requiredMemProps,
                                         const VkVideoProfileInfoKHR* pVideoProfile,
                                         bool                         useImageArray,
                                         bool                         useImageViewArray,
                                         bool                         useLinearImage)
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    if (numImages > m_imageResources.size()) {
        assert(!"Number of requested images exceeds the max size of the image array");
        return VK_ERROR_TOO_MANY_OBJECTS;
    }

    const bool reconfigureImages = (m_poolSize &&
        (m_imageCreateInfo.sType == VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)) &&
              ((m_imageCreateInfo.format != imageFormat) ||
               (m_imageCreateInfo.extent.width < maxImageExtent.width) ||
               (m_imageCreateInfo.extent.height < maxImageExtent.height));

    for (uint32_t imageIndex = m_poolSize; imageIndex < numImages; imageIndex++) {
        m_imageResources[imageIndex].Init();
        m_availablePoolNodes |= (1ULL << imageIndex);
    }

    if (useImageViewArray) {
        useImageArray = true;
    }

    if (pVideoProfile) {
        m_videoProfile.InitFromProfile(pVideoProfile);
    }

    m_queueFamilyIndex = queueFamilyIndex;
    m_requiredMemProps = requiredMemProps;

    // Image create info for the images
    m_imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    m_imageCreateInfo.pNext = pVideoProfile ? m_videoProfile.GetProfileListInfo() : nullptr;
    m_imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    m_imageCreateInfo.format = imageFormat;
    m_imageCreateInfo.extent = { maxImageExtent.width, maxImageExtent.height, 1 };
    m_imageCreateInfo.mipLevels = 1;
    m_imageCreateInfo.arrayLayers = useImageArray ? numImages : 1;
    m_imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    m_imageCreateInfo.tiling = useLinearImage ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
    m_imageCreateInfo.usage = imageUsage;
    m_imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    m_imageCreateInfo.queueFamilyIndexCount = 1;
    m_imageCreateInfo.pQueueFamilyIndices = &m_queueFamilyIndex;
    m_imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_imageCreateInfo.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

    if (useImageArray) {
        // Create an image that has the same number of layers as the DPB images required.
        VkResult result = VkImageResource::Create(&m_imageCreateInfo,
                                                  m_requiredMemProps,
                                                  m_imageArray);
        if (result != VK_SUCCESS) {
            return result;
        }
    } else {
        m_imageArray = nullptr;
    }

    if (useImageViewArray) {
        assert(m_imageArray);
        // Create an image view that has the same number of layers as the image.
        // In that scenario, while specifying the resource, the API must specifically choose the image layer.
        VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, numImages };
        VkResult result = VkImageResourceView::Create(m_imageArray,
                                                      subresourceRange,
                                                      m_imageViewArray);

        if (result != VK_SUCCESS) {
            return result;
        }
    }

    uint32_t firstIndex = reconfigureImages ? 0 : m_poolSize;
    uint32_t maxNumImages = std::max(m_poolSize, numImages);
    for (uint32_t imageIndex = firstIndex; imageIndex < maxNumImages; imageIndex++) {

        if (m_imageResources[imageIndex].ImageExist() && reconfigureImages) {

            m_imageResources[imageIndex].RespecImage();

        } else if (!m_imageResources[imageIndex].ImageExist()) {

            VkResult result =
                     m_imageResources[imageIndex].CreateImage(&m_imageCreateInfo,
                                                                       m_requiredMemProps,
                                                                       imageIndex,
                                                                       m_imageArray,
                                                                       m_imageViewArray);

            assert(result == VK_SUCCESS);
            if (result != VK_SUCCESS) {
                return result;
            }
        }
    }

    m_poolSize                = numImages;
    m_usesImageArray          = useImageArray;
    m_usesImageViewArray      = useImageViewArray;
    m_usesLinearImage         = useLinearImage;

    return VK_SUCCESS;
}

void VulkanVideoImagePool::Deinit()
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    for (size_t ndx = 0; ndx < m_poolSize; ndx++) {
        m_imageResources[ndx].Deinit();
    }

    m_imageViewArray = nullptr;
    m_imageArray     = nullptr;
    m_poolSize = 0;
}
