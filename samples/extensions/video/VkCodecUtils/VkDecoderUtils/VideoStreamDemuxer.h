/*
* Copyright 2023 NVIDIA Corporation.  All rights reserved.
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

#include <atomic>
#include <vulkan_interfaces.h>
#include "VkCodecUtils/VkVideoRefCountBase.h"

class VideoStreamDemuxer : public VkVideoRefCountBase {

    static VkSharedBaseObj<VideoStreamDemuxer>& invalidDemuxer;

public:

    static VkResult Create(const char *pFilePath,
                           VkVideoCodecOperationFlagBitsKHR codecType = VK_VIDEO_CODEC_OPERATION_NONE_KHR,
                           bool requiresStreamDemuxing = true,
                           int32_t defaultWidth = 1920,
                           int32_t defaultHeight = 1080,
                           int32_t defaultBitDepth = 12,
                           VkSharedBaseObj<VideoStreamDemuxer>& videoStreamDemuxer = invalidDemuxer);

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

    virtual VkVideoCodecOperationFlagBitsKHR GetVideoCodec() const = 0;
    virtual VkVideoComponentBitDepthFlagsKHR GetLumaBitDepth() const = 0;
    virtual VkVideoChromaSubsamplingFlagsKHR GetChromaSubsampling() const = 0;
    virtual VkVideoComponentBitDepthFlagsKHR GetChromaBitDepth() const = 0;
    virtual uint32_t GetProfileIdc() const = 0;

    virtual int32_t GetWidth()    const = 0;
    virtual int32_t GetHeight()   const = 0;
    virtual int32_t GetBitDepth() const = 0;

    virtual bool IsStreamDemuxerEnabled() const = 0;
    virtual bool HasFramePreparser() const = 0;
    virtual int64_t DemuxFrame(const uint8_t **ppVideo) = 0;
    virtual int64_t ReadBitstreamData(const uint8_t **ppVideo, int64_t offset) = 0;
    virtual void Rewind() = 0;

    virtual void DumpStreamParameters() const = 0;


protected:

    VideoStreamDemuxer()
        : m_refCount(0) { }

    virtual ~VideoStreamDemuxer() { }

    std::atomic<int32_t>    m_refCount;
};

VkResult ElementaryStreamCreate(const char *pFilePath,
                                VkVideoCodecOperationFlagBitsKHR codecType,
                                int32_t defaultWidth,
                                int32_t defaultHeight,
                                int32_t defaultBitDepth,
                                VkSharedBaseObj<VideoStreamDemuxer>& videoStreamDemuxer);
