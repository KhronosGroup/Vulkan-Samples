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

#include <string.h>
#include <fstream>
#include "mio/mio.hpp"
#include "VkDecoderUtils/VideoStreamDemuxer.h"

class ElementaryStream : public VideoStreamDemuxer {

public:
    ElementaryStream(const char *pFilePath,
                     VkVideoCodecOperationFlagBitsKHR forceParserType,
                     int32_t defaultWidth,
                     int32_t defaultHeight,
                     int32_t defaultBitDepth)
        : VideoStreamDemuxer(),
          m_width(defaultWidth)
        , m_height(defaultHeight)
        , m_bitDepth(defaultBitDepth)
        , m_videoCodecType(forceParserType)
        , m_inputVideoStreamMmap()
        , m_pBitstreamData(nullptr)
        , m_bitstreamDataSize(0)
        , m_bytesRead(0) {

        std::error_code error;
        m_inputVideoStreamMmap.map(pFilePath, 0, mio::map_entire_file, error);
        if (error) {
            assert(!"Can't map the input stream file!");
        }

        m_bitstreamDataSize = m_inputVideoStreamMmap.mapped_length();

        m_pBitstreamData = m_inputVideoStreamMmap.data();
    }

    ElementaryStream(const uint8_t *pInput, const size_t,
                     VkVideoCodecOperationFlagBitsKHR codecType)
        : m_width(176)
        , m_height(144)
        , m_bitDepth(8)
        , m_videoCodecType(codecType)
        , m_inputVideoStreamMmap()
        , m_pBitstreamData(pInput)
        , m_bitstreamDataSize(0)
        , m_bytesRead(0) {

    }

    int32_t Initialize() { return 0; }

    static VkResult Create(const char *pFilePath,
                           VkVideoCodecOperationFlagBitsKHR codecType,
                           int32_t defaultWidth,
                           int32_t defaultHeight,
                           int32_t defaultBitDepth,
                           VkSharedBaseObj<ElementaryStream>& elementaryStream)
    {
        VkSharedBaseObj<ElementaryStream> newElementaryStream(new ElementaryStream(pFilePath, codecType,
                                                                                   defaultWidth,
                                                                                   defaultHeight,
                                                                                   defaultBitDepth));

         if ((newElementaryStream) && (newElementaryStream->Initialize() >= 0)) {
             elementaryStream = newElementaryStream;
             return VK_SUCCESS;
         }
         return VK_ERROR_INITIALIZATION_FAILED;
    }

    virtual ~ElementaryStream() {
        m_inputVideoStreamMmap.unmap();
    }

    virtual bool IsStreamDemuxerEnabled() const { return false; }
    virtual bool HasFramePreparser() const { return false; }
    virtual void Rewind() { m_bytesRead = 0; }
    virtual VkVideoCodecOperationFlagBitsKHR GetVideoCodec() const { return m_videoCodecType; }

    virtual VkVideoComponentBitDepthFlagsKHR GetLumaBitDepth() const
    {
        switch (m_bitDepth) {
        case 8:
            return VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
            break;
        case 10:
            return VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR;
            break;
        case 12:
            return VK_VIDEO_COMPONENT_BIT_DEPTH_12_BIT_KHR;
            break;
        default:
            assert(!"Unknown Luma Bit Depth!");
        }
        assert(!"Unknown Luma Bit Depth!");
        return VK_VIDEO_COMPONENT_BIT_DEPTH_INVALID_KHR;
    }

    virtual VkVideoChromaSubsamplingFlagsKHR GetChromaSubsampling() const
    {
        return VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR;
    }

    virtual VkVideoComponentBitDepthFlagsKHR GetChromaBitDepth() const
    {
        switch (m_bitDepth) {
        case 8:
            return VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
            break;
        case 10:
            return VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR;
            break;
        case 12:
            return VK_VIDEO_COMPONENT_BIT_DEPTH_12_BIT_KHR;
            break;
        default:
            assert(!"Unknown Chroma Bit Depth!");
        }
        assert(!"Unknown Chroma Bit Depth!");
        return VK_VIDEO_COMPONENT_BIT_DEPTH_INVALID_KHR;
    }
    virtual uint32_t GetProfileIdc() const
    {
        return STD_VIDEO_H264_PROFILE_IDC_MAIN;
    }

    virtual int32_t GetWidth() const { return m_width; }
    virtual int32_t GetHeight() const { return m_height; }
    virtual int32_t GetBitDepth() const { return m_bitDepth; }
    virtual int64_t DemuxFrame(const uint8_t**) {
        return -1;
    }
    virtual int64_t ReadBitstreamData(const uint8_t **ppVideo, int64_t offset)
    {
        assert(m_bitstreamDataSize != 0);
        assert(m_pBitstreamData != nullptr);

        // Compute and return the pointer to data at new offset.
        *ppVideo = (m_pBitstreamData + offset);
        return m_bitstreamDataSize - offset;
    }

    virtual void DumpStreamParameters() const {
    }

private:
    int32_t    m_width, m_height, m_bitDepth;
    VkVideoCodecOperationFlagBitsKHR m_videoCodecType;
    mio::basic_mmap<mio::access_mode::read, uint8_t> m_inputVideoStreamMmap;
    const uint8_t* m_pBitstreamData;
    VkDeviceSize   m_bitstreamDataSize;
    VkDeviceSize   m_bytesRead;
};

VkResult ElementaryStreamCreate(const char *pFilePath,
                                VkVideoCodecOperationFlagBitsKHR codecType,
                                int32_t defaultWidth,
                                int32_t defaultHeight,
                                int32_t defaultBitDepth,
                                VkSharedBaseObj<VideoStreamDemuxer>& videoStreamDemuxer)
{
    VkSharedBaseObj<ElementaryStream> elementaryStream;
    VkResult result = ElementaryStream::Create(pFilePath,
                                               codecType,
                                               defaultWidth,
                                               defaultHeight,
                                               defaultBitDepth,
                                               elementaryStream);
    if (result == VK_SUCCESS) {
        videoStreamDemuxer = elementaryStream;
    }

    return result;
}
