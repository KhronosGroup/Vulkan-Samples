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
#include <array>
#include <atomic>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <vector>

#include "VkCodecUtils/Helpers.h"
#include "VkCodecUtils/HelpersDispatchTable.h"
#include "VkCodecUtils/VkImageResource.h"
#include "VkCodecUtils/VulkanDeviceContext.h"
#include "VkVideoCore/VkVideoCoreProfile.h"
#include "VulkanVideoFrameBuffer.h"
#include "vkvideo_parser/PictureBufferBase.h"

static VkSharedBaseObj<VkImageResourceView> emptyImageView;

class NvPerFrameDecodeResources : public vkPicBuffBase {

    struct ImageViewState {
        VkImageLayout                         currentLayerLayout;
        VkSharedBaseObj<VkImageResourceView>  view;
        VkSharedBaseObj<VkImageResourceView>  singleLevelView;
        uint32_t                              recreateImage : 1;
        uint32_t                              layerNum : 8;
    };

public:
    NvPerFrameDecodeResources()
        : m_picDispInfo()
        , m_frameCompleteFence()
        , m_frameCompleteSemaphore()
        , m_frameConsumerDoneFence()
        , m_frameConsumerDoneSemaphore()
        , m_hasFrameCompleteSignalFence(false)
        , m_hasFrameCompleteSignalSemaphore(false)
        , m_hasConsummerSignalFence(false)
        , m_hasConsummerSignalSemaphore(false)
        , m_inDecodeQueue(false)
        , m_inDisplayQueue(false)
        , m_ownedByConsummer(false)
        , m_imageViewState()
    {
    }

    VkResult CreateImage( const VulkanVideoFrameBuffer::ImageSpec* pImageSpec,
                          uint32_t imageIndex,
                          VkSharedBaseObj<VkImageResource>&  imageArrayParent,
                          VkSharedBaseObj<VkImageResourceView>& imageViewArrayParent);

    VkResult init( );

    void Deinit();

    NvPerFrameDecodeResources (const NvPerFrameDecodeResources &srcObj) = delete;
    NvPerFrameDecodeResources (NvPerFrameDecodeResources &&srcObj) = delete;

    ~NvPerFrameDecodeResources() override
    {
        Deinit();
    }

    VkSharedBaseObj<VkImageResourceView>& GetImageView(uint8_t imageTypeIdx) {
        if (ImageExist(imageTypeIdx))
		{
			return m_imageViewState[imageTypeIdx].view;
		}
		return emptyImageView;
	}

    VkSharedBaseObj<VkImageResourceView>& GetSingleLevelImageView(uint8_t imageTypeIdx) {
        if (ImageExist(imageTypeIdx))
		{
			return m_imageViewState[imageTypeIdx].singleLevelView;
		}
		return emptyImageView;
	}

    bool ImageExist(uint8_t imageTypeIdx) const {

        if ((imageTypeIdx == InvalidImageTypeIdx) || !(imageTypeIdx < DecodeFrameBufferIf::MAX_PER_FRAME_IMAGE_TYPES)) {
            return false;
        }

        return (!!m_imageViewState[imageTypeIdx].view && (m_imageViewState[imageTypeIdx].view->GetImageView() != VK_NULL_HANDLE));
    }

    void InvalidateImageLayout(uint8_t imageTypeIdx) {

        m_imageViewState[imageTypeIdx].currentLayerLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    void SetRecreateImage(uint8_t imageTypeIdx) {

        m_imageViewState[imageTypeIdx].recreateImage = true;
    }

    bool GetImageSetNewLayout(uint8_t imageTypeIdx, VkImageLayout newImageLayout,
                              VkVideoPictureResourceInfoKHR* pPictureResource,
                              VulkanVideoFrameBuffer::PictureResourceInfo* pPictureResourceInfo) {


        if (m_imageViewState[imageTypeIdx].recreateImage || !ImageExist(imageTypeIdx)) {
            return false;
        }

        if (pPictureResourceInfo) {
            pPictureResourceInfo->image = m_imageViewState[imageTypeIdx].view->GetImageResource()->GetImage();
            pPictureResourceInfo->imageFormat = m_imageViewState[imageTypeIdx].view->GetImageResource()->GetImageCreateInfo().format;
            pPictureResourceInfo->currentImageLayout = m_imageViewState[imageTypeIdx].currentLayerLayout;
        }

        if (VK_IMAGE_LAYOUT_MAX_ENUM != newImageLayout) {
            m_imageViewState[imageTypeIdx].currentLayerLayout = newImageLayout;
        }

        if (pPictureResource) {
            pPictureResource->imageViewBinding = m_imageViewState[imageTypeIdx].view->GetImageView();
        }

        return true;
    }

    VkParserDecodePictureInfo m_picDispInfo;
    VkFence m_frameCompleteFence;
    VkSemaphore m_frameCompleteSemaphore;
    VkFence m_frameConsumerDoneFence;
    VkSemaphore m_frameConsumerDoneSemaphore;
    DecodeFrameBufferIf::ImageSpecsIndex m_imageSpecsIndex;
    uint32_t m_hasFrameCompleteSignalFence : 1;
    uint32_t m_hasFrameCompleteSignalSemaphore : 1;
    uint32_t m_hasConsummerSignalFence : 1;
    uint32_t m_hasConsummerSignalSemaphore : 1;
    uint32_t m_inDecodeQueue : 1;
    uint32_t m_inDisplayQueue : 1;
    uint32_t m_ownedByConsummer : 1;
    // VPS
    VkSharedBaseObj<VkVideoRefCountBase>  stdVps;
    // SPS
    VkSharedBaseObj<VkVideoRefCountBase>  stdSps;
    // PPS
    VkSharedBaseObj<VkVideoRefCountBase>  stdPps;
    // The bitstream Buffer
    VkSharedBaseObj<VkVideoRefCountBase>  bitstreamData;

private:
    std::array<ImageViewState, DecodeFrameBufferIf::MAX_PER_FRAME_IMAGE_TYPES> m_imageViewState;
};

class NvPerFrameDecodeImageSet {
public:

    NvPerFrameDecodeImageSet()
        : m_queueFamilyIndex(static_cast<uint32_t>(-1))
        , m_numImages(0)
        , m_maxNumImageTypeIdx(0)
        , m_perFrameDecodeResources(VulkanVideoFrameBuffer::maxImages)
        , m_imageSpecs()
    {
    }

    int32_t init(const VkVideoProfileInfoKHR* pDecodeProfile,
        uint32_t                 numImages,
        uint32_t                 maxNumImageTypeIdx,
        const std::array<VulkanVideoFrameBuffer::ImageSpec, DecodeFrameBufferIf::MAX_PER_FRAME_IMAGE_TYPES>& imageSpecs,
        uint32_t                 queueFamilyIndex);

    void Deinit();

    ~NvPerFrameDecodeImageSet()
    {
        Deinit();
    }

    NvPerFrameDecodeResources& operator[](unsigned int index)
    {
        assert(index < m_perFrameDecodeResources.size());
        return m_perFrameDecodeResources[index];
    }

    uint32_t size() const
    {
        return m_numImages;
    }

    VkResult GetImageSetNewLayout(uint32_t imageIndex, uint8_t imageTypeIdx,
                                  VkImageLayout newImageLayout,
                                  VkVideoPictureResourceInfoKHR* pPictureResource = nullptr,
                                  VulkanVideoFrameBuffer::PictureResourceInfo* pPictureResourceInfo = nullptr) {

        VkResult result = VK_SUCCESS;
        if (pPictureResource) {
            if (m_imageSpecs[imageTypeIdx].imageViewArray) {
                // We have an image view that has the same number of layers as the image.
                // In that scenario, while specifying the resource, the API must specifically choose the image layer.
                pPictureResource->baseArrayLayer = imageIndex;
            } else {
                // Let the image view sub-resource specify the image layer.
                pPictureResource->baseArrayLayer = 0;
            }
        }

        bool validImage = m_perFrameDecodeResources[imageIndex].GetImageSetNewLayout(imageTypeIdx,
                                                                                     newImageLayout,
                                                                                     pPictureResource,
                                                                                     pPictureResourceInfo);

        if (!validImage) {
            result = m_perFrameDecodeResources[imageIndex].CreateImage(
                               &m_imageSpecs[imageTypeIdx],
                               imageIndex,
                               m_imageSpecs[imageTypeIdx].imageArray,
                               m_imageSpecs[imageTypeIdx].imageViewArray);

            if (result == VK_SUCCESS) {
                validImage = m_perFrameDecodeResources[imageIndex].GetImageSetNewLayout(imageTypeIdx,
                                                                                        newImageLayout,
                                                                                        pPictureResource,
                                                                                        pPictureResourceInfo);

                assert(validImage);
            }
        }

        return result;
    }

private:
    uint32_t                               m_queueFamilyIndex;
    VkVideoCoreProfile                     m_videoProfile;
    uint32_t                               m_numImages;
    uint32_t                               m_maxNumImageTypeIdx;
    std::vector<NvPerFrameDecodeResources> m_perFrameDecodeResources;
    std::array<VulkanVideoFrameBuffer::ImageSpec, DecodeFrameBufferIf::MAX_PER_FRAME_IMAGE_TYPES>    m_imageSpecs;
};

class VkVideoFrameBuffer : public VulkanVideoFrameBuffer {
public:

    VkVideoFrameBuffer()
        : m_refCount(0)
        , m_queryPool()
        , m_ownedByDisplayMask(0)
        , m_frameNumInDisplayOrder(0)
        , m_numberParameterUpdates(0)
        , m_maxNumImageTypeIdx(0)
        , m_debug()
    {
    }

    int32_t AddRef() override;
    int32_t Release() override;

    VkResult CreateVideoQueries(uint32_t numSlots, const VkVideoProfileInfoKHR* pDecodeProfile)
    {
        assert (numSlots <= maxImages);

        if ((m_queryPool == VK_NULL_HANDLE) && VulkanDeviceContext::GetThe()->GetVideoDecodeQueryResultStatusSupport()) {
            // It would be difficult to resize a query pool, so allocate the maximum possible slot.
            numSlots = maxImages;
            VkQueryPoolCreateInfo queryPoolCreateInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
            queryPoolCreateInfo.pNext = pDecodeProfile;
            queryPoolCreateInfo.queryType = VK_QUERY_TYPE_RESULT_STATUS_ONLY_KHR;
            queryPoolCreateInfo.queryCount = numSlots; // m_numDecodeSurfaces frames worth

            return VulkanDeviceContext::GetThe()->CreateQueryPool(VulkanDeviceContext::GetThe()->getDevice(), &queryPoolCreateInfo, nullptr, &m_queryPool);
        }

        return VK_SUCCESS;
    }

    void DestroyVideoQueries() {
        if (m_queryPool != VkQueryPool()) {
            VulkanDeviceContext::GetThe()->DestroyQueryPool(VulkanDeviceContext::GetThe()->getDevice(), m_queryPool, nullptr);
            m_queryPool = VkQueryPool();
        }
    }

    uint32_t  FlushDisplayQueue()
    {
        std::lock_guard lock(m_displayQueueMutex);

        uint32_t flushedImages = 0;
        while (!m_displayFrames.empty()) {
            int8_t pictureIndex = m_displayFrames.front();
            assert((pictureIndex >= 0) && ((uint32_t)pictureIndex < m_perFrameDecodeImageSet.size()));
            m_displayFrames.pop();
            if (m_perFrameDecodeImageSet[static_cast<uint32_t>(pictureIndex)].IsAvailable()) {
                // The frame is not released yet - force release it.
                m_perFrameDecodeImageSet[static_cast<uint32_t>(pictureIndex)].Release();
            }
            flushedImages++;
        }

        return flushedImages;
    }

    virtual int32_t InitImagePool(const VkVideoProfileInfoKHR* pDecodeProfile,
                                  uint32_t                 numImages,
                                  uint32_t                 maxNumImageTypeIdx,
                                  const std::array<ImageSpec, DecodeFrameBufferIf::MAX_PER_FRAME_IMAGE_TYPES>& imageSpecs,
                                  uint32_t                 queueFamilyIndex,
                                  int32_t                  numImagesToPreallocate)
    {
        std::lock_guard<std::mutex> lock(m_displayQueueMutex);

        assert(numImages && (numImages <= maxImages) && pDecodeProfile);

        VkResult result = CreateVideoQueries(numImages, pDecodeProfile);
        if (result != VK_SUCCESS) {
            return 0;
        }

        int32_t imageSetCreateResult =
                m_perFrameDecodeImageSet.init( pDecodeProfile,
                                              numImages,
                                              maxNumImageTypeIdx,
                                              imageSpecs,
                                              queueFamilyIndex);

        if (imageSetCreateResult >= 0) {
            m_maxNumImageTypeIdx = maxNumImageTypeIdx;
        }
        m_numberParameterUpdates++;

        return imageSetCreateResult;
    }

    void Deinitialize() {

        FlushDisplayQueue();

        DestroyVideoQueries();

        m_ownedByDisplayMask = 0;
        m_frameNumInDisplayOrder = 0;

        m_perFrameDecodeImageSet.Deinit();

        if (m_queryPool != VkQueryPool()) {
            VulkanDeviceContext::GetThe()->DestroyQueryPool(VulkanDeviceContext::GetThe()->getDevice(), m_queryPool, nullptr);
            m_queryPool = VkQueryPool();
        }
    };

    virtual int32_t QueueDecodedPictureForDisplay(int8_t picId, VulkanVideoDisplayPictureInfo* pDispInfo)
    {
        assert((uint32_t)picId < m_perFrameDecodeImageSet.size());

        std::lock_guard<std::mutex> lock(m_displayQueueMutex);
        m_perFrameDecodeImageSet[picId].m_displayOrder = m_frameNumInDisplayOrder++;
        m_perFrameDecodeImageSet[picId].m_timestamp = pDispInfo->timestamp;
        m_perFrameDecodeImageSet[picId].m_inDisplayQueue = true;
        m_perFrameDecodeImageSet[picId].AddRef();

        m_displayFrames.push((uint8_t)picId);

        if (m_debug) {
            std::cout << "==> Queue Display Picture picIdx: " << (uint32_t)picId
                      << "\t\tdisplayOrder: " << m_perFrameDecodeImageSet[picId].m_displayOrder << "\tdecodeOrder: " << m_perFrameDecodeImageSet[picId].m_decodeOrder
                      << "\ttimestamp " << m_perFrameDecodeImageSet[picId].m_timestamp << std::endl;
        }
        return picId;
    }

    virtual int32_t QueuePictureForDecode(int8_t picId, VkParserDecodePictureInfo* pDecodePictureInfo,
                                          ReferencedObjectsInfo* pReferencedObjectsInfo,
                                          FrameSynchronizationInfo* pFrameSynchronizationInfo)
    {
        assert((uint32_t)picId < m_perFrameDecodeImageSet.size());

        if (pFrameSynchronizationInfo->syncOnFrameCompleteFence == 1) {
            // Check here that the frame for this entry (for this command buffer) has already completed decoding.
            // Otherwise we may step over a hot command buffer by starting a new recording.
            // This fence wait should be NOP in 99.9% of the cases, because the decode queue is deep enough to
            // ensure the frame has already been completed.
            assert(m_perFrameDecodeImageSet[picId].m_frameCompleteFence != VK_NULL_HANDLE);
            vk::WaitAndResetFence(VulkanDeviceContext::GetThe()->getDevice(), m_perFrameDecodeImageSet[picId].m_frameCompleteFence,
                                  true, "frameCompleteFence");
        }

        if ((pFrameSynchronizationInfo->syncOnFrameConsumerDoneFence  == 1) &&
             ((m_perFrameDecodeImageSet[picId].m_hasConsummerSignalSemaphore == 0) ||
              (m_perFrameDecodeImageSet[picId].m_frameConsumerDoneSemaphore == VK_NULL_HANDLE)) &&
                (m_perFrameDecodeImageSet[picId].m_hasConsummerSignalFence == 1) &&
                (m_perFrameDecodeImageSet[picId].m_frameConsumerDoneFence != VK_NULL_HANDLE)) {

            vk::WaitAndResetFence(VulkanDeviceContext::GetThe()->getDevice(), m_perFrameDecodeImageSet[picId].m_frameConsumerDoneFence,
                                  true, "frameConsumerDoneFence");

        }

        std::lock_guard<std::mutex> lock(m_displayQueueMutex);
        m_perFrameDecodeImageSet[picId].m_picDispInfo = *pDecodePictureInfo;
        m_perFrameDecodeImageSet[picId].m_inDecodeQueue = true;
        m_perFrameDecodeImageSet[picId].m_imageSpecsIndex = pFrameSynchronizationInfo->imageSpecsIndex;
        m_perFrameDecodeImageSet[picId].stdPps = const_cast<VkVideoRefCountBase*>(pReferencedObjectsInfo->pStdPps);
        m_perFrameDecodeImageSet[picId].stdSps = const_cast<VkVideoRefCountBase*>(pReferencedObjectsInfo->pStdSps);
        m_perFrameDecodeImageSet[picId].stdVps = const_cast<VkVideoRefCountBase*>(pReferencedObjectsInfo->pStdVps);
        m_perFrameDecodeImageSet[picId].bitstreamData = const_cast<VkVideoRefCountBase*>(pReferencedObjectsInfo->pBitstreamData);

        if (m_debug) {
            std::cout << "==> Queue Decode Picture picIdx: " << (uint32_t)picId
                      << "\t\tdisplayOrder: " << m_perFrameDecodeImageSet[picId].m_displayOrder << "\tdecodeOrder: " << m_perFrameDecodeImageSet[picId].m_decodeOrder
                      << std::endl;
        }

        if (pFrameSynchronizationInfo->hasFrameCompleteSignalFence) {
            pFrameSynchronizationInfo->frameCompleteFence = m_perFrameDecodeImageSet[picId].m_frameCompleteFence;
            if (pFrameSynchronizationInfo->frameCompleteFence) {
                m_perFrameDecodeImageSet[picId].m_hasFrameCompleteSignalFence = true;
            }
        }

        if (m_perFrameDecodeImageSet[picId].m_hasConsummerSignalFence) {
            pFrameSynchronizationInfo->frameConsumerDoneFence = m_perFrameDecodeImageSet[picId].m_frameConsumerDoneFence;
            m_perFrameDecodeImageSet[picId].m_hasConsummerSignalFence = false;
        }

        if (pFrameSynchronizationInfo->hasFrameCompleteSignalSemaphore) {
            pFrameSynchronizationInfo->frameCompleteSemaphore = m_perFrameDecodeImageSet[picId].m_frameCompleteSemaphore;
            if (pFrameSynchronizationInfo->frameCompleteSemaphore) {
                m_perFrameDecodeImageSet[picId].m_hasFrameCompleteSignalSemaphore = true;
            }
        }

        if (m_perFrameDecodeImageSet[picId].m_hasConsummerSignalSemaphore) {
            pFrameSynchronizationInfo->frameConsumerDoneSemaphore = m_perFrameDecodeImageSet[picId].m_frameConsumerDoneSemaphore;
            m_perFrameDecodeImageSet[picId].m_hasConsummerSignalSemaphore = false;
        }

        pFrameSynchronizationInfo->queryPool = m_queryPool;
        pFrameSynchronizationInfo->startQueryId = picId;
        pFrameSynchronizationInfo->numQueries = 1;

        return picId;
    }

    // dequeue
    virtual int32_t DequeueDecodedPicture(VulkanDecodedFrame* pDecodedFrame)
    {
        int numberofPendingFrames = 0;
        int pictureIndex = -1;
        std::lock_guard<std::mutex> lock(m_displayQueueMutex);
        if (!m_displayFrames.empty()) {
            numberofPendingFrames = (int)m_displayFrames.size();
            pictureIndex = m_displayFrames.front();
            assert((pictureIndex >= 0) && ((uint32_t)pictureIndex < m_perFrameDecodeImageSet.size()));
            assert(!(m_ownedByDisplayMask & (1 << pictureIndex)));
            m_ownedByDisplayMask |= (1 << pictureIndex);
            m_displayFrames.pop();
            m_perFrameDecodeImageSet[pictureIndex].m_inDisplayQueue = false;
            m_perFrameDecodeImageSet[pictureIndex].m_ownedByConsummer = true;
        }

        if ((uint32_t)pictureIndex < m_perFrameDecodeImageSet.size()) {
            pDecodedFrame->pictureIndex = pictureIndex;

            pDecodedFrame->imageLayerIndex = m_perFrameDecodeImageSet[pictureIndex].m_picDispInfo.imageLayerIndex;

            {
                uint8_t displayOutImageType = m_perFrameDecodeImageSet[pictureIndex].m_imageSpecsIndex.displayOut;
                if (m_perFrameDecodeImageSet[pictureIndex].ImageExist(displayOutImageType)) {
                    pDecodedFrame->imageViews[VulkanDisplayFrame::IMAGE_VIEW_TYPE_OPTIMAL_DISPLAY].view = m_perFrameDecodeImageSet[pictureIndex].GetImageView(displayOutImageType);
                    pDecodedFrame->imageViews[VulkanDisplayFrame::IMAGE_VIEW_TYPE_OPTIMAL_DISPLAY].singleLevelView = m_perFrameDecodeImageSet[pictureIndex].GetSingleLevelImageView(displayOutImageType);
                    pDecodedFrame->imageViews[VulkanDisplayFrame::IMAGE_VIEW_TYPE_OPTIMAL_DISPLAY].inUse = true;
                }
            }

            {
                uint8_t linearOutImageType = m_perFrameDecodeImageSet[pictureIndex].m_imageSpecsIndex.linearOut;
                if (linearOutImageType == InvalidImageTypeIdx) {
                    linearOutImageType = m_perFrameDecodeImageSet[pictureIndex].m_imageSpecsIndex.decodeOut;
                }

                if (m_perFrameDecodeImageSet[pictureIndex].ImageExist(linearOutImageType)) {
                    pDecodedFrame->imageViews[VulkanDisplayFrame::IMAGE_VIEW_TYPE_LINEAR].view = m_perFrameDecodeImageSet[pictureIndex].GetImageView(linearOutImageType);
                    pDecodedFrame->imageViews[VulkanDisplayFrame::IMAGE_VIEW_TYPE_LINEAR].singleLevelView = m_perFrameDecodeImageSet[pictureIndex].GetSingleLevelImageView(linearOutImageType);
                    pDecodedFrame->imageViews[VulkanDisplayFrame::IMAGE_VIEW_TYPE_LINEAR].inUse = true;
                }
            }

            pDecodedFrame->displayWidth  = m_perFrameDecodeImageSet[pictureIndex].m_picDispInfo.displayWidth;
            pDecodedFrame->displayHeight = m_perFrameDecodeImageSet[pictureIndex].m_picDispInfo.displayHeight;

            if (m_perFrameDecodeImageSet[pictureIndex].m_hasFrameCompleteSignalFence) {
                pDecodedFrame->frameCompleteFence = m_perFrameDecodeImageSet[pictureIndex].m_frameCompleteFence;
                m_perFrameDecodeImageSet[pictureIndex].m_hasFrameCompleteSignalFence = false;
            } else {
                pDecodedFrame->frameCompleteFence = VkFence();
            }

            if (m_perFrameDecodeImageSet[pictureIndex].m_hasFrameCompleteSignalSemaphore) {
                pDecodedFrame->frameCompleteSemaphore = m_perFrameDecodeImageSet[pictureIndex].m_frameCompleteSemaphore;
                m_perFrameDecodeImageSet[pictureIndex].m_hasFrameCompleteSignalSemaphore = false;
            } else {
                pDecodedFrame->frameCompleteSemaphore = VkSemaphore();
            }

            pDecodedFrame->frameConsumerDoneFence = m_perFrameDecodeImageSet[pictureIndex].m_frameConsumerDoneFence;
            pDecodedFrame->frameConsumerDoneSemaphore = m_perFrameDecodeImageSet[pictureIndex].m_frameConsumerDoneSemaphore;

            pDecodedFrame->timestamp = m_perFrameDecodeImageSet[pictureIndex].m_timestamp;
            pDecodedFrame->decodeOrder = m_perFrameDecodeImageSet[pictureIndex].m_decodeOrder;
            pDecodedFrame->displayOrder = m_perFrameDecodeImageSet[pictureIndex].m_displayOrder;

            pDecodedFrame->queryPool = m_queryPool;
            pDecodedFrame->startQueryId = pictureIndex;
            pDecodedFrame->numQueries = 1;
        }

        if (m_debug) {
            std::cout << "<<<<<<<<<<< Dequeue from Display: " << pictureIndex << " out of "
                      << numberofPendingFrames << " ===========" << std::endl;
        }
        return numberofPendingFrames;
    }

    virtual int32_t ReleaseDisplayedPicture(DecodedFrameRelease** pDecodedFramesRelease, uint32_t numFramesToRelease)
    {
        std::lock_guard<std::mutex> lock(m_displayQueueMutex);
        for (uint32_t i = 0; i < numFramesToRelease; i++) {
            const DecodedFrameRelease* pDecodedFrameRelease = pDecodedFramesRelease[i];
            int picId = pDecodedFrameRelease->pictureIndex;
            assert((picId >= 0) && ((uint32_t)picId < m_perFrameDecodeImageSet.size()));

            assert(m_perFrameDecodeImageSet[picId].m_decodeOrder == pDecodedFrameRelease->decodeOrder);
            assert(m_perFrameDecodeImageSet[picId].m_displayOrder == pDecodedFrameRelease->displayOrder);

            assert(m_ownedByDisplayMask & (1 << picId));
            m_ownedByDisplayMask &= ~(1 << picId);
            m_perFrameDecodeImageSet[picId].m_inDecodeQueue = false;
            m_perFrameDecodeImageSet[picId].m_ownedByConsummer = false;
            m_perFrameDecodeImageSet[picId].Release();

            m_perFrameDecodeImageSet[picId].m_hasConsummerSignalFence = pDecodedFrameRelease->hasConsummerSignalFence;
            m_perFrameDecodeImageSet[picId].m_hasConsummerSignalSemaphore = pDecodedFrameRelease->hasConsummerSignalSemaphore;
        }
        return 0;
    }

    virtual int32_t GetImageResourcesByIndex(uint32_t numResources,
                                             const int8_t* referenceSlotIndexes,
                                             uint8_t imageTypeIdx,
                                             VkVideoPictureResourceInfoKHR* pPictureResources,
                                             PictureResourceInfo* pPictureResourcesInfo,
                                             VkImageLayout newImageLayerLayout = VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR)
    {
        assert(pPictureResources);
        std::lock_guard<std::mutex> lock(m_displayQueueMutex);
        for (unsigned int resId = 0; resId < numResources; resId++) {
            if ((uint32_t)referenceSlotIndexes[resId] < m_perFrameDecodeImageSet.size()) {

                VkResult result = m_perFrameDecodeImageSet.GetImageSetNewLayout(
                                     referenceSlotIndexes[resId], imageTypeIdx,
                                     newImageLayerLayout,
                                     &pPictureResources[resId],
                                     &pPictureResourcesInfo[resId]);

                assert(result == VK_SUCCESS);
                if (result != VK_SUCCESS) {
                    return -1;
                }

                assert(pPictureResources[resId].sType == VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR);
            }
        }
        return numResources;
    }

    virtual int32_t GetCurrentImageResourceByIndex(int8_t referenceSlotIndex, uint8_t imageTypeIdx,
                                                   VkVideoPictureResourceInfoKHR* pPictureResource,
                                                   PictureResourceInfo* pPictureResourceInfo,
                                                   VkImageLayout newImageLayerLayout = VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR)
    {
        assert(pPictureResource);
        std::lock_guard<std::mutex> lock(m_displayQueueMutex);
        if ((uint32_t)referenceSlotIndex < m_perFrameDecodeImageSet.size()) {

            VkResult result = m_perFrameDecodeImageSet.GetImageSetNewLayout(
                                                                            referenceSlotIndex,
                                                                            imageTypeIdx,
                                                                            newImageLayerLayout,
                                                                            pPictureResource,
                                                                            pPictureResourceInfo);
            assert(result == VK_SUCCESS);
            if (result != VK_SUCCESS) {
                return -1;
            }

            assert(pPictureResource->sType == VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR);

        }
        return referenceSlotIndex;
    }

    virtual int32_t GetCurrentImageResourceByIndex(int8_t referenceSlotIndex, uint8_t imageTypeIdx,
                                                   VkSharedBaseObj<VkImageResourceView>& imageView)
    {
        std::lock_guard<std::mutex> lock(m_displayQueueMutex);
        if ((uint32_t)referenceSlotIndex < m_perFrameDecodeImageSet.size()) {
            imageView = m_perFrameDecodeImageSet[referenceSlotIndex].GetImageView(imageTypeIdx);
            return referenceSlotIndex;
        }
        return -1;
    }

    virtual int32_t ReleaseImageResources(uint32_t numResources, const uint32_t* indexes)
    {
        std::lock_guard<std::mutex> lock(m_displayQueueMutex);
        for (unsigned int resId = 0; resId < numResources; resId++) {
            if ((uint32_t)indexes[resId] < m_perFrameDecodeImageSet.size()) {
                m_perFrameDecodeImageSet[indexes[resId]].Deinit();
            }
        }
        return (int32_t)m_perFrameDecodeImageSet.size();
    }

    virtual uint64_t SetPicNumInDecodeOrder(int32_t picId, uint64_t picNumInDecodeOrder)
    {
        std::lock_guard<std::mutex> lock(m_displayQueueMutex);
        if ((uint32_t)picId < m_perFrameDecodeImageSet.size()) {
            uint64_t oldPicNumInDecodeOrder = m_perFrameDecodeImageSet[picId].m_decodeOrder;
            m_perFrameDecodeImageSet[picId].m_decodeOrder = picNumInDecodeOrder;
            return oldPicNumInDecodeOrder;
        }
        assert(false);
        return (uint64_t)-1;
    }

    virtual int32_t SetPicNumInDisplayOrder(int32_t picId, int32_t picNumInDisplayOrder)
    {
        std::lock_guard<std::mutex> lock(m_displayQueueMutex);
        if ((uint32_t)picId < m_perFrameDecodeImageSet.size()) {
            int32_t oldPicNumInDisplayOrder = m_perFrameDecodeImageSet[picId].m_displayOrder;
            m_perFrameDecodeImageSet[picId].m_displayOrder = picNumInDisplayOrder;
            return oldPicNumInDisplayOrder;
        }
        assert(false);
        return -1;
    }

    virtual vkPicBuffBase* ReservePictureBuffer()
    {
        std::lock_guard<std::mutex> lock(m_displayQueueMutex);
        int32_t foundPicId = -1;
        int64_t minDecodeOrder = m_perFrameDecodeImageSet[0].m_decodeOrder + 1000;
        uint32_t numAvailablePictures = 0;
        for (uint32_t picId = 0; picId < m_perFrameDecodeImageSet.size(); picId++) {
            if (m_perFrameDecodeImageSet[picId].IsAvailable()) {
                numAvailablePictures++;
                if ((int64_t)m_perFrameDecodeImageSet[picId].m_decodeOrder < minDecodeOrder) {
                    foundPicId = picId;
                    minDecodeOrder = (int64_t)m_perFrameDecodeImageSet[picId].m_decodeOrder;
                }
            }
        }

        if (foundPicId >= 0) {
            m_perFrameDecodeImageSet[foundPicId].Reset();
            m_perFrameDecodeImageSet[foundPicId].AddRef();
            m_perFrameDecodeImageSet[foundPicId].m_picIdx = foundPicId;

            if (m_debug) {
                std::cout << "==> ReservePictureBuffer picIdx: " << (uint32_t)foundPicId << " of " << numAvailablePictures
                          << "\t\tdisplayOrder: " << m_perFrameDecodeImageSet[foundPicId].m_decodeOrder << "\tdecodeOrder: "
                          << m_perFrameDecodeImageSet[foundPicId].m_decodeOrder
                          << "\ttimestamp " << m_perFrameDecodeImageSet[foundPicId].m_timestamp << std::endl;
            }

            return &m_perFrameDecodeImageSet[foundPicId];
        }

        assert(foundPicId >= 0);
        return NULL;
    }

    virtual uint32_t GetCurrentNumberQueueSlots() const
    {
        std::lock_guard<std::mutex> lock(m_displayQueueMutex);
        return m_perFrameDecodeImageSet.size();
    }

    virtual ~VkVideoFrameBuffer()
    {
        Deinitialize();
    }

private:
    std::atomic<int32_t>     m_refCount;
    mutable std::mutex       m_displayQueueMutex;
    NvPerFrameDecodeImageSet m_perFrameDecodeImageSet;
    std::queue<uint8_t>      m_displayFrames;
    VkQueryPool              m_queryPool;
    uint32_t                 m_ownedByDisplayMask;
    int32_t                  m_frameNumInDisplayOrder;
    uint32_t                 m_numberParameterUpdates;
    uint32_t                 m_maxNumImageTypeIdx : 4;
    uint32_t                 m_debug : 1;
};

VkResult VulkanVideoFrameBuffer::Create(VkSharedBaseObj<VulkanVideoFrameBuffer>& vkVideoFrameBuffer)
{
    VkSharedBaseObj videoFrameBuffer(new VkVideoFrameBuffer());
    if (videoFrameBuffer) {
        vkVideoFrameBuffer = videoFrameBuffer;
        return VK_SUCCESS;
    }
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

int32_t VkVideoFrameBuffer::AddRef()
{
    return ++m_refCount;
}

int32_t VkVideoFrameBuffer::Release()
{
    uint32_t ret;
    ret = --m_refCount;
    // Destroy the device if refcount reaches zero
    if (ret == 0) {
        delete this;
    }
    return ret;
}

VkResult NvPerFrameDecodeResources::CreateImage( const VulkanVideoFrameBuffer::ImageSpec* pImageSpec,
                                                 uint32_t                                 imageIndex,
                                                 VkSharedBaseObj<VkImageResource>&        imageArrayParent,
                                                 VkSharedBaseObj<VkImageResourceView>&    imageViewArrayParent)
{
    VkResult result = VK_SUCCESS;

    if (false) {
        std::cout << "Create FB Image: " << (int)pImageSpec->imageTypeIdx << " : " << imageIndex
                                                               << ", extent: " << pImageSpec->createInfo.extent.width << " x "
                                                               << pImageSpec->createInfo.extent.height << ", format "
                                                               << pImageSpec->createInfo.format
                                                               << std::endl;
    }
    if (!ImageExist(pImageSpec->imageTypeIdx) || m_imageViewState[pImageSpec->imageTypeIdx].recreateImage) {

        m_imageViewState[pImageSpec->imageTypeIdx].currentLayerLayout = pImageSpec->createInfo.initialLayout;

        VkSharedBaseObj<VkImageResource> imageResource;
        if (!imageArrayParent) {
            result = VkImageResource::Create(&pImageSpec->createInfo,
                                             pImageSpec->memoryProperty,
                                             imageResource);
            if (result != VK_SUCCESS) {
                return result;
            }
        } else {
            // We are using a parent array image
            imageResource = imageArrayParent;
        }

        if (!imageViewArrayParent) {

            uint32_t baseArrayLayer = imageArrayParent ? imageIndex : 0;
            VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, baseArrayLayer, 1 };
            result = VkImageResourceView::Create( imageResource,
                                                 subresourceRange,
                                                 m_imageViewState[pImageSpec->imageTypeIdx].view);

            if (result != VK_SUCCESS) {
                return result;
            }

            m_imageViewState[pImageSpec->imageTypeIdx].singleLevelView = m_imageViewState[pImageSpec->imageTypeIdx].view;

        } else {

            m_imageViewState[pImageSpec->imageTypeIdx].view = imageViewArrayParent;

            VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, imageIndex, 1 };
            result = VkImageResourceView::Create( imageResource,
                                                 subresourceRange,
                                                 m_imageViewState[pImageSpec->imageTypeIdx].singleLevelView);
            if (result != VK_SUCCESS) {
                return result;
            }
        }
    }

    m_imageViewState[pImageSpec->imageTypeIdx].currentLayerLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_imageViewState[pImageSpec->imageTypeIdx].recreateImage = false;
    return result;
}

VkResult NvPerFrameDecodeResources::init()
{
    // The fence waited on for the first frame should be signaled.
    const VkFenceCreateInfo fenceFrameCompleteInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr,
                                                       VK_FENCE_CREATE_SIGNALED_BIT };
    VkResult result = VulkanDeviceContext::GetThe()->CreateFence(VulkanDeviceContext::GetThe()->getDevice(), &fenceFrameCompleteInfo, nullptr, &m_frameCompleteFence);

    const VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr };
    result = VulkanDeviceContext::GetThe()->CreateFence(VulkanDeviceContext::GetThe()->getDevice(), &fenceInfo, nullptr, &m_frameConsumerDoneFence);
    assert(result == VK_SUCCESS);

    const VkSemaphoreCreateInfo semInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr };
    result = VulkanDeviceContext::GetThe()->CreateSemaphore(VulkanDeviceContext::GetThe()->getDevice(), &semInfo, nullptr, &m_frameCompleteSemaphore);
    assert(result == VK_SUCCESS);
    result = VulkanDeviceContext::GetThe()->CreateSemaphore(VulkanDeviceContext::GetThe()->getDevice(), &semInfo, nullptr, &m_frameConsumerDoneSemaphore);
    assert(result == VK_SUCCESS);

    Reset();

    return result;
}

void NvPerFrameDecodeResources::Deinit()
{
    bitstreamData = nullptr;
    stdPps = nullptr;
    stdSps = nullptr;
    stdVps = nullptr;

    if (m_frameCompleteFence != VkFence()) {
        VulkanDeviceContext::GetThe()->DestroyFence(VulkanDeviceContext::GetThe()->getDevice(), m_frameCompleteFence, nullptr);
        m_frameCompleteFence = VkFence();
    }

    if (m_frameConsumerDoneFence != VkFence()) {
        VulkanDeviceContext::GetThe()->DestroyFence(VulkanDeviceContext::GetThe()->getDevice(), m_frameConsumerDoneFence, nullptr);
        m_frameConsumerDoneFence = VkFence();
    }

    if (m_frameCompleteSemaphore != VkSemaphore()) {
        VulkanDeviceContext::GetThe()->DestroySemaphore(VulkanDeviceContext::GetThe()->getDevice(), m_frameCompleteSemaphore, nullptr);
        m_frameCompleteSemaphore = VkSemaphore();
    }

    if (m_frameConsumerDoneSemaphore != VkSemaphore()) {
        VulkanDeviceContext::GetThe()->DestroySemaphore(VulkanDeviceContext::GetThe()->getDevice(), m_frameConsumerDoneSemaphore, nullptr);
        m_frameConsumerDoneSemaphore = VkSemaphore();
    }

    for (uint32_t imageTypeIdx = 0; imageTypeIdx < DecodeFrameBufferIf::MAX_PER_FRAME_IMAGE_TYPES; imageTypeIdx++) {

        m_imageViewState[imageTypeIdx].view = nullptr;
        m_imageViewState[imageTypeIdx].singleLevelView = nullptr;
    }

    Reset();
}

int32_t NvPerFrameDecodeImageSet::init(const VkVideoProfileInfoKHR* pDecodeProfile,
                                       uint32_t                 numImages,
                                       uint32_t                 maxNumImageTypeIdx,
                                       const std::array<VulkanVideoFrameBuffer::ImageSpec, DecodeFrameBufferIf::MAX_PER_FRAME_IMAGE_TYPES>& imageSpecs,
                                       uint32_t                 queueFamilyIndex)
{
    if (numImages > m_perFrameDecodeResources.size()) {
        assert(!"Number of requested images exceeds the max size of the image array");
        return -1;
    }

    for (uint32_t imageIndex = m_numImages; imageIndex < numImages; imageIndex++) {
        VkResult result = m_perFrameDecodeResources[imageIndex].init();
        assert(result == VK_SUCCESS);
        if (result != VK_SUCCESS) {
            return -1;
        }
    }

    m_videoProfile.InitFromProfile(pDecodeProfile);

    m_queueFamilyIndex = queueFamilyIndex;

    for (uint32_t imageTypeIdx = 0; imageTypeIdx < maxNumImageTypeIdx; imageTypeIdx++) {

        if (!(imageSpecs[imageTypeIdx].imageTypeIdx < DecodeFrameBufferIf::MAX_PER_FRAME_IMAGE_TYPES)) {
            continue;
        }

        const bool reconfigureImages = ((m_numImages != 0) &&
                (m_imageSpecs[imageTypeIdx].createInfo.sType == VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)) &&
                      ((m_imageSpecs[imageTypeIdx].createInfo.format != imageSpecs[imageTypeIdx].createInfo.format) ||
                       (m_imageSpecs[imageTypeIdx].createInfo.extent.width < imageSpecs[imageTypeIdx].createInfo.extent.width) ||
                       (m_imageSpecs[imageTypeIdx].createInfo.extent.height < imageSpecs[imageTypeIdx].createInfo.extent.height));


        const bool usesImageViewArray = imageSpecs[imageTypeIdx].usesImageViewArray;
        const bool usesImageArray     = usesImageViewArray ? true : imageSpecs[imageTypeIdx].usesImageArray;

        const bool updateFrameBufferGeometry = (m_numImages != 0) &&
                (m_imageSpecs[imageTypeIdx].createInfo.extent != imageSpecs[imageTypeIdx].createInfo.extent);

        VkExtent3D maxExtent(imageSpecs[imageTypeIdx].createInfo.extent);
        if (reconfigureImages || updateFrameBufferGeometry) {

            if (false) {
                std::cout << "Reconfigure FB: " << (int)imageTypeIdx << ", extent: " << m_imageSpecs[imageTypeIdx].createInfo.extent.width << " x "
                                                                       << m_imageSpecs[imageTypeIdx].createInfo.extent.height << " to "
                                                                       << imageSpecs[imageTypeIdx].createInfo.extent.width << " x "
                                                                       << imageSpecs[imageTypeIdx].createInfo.extent.height
                                                                       << std::endl;
            }
            assert(m_imageSpecs[imageTypeIdx].usesImageArray == imageSpecs[imageTypeIdx].usesImageArray);
            assert(m_imageSpecs[imageTypeIdx].usesImageViewArray == imageSpecs[imageTypeIdx].usesImageViewArray);
            maxExtent.width  = std::max(m_imageSpecs[imageTypeIdx].createInfo.extent.width,  imageSpecs[imageTypeIdx].createInfo.extent.width);
            maxExtent.height = std::max(m_imageSpecs[imageTypeIdx].createInfo.extent.height, imageSpecs[imageTypeIdx].createInfo.extent.height);
            maxExtent.depth  = std::max(m_imageSpecs[imageTypeIdx].createInfo.extent.depth,  imageSpecs[imageTypeIdx].createInfo.extent.depth);
        }

        m_imageSpecs[imageTypeIdx] = imageSpecs[imageTypeIdx];
        if (reconfigureImages || updateFrameBufferGeometry) {
            m_imageSpecs[imageTypeIdx].createInfo.extent = maxExtent;
        }
        m_imageSpecs[imageTypeIdx].createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        m_imageSpecs[imageTypeIdx].createInfo.pNext = m_videoProfile.GetProfileListInfo();
        m_imageSpecs[imageTypeIdx].createInfo.queueFamilyIndexCount = 1;
        m_imageSpecs[imageTypeIdx].createInfo.pQueueFamilyIndices = &m_queueFamilyIndex;
        m_imageSpecs[imageTypeIdx].usesImageViewArray = usesImageViewArray;
        m_imageSpecs[imageTypeIdx].usesImageArray = usesImageArray;

        if (usesImageArray) {
            // Create an image that has the same number of layers as the DPB images required.
            VkResult result = VkImageResource::Create(
                                                      &m_imageSpecs[imageTypeIdx].createInfo,
                                                      m_imageSpecs[imageTypeIdx].memoryProperty,
                                                      m_imageSpecs[imageTypeIdx].imageArray);
            if (result != VK_SUCCESS) {
                return -1;
            }
        } else {
            m_imageSpecs[imageTypeIdx].imageArray = nullptr;
        }

        if (usesImageViewArray) {
            assert(m_imageSpecs[imageTypeIdx].imageArray);
            // Create an image view that has the same number of layers as the image.
            // In that scenario, while specifying the resource, the API must specifically choose the image layer.
            VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, numImages };
            VkResult result = VkImageResourceView::Create( m_imageSpecs[imageTypeIdx].imageArray,
                                                          subresourceRange,
                                                          m_imageSpecs[imageTypeIdx].imageViewArray);

            if (result != VK_SUCCESS) {
                return -1;
            }
        }

        uint32_t firstIndex = reconfigureImages ? 0 : m_numImages;
        uint32_t maxNumImages = std::max(m_numImages, numImages);
        for (uint32_t imageIndex = firstIndex; imageIndex < maxNumImages; imageIndex++) {

            bool imageExist = m_perFrameDecodeResources[imageIndex].ImageExist(imageTypeIdx);

            if (imageExist && reconfigureImages) {

                m_perFrameDecodeResources[imageIndex].SetRecreateImage(imageTypeIdx);

            } else if (!imageExist && !m_imageSpecs[imageTypeIdx].deferCreate) {

                VkResult result = m_perFrameDecodeResources[imageIndex].CreateImage(
                                                                                    &m_imageSpecs[imageTypeIdx],
                                                                                    imageIndex,
                                                                                    m_imageSpecs[imageTypeIdx].imageArray,
                                                                                    m_imageSpecs[imageTypeIdx].imageViewArray);

                assert(result == VK_SUCCESS);
                if (result != VK_SUCCESS) {
                    return -1;
                }

            }
        }

        if (!reconfigureImages) {
            // If we are not reconfiguring (resizing) images, invalidate the existing images
            // layout in order for them to be cleared next time before use by transitioning
            // their layout from undefined to any of the encoder/decode/DPB.
            for (uint32_t imageIndex = 0; imageIndex < m_numImages; imageIndex++) {
                m_perFrameDecodeResources[imageIndex].InvalidateImageLayout(imageTypeIdx);
            }
        }
    }

    m_numImages               = std::max(m_numImages, numImages); // Don't trim the size
    m_maxNumImageTypeIdx      = maxNumImageTypeIdx;

    return (int32_t)numImages;
}

void NvPerFrameDecodeImageSet::Deinit()
{
    for (size_t ndx = 0; ndx < m_numImages; ndx++) {
        m_perFrameDecodeResources[ndx].Deinit();
    }

    for (uint32_t imageTypeIdx = 0; imageTypeIdx < DecodeFrameBufferIf::MAX_PER_FRAME_IMAGE_TYPES; imageTypeIdx++) {

        m_imageSpecs[imageTypeIdx].imageViewArray = nullptr;
        m_imageSpecs[imageTypeIdx].imageArray = nullptr;
    }

    m_numImages = 0;
}
