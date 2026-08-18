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

#ifndef _VULKANVIDEOFRAMEBUFFER_H_
#define _VULKANVIDEOFRAMEBUFFER_H_

#include <cstdint>

#include "VkCodecUtils/VkVideoRefCountBase.h"
#include "vkvideo_parser/VulkanVideoParser.h"
#include "vulkan_interfaces.h"
#include "VkCodecUtils/VkImageResource.h"
#include "VkCodecUtils/VulkanDecodedFrame.h"
#include "VkVideoCore/DecodeFrameBufferIf.h"

// Overload the equality operator
inline bool operator==(const VkExtent3D& lhs, const VkExtent3D& rhs) {
    return (lhs.width == rhs.width) &&
           (lhs.height == rhs.height) &&
           (lhs.depth == rhs.depth);
}

// Overload the inequality operator
inline bool operator!=(const VkExtent3D& lhs, const VkExtent3D& rhs) {
    return !(lhs == rhs);
}

struct DecodedFrameRelease {
    int32_t pictureIndex;
    VkVideotimestamp timestamp;
    uint32_t hasConsummerSignalFence : 1;
    uint32_t hasConsummerSignalSemaphore : 1;
    // For debugging
    uint64_t displayOrder;
    uint64_t decodeOrder;
};

class VkParserVideoPictureParameters;

class VulkanVideoFrameBuffer : public IVulkanVideoFrameBufferParserCb {
public:

    static constexpr size_t maxImages = 32;

    enum class InitType : uint8_t {
            INIT_INVALIDATE_IMAGES_LAYOUT = 0,      // Only invalidate image layouts, don't recreate or add images.
            INIT_RECREATE_IMAGES          = 1 << 1, // Recreate images because their formats or extent has increased
            INIT_INCRESE_NUM_SLOTS        = 1 << 2, // Increase the number of the slots available.
        };

    // Synchronization
    struct FrameSynchronizationInfo {
        VkFence frameCompleteFence;
        VkSemaphore frameCompleteSemaphore;
        VkFence frameConsumerDoneFence;
        VkSemaphore frameConsumerDoneSemaphore;
        VkQueryPool queryPool;
        uint32_t startQueryId;
        uint32_t numQueries;
        DecodeFrameBufferIf::ImageSpecsIndex imageSpecsIndex;
        uint32_t hasFrameCompleteSignalFence : 1;
        uint32_t hasFrameCompleteSignalSemaphore : 1;
        uint32_t syncOnFrameCompleteFence : 1;
        uint32_t syncOnFrameConsumerDoneFence : 1;
    };

    struct ReferencedObjectsInfo {

        // The bitstream Buffer
        const VkVideoRefCountBase*     pBitstreamData;
        // PPS
        const VkVideoRefCountBase*     pStdPps;
        // SPS
        const VkVideoRefCountBase*     pStdSps;
        // VPS
        const VkVideoRefCountBase*     pStdVps;

        ReferencedObjectsInfo(const VkVideoRefCountBase* pBitstreamDataRef,
                              const VkVideoRefCountBase* pStdPpsRef,
                              const VkVideoRefCountBase* pStdSpsRef,
                              const VkVideoRefCountBase* pStdVpsRef = nullptr)
        : pBitstreamData(pBitstreamDataRef)
        , pStdPps(pStdPpsRef)
        , pStdSps(pStdSpsRef)
        , pStdVps(pStdVpsRef) {}
    };

    struct PictureResourceInfo {
        VkImage       image;
        VkFormat      imageFormat;
        VkImageLayout currentImageLayout;
    };

    struct ImageSpec {

        ImageSpec()
        : imageTypeIdx(InvalidImageTypeIdx)
        , reserved(0)
        , imageTypeMask(0)
        , usesImageArray(false)
        , usesImageViewArray(false)
        , deferCreate(false)
        , createInfo()
        , memoryProperty() {}

        uint8_t               imageTypeIdx;  // InvalidImageTypeIdx is an invalid index and the entry is skipped
        uint8_t               reserved;
        uint16_t              imageTypeMask;
        uint32_t              usesImageArray     : 1;
        uint32_t              usesImageViewArray : 1;
        uint32_t              deferCreate        : 1;
        VkImageCreateInfo     createInfo;
        VkMemoryPropertyFlags memoryProperty;
        // must be valid if m_usesImageArray is true
        VkSharedBaseObj<VkImageResource>     imageArray;
        // must be valid if m_usesImageViewArray is true
        VkSharedBaseObj<VkImageResourceView> imageViewArray;
    };

    virtual int32_t InitImagePool(const VkVideoProfileInfoKHR* pDecodeProfile,
                                  uint32_t                 numImages,
                                  uint32_t                 maxNumImageTypeIdx,
                                  const std::array<VulkanVideoFrameBuffer::ImageSpec, DecodeFrameBufferIf::MAX_PER_FRAME_IMAGE_TYPES>& imageSpecs,
                                  uint32_t                 queueFamilyIndex,
                                  int32_t                  numImagesToPreallocate) = 0;

    virtual int32_t QueuePictureForDecode(int8_t picId, VkParserDecodePictureInfo* pDecodePictureInfo,
                                          ReferencedObjectsInfo* pReferencedObjectsInfo,
                                          FrameSynchronizationInfo* pFrameSynchronizationInfo) = 0;
    virtual int32_t DequeueDecodedPicture(VulkanDecodedFrame* pDecodedFrame) = 0;
    virtual int32_t ReleaseDisplayedPicture(DecodedFrameRelease** pDecodedFramesRelease, uint32_t numFramesToRelease) = 0;
    virtual int32_t GetImageResourcesByIndex(uint32_t numResources, const int8_t* referenceSlotIndexes, uint8_t imageTypeIdx,
                                             VkVideoPictureResourceInfoKHR* pictureResources,
                                             PictureResourceInfo* pictureResourcesInfo,
                                             VkImageLayout newImageLayerLayout = VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR) = 0;
    virtual int32_t GetCurrentImageResourceByIndex(int8_t referenceSlotIndex, uint8_t imageTypeIdx,
                                                   VkVideoPictureResourceInfoKHR* pPictureResource,
                                                   PictureResourceInfo* pPictureResourceInfo,
                                                   VkImageLayout newImageLayerLayout = VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR) = 0;
    virtual int32_t GetCurrentImageResourceByIndex(int8_t referenceSlotIndex, uint8_t imageTypeIdx,
                                                   VkSharedBaseObj<VkImageResourceView>& imageView) = 0;
    virtual int32_t ReleaseImageResources(uint32_t numResources, const uint32_t* indexes) = 0;
    virtual uint64_t SetPicNumInDecodeOrder(int32_t picId, uint64_t picNumInDecodeOrder) = 0;
    virtual int32_t SetPicNumInDisplayOrder(int32_t picId, int32_t picNumInDisplayOrder) = 0;
    virtual uint32_t GetCurrentNumberQueueSlots() const = 0;

    ~VulkanVideoFrameBuffer() override { }

    static VkResult Create(VkSharedBaseObj<VulkanVideoFrameBuffer>& vkVideoFrameBuffer);
};

#endif /* _VULKANVIDEOFRAMEBUFFER_H_ */
