/*
 * Copyright 2018-2022 NVIDIA Corporation.
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

#ifndef _NVVIDEOPROFILE_H_
#define _NVVIDEOPROFILE_H_

#include <assert.h>
#include <iostream>
#include "vk_video/vulkan_video_codecs_common.h"
#include "vk_video/vulkan_video_codec_h264std.h"
#include "vk_video/vulkan_video_codec_h265std.h"
#include "vk_video/vulkan_video_codec_av1std.h"

#include "vulkan/vulkan.h"
#include "nvidia_utils/vulkan/ycbcr_utils.h"

typedef enum StdChromaFormatIdc {
    chroma_format_idc_monochrome  = STD_VIDEO_H264_CHROMA_FORMAT_IDC_MONOCHROME,
    chroma_format_idc_420         = STD_VIDEO_H264_CHROMA_FORMAT_IDC_420,
    chroma_format_idc_422         = STD_VIDEO_H264_CHROMA_FORMAT_IDC_422,
    chroma_format_idc_444         = STD_VIDEO_H264_CHROMA_FORMAT_IDC_444,
} StdChromaFormatIdc;

#if defined(__linux) || defined(__linux__) || defined(linux)
static_assert((uint32_t)chroma_format_idc_monochrome == (uint32_t)STD_VIDEO_H265_CHROMA_FORMAT_IDC_MONOCHROME, "");
static_assert((uint32_t)chroma_format_idc_420        == (uint32_t)STD_VIDEO_H265_CHROMA_FORMAT_IDC_420, "");
static_assert((uint32_t)chroma_format_idc_422        == (uint32_t)STD_VIDEO_H265_CHROMA_FORMAT_IDC_422, "");
static_assert((uint32_t)chroma_format_idc_444        == (uint32_t)STD_VIDEO_H265_CHROMA_FORMAT_IDC_444, "");
#endif

class VkVideoCoreProfile
{

public:

    static bool isValidCodec(VkVideoCodecOperationFlagsKHR videoCodecOperations)
    {
        return  (videoCodecOperations & (VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR |
                                         VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR |
                                         VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR |
                                         VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR |
                                         VK_VIDEO_CODEC_OPERATION_ENCODE_H265_BIT_KHR));
    }

    bool PopulateProfileExt(VkBaseInStructure const * pVideoProfileExt)
    {

        if (m_profile.videoCodecOperation == VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR) {
            VkVideoDecodeH264ProfileInfoKHR const * pProfileExt = (VkVideoDecodeH264ProfileInfoKHR const *)pVideoProfileExt;
            if (pProfileExt && (pProfileExt->sType != VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_INFO_KHR)) {
                m_profile.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
                return false;
            }
            if (pProfileExt) {
                m_h264DecodeProfile = *pProfileExt;
            } else {
                //  Use default ext profile parameters
                m_h264DecodeProfile.sType         = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_INFO_KHR;
                m_h264DecodeProfile.stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_MAIN;
                m_h264DecodeProfile.pictureLayout = VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_INTERLACED_INTERLEAVED_LINES_BIT_KHR;
            }
            m_profile.pNext = &m_h264DecodeProfile;
            m_h264DecodeProfile.pNext = NULL;
        } else if (m_profile.videoCodecOperation == VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR) {
            VkVideoDecodeH265ProfileInfoKHR const * pProfileExt = (VkVideoDecodeH265ProfileInfoKHR const *)pVideoProfileExt;
            if (pProfileExt && (pProfileExt->sType != VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PROFILE_INFO_KHR)) {
                m_profile.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
                return false;
            }
            if (pProfileExt) {
                m_h265DecodeProfile = *pProfileExt;
            } else {
                //  Use default ext profile parameters
                m_h265DecodeProfile.sType         = VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PROFILE_INFO_KHR;
                m_h265DecodeProfile.stdProfileIdc = STD_VIDEO_H265_PROFILE_IDC_MAIN;
            }
            m_profile.pNext = &m_h265DecodeProfile;
            m_h265DecodeProfile.pNext = NULL;
        } else if (m_profile.videoCodecOperation == VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR) {
            VkVideoDecodeAV1ProfileInfoKHR const * pProfileExt = (VkVideoDecodeAV1ProfileInfoKHR const *)pVideoProfileExt;
            if (pProfileExt && (pProfileExt->sType != VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_PROFILE_INFO_KHR)) {
                m_profile.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
                return false;
            }
            if (pProfileExt) {
                m_av1DecodeProfile = *pProfileExt;
            } else {
                //  Use default ext profile parameters
                m_av1DecodeProfile.sType      = VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PROFILE_INFO_KHR;
                m_av1DecodeProfile.stdProfile = STD_VIDEO_AV1_PROFILE_MAIN;
            }
            m_profile.pNext = &m_av1DecodeProfile;
            m_av1DecodeProfile.pNext = NULL;

        } else if (m_profile.videoCodecOperation == VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR) {
            VkVideoEncodeH264ProfileInfoKHR const * pProfileExt = (VkVideoEncodeH264ProfileInfoKHR const *)pVideoProfileExt;
            if (pProfileExt && (pProfileExt->sType != VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PROFILE_INFO_KHR)) {
                m_profile.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
                return false;
            }
            if (pProfileExt) {
                m_h264EncodeProfile = *pProfileExt;
                VkVideoEncodeUsageInfoKHR const * pEncodeUsageInfo = (VkVideoEncodeUsageInfoKHR const *)pProfileExt->pNext;
                if (pEncodeUsageInfo && (pEncodeUsageInfo->sType == VK_STRUCTURE_TYPE_VIDEO_ENCODE_USAGE_INFO_KHR)) {
                    m_encodeUsageInfo = *pEncodeUsageInfo;
                    m_h264EncodeProfile.pNext = &m_encodeUsageInfo;
                }
            } else {
                //  Use default ext profile parameters
                m_h264DecodeProfile.sType         = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PROFILE_INFO_KHR;
                m_h264DecodeProfile.stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_MAIN;
                m_h264EncodeProfile.pNext         = nullptr;
            }
            m_profile.pNext = &m_h264EncodeProfile;
        } else if (m_profile.videoCodecOperation == VK_VIDEO_CODEC_OPERATION_ENCODE_H265_BIT_KHR) {
            VkVideoEncodeH265ProfileInfoKHR const * pProfileExt = (VkVideoEncodeH265ProfileInfoKHR const *)pVideoProfileExt;
            if (pProfileExt && (pProfileExt->sType != VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_PROFILE_INFO_KHR)) {
                m_profile.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
                return false;
            }
            if (pProfileExt) {
                m_h265EncodeProfile = *pProfileExt;
                VkVideoEncodeUsageInfoKHR const * pEncodeUsageInfo = (VkVideoEncodeUsageInfoKHR const *)pProfileExt->pNext;
                if (pEncodeUsageInfo && (pEncodeUsageInfo->sType == VK_STRUCTURE_TYPE_VIDEO_ENCODE_USAGE_INFO_KHR)) {
                    m_encodeUsageInfo = *pEncodeUsageInfo;
                    m_h265EncodeProfile.pNext = &m_encodeUsageInfo;
                }
            } else {
              //  Use default ext profile parameters
                m_h265EncodeProfile.sType         = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_PROFILE_INFO_KHR;
                m_h265EncodeProfile.stdProfileIdc = STD_VIDEO_H265_PROFILE_IDC_MAIN;
                m_h265EncodeProfile.pNext         = nullptr;
            }
            m_profile.pNext = &m_h265EncodeProfile;
        } else {
            assert(!"Unknown codec!");
            return false;
        }

        return true;
    }

    bool InitFromProfile(const VkVideoProfileInfoKHR* pVideoProfile)
    {
        m_profile = *pVideoProfile;
        m_profile.pNext = NULL;
        return PopulateProfileExt((VkBaseInStructure const *)pVideoProfile->pNext);
    }

    VkVideoCoreProfile(const VkVideoProfileInfoKHR* pVideoProfile)
        : m_profile(*pVideoProfile)
    {

        PopulateProfileExt((VkBaseInStructure const *)pVideoProfile->pNext);
    }

    VkVideoCoreProfile( VkVideoCodecOperationFlagBitsKHR videoCodecOperation = VK_VIDEO_CODEC_OPERATION_NONE_KHR,
                          VkVideoChromaSubsamplingFlagsKHR chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_INVALID_KHR,
                          VkVideoComponentBitDepthFlagsKHR lumaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_INVALID_KHR,
                          VkVideoComponentBitDepthFlagsKHR chromaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_INVALID_KHR,
                          uint32_t videoH26xProfileIdc = 0,
                          VkVideoEncodeTuningModeKHR tuningMode = VK_VIDEO_ENCODE_TUNING_MODE_DEFAULT_KHR)
        : m_profile({VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR, NULL,
                     videoCodecOperation, chromaSubsampling, lumaBitDepth, chromaBitDepth}),
          m_profileList({VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR, NULL, 1, &m_profile})
    {
        if (!isValidCodec(videoCodecOperation)) {
            return;
        }

        VkVideoDecodeH264ProfileInfoKHR decodeH264ProfilesRequest;
        VkVideoDecodeH265ProfileInfoKHR decodeH265ProfilesRequest;
        VkVideoDecodeAV1ProfileInfoKHR  decodeAV1ProfilesRequest;
        VkVideoEncodeH264ProfileInfoKHR encodeH264ProfilesRequest;
        VkVideoEncodeH265ProfileInfoKHR encodeH265ProfilesRequest;
        VkBaseInStructure* pVideoProfileExt = NULL;

        VkVideoEncodeUsageInfoKHR encodeUsageInfoRequest{VK_STRUCTURE_TYPE_VIDEO_ENCODE_USAGE_INFO_KHR,
                                                         NULL, 0, 0, tuningMode};
        VkBaseInStructure* pEncodeUsageInfo = (VkBaseInStructure*)&encodeUsageInfoRequest;

        if (videoCodecOperation == VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR) {
            decodeH264ProfilesRequest.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_INFO_KHR;
            decodeH264ProfilesRequest.pNext = NULL;
            decodeH264ProfilesRequest.stdProfileIdc = (videoH26xProfileIdc == 0) ?
                                                       STD_VIDEO_H264_PROFILE_IDC_INVALID :
                                                       (StdVideoH264ProfileIdc)videoH26xProfileIdc;
            decodeH264ProfilesRequest.pictureLayout = VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_INTERLACED_INTERLEAVED_LINES_BIT_KHR;
            pVideoProfileExt = (VkBaseInStructure*)&decodeH264ProfilesRequest;
        } else if (videoCodecOperation == VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR) {
            decodeAV1ProfilesRequest.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_PROFILE_INFO_KHR;
            decodeAV1ProfilesRequest.pNext = nullptr;
            // @fixme: The KHR spec doesn't follow the same pattern for profile naming, 0 is valid profile for example.
            switch (videoH26xProfileIdc) {
            case STD_VIDEO_AV1_PROFILE_MAIN:
            case STD_VIDEO_AV1_PROFILE_HIGH:
            case STD_VIDEO_AV1_PROFILE_PROFESSIONAL:
                break;
            default:
                assert(false && "Bad profile IDC");
            }
            decodeAV1ProfilesRequest.stdProfile = (StdVideoAV1Profile)videoH26xProfileIdc;
            pVideoProfileExt = (VkBaseInStructure*)&decodeAV1ProfilesRequest;
        } else if (videoCodecOperation == VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR) {
            decodeH265ProfilesRequest.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PROFILE_INFO_KHR;
            decodeH265ProfilesRequest.pNext = NULL;
            decodeH265ProfilesRequest.stdProfileIdc = (videoH26xProfileIdc == 0) ?
                                                       STD_VIDEO_H265_PROFILE_IDC_INVALID :
                                                       (StdVideoH265ProfileIdc)videoH26xProfileIdc;
            pVideoProfileExt = (VkBaseInStructure*)&decodeH265ProfilesRequest;
        } else if (videoCodecOperation == VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR) {
            encodeH264ProfilesRequest.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PROFILE_INFO_KHR;
            encodeH264ProfilesRequest.pNext = pEncodeUsageInfo;
            encodeH264ProfilesRequest.stdProfileIdc = (videoH26xProfileIdc == 0) ?
                                                       STD_VIDEO_H264_PROFILE_IDC_INVALID :
                                                       (StdVideoH264ProfileIdc)videoH26xProfileIdc;
            pVideoProfileExt = (VkBaseInStructure*)&encodeH264ProfilesRequest;
        } else if (videoCodecOperation == VK_VIDEO_CODEC_OPERATION_ENCODE_H265_BIT_KHR) {
            encodeH265ProfilesRequest.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_PROFILE_INFO_KHR;
            encodeH265ProfilesRequest.pNext = pEncodeUsageInfo;
            encodeH265ProfilesRequest.stdProfileIdc = (videoH26xProfileIdc == 0) ?
                                                       STD_VIDEO_H265_PROFILE_IDC_INVALID :
                                                       (StdVideoH265ProfileIdc)videoH26xProfileIdc;
            pVideoProfileExt = (VkBaseInStructure*)&encodeH265ProfilesRequest;
        } else {
            assert(!"Unknown codec!");
            return;
        }

        PopulateProfileExt(pVideoProfileExt);
    }

    VkVideoCodecOperationFlagBitsKHR GetCodecType() const
    {
        return m_profile.videoCodecOperation;
    }

    bool IsEncodeCodecType() const
    {
        return ((m_profile.videoCodecOperation == VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR) ||
                (m_profile.videoCodecOperation == VK_VIDEO_CODEC_OPERATION_ENCODE_H265_BIT_KHR));
    }

    bool IsDecodeCodecType() const
    {
        return ((m_profile.videoCodecOperation == VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR) ||
                (m_profile.videoCodecOperation == VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR));
    }

    operator bool() const
    {
        return (m_profile.sType == VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR);
    }

    const VkVideoProfileInfoKHR* GetProfile() const
    {
        if (m_profile.sType == VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR) {
            return &m_profile;
        } else {
            return NULL;
        }
    }

    const VkVideoProfileListInfoKHR* GetProfileListInfo() const
    {
        if (m_profileList.sType == VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR) {
            return &m_profileList;
        } else {
            return NULL;
        }
    }

    const VkVideoDecodeH264ProfileInfoKHR* GetDecodeH264Profile() const
    {
        if (m_h264DecodeProfile.sType == VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_INFO_KHR) {
            return &m_h264DecodeProfile;
        } else {
            return NULL;
        }
    }

    const VkVideoDecodeH265ProfileInfoKHR* GetDecodeH265Profile() const
    {
        if (m_h265DecodeProfile.sType == VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PROFILE_INFO_KHR) {
            return &m_h265DecodeProfile;
        } else {
            return NULL;
        }
    }

    const VkVideoDecodeAV1ProfileInfoKHR* GetDecodeAV1Profile() const
    {
        if (m_av1DecodeProfile.sType == VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_PROFILE_INFO_KHR) {
            return &m_av1DecodeProfile;
        } else {
            return NULL;
        }
    }

    const VkVideoEncodeH264ProfileInfoKHR* GetEncodeH264Profile() const
    {
        if (m_h264EncodeProfile.sType == VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PROFILE_INFO_KHR) {
            return &m_h264EncodeProfile;
        } else {
            return NULL;
        }
    }

    const VkVideoEncodeH265ProfileInfoKHR* GetEncodeH265Profile() const
    {
        if (m_h265EncodeProfile.sType == VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_PROFILE_INFO_KHR) {
            return &m_h265EncodeProfile;
        } else {
            return NULL;
        }
    }

    bool copyProfile(const VkVideoCoreProfile& src)
    {
        if (!src) {
            return false;
        }

        m_profile = src.m_profile;
        m_profile.pNext = nullptr;

        m_profileList = src.m_profileList;
        m_profileList.pNext = nullptr;

        m_profileList.pProfiles = &m_profile;

        PopulateProfileExt((VkBaseInStructure const *)src.m_profile.pNext);

        return true;
    }

    VkVideoCoreProfile(const VkVideoCoreProfile& other)
    {
        copyProfile(other);
    }

    VkVideoCoreProfile& operator= (const VkVideoCoreProfile& other)
    {
        copyProfile(other);
        return *this;
    }


    bool operator ==(const VkVideoCoreProfile &other) const
    {
        if (m_profile.videoCodecOperation != other.m_profile.videoCodecOperation) {
            return false;
        }

        if (m_profile.chromaSubsampling != other.m_profile.chromaSubsampling) {
            return false;
        }

        if (m_profile.lumaBitDepth != other.m_profile.lumaBitDepth) {
            return false;
        }

        if (m_profile.chromaBitDepth != other.m_profile.chromaBitDepth) {
            return false;
        }

        return true;
    }

    bool operator !=(const VkVideoCoreProfile &other) const
    {
        return !(*this == other);
    }

    VkVideoChromaSubsamplingFlagsKHR GetColorSubsampling() const
    {
        return m_profile.chromaSubsampling;
    }

    StdChromaFormatIdc GetColorSubsamplingGeneric() const
    {
        if (m_profile.chromaSubsampling & VK_VIDEO_CHROMA_SUBSAMPLING_MONOCHROME_BIT_KHR ) {
            return chroma_format_idc_monochrome;
        } else if (m_profile.chromaSubsampling & VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR ) {
            return chroma_format_idc_420;
        } else if (m_profile.chromaSubsampling & VK_VIDEO_CHROMA_SUBSAMPLING_422_BIT_KHR ) {
            return chroma_format_idc_422;
        } else if (m_profile.chromaSubsampling & VK_VIDEO_CHROMA_SUBSAMPLING_444_BIT_KHR ) {
            return chroma_format_idc_444;
        }

        return chroma_format_idc_monochrome;
    }

    uint32_t GetLumaBitDepthMinus8() const
    {
        if (m_profile.lumaBitDepth & VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR ) {
            return 8 - 8;
        } else if (m_profile.lumaBitDepth & VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR ) {
            return 10 - 8;
        } else if (m_profile.lumaBitDepth & VK_VIDEO_COMPONENT_BIT_DEPTH_12_BIT_KHR ) {
            return 12 - 8;
        }
        return 0;
    }

    uint32_t GetChromaBitDepthMinus8() const
    {
        if (m_profile.chromaBitDepth & VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR ) {
            return 8 - 8;
        } else if (m_profile.chromaBitDepth & VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR ) {
            return 10 - 8;
        } else if (m_profile.chromaBitDepth & VK_VIDEO_COMPONENT_BIT_DEPTH_12_BIT_KHR ) {
            return 12 - 8;
        }
        return 0;
    }

    bool Is16BitFormat() const
    {
        return !!GetLumaBitDepthMinus8() || !!GetChromaBitDepthMinus8();
    }

    static VkFormat CodecGetVkFormat(VkVideoChromaSubsamplingFlagBitsKHR chromaSubsampling,
                                     VkVideoComponentBitDepthFlagBitsKHR lumaBitDepth,
                                     bool isSemiPlanar)
    {
        VkFormat vkFormat = VK_FORMAT_UNDEFINED;
        switch (chromaSubsampling) {
        case VK_VIDEO_CHROMA_SUBSAMPLING_MONOCHROME_BIT_KHR:
            switch (lumaBitDepth) {
            case VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR:
                vkFormat = VK_FORMAT_R8_UNORM;
                break;
            case VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR:
                vkFormat = VK_FORMAT_R10X6_UNORM_PACK16;
                break;
            case VK_VIDEO_COMPONENT_BIT_DEPTH_12_BIT_KHR:
                vkFormat = VK_FORMAT_R12X4_UNORM_PACK16;
                break;
            default:
                assert(0);
            }
            break;
        case VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR:
            switch (lumaBitDepth) {
            case VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR:
                vkFormat = isSemiPlanar ? VK_FORMAT_G8_B8R8_2PLANE_420_UNORM : VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM;
                break;
            case VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR:
                vkFormat = isSemiPlanar ? VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16
                                        : VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16;
                break;
            case VK_VIDEO_COMPONENT_BIT_DEPTH_12_BIT_KHR:
                vkFormat = isSemiPlanar ? VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16
                                        : VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16;
                break;
            default:
                assert(0);
            }
            break;
        case VK_VIDEO_CHROMA_SUBSAMPLING_422_BIT_KHR:
            switch (lumaBitDepth) {
            case VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR:
                vkFormat = isSemiPlanar ? VK_FORMAT_G8_B8R8_2PLANE_422_UNORM : VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM;
                break;
            case VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR:
                vkFormat = isSemiPlanar ? VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16
                                        : VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16;
                break;
            case VK_VIDEO_COMPONENT_BIT_DEPTH_12_BIT_KHR:
                vkFormat = isSemiPlanar ? VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16
                                        : VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16;
                break;
            default:
                assert(0);
            }
            break;
        case VK_VIDEO_CHROMA_SUBSAMPLING_444_BIT_KHR:
            switch (lumaBitDepth) {
            case VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR:
                vkFormat = isSemiPlanar ? VK_FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT : VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM;
                break;
            case VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR:
                vkFormat = isSemiPlanar ? VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT
                                        : VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16;
                break;
            case VK_VIDEO_COMPONENT_BIT_DEPTH_12_BIT_KHR:
                vkFormat = isSemiPlanar ? VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT
                                        : VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16;
                break;
            default:
                assert(0);
            }
            break;
        default:
            assert(0);
        }

        return vkFormat;
    }

    static StdChromaFormatIdc GetVideoChromaFormatFromVkFormat(VkFormat format)
    {
        StdChromaFormatIdc videoChromaFormat = chroma_format_idc_420;
        switch ((uint32_t)format) {
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R10X6_UNORM_PACK16:
        case VK_FORMAT_R12X4_UNORM_PACK16:
            videoChromaFormat = chroma_format_idc_monochrome;
            break;

        case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
            videoChromaFormat = chroma_format_idc_420;
            break;

        case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
            videoChromaFormat = chroma_format_idc_422;
            break;

        case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
        case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT:
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT:
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT:
        case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM_EXT:
            videoChromaFormat = chroma_format_idc_444;
            break;
        default:
            assert(0);
        }

        return videoChromaFormat;
    }

    static const char* CodecToName(VkVideoCodecOperationFlagBitsKHR codec)
    {
        switch ((int32_t)codec) {
        case VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR:
            return "decode h.264";
        case VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR:
            return "decode h.265";
        case VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR:
            return "decode av1";
        case VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR:
            return "encode h.264";
        case VK_VIDEO_CODEC_OPERATION_ENCODE_H265_BIT_KHR:
            return "encode h.265";
        default:;
        }
        assert(!"Unknown codec");
        return "UNKNON";
    }

    static void DumpFormatProfiles(VkVideoProfileInfoKHR* pVideoProfile)
    {
        // formatProfile info based on supported chroma_format_idc
        if (pVideoProfile->chromaSubsampling & VK_VIDEO_CHROMA_SUBSAMPLING_MONOCHROME_BIT_KHR) {
            std::cout << "MONO, ";
        }
        if (pVideoProfile->chromaSubsampling & VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR) {
            std::cout << " 420, ";
        }
        if (pVideoProfile->chromaSubsampling & VK_VIDEO_CHROMA_SUBSAMPLING_422_BIT_KHR) {
            std::cout << " 422, ";
        }
        if (pVideoProfile->chromaSubsampling & VK_VIDEO_CHROMA_SUBSAMPLING_444_BIT_KHR) {
            std::cout << " 444, ";
        }

        // Profile info based on max bit_depth_luma_minus8
        if (pVideoProfile->lumaBitDepth & VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR) {
            std::cout << "LUMA:   8-bit, ";
        }
        if (pVideoProfile->lumaBitDepth & VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR) {
            std::cout << "LUMA:  10-bit, ";
        }
        if (pVideoProfile->lumaBitDepth & VK_VIDEO_COMPONENT_BIT_DEPTH_12_BIT_KHR) {
            std::cout << "LUMA:  12-bit, ";
        }

        // Profile info based on max bit_depth_chroma_minus8
        if (pVideoProfile->chromaBitDepth & VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR) {
            std::cout << "CHROMA: 8-bit, ";
        }
        if (pVideoProfile->chromaBitDepth & VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR) {
            std::cout << "CHROMA:10-bit, ";
        }
        if (pVideoProfile->chromaBitDepth & VK_VIDEO_COMPONENT_BIT_DEPTH_12_BIT_KHR) {
            std::cout << "CHROMA:12-bit,";
        }
    }

    static void DumpH264Profiles(VkVideoDecodeH264ProfileInfoKHR* pH264Profiles)
    {
        switch (pH264Profiles->stdProfileIdc) {
        case STD_VIDEO_H264_PROFILE_IDC_BASELINE:
            std::cout << "BASELINE, ";
            break;
        case STD_VIDEO_H264_PROFILE_IDC_MAIN:
            std::cout << "MAIN, ";
            break;
        case STD_VIDEO_H264_PROFILE_IDC_HIGH:
            std::cout << "HIGH, ";
            break;
        case STD_VIDEO_H264_PROFILE_IDC_HIGH_444_PREDICTIVE:
            std::cout << "HIGH_444_PREDICTIVE, ";
            break;
        default:
            std::cout << "UNKNOWN PROFILE, ";
            break;
        }
    }

    static void DumpH265Profiles(VkVideoDecodeH265ProfileInfoKHR* pH265Profiles)
    {
        switch (pH265Profiles->stdProfileIdc) {
        case STD_VIDEO_H265_PROFILE_IDC_MAIN:
            std::cout << "MAIN, ";
            break;
        case STD_VIDEO_H265_PROFILE_IDC_MAIN_10:
            std::cout << "MAIN_10, ";
            break;
        case STD_VIDEO_H265_PROFILE_IDC_MAIN_STILL_PICTURE:
            std::cout << "MAIN_STILL_PICTURE, ";
            break;
        case STD_VIDEO_H265_PROFILE_IDC_FORMAT_RANGE_EXTENSIONS:
            std::cout << "FORMAT_RANGE_EXTENSIONS, ";
            break;
        case STD_VIDEO_H265_PROFILE_IDC_SCC_EXTENSIONS:
            std::cout << "SCC_EXTENSIONS, ";
            break;
        default:
            std::cout << "UNKNOWN PROFILE, ";
            break;
        }
    }

    /*
    VkComponentMapping               components;
    VkChromaLocation                 xChromaOffset;
    VkChromaLocation                 yChromaOffset;
     */

    static VkSamplerYcbcrRange CodecFullRangeToYCbCrRange(bool video_full_range_flag) {

        if (video_full_range_flag) {
            return VK_SAMPLER_YCBCR_RANGE_ITU_FULL;
        }
        return VK_SAMPLER_YCBCR_RANGE_ITU_NARROW;
    }

    static VkSamplerYcbcrModelConversion CodecColorPrimariesToYCbCrModel(uint32_t colour_primaries)
    {
        switch (colour_primaries) {
        case 1: // Rec. ITU-R BT.709-6
            return VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709;
            break;
        case 5: // Rec. ITU-R BT.601-7 625, Rec. ITU-R BT.1700-0 625 PAL and 625 SECAM
        case 6: // Rec. ITU-R BT.601-7 525, Rec. ITU-R BT.1700-0 NTSC
            return VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601;
            break;
        case 9: // Rec. ITU-R BT.2020-2, Rec. ITU-R BT.2100-2
            return VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_2020;
            break;
        default:
            return VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_IDENTITY;
            break;
        }
        return VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_IDENTITY;
    }

    static YcbcrPrimariesConstants CodecGetMatrixCoefficients(uint32_t matrix_coefficients)
    {
        switch (matrix_coefficients) {
        case 1: // Rec. ITU-R BT.709-6
            return GetYcbcrPrimariesConstants(YcbcrBtStandardBt709);
            break;
        case 5: // Rec. ITU-R BT.601-7 625, Rec. ITU-R BT.1700-0 625 PAL and 625 SECAM
        case 6: // Rec. ITU-R BT.601-7 525, Rec. ITU-R BT.1700-0 NTSC
            return GetYcbcrPrimariesConstants(YcbcrBtStandardBt601Ebu);
            break;
        case 7: // Society of Motion Picture and Television Engineers 240M
            return GetYcbcrPrimariesConstants(YcbcrBtStandardBt601Smtpe);
            break;
        case 9: // Rec. ITU-R BT.2020-2, Rec. ITU-R BT.2100-2
            return GetYcbcrPrimariesConstants(YcbcrBtStandardBt2020);
            break;
        default:
            return YcbcrPrimariesConstants{1.0, 1.0};
            break;
        }
        return YcbcrPrimariesConstants{1.0, 1.0};
    }



private:
    VkVideoProfileInfoKHR     m_profile;
    VkVideoProfileListInfoKHR m_profileList;
    VkVideoEncodeUsageInfoKHR m_encodeUsageInfo;
    union
    {
        VkVideoDecodeH264ProfileInfoKHR m_h264DecodeProfile;
        VkVideoDecodeH265ProfileInfoKHR m_h265DecodeProfile;
        VkVideoDecodeAV1ProfileInfoKHR  m_av1DecodeProfile;
        VkVideoEncodeH264ProfileInfoKHR m_h264EncodeProfile;
        VkVideoEncodeH265ProfileInfoKHR m_h265EncodeProfile;
    };
};

#endif /* _NVVIDEOPROFILE_H_ */
