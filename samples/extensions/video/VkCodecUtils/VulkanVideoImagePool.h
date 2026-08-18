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

#ifndef _VULKANVIDEOIMAGEPOOL_H_
#define _VULKANVIDEOIMAGEPOOL_H_

#include <assert.h>
#include <stdint.h>

#include "VkCodecUtils/VkVideoRefCountBase.h"
#include "vulkan_interfaces.h"
#include "VkVideoCore/VkVideoCoreProfile.h"
#include "VkCodecUtils/VkImageResource.h"

class VulkanVideoImagePool;

class VulkanVideoImagePoolNode : public VkVideoRefCountBase {
public:

    // VulkanVideoImagePool is a friend class to be able to call SetParent()
    friend class VulkanVideoImagePool;

    VulkanVideoImagePoolNode()
        : m_refCount()
        , m_currentImageLayout(VK_IMAGE_LAYOUT_UNDEFINED)
        , m_pictureResourceInfo()
        , m_parentIndex(-1)
        , m_recreateImage()
    {
    }

    virtual int32_t AddRef()
    {
        return ++m_refCount;
    }

    virtual int32_t Release();

    VkResult CreateImage(const VkImageCreateInfo              *pImageCreateInfo,
						 VkMemoryPropertyFlags                 requiredMemProps,
	                     uint32_t                              imageIndex,
	                     VkSharedBaseObj<VkImageResource>     &imageArrayParent,
	                     VkSharedBaseObj<VkImageResourceView> &imageViewArrayParent);

    VkResult Init();

    void Deinit();

    VulkanVideoImagePoolNode (const VulkanVideoImagePoolNode &srcObj) = delete;
    VulkanVideoImagePoolNode (VulkanVideoImagePoolNode &&srcObj) = delete;

    ~VulkanVideoImagePoolNode()
    {
        Deinit();
    }

    bool GetImageView(VkSharedBaseObj<VkImageResourceView>& imageResourceView) {
        if (ImageExist()) {
            imageResourceView = m_imageResourceView;
            return true;
        }
        return false;
    }

    bool ImageExist() {

        return (!!m_imageResourceView && (m_imageResourceView->GetImageView() != VK_NULL_HANDLE));
    }

    bool RecreateImage() {
        return !ImageExist() || m_recreateImage;
    }

    void RespecImage() {
        m_recreateImage = true;
    }

    bool SetNewLayout(VkImageLayout newImageLayout)
    {
        if (RecreateImage() || !ImageExist()) {
            return false;
        }

        m_currentImageLayout = newImageLayout;

        return true;
    }

    VkVideoPictureResourceInfoKHR* GetPictureResourceInfo() { return &m_pictureResourceInfo; }

    int32_t GetImageIndex() { return m_parentIndex; }

    virtual int32_t GetRefCount()
    {
        assert(m_refCount > 0);
        return m_refCount;
    }

private:
    VkResult SetParent(VulkanVideoImagePool* imagePool, int32_t parentIndex);

  std::atomic<int32_t>                  m_refCount;
    VkImageLayout                         m_currentImageLayout;
    VkVideoPictureResourceInfoKHR         m_pictureResourceInfo;
    VkSharedBaseObj<VkImageResourceView>  m_imageResourceView;
    VkSharedBaseObj<VulkanVideoImagePool> m_parent;
    int32_t                               m_parentIndex;
    uint32_t                              m_recreateImage : 1;
};

class VulkanVideoImagePool : public VkVideoRefCountBase {
public:

    static constexpr size_t maxImages = 64;

    VulkanVideoImagePool()
        : m_refCount()
        , m_queueFamilyIndex((uint32_t)-1)
        , m_imageCreateInfo()
        , m_requiredMemProps(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        , m_poolSize(0)
        , m_nextNodeToUse(0)
        , m_usesImageArray(false)
        , m_usesImageViewArray(false)
        , m_usesLinearImage(false)
        , m_availablePoolNodes(0UL)
        , m_imageResources(maxImages)
        , m_imageArray()
        , m_imageViewArray()
    {
    }

    static VkResult Create(const VulkanDeviceContext* vkDevCtx,
                           VkSharedBaseObj<VulkanVideoImagePool>& imagePool);

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

    VkResult Configure(uint32_t                     numImages,
                       VkFormat                     imageFormat,
                       const VkExtent2D&            maxImageExtent,
                       VkImageUsageFlags            imageUsage,
                       uint32_t                     queueFamilyIndex,
                       VkMemoryPropertyFlags        requiredMemProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       const VkVideoProfileInfoKHR* pVideoProfile = nullptr,
                       bool                         useImageArray = false,
                       bool                         useImageViewArray = false,
                       bool                         useLinear = false);

    void Deinit();

    ~VulkanVideoImagePool()
    {
        Deinit();
    }

    VulkanVideoImagePoolNode& operator[](unsigned int index)
    {
        assert(index < m_imageResources.size());
        return m_imageResources[index];
    }

    size_t size()
    {
        return m_poolSize;
    }

    bool GetAvailableImage(VkSharedBaseObj<VulkanVideoImagePoolNode>&  imageResource,
                           VkImageLayout newImageLayout);

    bool ReleaseImageToPool(uint32_t imageIndex);

private:
    VkResult GetImageSetNewLayout(uint32_t imageIndex,
                                  VkImageLayout newImageLayout);

  std::atomic<int32_t>                  m_refCount;
    std::mutex                            m_queueMutex;
    uint32_t                              m_queueFamilyIndex;
    VkVideoCoreProfile                    m_videoProfile;
    VkImageCreateInfo                     m_imageCreateInfo;
    VkMemoryPropertyFlags                 m_requiredMemProps;
    uint32_t                              m_poolSize;
    uint32_t                              m_nextNodeToUse;
    uint32_t                              m_usesImageArray : 1;
    uint32_t                              m_usesImageViewArray : 1;
    uint32_t                              m_usesLinearImage : 1;
    uint64_t                              m_availablePoolNodes;
    std::vector<VulkanVideoImagePoolNode> m_imageResources;
    VkSharedBaseObj<VkImageResource>      m_imageArray;     // must be valid if m_usesImageArray is true
    VkSharedBaseObj<VkImageResourceView>  m_imageViewArray; // must be valid if m_usesImageViewArray is true
};

#endif /* _VULKANVIDEOIMAGEPOOL_H_ */
