/*
* Copyright 2022 NVIDIA Corporation.
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

#include <atomic>
#include "VkCodecUtils/HelpersDispatchTable.h"
#include "VkCodecUtils/Helpers.h"
#include "VkCodecUtils/VulkanDeviceContext.h"
#include "nvidia_utils/vulkan/ycbcrvkinfo.h"
#include "VkImageResource.h"

VkImageResource::VkImageResource(
                const VkImageCreateInfo* pImageCreateInfo,
                VkImage image, VkDeviceSize imageOffset, VkDeviceSize imageSize,
                VkSharedBaseObj<VulkanDeviceMemoryImpl>& vulkanDeviceMemory)
   : m_refCount(0), m_imageCreateInfo(*pImageCreateInfo)
   , m_image(image), m_imageOffset(imageOffset), m_imageSize(imageSize)
   , m_vulkanDeviceMemory(vulkanDeviceMemory), m_layouts{}
   , m_isLinearImage(false), m_is16Bit(false), m_isSubsampledX(false), m_isSubsampledY(false) {

    const VkMpFormatInfo* mpInfo = YcbcrVkFormatInfo(pImageCreateInfo->format);
    VkImageSubresource subResource = {};
    if (mpInfo == nullptr) {
        m_isLinearImage = true;
        subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &m_layouts[0]);
        return;
    }

    m_isSubsampledX = (mpInfo && mpInfo->planesLayout.secondaryPlaneSubsampledX);
    m_isSubsampledY = (mpInfo && mpInfo->planesLayout.secondaryPlaneSubsampledY);

    // Treat all non 8bpp formats as 16bpp for output to prevent any loss.
    m_is16Bit = (mpInfo->planesLayout.bpp != YCBCRA_8BPP);

    VkMemoryPropertyFlags memoryPropertyFlags = vulkanDeviceMemory->GetMemoryPropertyFlags();
    if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) {
        return;
    }

    m_isLinearImage = true;
    bool isUnnormalizedRgba = false;
    if (mpInfo && (mpInfo->planesLayout.layout == YCBCR_SINGLE_PLANE_UNNORMALIZED) && !(mpInfo->planesLayout.disjoint)) {
        isUnnormalizedRgba = true;
    }

    if (mpInfo && !isUnnormalizedRgba) {
        switch (mpInfo->planesLayout.layout) {
            case YCBCR_SINGLE_PLANE_UNNORMALIZED:
            case YCBCR_SINGLE_PLANE_INTERLEAVED:
                subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
                VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &m_layouts[0]);
                break;
            case YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED:
                subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
                VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &m_layouts[0]);
                subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
                VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &m_layouts[1]);
                break;
            case YCBCR_PLANAR_CBCR_STRIDE_INTERLEAVED:
            case YCBCR_PLANAR_CBCR_BLOCK_JOINED:
            case YCBCR_PLANAR_STRIDE_PADDED:
                subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
                VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &m_layouts[0]);
                subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
                VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &m_layouts[1]);
                subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_2_BIT;
                VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &m_layouts[2]);
                break;
            default:
                assert(0);
        }

    } else {
        VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &m_layouts[0]);
    }
}

VkResult VkImageResource::Create(
                                 const VkImageCreateInfo* pImageCreateInfo,
                                 VkMemoryPropertyFlags memoryPropertyFlags,
                                 VkSharedBaseObj<VkImageResource>& imageResource)
{
    VkResult result = VK_ERROR_INITIALIZATION_FAILED;

    VkDevice device = VulkanDeviceContext::GetThe()->getDevice();
    VkImage image = VK_NULL_HANDLE;

    do {

        result = VulkanDeviceContext::GetThe()->CreateImage(device, pImageCreateInfo, nullptr, &image);
        if (result != VK_SUCCESS) {
            assert(!"CreateImage Failed!");
            break;
        }

        VkMemoryRequirements memoryRequirements = { };
        VulkanDeviceContext::GetThe()->GetImageMemoryRequirements(device, image, &memoryRequirements);

        // Allocate memory for the image
        VkSharedBaseObj<VulkanDeviceMemoryImpl> vkDeviceMemory;
        result = VulkanDeviceMemoryImpl::Create(memoryRequirements,
                                                memoryPropertyFlags,
                                                nullptr,  // pInitializeMemory
                                                0ULL,     // initializeMemorySize
                                                false,    // clearMemory
                                                vkDeviceMemory);
        if (result != VK_SUCCESS) {
            assert(!"Create Memory Failed!");
            break;
        }

        VkDeviceSize imageOffset = 0;
        result = VulkanDeviceContext::GetThe()->BindImageMemory(device, image, *vkDeviceMemory, imageOffset);
        if (result != VK_SUCCESS) {
            assert(!"BindImageMemory Failed!");
            break;
        }

        imageResource = new VkImageResource(
                                            pImageCreateInfo,
                                            image,
                                            imageOffset,
                                            memoryRequirements.size,
                                            vkDeviceMemory);
        if (imageResource == nullptr) {
            break;
        }
        return result;

    } while (0);

    if (device != VK_NULL_HANDLE) {

        if (image != VK_NULL_HANDLE) {
            VulkanDeviceContext::GetThe()->DestroyImage(device, image, nullptr);
        }
    }

    return result;
}


void VkImageResource::Destroy()
{
    if (m_image != VK_NULL_HANDLE) {
        VulkanDeviceContext::GetThe()->DestroyImage(VulkanDeviceContext::GetThe()->getDevice(), m_image, nullptr);
        m_image = VK_NULL_HANDLE;
    }

    m_vulkanDeviceMemory = nullptr;
}

VkResult VkImageResourceView::Create(VkSharedBaseObj<VkImageResource>& imageResource,
                                     VkImageSubresourceRange &imageSubresourceRange,
                                     VkSharedBaseObj<VkImageResourceView>& imageResourceView)
{
    VkDevice device = VulkanDeviceContext::GetThe()->getDevice();
    VkImageView  imageViews[4];
    uint32_t numViews = 0;
    VkImageViewCreateInfo viewInfo = VkImageViewCreateInfo();
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.pNext = nullptr;
    viewInfo.image = imageResource->GetImage();
    viewInfo.viewType = (imageSubresourceRange.layerCount > 1) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = imageResource->GetImageCreateInfo().format;
    viewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                            VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    viewInfo.subresourceRange = imageSubresourceRange;
    viewInfo.flags = 0;
    VkResult result = VulkanDeviceContext::GetThe()->CreateImageView(device, &viewInfo, nullptr, &imageViews[numViews]);
    if (result != VK_SUCCESS) {
        return result;
    }
    numViews++;

    const VkMpFormatInfo* mpInfo = YcbcrVkFormatInfo(viewInfo.format);
    if (mpInfo) {
        uint32_t numPlanes = 0;
        // Create separate image views for Y and CbCr planes
        viewInfo.format = mpInfo->vkPlaneFormat[numPlanes];  // For the Y plane
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT << numPlanes;
        result = VulkanDeviceContext::GetThe()->CreateImageView(device, &viewInfo, nullptr, &imageViews[numViews]);
        if (result != VK_SUCCESS) {
            return result;
        }
        numViews++;
        numPlanes++;

        if (mpInfo->planesLayout.numberOfExtraPlanes > 0) {
            viewInfo.format = mpInfo->vkPlaneFormat[numPlanes];  // For the CbCr plane
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT << numPlanes;
            result = VulkanDeviceContext::GetThe()->CreateImageView(device, &viewInfo, nullptr, &imageViews[numViews]);
            if (result != VK_SUCCESS) {
                return result;
            }
            numViews++;
            numPlanes++;

            if (mpInfo->planesLayout.numberOfExtraPlanes > 1) {
                viewInfo.format = mpInfo->vkPlaneFormat[numPlanes];  // For the CbCr plane
                viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT << numPlanes;
                result = VulkanDeviceContext::GetThe()->CreateImageView(device, &viewInfo, nullptr, &imageViews[numViews]);
                if (result != VK_SUCCESS) {
                    return result;
                }
                numViews++;
                numPlanes++;
            }
        }
    }

    imageResourceView = new VkImageResourceView(imageResource,
                                                numViews, numViews - 1,
                                                imageViews, imageSubresourceRange);

    return result;
}

VkImageResourceView::~VkImageResourceView()
{
    for (uint32_t imageViewIndx = 0; imageViewIndx < m_numViews; imageViewIndx++) {
        if (m_imageViews[imageViewIndx] != VK_NULL_HANDLE) {
            VulkanDeviceContext::GetThe()->DestroyImageView(VulkanDeviceContext::GetThe()->getDevice(), m_imageViews[imageViewIndx], nullptr);
            m_imageViews[imageViewIndx] = VK_NULL_HANDLE;
        }
    }

    m_imageResource = nullptr;
}
