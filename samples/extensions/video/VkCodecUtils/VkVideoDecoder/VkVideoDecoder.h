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

#pragma once

#include <cassert>
#include <atomic>
#include <iostream>
#include <queue>
#include <cstdint>
#include <string>
#include <vector>

#include "VkCodecUtils/VulkanBistreamBufferImpl.h"
#include "VkCodecUtils/VulkanDeviceContext.h"
#include "VkCodecUtils/VulkanFilterYuvCompute.h"
#include "VkCodecUtils/VulkanVideoReferenceCountedPool.h"
#include "VkCodecUtils/VulkanVideoSession.h"
#include "VkParserVideoPictureParameters.h"
#include "VulkanVideoFrameBuffer/VulkanVideoFrameBuffer.h"
#include "vkvideo_parser/StdVideoPictureParametersSet.h"
#include "vkvideo_parser/VulkanVideoParser.h"
#include "vkvideo_parser/VulkanVideoParserIf.h"
#include "vulkan_interfaces.h"

struct Rect {
    int32_t l;
    int32_t t;
    int32_t r;
    int32_t b;
};

struct Dim {
    int w, h;
};

struct NvVkDecodeFrameDataSlot {
    uint32_t                                            slot;
    VkCommandBuffer                                     commandBuffer;
};

class NvVkDecodeFrameData {

    using VulkanBitstreamBufferPool = VulkanVideoRefCountedPool<VulkanBitstreamBufferImpl, 64>;

public:
    explicit NvVkDecodeFrameData()
       : m_videoCommandPool() {}

    void deinit() {

        if (m_videoCommandPool) {
            VulkanDeviceContext::GetThe()->FreeCommandBuffers(VulkanDeviceContext::GetThe()->getDevice(), m_videoCommandPool, (uint32_t)m_commandBuffers.size(), &m_commandBuffers[0]);
            VulkanDeviceContext::GetThe()->DestroyCommandPool(VulkanDeviceContext::GetThe()->getDevice(), m_videoCommandPool, nullptr);
            m_videoCommandPool = VkCommandPool();
        }
    }

    ~NvVkDecodeFrameData() {
        deinit();
    }

    size_t resize(size_t maxDecodeFramesCount) {
        size_t allocatedCommandBuffers = 0;
        if (!m_videoCommandPool) {
            VkCommandPoolCreateInfo cmdPoolInfo = {};
            cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            cmdPoolInfo.queueFamilyIndex = VulkanDeviceContext::GetThe()->GetVideoDecodeQueueFamilyIdx();
            VkResult result = VulkanDeviceContext::GetThe()->CreateCommandPool(VulkanDeviceContext::GetThe()->getDevice(), &cmdPoolInfo, nullptr, &m_videoCommandPool);
            assert(result == VK_SUCCESS);
            if (result != VK_SUCCESS) {
                fprintf(stderr, "\nERROR: CreateCommandPool() result: 0x%x\n", result);
            }

            VkCommandBufferAllocateInfo cmdInfo = {};
            cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdInfo.commandBufferCount = (uint32_t)maxDecodeFramesCount;
            cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdInfo.commandPool = m_videoCommandPool;

            m_commandBuffers.resize(maxDecodeFramesCount);
            result = VulkanDeviceContext::GetThe()->AllocateCommandBuffers(VulkanDeviceContext::GetThe()->getDevice(), &cmdInfo, &m_commandBuffers[0]);
            assert(result == VK_SUCCESS);
            if (result != VK_SUCCESS) {
                fprintf(stderr, "\nERROR: AllocateCommandBuffers() result: 0x%x\n", result);
            } else {
                allocatedCommandBuffers = maxDecodeFramesCount;
            }
        } else {
            allocatedCommandBuffers = m_commandBuffers.size();
            assert(maxDecodeFramesCount <= allocatedCommandBuffers);
        }

        return allocatedCommandBuffers;
    }

    VkCommandBuffer GetCommandBuffer(uint32_t slot) {
        assert(slot < m_commandBuffers.size());
        return m_commandBuffers[slot];
    }

    size_t size() {
        return m_commandBuffers.size();
    }

    VulkanBitstreamBufferPool& GetBitstreamBuffersQueue() { return m_bitstreamBuffersQueue; }

private:
    VkCommandPool                                             m_videoCommandPool;
    std::vector<VkCommandBuffer>                              m_commandBuffers;
    VulkanBitstreamBufferPool                                 m_bitstreamBuffersQueue;
};

/**
 * @brief Base class for decoder interface.
 */
class VkVideoDecoder : public IVulkanVideoDecoderHandler {
public:
    static VkPhysicalDevice GetPhysDevice() { return VulkanDeviceContext::GetThe() ? VulkanDeviceContext::GetThe()->getPhysicalDevice() : VK_NULL_HANDLE; }
    enum { MAX_RENDER_TARGETS = 32 }; // Must be 32 or less (used as uint32_t bitmask of active render targets)

    static VkSharedBaseObj<VkVideoDecoder> invalidVkDecoder;

    enum DecoderFeatures { ENABLE_LINEAR_OUTPUT       = (1 << 0),
                           ENABLE_HW_LOAD_BALANCING   = (1 << 1),
                           ENABLE_POST_PROCESS_FILTER = (1 << 2),
                           ENABLE_GRAPHICS_TEXTURE_SAMPLING = (1 << 3),
                         };

    static VkResult Create(VkSharedBaseObj<VulkanVideoFrameBuffer>& videoFrameBuffer,
                           int32_t videoQueueIndx = 0,
                           uint32_t enableDecoderFeatures = 0, // a list of DecoderFeatures
                           VulkanFilterYuvCompute::FilterType filterType = VulkanFilterYuvCompute::YCBCRCOPY,
                           int32_t numDecodeImagesInFlight = 8,
                           int32_t numDecodeImagesToPreallocate = -1, // preallocate the maximum required
                           int32_t numBitstreamBuffersToPreallocate = 8,
                           VkSharedBaseObj<VkVideoDecoder>& vkVideoDecoder = invalidVkDecoder);

    static const char* GetVideoCodecString(VkVideoCodecOperationFlagBitsKHR codec);
    static const char* GetVideoChromaFormatString(VkVideoChromaSubsamplingFlagBitsKHR chromaFormat);

    int32_t AddRef() override;
    int32_t Release() override;

    /**
     *   @brief  This function is used to get information about the video stream (codec, display parameters etc)
     */
    const VkParserDetectedVideoFormat* GetVideoFormatInfo() const
	{
        assert(m_videoFormat.coded_width);
        return &m_videoFormat;
    }

    /**
    *   @brief  This callback function gets called when when decoding of sequence starts,
    */
    int32_t StartVideoSequence(VkParserDetectedVideoFormat* pVideoFormat) override;

    bool UpdatePictureParameters(VkSharedBaseObj<StdVideoPictureParametersSet>& pictureParametersObject,
                                         VkSharedBaseObj<VkVideoRefCountBase>& client) override;

    /**
     *   @brief  This callback function gets called when a picture is ready to be decoded.
     */
    int32_t DecodePictureWithParameters(VkParserPerFrameDecodeParameters* pPicParams, VkParserDecodePictureInfo* pDecodePictureInfo) override;

    VkDeviceSize GetBitstreamBuffer(VkDeviceSize size,
                                      VkDeviceSize minBitstreamBufferOffsetAlignment,
                                      VkDeviceSize minBitstreamBufferSizeAlignment,
                                      const uint8_t* pInitializeBufferMemory,
                                      VkDeviceSize initializeBufferMemorySize,
                                      VkSharedBaseObj<VulkanBitstreamBuffer>& bitstreamBuffer) override;
private:

    explicit VkVideoDecoder(
                   VkSharedBaseObj<VulkanVideoFrameBuffer>& videoFrameBuffer,
                   int32_t  videoQueueIndx = 0,
                   uint32_t enableDecoderFeatures = 0, // a list of DecoderFeatures
                   VulkanFilterYuvCompute::FilterType filterType = VulkanFilterYuvCompute::YCBCRCOPY,
                   int32_t  numDecodeImagesInFlight = 8,
                   int32_t  numDecodeImagesToPreallocate = -1, // preallocate the maximum required
                   int32_t  numBitstreamBuffersToPreallocate = 8)
        : m_currentVideoQueueIndx(videoQueueIndx)
        , m_refCount(0)
        , m_codedExtent {}
        , m_videoFormat {}
        , m_numDecodeImagesInFlight(numDecodeImagesInFlight)
        , m_numDecodeImagesToPreallocate(numDecodeImagesToPreallocate)
        , m_capabilityFlags()
        , m_videoSession(nullptr)
        , m_videoFrameBuffer(videoFrameBuffer)
        , m_decodePicCount(0)
        , m_hwLoadBalancingTimelineSemaphore()
        , m_dpbAndOutputCoincide(VK_TRUE)
        , m_videoMaintenance1FeaturesSupported(VK_FALSE)
        , m_enableDecodeComputeFilter((enableDecoderFeatures & ENABLE_POST_PROCESS_FILTER) != 0)
        , m_enableGraphicsSampleFromDecodeOutput((enableDecoderFeatures & ENABLE_GRAPHICS_TEXTURE_SAMPLING) != 0)
        , m_useImageArray(VK_FALSE)
        , m_useImageViewArray(VK_FALSE)
        , m_useSeparateOutputImages(VK_FALSE)
        , m_useLinearOutput((enableDecoderFeatures & ENABLE_LINEAR_OUTPUT) != 0)
        , m_useSeparateLinearImages(VK_FALSE)
        , m_useTransferOperation(VK_FALSE)
        , m_resetDecoder(VK_TRUE)
        , m_dumpDecodeData(VK_FALSE)
        , m_numImageTypes(1) // At least the decoder requires images for DPB
        , m_numImageTypesEnabled(DecodeFrameBufferIf::IMAGE_TYPE_MASK_DECODE_DPB), m_numBitstreamBuffersToPreallocate(numBitstreamBuffersToPreallocate)
        , m_maxStreamBufferSize()
        , m_filterType(filterType)
    {

        assert(VulkanDeviceContext::GetThe()->GetVideoDecodeQueueFamilyIdx() != -1);
        assert(VulkanDeviceContext::GetThe()->GetVideoDecodeNumQueues() > 0);

        if (m_currentVideoQueueIndx < 0) {
            m_currentVideoQueueIndx = VulkanDeviceContext::GetThe()->GetVideoDecodeDefaultQueueIndex();
        } else if (VulkanDeviceContext::GetThe()->GetVideoDecodeNumQueues() > 1) {
            m_currentVideoQueueIndx %= VulkanDeviceContext::GetThe()->GetVideoDecodeNumQueues();
            assert(m_currentVideoQueueIndx < VulkanDeviceContext::GetThe()->GetVideoDecodeNumQueues());
            assert(m_currentVideoQueueIndx >= 0);
        } else {
            m_currentVideoQueueIndx = 0;
        }

        if (enableDecoderFeatures & ENABLE_HW_LOAD_BALANCING) {

            if (VulkanDeviceContext::GetThe()->GetVideoDecodeNumQueues() < 2) {
                std::cout << "\t WARNING: Enabling HW Load Balancing for device with only " <<
                        VulkanDeviceContext::GetThe()->GetVideoDecodeNumQueues() << " queue!!!" << std::endl;
            }

            // Create the timeline semaphore object for the HW LoadBalancing Timeline Semaphore
            VkSemaphoreTypeCreateInfo timelineCreateInfo;
            timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            timelineCreateInfo.pNext = nullptr;
            timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            timelineCreateInfo.initialValue = static_cast<uint64_t>(-1); // assuming m_decodePicCount starts at 0.

            VkSemaphoreCreateInfo createInfo;
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            createInfo.pNext = &timelineCreateInfo;
            createInfo.flags = 0;

            VkResult result = VulkanDeviceContext::GetThe()->CreateSemaphore(VulkanDeviceContext::GetThe()->getDevice(), &createInfo, nullptr, &m_hwLoadBalancingTimelineSemaphore);
            if (result == VK_SUCCESS) {
                m_currentVideoQueueIndx = 0; // start with index zero
            }
            std::cout << "\t Enabling HW Load Balancing for device with "
                      << VulkanDeviceContext::GetThe()->GetVideoDecodeNumQueues() << " queues" << std::endl;
        }

    }

    ~VkVideoDecoder() override;
    void Deinitialize();

    int CopyOptimalToLinearImage(VkCommandBuffer& commandBuffer,
                                 const VkVideoPictureResourceInfoKHR& srcPictureResource,
                                 const VulkanVideoFrameBuffer::PictureResourceInfo& srcPictureResourceInfo,
                                 const VkVideoPictureResourceInfoKHR& dstPictureResource,
                                 const VulkanVideoFrameBuffer::PictureResourceInfo& dstPictureResourceInfo,
                                 const VulkanVideoFrameBuffer::FrameSynchronizationInfo *pFrameSynchronizationInfo);

    int32_t GetCurrentFrameData(uint32_t slotId, NvVkDecodeFrameDataSlot& frameDataSlot)
    {
        if (slotId < m_decodeFramesData.size()) {
            frameDataSlot.commandBuffer   = m_decodeFramesData.GetCommandBuffer(slotId);
            frameDataSlot.slot = slotId;
            return static_cast<int32_t>(slotId);
        }
        return -1;
    }

	int32_t                     m_currentVideoQueueIndx;
    std::atomic<int32_t>        m_refCount;
    // The current decoder coded extent
    VkExtent2D                  m_codedExtent;
    // dimension of the output
    VkParserDetectedVideoFormat m_videoFormat;
    int32_t                     m_numDecodeImagesInFlight; // driven by how deep is the decoder queue
    int32_t                     m_numDecodeImagesToPreallocate; // -1 means pre-allocate all required images on setup

    VkVideoDecodeCapabilityFlagsKHR         m_capabilityFlags;
    VkSharedBaseObj<VulkanVideoSession>     m_videoSession;
    VkSharedBaseObj<VulkanVideoFrameBuffer> m_videoFrameBuffer;
    NvVkDecodeFrameData                     m_decodeFramesData;

    uint64_t                                         m_decodePicCount; // Also used for the HW load balancing timeline semaphore
    VkSharedBaseObj<VkParserVideoPictureParameters>  m_currentPictureParameters;
    VkSemaphore m_hwLoadBalancingTimelineSemaphore;
    uint32_t m_dpbAndOutputCoincide : 1;
    uint32_t m_videoMaintenance1FeaturesSupported : 1;
    uint32_t m_enableDecodeComputeFilter : 1;
    uint32_t m_enableGraphicsSampleFromDecodeOutput : 1;
    uint32_t m_useImageArray : 1;
    uint32_t m_useImageViewArray : 1;
    uint32_t m_useSeparateOutputImages : 1;
    uint32_t m_useLinearOutput : 1;
    uint32_t m_useSeparateLinearImages : 1;
    uint32_t m_useTransferOperation : 1;
    uint32_t m_resetDecoder : 1;
    uint32_t m_dumpDecodeData : 1;
    uint32_t m_numImageTypes;
    uint32_t m_numImageTypesEnabled;
    DecodeFrameBufferIf::ImageSpecsIndex m_imageSpecsIndex;
    int32_t  m_numBitstreamBuffersToPreallocate;
    VkDeviceSize   m_maxStreamBufferSize;
    VulkanFilterYuvCompute::FilterType m_filterType;
    VkSharedBaseObj<VulkanFilter> m_yuvFilter;
};
