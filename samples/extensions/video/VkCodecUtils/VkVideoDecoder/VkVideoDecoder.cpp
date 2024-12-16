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
#include <chrono>
#include <iostream>

#include "VkVideoCore/VulkanVideoCapabilities.h"
#include "VkVideoDecoder/VkVideoDecoder.h"
#include "nvidia_utils/vulkan/ycbcrvkinfo.h"

#undef max
#undef min

#define GPU_ALIGN(x) (((x) + 0xff) & ~0xff)

const uint64_t gFenceTimeout = 100 * 1000 * 1000 /* 100 mSec */;
const uint64_t gLongTimeout  = 1000 * 1000 * 1000 /* 1000 mSec */;

const char* VkVideoDecoder::GetVideoCodecString(VkVideoCodecOperationFlagBitsKHR codec)
{
    static struct {
        VkVideoCodecOperationFlagBitsKHR eCodec;
        const char* name;
    } aCodecName[] = {
        { VK_VIDEO_CODEC_OPERATION_NONE_KHR, "None" },
        { VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR, "AVC/H.264" },
        { VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR, "H.265/HEVC" },
#ifdef VK_EXT_video_decode_vp9
        { VK_VIDEO_CODEC_OPERATION_DECODE_VP9_BIT_KHR, "VP9" },
#endif // VK_EXT_video_decode_vp9
#ifdef vulkan_video_codec_av1std
        { VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR, "AV1" },
#endif // VK_EXT_video_decode_av1
    };

    for (unsigned i = 0; i < sizeof(aCodecName) / sizeof(aCodecName[0]); i++) {
        if (codec == aCodecName[i].eCodec) {
            return aCodecName[i].name;
        }
    }

    return "Unknown";
}

const char* VkVideoDecoder::GetVideoChromaFormatString(VkVideoChromaSubsamplingFlagBitsKHR chromaFormat)
{

    switch (chromaFormat) {
    case VK_VIDEO_CHROMA_SUBSAMPLING_MONOCHROME_BIT_KHR:
        return "YCbCr 400 (Monochrome)";
    case VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR:
        return "YCbCr 420";
    case VK_VIDEO_CHROMA_SUBSAMPLING_422_BIT_KHR:
        return "YCbCr 422";
    case VK_VIDEO_CHROMA_SUBSAMPLING_444_BIT_KHR:
        return "YCbCr 444";
    default:
        assert(!"Unknown Chroma sub-sampled format");
    };

    return "Unknown";
}

/* Callback function to be registered for getting a callback when decoding of
 * sequence starts. Return value from HandleVideoSequence() are interpreted as :
 *  0: fail, 1: suceeded, > 1: override dpb size of parser (set by
 * nvVideoParseParameters::ulMaxNumDecodeSurfaces while creating parser)
 */
int32_t VkVideoDecoder::StartVideoSequence(VkParserDetectedVideoFormat* pVideoFormat)
{
    const bool testUseLargestSurfaceExtent = false;
    // Assume 4k content for testing surfaces
    const uint32_t surfaceMinWidthExtent  = 4096;
    const uint32_t surfaceMinHeightExtent = 4096;

    m_codedExtent.width  = pVideoFormat->coded_width;
    m_codedExtent.height = pVideoFormat->coded_height;

    // Width and height of the image surface
    VkExtent3D imageExtent = VkExtent3D { std::max((uint32_t)(pVideoFormat->display_area.right  - pVideoFormat->display_area.left), pVideoFormat->coded_width),
                                          std::max((uint32_t)(pVideoFormat->display_area.bottom - pVideoFormat->display_area.top),  pVideoFormat->coded_height),
                                          1};

    // If we are testing content with different sizes against max sized surface vs. images dynamic resize
    // then set the imageExtent to the max surface size selected.
    if (testUseLargestSurfaceExtent) {
        imageExtent = { std::max(surfaceMinWidthExtent,  imageExtent.width),
                        std::max(surfaceMinHeightExtent, imageExtent.height),
                        1};
    }

    std::cout << "Video Input Information" << std::endl
              << "\tCodec        : " << GetVideoCodecString(pVideoFormat->codec) << std::endl
              << "\tFrame rate   : " << pVideoFormat->frame_rate.numerator << "/" << pVideoFormat->frame_rate.denominator << " = "
              << ((pVideoFormat->frame_rate.denominator != 0) ? (1.0 * pVideoFormat->frame_rate.numerator / pVideoFormat->frame_rate.denominator) : 0.0) << " fps" << std::endl
              << "\tSequence     : " << (pVideoFormat->progressive_sequence ? "Progressive" : "Interlaced") << std::endl
              << "\tCoded size   : [" << m_codedExtent.width << ", " << m_codedExtent.height << "]" << std::endl
              << "\tDisplay area : [" << pVideoFormat->display_area.left << ", " << pVideoFormat->display_area.top << ", "
              << pVideoFormat->display_area.right << ", " << pVideoFormat->display_area.bottom << "]" << std::endl
              << "\tChroma       : " << GetVideoChromaFormatString(pVideoFormat->chromaSubsampling) << std::endl
              << "\tBit depth    : " << pVideoFormat->bit_depth_luma_minus8 + 8 << std::endl;

    uint32_t numDecodeSurfaces = std::max(m_videoFrameBuffer->GetCurrentNumberQueueSlots(),
                                          (pVideoFormat->minNumDecodeSurfaces + m_numDecodeImagesInFlight));
    assert(numDecodeSurfaces <= VulkanVideoFrameBuffer::maxImages);

    int32_t videoQueueFamily = VulkanDeviceContext::GetThe()->GetVideoDecodeQueueFamilyIdx();
    VkVideoCodecOperationFlagsKHR videoCodecs = VulkanVideoCapabilities::GetSupportedCodecs(VulkanDeviceContext::GetThe(),
            VulkanDeviceContext::GetThe()->getPhysicalDevice(),
            &videoQueueFamily,
            VK_QUEUE_VIDEO_DECODE_BIT_KHR,
            VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR
            | VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR
            | VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR
    );
    assert(videoCodecs != VK_VIDEO_CODEC_OPERATION_NONE_KHR);

    if (m_dumpDecodeData) {
        std::cout << "\t" << std::hex << videoCodecs << " HW codec types are available: " << std::dec << std::endl;
    }

    VkVideoCodecOperationFlagBitsKHR videoCodec = pVideoFormat->codec;

    if (m_dumpDecodeData) {
        std::cout << "\tcodec " << VkVideoCoreProfile::CodecToName(videoCodec) << std::endl;
    }

    VkVideoCoreProfile videoProfile(videoCodec, pVideoFormat->chromaSubsampling, pVideoFormat->lumaBitDepth, pVideoFormat->chromaBitDepth,
                                    pVideoFormat->codecProfile);
    if (!VulkanVideoCapabilities::IsCodecTypeSupported(VulkanDeviceContext::GetThe(),
                                                       VulkanDeviceContext::GetThe()->GetVideoDecodeQueueFamilyIdx(),
                                                       videoCodec)) {
        std::cout << "*** The video codec " << VkVideoCoreProfile::CodecToName(videoCodec) << " is not supported! ***" << std::endl;
        assert(!"The video codec is not supported");
        return -1;
    }

    if (m_videoFormat.coded_width && m_videoFormat.coded_height) {
        // CreateDecoder() has been called before, and now there's possible config change
        VulkanDeviceContext::GetThe()->MultiThreadedQueueWaitIdle(VulkanDeviceContext::DECODE, m_currentVideoQueueIndx);

        if (*VulkanDeviceContext::GetThe()) {
            VulkanDeviceContext::GetThe()->DeviceWaitIdle();
        }
    }

    std::cout << "Video Decoding Params:" << std::endl
              << "\tNum Surfaces : " << numDecodeSurfaces << std::endl
              << "\tResize       : " << m_codedExtent.width << " x " << m_codedExtent.height << std::endl;

    uint32_t maxDpbSlotCount = pVideoFormat->maxNumDpbSlots;

    assert(VK_VIDEO_CHROMA_SUBSAMPLING_MONOCHROME_BIT_KHR == pVideoFormat->chromaSubsampling ||
           VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR == pVideoFormat->chromaSubsampling ||
           VK_VIDEO_CHROMA_SUBSAMPLING_422_BIT_KHR == pVideoFormat->chromaSubsampling ||
           VK_VIDEO_CHROMA_SUBSAMPLING_444_BIT_KHR == pVideoFormat->chromaSubsampling);


    VkVideoCapabilitiesKHR videoCapabilities;
    VkVideoDecodeCapabilitiesKHR videoDecodeCapabilities;
    VkResult result = VulkanVideoCapabilities::GetVideoDecodeCapabilities(VulkanDeviceContext::GetThe(), videoProfile,
                                                                          videoCapabilities,
                                                                          videoDecodeCapabilities);
    if (result != VK_SUCCESS) {
        std::cout << "*** Could not get Video Capabilities :" << result << " ***" << std::endl;
        assert(!"Could not get Video Capabilities!");
        return -1;
    }
    m_capabilityFlags = videoDecodeCapabilities.flags;
    m_dpbAndOutputCoincide = (m_capabilityFlags & VK_VIDEO_DECODE_CAPABILITY_DPB_AND_OUTPUT_COINCIDE_BIT_KHR);
    VkFormat dpbImageFormat = VK_FORMAT_UNDEFINED;
    VkFormat outImageFormat = VK_FORMAT_UNDEFINED;
    result = VulkanVideoCapabilities::GetSupportedVideoFormats(VulkanDeviceContext::GetThe(), videoProfile,
                                                               m_capabilityFlags,
                                                               outImageFormat,
                                                               dpbImageFormat);
    if (result != VK_SUCCESS) {
        std::cout << "*** Could not get supported video formats :" << result << " ***" << std::endl;
        assert(!"Could not get supported video formats!");
        return -1;
    }

    imageExtent.width  = std::max(imageExtent.width, videoCapabilities.minCodedExtent.width);
    imageExtent.height = std::max(imageExtent.height, videoCapabilities.minCodedExtent.height);

    uint32_t alignWidth = videoCapabilities.pictureAccessGranularity.width - 1;
    imageExtent.width = ((imageExtent.width + alignWidth) & ~alignWidth);
    uint32_t alignHeight = videoCapabilities.pictureAccessGranularity.height - 1;
    imageExtent.height = ((imageExtent.height + alignHeight) & ~alignHeight);

    VkVideoSessionCreateFlagsKHR sessionCreateFlags{};

#ifdef VK_KHR_video_maintenance1
    m_videoMaintenance1FeaturesSupported = VulkanVideoCapabilities::GetVideoMaintenance1FeatureSupported(VulkanDeviceContext::GetThe());
    if (m_videoMaintenance1FeaturesSupported) {
        sessionCreateFlags |= VK_VIDEO_SESSION_CREATE_INLINE_QUERIES_BIT_KHR;
    }
#endif // VK_KHR_video_maintenance1
    const VkExtent2D sessionMaxCodedExtent{imageExtent.width, imageExtent.height};
    if (!m_videoSession ||
            !m_videoSession->IsCompatible( VulkanDeviceContext::GetThe(),
                                           sessionCreateFlags,
                                           VulkanDeviceContext::GetThe()->GetVideoDecodeQueueFamilyIdx(),
                                           &videoProfile,
                                           outImageFormat,
                                           sessionMaxCodedExtent,
                                           dpbImageFormat,
                                           maxDpbSlotCount,
                                           std::max<uint32_t>(maxDpbSlotCount, VkParserPerFrameDecodeParameters::MAX_DPB_REF_SLOTS)) ) {

        result = VulkanVideoSession::Create( sessionCreateFlags,
                                             VulkanDeviceContext::GetThe()->GetVideoDecodeQueueFamilyIdx(),
                                             &videoProfile,
                                             outImageFormat,
                                             sessionMaxCodedExtent,
                                             dpbImageFormat,
                                             maxDpbSlotCount,
                                             std::min<uint32_t>(maxDpbSlotCount, VkParserPerFrameDecodeParameters::MAX_DPB_REF_SLOTS),
                                             m_videoSession);

        // after creating a new video session, we need codec reset.
        m_resetDecoder = VK_TRUE;
        assert(result == VK_SUCCESS);
    }

    uint8_t imageSpecsIndex = 0;
    m_imageSpecsIndex.decodeDpb = imageSpecsIndex++;
    std::array<VulkanVideoFrameBuffer::ImageSpec, DecodeFrameBufferIf::MAX_PER_FRAME_IMAGE_TYPES> imageSpecs;
    imageSpecs[m_imageSpecsIndex.decodeDpb].imageTypeIdx = m_imageSpecsIndex.decodeDpb;
    imageSpecs[m_imageSpecsIndex.decodeDpb].imageTypeMask |= DecodeFrameBufferIf::IMAGE_TYPE_MASK_DECODE_DPB;

    assert((videoCodec == VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR) || !pVideoFormat->filmGrainUsed);
    bool filmGrainEnabled = ((videoCodec == VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR) && pVideoFormat->filmGrainUsed);

    if ((m_dpbAndOutputCoincide == VK_FALSE) || filmGrainEnabled) {
        // The implementation does not support dpbAndOutputCoincide or can support filmGrain output
        m_useSeparateOutputImages = VK_TRUE;

        // At least two (2) image types : one for DPB and another for the output and/or AV1 FilmGrain
        m_imageSpecsIndex.decodeOut = imageSpecsIndex++;
        imageSpecs[m_imageSpecsIndex.decodeOut].imageTypeIdx = m_imageSpecsIndex.decodeOut;
        imageSpecs[m_imageSpecsIndex.decodeOut].imageTypeMask |= DecodeFrameBufferIf::IMAGE_TYPE_MASK_DECODE_OUT;

        if (filmGrainEnabled) {
            m_imageSpecsIndex.filmGrainOut  = m_imageSpecsIndex.decodeOut;
            imageSpecs[m_imageSpecsIndex.decodeOut].imageTypeMask |= DecodeFrameBufferIf::IMAGE_TYPE_MASK_FILM_GRAIN_OUT;
            m_numImageTypesEnabled |= DecodeFrameBufferIf::IMAGE_TYPE_MASK_FILM_GRAIN_OUT;
        }

    } else {

        // outImageSpecsIndex = 0; // decode_dpb == decode_out
        imageSpecs[m_imageSpecsIndex.decodeDpb].imageTypeMask |= DecodeFrameBufferIf::IMAGE_TYPE_MASK_DECODE_OUT;

    }
    m_numImageTypesEnabled |= DecodeFrameBufferIf::IMAGE_TYPE_MASK_DECODE_OUT;

    if(!(videoCapabilities.flags & VK_VIDEO_CAPABILITY_SEPARATE_REFERENCE_IMAGES_BIT_KHR)) {
        // The implementation does not support individual images for DPB and so must use arrays
        m_useImageArray = VK_TRUE;
        m_useImageViewArray = VK_FALSE;
    }

    if (m_enableDecodeComputeFilter) {

        const VkSamplerYcbcrRange ycbcrRange = VkVideoCoreProfile::CodecFullRangeToYCbCrRange(
                pVideoFormat->video_signal_description.video_full_range_flag);
        const VkSamplerYcbcrModelConversion ycbcrModelConversion = VkVideoCoreProfile::CodecColorPrimariesToYCbCrModel(
                pVideoFormat->video_signal_description.color_primaries);
        const YcbcrPrimariesConstants ycbcrPrimariesConstants = VkVideoCoreProfile::CodecGetMatrixCoefficients(
                pVideoFormat->video_signal_description.matrix_coefficients);

        const VkFormat inputFormat = dpbImageFormat;
        const VkFormat outputFormat = outImageFormat;

        const VkSamplerYcbcrConversionCreateInfo ycbcrConversionCreateInfo {
                   VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO,
                   nullptr,
                   inputFormat,
                   ycbcrModelConversion,
                   ycbcrRange,
                   { VK_COMPONENT_SWIZZLE_IDENTITY,
                     VK_COMPONENT_SWIZZLE_IDENTITY,
                     VK_COMPONENT_SWIZZLE_IDENTITY,
                     VK_COMPONENT_SWIZZLE_IDENTITY
                   },
                   VK_CHROMA_LOCATION_MIDPOINT,
                   VK_CHROMA_LOCATION_MIDPOINT,
                   VK_FILTER_LINEAR,
                   false
                   };

        static const VkSamplerCreateInfo samplerInfo = {
                   VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                   nullptr,
                   0,
                   VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST,
                   VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                   // mipLodBias  anisotropyEnable  maxAnisotropy  compareEnable      compareOp         minLod  maxLod          borderColor
                   // unnormalizedCoordinates
                   0.0, false, 0.00, false, VK_COMPARE_OP_NEVER, 0.0, 16.0, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, false
        };

        result = VulkanFilterYuvCompute::Create(VulkanDeviceContext::GetThe(),
                                                VulkanDeviceContext::GetThe()->GetComputeQueueFamilyIdx(),
                                                0,
                                                m_filterType,
                                                numDecodeSurfaces,
                                                inputFormat,
                                                outputFormat,
                                                &ycbcrConversionCreateInfo,
                                                &ycbcrPrimariesConstants,
                                                &samplerInfo,
                                                m_yuvFilter);
        if (result == VK_SUCCESS) {

            // We need extra image for the filter output - linear or optimal image
            m_imageSpecsIndex.filterOut = imageSpecsIndex++;
            imageSpecs[m_imageSpecsIndex.filterOut].imageTypeIdx = m_imageSpecsIndex.filterOut;
            imageSpecs[m_imageSpecsIndex.filterOut].imageTypeMask |= DecodeFrameBufferIf::IMAGE_TYPE_MASK_FILTER_OUT;
            m_numImageTypesEnabled |= DecodeFrameBufferIf::IMAGE_TYPE_MASK_FILTER_OUT;

            if (m_useLinearOutput == VK_TRUE) {
                // TODO: Check if the compute operation supports an output against linear images.
                // At this point the assumption is that if the compute filter is enabled
                // it also supports linear image output, which may be wrong.
                imageSpecs[m_imageSpecsIndex.filterOut].imageTypeMask |= DecodeFrameBufferIf::IMAGE_TYPE_MASK_LINEAR_OUT;
                m_numImageTypesEnabled |= DecodeFrameBufferIf::IMAGE_TYPE_MASK_LINEAR_OUT;

                // When we use the compute filter, the assumption is that it can output directly
                // to a linear layout. Set the linearOut to the same index of the filterOut.
                m_imageSpecsIndex.linearOut = m_imageSpecsIndex.filterOut;
            }

        } else {

            m_enableDecodeComputeFilter = VK_FALSE;
        }

    }

    if ((m_enableDecodeComputeFilter == VK_FALSE) && (m_useLinearOutput == VK_TRUE)) {

        // If the compute filter is not enabled and we need linear images

        if (m_dpbAndOutputCoincide == VK_TRUE) {
            // Use a transfer operation to copy the decoder's output to a linear image.
            m_useTransferOperation = VK_TRUE;

            // We need an extra image for the filter output for coincide - linear or optimal image
            m_imageSpecsIndex.linearOut = imageSpecsIndex++;
            imageSpecs[m_imageSpecsIndex.linearOut].imageTypeIdx = m_imageSpecsIndex.linearOut;
            imageSpecs[m_imageSpecsIndex.linearOut].imageTypeMask |= DecodeFrameBufferIf::IMAGE_TYPE_MASK_LINEAR_OUT;

        } else { // for distinct mode, we assume the output supports linear images

            // TODO: Check if the decoder's output supports linear images.
            // At this point the assumption is that if the decoder uses a separate
            // output, then it also supports linear output, which may be the wrong assumption.

            imageSpecs[m_imageSpecsIndex.decodeOut].imageTypeMask |= DecodeFrameBufferIf::IMAGE_TYPE_MASK_LINEAR_OUT;

        }

        m_numImageTypesEnabled |= DecodeFrameBufferIf::IMAGE_TYPE_MASK_LINEAR_OUT;
    }

    m_numImageTypes = imageSpecsIndex;

    VkImageUsageFlags outImageUsage = VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR;
    VkImageUsageFlags dpbImageUsage = VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR;

    VkImageUsageFlags extraImageUsage = 0;
    if (m_enableDecodeComputeFilter == VK_TRUE) {

        // If we need to read with a compute shader from the decoder's output
        extraImageUsage |= (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT);;

    } else if (m_useTransferOperation == VK_TRUE) {
        // If we need to transfer from the decoder's output
        extraImageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    if (m_enableGraphicsSampleFromDecodeOutput == VK_TRUE) {
        // If we need to read with a fragment shader from the decoder's output
        extraImageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    if (m_dpbAndOutputCoincide == VK_TRUE) {

        if (!filmGrainEnabled) {
            // AV1 filmGrain uses the output of the decoder, even when in coincide mode
            // Otherwise the output is the same as the setup DPB image
            outImageUsage &= ~VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR;
        }

        // The output in coincide mode is the same as the DPB setup image
        // I.e. the image is used for both, setup DPB and output
        dpbImageUsage |= VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR;

        // Apply the extra usage flags for the decoder's DPB
        dpbImageUsage |= extraImageUsage;

        if (filmGrainEnabled) {
            // For filmGrain, we do also need the decoder's output,
            // because we could be switching between the DPB and the output
            // for each frame.
            outImageUsage |= extraImageUsage;
        }

    } else { // Distinct mode

        // For distinct mode, usually there is no access allowed to the DPB images
        outImageUsage |= extraImageUsage;

    }

    // Image create info for the DPBs
    VulkanVideoFrameBuffer::ImageSpec& imageSpecDpb = imageSpecs[m_imageSpecsIndex.decodeDpb];
    imageSpecDpb.createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageSpecDpb.createInfo.pNext = nullptr; // the profile will get set in the FB
    imageSpecDpb.createInfo.format = dpbImageFormat;
    imageSpecDpb.createInfo.extent = imageExtent;
    imageSpecDpb.createInfo.arrayLayers = m_useImageArray ? numDecodeSurfaces : 1;
    imageSpecDpb.createInfo.imageType = VK_IMAGE_TYPE_2D;
    imageSpecDpb.createInfo.mipLevels = 1;
    imageSpecDpb.createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageSpecDpb.createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageSpecDpb.createInfo.usage = dpbImageUsage;
    imageSpecDpb.createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageSpecDpb.createInfo.queueFamilyIndexCount = 1;
    imageSpecDpb.createInfo.pQueueFamilyIndices = nullptr; // the profile will get set in the FB
    imageSpecDpb.createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageSpecDpb.createInfo.flags = 0;
    imageSpecDpb.usesImageArray = m_useImageArray;
    imageSpecDpb.usesImageViewArray = m_useImageViewArray;

    imageSpecDpb.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    assert(imageSpecDpb.imageTypeIdx == m_imageSpecsIndex.decodeDpb);

    if ((m_imageSpecsIndex.decodeOut != InvalidImageTypeIdx) && (m_imageSpecsIndex.decodeOut != m_imageSpecsIndex.decodeDpb)) {
        // Specify the separate from the DPB output image
        VulkanVideoFrameBuffer::ImageSpec& imageSpecOut = imageSpecs[m_imageSpecsIndex.decodeOut];
        imageSpecOut.createInfo = imageSpecDpb.createInfo;
        imageSpecOut.createInfo.format = outImageFormat;
        imageSpecOut.createInfo.arrayLayers = 1;
        if (filmGrainEnabled) {
            // FIXME: This may not be true. Some implementations may support linear output as filmGrain
            imageSpecOut.createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        } else {
            // FIXME: This may not be true. Some implementations may NOT support linear output
            imageSpecOut.createInfo.tiling = m_useLinearOutput ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
            imageSpecOut.memoryProperty    = m_useLinearOutput ? (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT    |
                                                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                                                    VK_MEMORY_PROPERTY_HOST_CACHED_BIT)  :
                                                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }

        imageSpecOut.createInfo.usage = outImageUsage;
        if (m_useSeparateOutputImages == VK_TRUE) {
            // Add one more image for the separate output image used for platforms
            // requiring a separate output image or the output needs to be linear

            // We will use discrete images for the output, for now.
            // TODO: AV1 needs an output array that matches the DPB array when film grain is enabled.
            // imageSpecs.usesImageArray = m_useImageArray;
            // imageSpecs.usesImageViewArray = m_useImageViewArray;

            if ((outImageUsage & VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR) == 0) {
                // A simple output image not directly used by the decoder
                imageSpecOut.createInfo.pNext = nullptr;
            }
        }
        assert(imageSpecOut.imageTypeIdx == m_imageSpecsIndex.decodeOut);
    }

    uint8_t filterOutImageSpecsIndex = (m_imageSpecsIndex.filterOut != InvalidImageTypeIdx) ?
                                        m_imageSpecsIndex.filterOut : m_imageSpecsIndex.linearOut;

    if (filterOutImageSpecsIndex != InvalidImageTypeIdx) {
        VulkanVideoFrameBuffer::ImageSpec& imageSpecFilter = imageSpecs[filterOutImageSpecsIndex];
        imageSpecFilter.createInfo = imageSpecDpb.createInfo;
        imageSpecFilter.createInfo.format = outImageFormat;
        imageSpecFilter.createInfo.arrayLayers = 1;

        if (m_enableDecodeComputeFilter == VK_TRUE) {

            // This is the image for the compute filter output: VK_IMAGE_USAGE_STORAGE_BIT
            imageSpecFilter.createInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        } else if (m_useTransferOperation == VK_TRUE) {

            // This is the image for the transfer output operation to linear : VK_IMAGE_USAGE_TRANSFER_DST_BIT
            imageSpecFilter.createInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        } else {
            assert(!"Invalid filter usage - you must use the compute or transfer filter");
        }

        if (m_enableGraphicsSampleFromDecodeOutput == VK_TRUE) {
            // This image can be also used as a sampled texture for the display presentation : VK_IMAGE_USAGE_SAMPLED_BIT
            imageSpecFilter.createInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }

        imageSpecFilter.createInfo.tiling = (m_useLinearOutput == VK_TRUE) ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
        imageSpecFilter.memoryProperty    = (m_useLinearOutput == VK_TRUE) ? ( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT    |
                                                                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                                                                   VK_MEMORY_PROPERTY_HOST_CACHED_BIT)  :
                                                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        assert(imageSpecFilter.imageTypeIdx == filterOutImageSpecsIndex);
    }

    assert(imageSpecsIndex < DecodeFrameBufferIf::MAX_PER_FRAME_IMAGE_TYPES);

    int32_t ret = m_videoFrameBuffer->InitImagePool(videoProfile.GetProfile(),
                                                    numDecodeSurfaces,
                                                    imageSpecsIndex,
                                                    imageSpecs,
                                                    VulkanDeviceContext::GetThe()->GetVideoDecodeQueueFamilyIdx(),
                                                    m_numDecodeImagesToPreallocate);

    assert((uint32_t)ret == numDecodeSurfaces);
    if ((uint32_t)ret != numDecodeSurfaces) {
        fprintf(stderr, "\nERROR: InitImagePool() ret(%d) != m_numDecodeSurfaces(%d)\n", ret, numDecodeSurfaces);
    }

    if (m_dumpDecodeData) {
        std::cout << "Allocating Video Device Memory" << std::endl
                  << "Allocating " << numDecodeSurfaces << " Num Decode Surfaces and "
                  << maxDpbSlotCount << " Video Device Memory Images for DPB " << std::endl
                  << imageExtent.width << " x " << imageExtent.height << std::endl;
    }

    const uint32_t maxDecodeFramesCount = std::max(numDecodeSurfaces, m_videoFrameBuffer->GetCurrentNumberQueueSlots());
    // There will be no more than VulkanVideoFrameBuffer::maxImages frames in the queue.
    m_decodeFramesData.resize(std::max<uint32_t>(maxDecodeFramesCount, VulkanVideoFrameBuffer::maxImages));

    int32_t availableBuffers = (int32_t)m_decodeFramesData.GetBitstreamBuffersQueue().
                                                      GetAvailableNodesNumber();
    if (availableBuffers < m_numBitstreamBuffersToPreallocate) {

        uint32_t allocateNumBuffers = std::min<uint32_t>(
                m_decodeFramesData.GetBitstreamBuffersQueue().GetMaxNodes(),
                (m_numBitstreamBuffersToPreallocate - availableBuffers));

        allocateNumBuffers = std::min<uint32_t>(allocateNumBuffers,
                m_decodeFramesData.GetBitstreamBuffersQueue().GetFreeNodesNumber());

        for (uint32_t i = 0; i < allocateNumBuffers; i++) {

            VkSharedBaseObj<VulkanBitstreamBufferImpl> bitstreamBuffer;
            VkDeviceSize allocSize = std::max<VkDeviceSize>(m_maxStreamBufferSize, 2 * 1024 * 1024);

            result = VulkanBitstreamBufferImpl::Create(
                    VulkanDeviceContext::GetThe()->GetVideoDecodeQueueFamilyIdx(),
                    VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR,
                    allocSize,
                    videoCapabilities.minBitstreamBufferOffsetAlignment,
                    videoCapabilities.minBitstreamBufferSizeAlignment,
                    nullptr, 0, bitstreamBuffer);
            assert(result == VK_SUCCESS);
            if (result != VK_SUCCESS) {
                fprintf(stderr, "\nERROR: VulkanBitstreamBufferImpl::Create() result: 0x%x\n", result);
                break;
            }

            int32_t nodeAddedWithIndex = m_decodeFramesData.GetBitstreamBuffersQueue().
                                                 AddNodeToPool(bitstreamBuffer, false);
            if (nodeAddedWithIndex < 0) {
                assert("Could not add the new node to the pool");
                break;
            }
        }
    }

    // Save the original config
    m_videoFormat = *pVideoFormat;
    return numDecodeSurfaces;
}

bool VkVideoDecoder::UpdatePictureParameters(VkSharedBaseObj<StdVideoPictureParametersSet>& pictureParametersObject,
                                             VkSharedBaseObj<VkVideoRefCountBase>& client)
{
    VkResult result = VkParserVideoPictureParameters::AddPictureParameters(VulkanDeviceContext::GetThe(),
                                                                           m_videoSession,
                                                                           pictureParametersObject,
                                                                           m_currentPictureParameters);

    client = m_currentPictureParameters;
    return (result == VK_SUCCESS);
}


int VkVideoDecoder::CopyOptimalToLinearImage(VkCommandBuffer& commandBuffer,
                                             const VkVideoPictureResourceInfoKHR& srcPictureResource,
                                             const VulkanVideoFrameBuffer::PictureResourceInfo& srcPictureResourceInfo,
                                             const VkVideoPictureResourceInfoKHR& dstPictureResource,
                                             const VulkanVideoFrameBuffer::PictureResourceInfo& dstPictureResourceInfo,
                                             const VulkanVideoFrameBuffer::FrameSynchronizationInfo* )

{
    // Bind memory for the image.
    const VkMpFormatInfo* mpInfo = YcbcrVkFormatInfo(srcPictureResourceInfo.imageFormat);

    // Currently formats that have more than 2 output planes are not supported. 444 formats have a shared CbCr planes in all current tests
    assert((mpInfo->vkPlaneFormat[2] == VK_FORMAT_UNDEFINED) && (mpInfo->vkPlaneFormat[3] == VK_FORMAT_UNDEFINED));

    // Copy src buffer to image.
    VkImageCopy copyRegion[3];
    memset(&copyRegion, 0, sizeof(copyRegion));
    copyRegion[0].extent.width = srcPictureResource.codedExtent.width;
    copyRegion[0].extent.height = srcPictureResource.codedExtent.height;
    copyRegion[0].extent.depth = 1;
    copyRegion[0].srcSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
    copyRegion[0].srcSubresource.mipLevel = 0;
    copyRegion[0].srcSubresource.baseArrayLayer = srcPictureResource.baseArrayLayer;
    copyRegion[0].srcSubresource.layerCount = 1;
    copyRegion[0].dstSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
    copyRegion[0].dstSubresource.mipLevel = 0;
    copyRegion[0].dstSubresource.baseArrayLayer = dstPictureResource.baseArrayLayer;
    copyRegion[0].dstSubresource.layerCount = 1;
    copyRegion[1].extent.width = copyRegion[0].extent.width;
    if (mpInfo->planesLayout.secondaryPlaneSubsampledX != 0) {
        copyRegion[1].extent.width /= 2;
    }

    copyRegion[1].extent.height = copyRegion[0].extent.height;
    if (mpInfo->planesLayout.secondaryPlaneSubsampledY != 0) {
        copyRegion[1].extent.height /= 2;
    }

    copyRegion[1].extent.depth = 1;
    copyRegion[1].srcSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
    copyRegion[1].srcSubresource.mipLevel = 0;
    copyRegion[1].srcSubresource.baseArrayLayer = srcPictureResource.baseArrayLayer;
    copyRegion[1].srcSubresource.layerCount = 1;
    copyRegion[1].dstSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
    copyRegion[1].dstSubresource.mipLevel = 0;
    copyRegion[1].dstSubresource.baseArrayLayer = dstPictureResource.baseArrayLayer;
    copyRegion[1].dstSubresource.layerCount = 1;

    VulkanDeviceContext::GetThe()->CmdCopyImage(commandBuffer, srcPictureResourceInfo.image, srcPictureResourceInfo.currentImageLayout,
                                    dstPictureResourceInfo.image, dstPictureResourceInfo.currentImageLayout,
                                    (uint32_t)2, copyRegion);

    {
        VkMemoryBarrier memoryBarrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        memoryBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        VulkanDeviceContext::GetThe()->CmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                               1, &memoryBarrier, 0,
                                0, 0, 0);
    }

    return 0;
}

/* Callback function to be registered for getting a callback when a decoded
 * frame is ready to be decoded. Return value from HandlePictureDecode() are
 * interpreted as: 0: fail, >=1: suceeded
 */
int VkVideoDecoder::DecodePictureWithParameters(VkParserPerFrameDecodeParameters* pCurrFrameDecParams,
                                                VkParserDecodePictureInfo* pDecodePictureInfo)
{
    if (!m_videoSession) {
        assert(!"Decoder not initialized!");
        return -1;
    }

    assert((m_videoFormat.codec == VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR) ||
           (pDecodePictureInfo->flags.applyFilmGrain == VK_FALSE));

    int32_t currPicIdx = pCurrFrameDecParams->currPicIdx;
    assert((uint32_t)currPicIdx < m_videoFrameBuffer->GetCurrentNumberQueueSlots());

    int32_t picNumInDecodeOrder = (int32_t)(uint32_t)m_decodePicCount;
    if (m_dumpDecodeData) {
        std::cout << "currPicIdx: " << currPicIdx << ", currentVideoQueueIndx: " << m_currentVideoQueueIndx << ", decodePicCount: " << m_decodePicCount << std::endl;
    }
    m_videoFrameBuffer->SetPicNumInDecodeOrder(currPicIdx, picNumInDecodeOrder);

    NvVkDecodeFrameDataSlot frameDataSlot;
    int32_t retPicIdx = GetCurrentFrameData((uint32_t)currPicIdx, frameDataSlot);
    assert(retPicIdx == currPicIdx);

    if (retPicIdx != currPicIdx) {
        fprintf(stderr, "\nERROR: DecodePictureWithParameters() retPicIdx(%d) != currPicIdx(%d)\n", retPicIdx, currPicIdx);
    }

    assert(pCurrFrameDecParams->bitstreamData->GetMaxSize() >= pCurrFrameDecParams->bitstreamDataLen);

    pCurrFrameDecParams->decodeFrameInfo.srcBuffer = pCurrFrameDecParams->bitstreamData->GetBuffer();
    assert(pCurrFrameDecParams->bitstreamDataOffset == 0);
    assert(pCurrFrameDecParams->firstSliceIndex == 0);
    // TODO: Assert if bitstreamDataOffset is aligned to VkVideoCapabilitiesKHR::minBitstreamBufferOffsetAlignment
    pCurrFrameDecParams->decodeFrameInfo.srcBufferOffset = pCurrFrameDecParams->bitstreamDataOffset;
    // TODO: Assert if bitstreamDataLen is aligned to VkVideoCapabilitiesKHR::minBitstreamBufferSizeAlignment
    pCurrFrameDecParams->decodeFrameInfo.srcBufferRange =  pCurrFrameDecParams->bitstreamDataLen;

    VkVideoBeginCodingInfoKHR decodeBeginInfo = { VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR };
    decodeBeginInfo.pNext = pCurrFrameDecParams->beginCodingInfoPictureParametersExt;

    decodeBeginInfo.videoSession = m_videoSession->GetVideoSession();

    assert(pCurrFrameDecParams->decodeFrameInfo.srcBuffer);
    const VkBufferMemoryBarrier2KHR bitstreamBufferMemoryBarrier = {
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR,
        nullptr,
        VK_PIPELINE_STAGE_2_NONE_KHR,
        VK_ACCESS_2_HOST_WRITE_BIT_KHR,
        VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR,
        VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR,
        VK_QUEUE_FAMILY_IGNORED,
        (uint32_t)VulkanDeviceContext::GetThe()->GetVideoDecodeQueueFamilyIdx(),
        pCurrFrameDecParams->decodeFrameInfo.srcBuffer,
        pCurrFrameDecParams->decodeFrameInfo.srcBufferOffset,
        pCurrFrameDecParams->decodeFrameInfo.srcBufferRange
    };

    uint32_t baseArrayLayer = (m_useImageArray || m_useImageViewArray) ? pCurrFrameDecParams->currPicIdx : 0;
    const VkImageMemoryBarrier2KHR dpbBarrierTemplates[1] = {
        { // VkImageMemoryBarrier

            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR, // VkStructureType sType
            nullptr, // const void*     pNext
            VK_PIPELINE_STAGE_2_NONE_KHR, // VkPipelineStageFlags2KHR srcStageMask
            0, // VkAccessFlags2KHR        srcAccessMask
            VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR, // VkPipelineStageFlags2KHR dstStageMask;
            VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR, // VkAccessFlags   dstAccessMask
            VK_IMAGE_LAYOUT_UNDEFINED, // VkImageLayout   oldLayout
            VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR, // VkImageLayout   newLayout
            VK_QUEUE_FAMILY_IGNORED, // uint32_t        srcQueueFamilyIndex
            (uint32_t)VulkanDeviceContext::GetThe()->GetVideoDecodeQueueFamilyIdx(), // uint32_t   dstQueueFamilyIndex
            VkImage(), // VkImage         image;
            {
                // VkImageSubresourceRange   subresourceRange
                VK_IMAGE_ASPECT_COLOR_BIT, // VkImageAspectFlags aspectMask
                0, // uint32_t           baseMipLevel
                1, // uint32_t           levelCount
                baseArrayLayer, // uint32_t           baseArrayLayer
                1, // uint32_t           layerCount;
            } },
    };

    uint32_t numDpbBarriers = 0;
    VkImageMemoryBarrier2KHR imageBarriers[VkParserPerFrameDecodeParameters::MAX_DPB_REF_AND_SETUP_SLOTS];

    VulkanVideoFrameBuffer::PictureResourceInfo dpbSetupPictureResourceInfo = VulkanVideoFrameBuffer::PictureResourceInfo();
    int resourceIndexDpb = m_videoFrameBuffer->GetCurrentImageResourceByIndex(
                                                          pCurrFrameDecParams->currPicIdx,
                                                          m_imageSpecsIndex.decodeDpb,
                                                          &pCurrFrameDecParams->dpbSetupPictureResource,
                                                          &dpbSetupPictureResourceInfo,
                                                          VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR);

    if (pCurrFrameDecParams->currPicIdx != resourceIndexDpb) {
        assert(!"GetImageResourcesByIndex has failed");
    }

    pCurrFrameDecParams->dpbSetupPictureResource.codedOffset = { 0, 0 }; // FIXME: This parameter must to be adjusted based on the interlaced mode.
    pCurrFrameDecParams->dpbSetupPictureResource.codedExtent = m_codedExtent;

    if (dpbSetupPictureResourceInfo.currentImageLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
        imageBarriers[numDpbBarriers] = dpbBarrierTemplates[0];
        imageBarriers[numDpbBarriers].oldLayout = dpbSetupPictureResourceInfo.currentImageLayout;
        imageBarriers[numDpbBarriers].newLayout = VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR;
        imageBarriers[numDpbBarriers].image = dpbSetupPictureResourceInfo.image;
        imageBarriers[numDpbBarriers].dstAccessMask = VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR;
        assert(imageBarriers[numDpbBarriers].image);
        numDpbBarriers++;
    }

    // Decoder's output picture resource, if enabled
    VkVideoPictureResourceInfoKHR* pOutputPictureResource = &pCurrFrameDecParams->decodeFrameInfo.dstPictureResource;
    // Decoder's output output picture resource info, if enabled
    VulkanVideoFrameBuffer::PictureResourceInfo currentOutputPictureResourceInfo = VulkanVideoFrameBuffer::PictureResourceInfo();
    const VulkanVideoFrameBuffer::PictureResourceInfo* pOutputPictureResourceInfo = &dpbSetupPictureResourceInfo;

    // If the implementation does not support dpb and output image coincide, use a separate image for the output.
    // Also, when FG is enabled and applied, the output is always used for the FG post-processed data.
    const bool useSeparateDecodeOutput = ((m_dpbAndOutputCoincide == VK_FALSE) ||
                                          (pDecodePictureInfo->flags.applyFilmGrain == VK_TRUE));

    if (useSeparateDecodeOutput) {

        assert(m_useSeparateOutputImages != VK_FALSE);

        int resourceIndexOut = m_videoFrameBuffer->GetCurrentImageResourceByIndex(
                                                              pCurrFrameDecParams->currPicIdx,
                                                              m_imageSpecsIndex.decodeOut,
                                                              pOutputPictureResource,
                                                              &currentOutputPictureResourceInfo,
                                                              VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR);

        // When the pOutputPictureResource is set, the pOutputPictureResourceInfo is also needed.
        pOutputPictureResourceInfo = &currentOutputPictureResourceInfo;

        if (pCurrFrameDecParams->currPicIdx != resourceIndexOut) {
            assert(!"GetImageResourcesByIndex has failed");
        }

        pOutputPictureResource->codedOffset = { 0, 0 }; // FIXME: This parameter must to be adjusted based on the interlaced mode.
        pOutputPictureResource->codedExtent = m_codedExtent;

        // For Output Distinct transition the image to DECODE_DST
        if (pOutputPictureResourceInfo->currentImageLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
            imageBarriers[numDpbBarriers] = dpbBarrierTemplates[0];
            imageBarriers[numDpbBarriers].oldLayout = pOutputPictureResourceInfo->currentImageLayout;
            imageBarriers[numDpbBarriers].newLayout = VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR;
            imageBarriers[numDpbBarriers].image = pOutputPictureResourceInfo->image;
            imageBarriers[numDpbBarriers].dstAccessMask = VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR;
            assert(imageBarriers[numDpbBarriers].image);
            numDpbBarriers++;
        }

    } else {

        // For the Output Coincide, the DPB and destination output resources are the same.
        pCurrFrameDecParams->decodeFrameInfo.dstPictureResource = pCurrFrameDecParams->dpbSetupPictureResource;

        // Also, when we are copying the output we need to know which layer is used for the current frame.
        // This is if a multi-layered image is used for the DPB and the output (since they coincide).
        pDecodePictureInfo->imageLayerIndex = pCurrFrameDecParams->dpbSetupPictureResource.baseArrayLayer;

    }

    if (m_dumpDecodeData) {
        std::cout << "currPicIdx: " << currPicIdx << ", OutInfo: " << pOutputPictureResource->codedExtent.width << " x "
                                                                   << pOutputPictureResource->codedExtent.height << " with layout "
                                                                   << ((pOutputPictureResourceInfo->currentImageLayout == VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR) ||
                                                                           (pOutputPictureResourceInfo->currentImageLayout == VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR) ? "OUT" : "INVALID")
                                                                   << std::endl;
    }

    VkVideoPictureResourceInfoKHR currentFilterOutPictureResource { VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR, nullptr};
    VulkanVideoFrameBuffer::PictureResourceInfo currentFilterOutPictureResourceInfo = VulkanVideoFrameBuffer::PictureResourceInfo();
    VkVideoPictureResourceInfoKHR* pFrameFilterOutResource = nullptr;
    VulkanVideoFrameBuffer::PictureResourceInfo* pFrameFilterOutResourceInfo = nullptr;
    if ((m_useTransferOperation == VK_TRUE) || (m_enableDecodeComputeFilter == VK_TRUE)) {
        pFrameFilterOutResource = &currentFilterOutPictureResource;
        pFrameFilterOutResourceInfo = &currentFilterOutPictureResourceInfo;
    }

    uint8_t filterOutImageSpecsIndex = (m_imageSpecsIndex.filterOut != InvalidImageTypeIdx) ?
                                            m_imageSpecsIndex.filterOut : m_imageSpecsIndex.linearOut ;

    if (filterOutImageSpecsIndex != InvalidImageTypeIdx) {

        // FIXME: VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR is incorrect layout for linear or filtered output
        int resourceIndexFilter = m_videoFrameBuffer->GetCurrentImageResourceByIndex(
                                                                 pCurrFrameDecParams->currPicIdx,
                                                                 filterOutImageSpecsIndex,
                                                                 pFrameFilterOutResource,
                                                                 pFrameFilterOutResourceInfo,
                                                                 (m_enableDecodeComputeFilter == VK_TRUE) ?
                                                                         VK_IMAGE_LAYOUT_GENERAL :
                                                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        if (pCurrFrameDecParams->currPicIdx != resourceIndexFilter) {
            assert(!"GetImageResourcesByIndex has failed");
        }
    }

    VulkanVideoFrameBuffer::PictureResourceInfo pictureResourcesInfo[VkParserPerFrameDecodeParameters::MAX_DPB_REF_AND_SETUP_SLOTS];
    memset(&pictureResourcesInfo[0], 0, sizeof(pictureResourcesInfo));
    const int8_t* pGopReferenceImagesIndexes = pCurrFrameDecParams->pGopReferenceImagesIndexes;
    if (pCurrFrameDecParams->numGopReferenceSlots != 0) {
        int dpbResourceIndex = m_videoFrameBuffer->GetImageResourcesByIndex(
                                                       pCurrFrameDecParams->numGopReferenceSlots,
                                                       pGopReferenceImagesIndexes,
                                                       m_imageSpecsIndex.decodeDpb,
                                                       pCurrFrameDecParams->pictureResources,
                                                       pictureResourcesInfo,
                                                       VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR);

        if (pCurrFrameDecParams->numGopReferenceSlots != dpbResourceIndex) {
            assert(!"GetImageResourcesByIndex has failed");
        }

        for (int32_t resId = 0; resId < pCurrFrameDecParams->numGopReferenceSlots; resId++) {
            // slotLayer requires NVIDIA specific extension VK_KHR_video_layers, not enabled, just yet.
            // pGopReferenceSlots[resId].slotLayerIndex = 0;
            // pictureResourcesInfo[resId].image can be a nullptr handle if the picture is not-existent.
            if (pictureResourcesInfo[resId].image && (pictureResourcesInfo[resId].currentImageLayout != VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR)) {
                imageBarriers[numDpbBarriers] = dpbBarrierTemplates[0];
                imageBarriers[numDpbBarriers].subresourceRange.baseArrayLayer = pCurrFrameDecParams->pictureResources[resId].baseArrayLayer;
                imageBarriers[numDpbBarriers].oldLayout = pictureResourcesInfo[resId].currentImageLayout;
                imageBarriers[numDpbBarriers].newLayout = VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR;
                imageBarriers[numDpbBarriers].image = pictureResourcesInfo[resId].image;
                assert(imageBarriers[numDpbBarriers].image);
                numDpbBarriers++;
            }

            if (pictureResourcesInfo[resId].image != VK_NULL_HANDLE) {

                // FIXME: m_codedExtent should have already be populated in in the
                // picture resource above from the FB.
                pCurrFrameDecParams->pictureResources[resId].codedExtent = m_codedExtent;
                // FIXME: This parameter must to be adjusted based on the interlaced mode.
                pCurrFrameDecParams->pictureResources[resId].codedOffset = { 0, 0 };
            }

            if (m_dumpDecodeData) {
                std::cout << "\tdpb: " << (int)pCurrFrameDecParams->pGopReferenceImagesIndexes[resId]
                                       << ", DpbInfo: " << pOutputPictureResource->codedExtent.width << " x "
                                       << pOutputPictureResource->codedExtent.height << " with layout "
                                       << ((pictureResourcesInfo[resId].currentImageLayout == VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR) ? "DPB" : "INVALID")
                                       << std::endl;
            }
        }
    }

    decodeBeginInfo.referenceSlotCount = pCurrFrameDecParams->decodeFrameInfo.referenceSlotCount;
    decodeBeginInfo.pReferenceSlots = pCurrFrameDecParams->decodeFrameInfo.pReferenceSlots;

    m_imageSpecsIndex.displayOut = ((m_dpbAndOutputCoincide == VK_TRUE) &&
                                    !(pDecodePictureInfo->flags.applyFilmGrain == VK_TRUE)) ?
                                       m_imageSpecsIndex.decodeDpb :
                                       m_imageSpecsIndex.decodeOut;

    if (m_enableDecodeComputeFilter == VK_TRUE) {
        // If we are using the filter, then display the result after the filter
        m_imageSpecsIndex.filterIn = m_imageSpecsIndex.displayOut;
        m_imageSpecsIndex.displayOut = m_imageSpecsIndex.filterOut;
    }


    if (pDecodePictureInfo->flags.unpairedField) {
        // assert(pFrameSyncinfo->frameCompleteSemaphore == VkSemaphore());
        pDecodePictureInfo->flags.syncFirstReady = true;
    }
    // FIXME: the below sequence for interlaced synchronization.
    pDecodePictureInfo->flags.syncToFirstField = false;

    VulkanVideoFrameBuffer::FrameSynchronizationInfo frameSynchronizationInfo = VulkanVideoFrameBuffer::FrameSynchronizationInfo();
    frameSynchronizationInfo.hasFrameCompleteSignalFence = true;
    frameSynchronizationInfo.hasFrameCompleteSignalSemaphore = true;
    frameSynchronizationInfo.syncOnFrameCompleteFence = true;
    frameSynchronizationInfo.syncOnFrameConsumerDoneFence = true;
    frameSynchronizationInfo.imageSpecsIndex = m_imageSpecsIndex;

    VkSharedBaseObj<VkVideoRefCountBase> currentVkPictureParameters;
    if (m_videoFormat.codec == VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR) { // AV1

        bool valid = pCurrFrameDecParams->pStdSps->GetClientObject(currentVkPictureParameters);
        assert(valid);
        VkParserVideoPictureParameters* pOwnerPictureParameters =
            VkParserVideoPictureParameters::VideoPictureParametersFromBase(currentVkPictureParameters);

        assert(pOwnerPictureParameters);
        assert(pOwnerPictureParameters->GetId() <= m_currentPictureParameters->GetId());
        int32_t ret = pOwnerPictureParameters->FlushPictureParametersQueue(m_videoSession);
        assert(ret >= 0);
        if (!(ret >= 0)) {
            return -1;
        }

        decodeBeginInfo.videoSessionParameters = *pOwnerPictureParameters;

    } else if (pCurrFrameDecParams->useInlinedPictureParameters == false) {
        // out of band parameters
        bool valid = pCurrFrameDecParams->pStdPps->GetClientObject(currentVkPictureParameters);
        assert(currentVkPictureParameters && valid);
        if (!(currentVkPictureParameters && valid)) {
            return -1;
        }
        VkParserVideoPictureParameters* pOwnerPictureParameters =
                VkParserVideoPictureParameters::VideoPictureParametersFromBase(currentVkPictureParameters);
        assert(pOwnerPictureParameters);
        assert(pOwnerPictureParameters->GetId() <= m_currentPictureParameters->GetId());
        int32_t ret = pOwnerPictureParameters->FlushPictureParametersQueue(m_videoSession);
        assert(ret >= 0);
        if (!(ret >= 0)) {
            return -1;
        }
        bool isSps = false;
        int32_t spsId = pCurrFrameDecParams->pStdPps->GetSpsId(isSps);
        assert(!isSps);
        assert(spsId >= 0);
        assert(pOwnerPictureParameters->HasSpsId(spsId));
        bool isPps = false;
        int32_t ppsId =  pCurrFrameDecParams->pStdPps->GetPpsId(isPps);
        assert(isPps);
        assert(ppsId >= 0);
        assert(pOwnerPictureParameters->HasPpsId(ppsId));

        decodeBeginInfo.videoSessionParameters = *pOwnerPictureParameters;

        if (m_dumpDecodeData) {
            std::cout << "Using object " << decodeBeginInfo.videoSessionParameters <<
                 " with ID: (" << pOwnerPictureParameters->GetId() << ")" <<
                 " for SPS: " <<  spsId << ", PPS: " << ppsId << std::endl;
        }
    } else {
        decodeBeginInfo.videoSessionParameters = VK_NULL_HANDLE;
    }

    VulkanVideoFrameBuffer::ReferencedObjectsInfo referencedObjectsInfo(pCurrFrameDecParams->bitstreamData,
                                                                        pCurrFrameDecParams->pStdPps,
                                                                        pCurrFrameDecParams->pStdSps,
                                                                        pCurrFrameDecParams->pStdVps);
    int32_t retVal = m_videoFrameBuffer->QueuePictureForDecode(currPicIdx, pDecodePictureInfo,
                                                               &referencedObjectsInfo,
                                                               &frameSynchronizationInfo);
    if (currPicIdx != retVal) {
        assert(!"QueuePictureForDecode has failed");
    }

    assert(VK_NOT_READY == VulkanDeviceContext::GetThe()->GetFenceStatus(*VulkanDeviceContext::GetThe(), frameSynchronizationInfo.frameCompleteFence));

    VkFence frameCompleteFence = frameSynchronizationInfo.frameCompleteFence;
    VkSemaphore frameCompleteSemaphore = frameSynchronizationInfo.frameCompleteSemaphore;
    VkSemaphore frameConsumerDoneSemaphore = frameSynchronizationInfo.frameConsumerDoneSemaphore;
    // By default, the frameCompleteSemaphore is the videoDecodeCompleteSemaphore.
    // If the video frame filter is enabled, since it is executed after the decoder's queue,
    // the filter will provide its own semaphore for the video decoder to signal, instead.
    // Then the frameCompleteSemaphore will be signaled by the filter of its completion.
    VkFence videoDecodeCompleteFence = frameCompleteFence;
    VkSemaphore videoDecodeCompleteSemaphore = frameCompleteSemaphore;


    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    VulkanDeviceContext::GetThe()->BeginCommandBuffer(frameDataSlot.commandBuffer, &beginInfo);

    if (frameSynchronizationInfo.queryPool) {
        VulkanDeviceContext::GetThe()->CmdResetQueryPool(frameDataSlot.commandBuffer, frameSynchronizationInfo.queryPool,
                                      frameSynchronizationInfo.startQueryId, frameSynchronizationInfo.numQueries);
    }

    VulkanDeviceContext::GetThe()->CmdBeginVideoCodingKHR(frameDataSlot.commandBuffer, &decodeBeginInfo);

    if (m_resetDecoder != false) {
        VkVideoCodingControlInfoKHR codingControlInfo = { VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR,
                                                          nullptr,
                                                          VK_VIDEO_CODING_CONTROL_RESET_BIT_KHR };

        // Video spec requires mandatory codec reset before the first frame.
        VulkanDeviceContext::GetThe()->CmdControlVideoCodingKHR(frameDataSlot.commandBuffer, &codingControlInfo);
        // Done with the reset
        m_resetDecoder = false;
    }

    const VkDependencyInfoKHR dependencyInfo = {
        VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
        nullptr,
        VK_DEPENDENCY_BY_REGION_BIT,
        0,
        nullptr,
        1,
        &bitstreamBufferMemoryBarrier,
        numDpbBarriers,
        imageBarriers,
    };
    VulkanDeviceContext::GetThe()->CmdPipelineBarrier2KHR(frameDataSlot.commandBuffer, &dependencyInfo);

#ifdef VK_KHR_video_maintenance1
    VkVideoInlineQueryInfoKHR inlineQueryInfo { VK_STRUCTURE_TYPE_VIDEO_INLINE_QUERY_INFO_KHR,
                                                nullptr,
                                                frameSynchronizationInfo.queryPool,
                                                frameSynchronizationInfo.startQueryId,
                                                frameSynchronizationInfo.numQueries };
#endif // VK_KHR_video_maintenance1

    if (frameSynchronizationInfo.queryPool != VK_NULL_HANDLE) {

#ifdef VK_KHR_video_maintenance1
        if (m_videoMaintenance1FeaturesSupported == 1) {
            inlineQueryInfo.pNext = pCurrFrameDecParams->decodeFrameInfo.pNext;
            pCurrFrameDecParams->decodeFrameInfo.pNext = &inlineQueryInfo;
        } else
#endif // VK_KHR_video_maintenance1
        {
            VulkanDeviceContext::GetThe()->CmdBeginQuery(frameDataSlot.commandBuffer, frameSynchronizationInfo.queryPool,
                                      frameSynchronizationInfo.startQueryId, VkQueryControlFlags());
        }
    }

    VulkanDeviceContext::GetThe()->CmdDecodeVideoKHR(frameDataSlot.commandBuffer, &pCurrFrameDecParams->decodeFrameInfo);

    if ((frameSynchronizationInfo.queryPool != VK_NULL_HANDLE) && (m_videoMaintenance1FeaturesSupported == 0)) {
        VulkanDeviceContext::GetThe()->CmdEndQuery(frameDataSlot.commandBuffer, frameSynchronizationInfo.queryPool,
                                frameSynchronizationInfo.startQueryId);
    }

    VkVideoEndCodingInfoKHR decodeEndInfo = { VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR };
    VulkanDeviceContext::GetThe()->CmdEndVideoCodingKHR(frameDataSlot.commandBuffer, &decodeEndInfo);

    if (m_useTransferOperation == VK_TRUE) {

        assert((pOutputPictureResource != nullptr) && (pOutputPictureResourceInfo != nullptr));

        CopyOptimalToLinearImage(frameDataSlot.commandBuffer,
                                 *pOutputPictureResource,
                                 *pOutputPictureResourceInfo,
                                 *pFrameFilterOutResource,
                                 *pFrameFilterOutResourceInfo,
                                 &frameSynchronizationInfo);
    }

    VulkanDeviceContext::GetThe()->EndCommandBuffer(frameDataSlot.commandBuffer);

    if (m_enableDecodeComputeFilter) {

        // frameCompleteSemaphore is the semaphore that the filter is going to signal on completion when enabled.
        // The videoDecodeCompleteSemaphore semaphore will be signaled by the decoder and then used by the filter to wait on.
        videoDecodeCompleteFence     = m_yuvFilter->GetFilterSignalFence(currPicIdx);
        videoDecodeCompleteSemaphore = m_yuvFilter->GetFilterWaitSemaphore(currPicIdx);
    }

    const uint32_t waitSemaphoreMaxCount = 3;
    VkSemaphore waitSemaphores[waitSemaphoreMaxCount] = { VK_NULL_HANDLE };

    const uint32_t signalSemaphoreMaxCount = 3;
    VkSemaphore signalSemaphores[signalSemaphoreMaxCount] = { VK_NULL_HANDLE };

    uint32_t waitSemaphoreCount = 0;
    if (frameConsumerDoneSemaphore != VK_NULL_HANDLE) {
        waitSemaphores[waitSemaphoreCount] = frameConsumerDoneSemaphore;
        waitSemaphoreCount++;
    }

    uint32_t signalSemaphoreCount = 0;
    if (videoDecodeCompleteSemaphore != VK_NULL_HANDLE) {
        signalSemaphores[signalSemaphoreCount] = videoDecodeCompleteSemaphore;
        signalSemaphoreCount++;
    }

    uint64_t waitTlSemaphoresValues[waitSemaphoreMaxCount] = { 0 /* ignored for binary semaphores */ };
    uint64_t signalTlSemaphoresValues[signalSemaphoreMaxCount] = { 0 /* ignored for binary semaphores */ };
    VkTimelineSemaphoreSubmitInfo timelineSemaphoreInfos = {};
    if (m_hwLoadBalancingTimelineSemaphore != VK_NULL_HANDLE) {

        if (m_dumpDecodeData) {
            uint64_t  currSemValue = 0;
            VkResult semResult = VulkanDeviceContext::GetThe()->GetSemaphoreCounterValue(*VulkanDeviceContext::GetThe(), m_hwLoadBalancingTimelineSemaphore, &currSemValue);
            std::cout << "\t TL semaphore value: " << currSemValue << ", status: " << semResult << std::endl;
        }

        waitSemaphores[waitSemaphoreCount] = m_hwLoadBalancingTimelineSemaphore;
        waitTlSemaphoresValues[waitSemaphoreCount] = m_decodePicCount - 1; // wait for the previous value to be signaled
        waitSemaphoreCount++;

        signalSemaphores[signalSemaphoreCount] = m_hwLoadBalancingTimelineSemaphore;
        signalTlSemaphoresValues[signalSemaphoreCount] = m_decodePicCount; // signal the current m_decodePicCount value
        signalSemaphoreCount++;

        timelineSemaphoreInfos.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        timelineSemaphoreInfos.pNext = NULL;
        assert(waitSemaphoreCount < waitSemaphoreMaxCount);
        timelineSemaphoreInfos.waitSemaphoreValueCount = waitSemaphoreCount;
        timelineSemaphoreInfos.pWaitSemaphoreValues = waitTlSemaphoresValues;
        assert(signalSemaphoreCount < signalSemaphoreMaxCount);
        timelineSemaphoreInfos.signalSemaphoreValueCount = signalSemaphoreCount;
        timelineSemaphoreInfos.pSignalSemaphoreValues = signalTlSemaphoresValues;
        if (m_dumpDecodeData) {
            std::cout << "\t Wait for: " << (waitSemaphoreCount ? waitTlSemaphoresValues[waitSemaphoreCount - 1] : 0) <<
                             ", signal at " << signalTlSemaphoresValues[signalSemaphoreCount - 1] << std::endl;
        }
    }

    assert(waitSemaphoreCount <= waitSemaphoreMaxCount);
    assert(signalSemaphoreCount <= signalSemaphoreMaxCount);

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr };
    const VkPipelineStageFlags videoDecodeSubmitWaitStages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    submitInfo.pNext = (m_hwLoadBalancingTimelineSemaphore != VK_NULL_HANDLE) ? &timelineSemaphoreInfos : nullptr;
    submitInfo.waitSemaphoreCount = waitSemaphoreCount;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = &videoDecodeSubmitWaitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frameDataSlot.commandBuffer;
    submitInfo.signalSemaphoreCount = signalSemaphoreCount;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (m_dumpDecodeData) {
        if (m_hwLoadBalancingTimelineSemaphore != VK_NULL_HANDLE) {
            std::cout << "\t\t waitSemaphoreValueCount: " << timelineSemaphoreInfos.waitSemaphoreValueCount << std::endl;
            std::cout << "\t pWaitSemaphoreValues: " << timelineSemaphoreInfos.pWaitSemaphoreValues[0] << ", " <<
                                                timelineSemaphoreInfos.pWaitSemaphoreValues[1] << ", " <<
                                                timelineSemaphoreInfos.pWaitSemaphoreValues[2] << std::endl;
            std::cout << "\t\t signalSemaphoreValueCount: " << timelineSemaphoreInfos.signalSemaphoreValueCount << std::endl;
            std::cout << "\t pSignalSemaphoreValues: " << timelineSemaphoreInfos.pSignalSemaphoreValues[0] << ", " <<
                                                timelineSemaphoreInfos.pSignalSemaphoreValues[1] << ", " <<
                                                timelineSemaphoreInfos.pSignalSemaphoreValues[2] << std::endl;
        }

        std::cout << "\t waitSemaphoreCount: " << submitInfo.waitSemaphoreCount << std::endl;
        std::cout << "\t\t pWaitSemaphores: " << submitInfo.pWaitSemaphores[0] << ", " <<
                                                 submitInfo.pWaitSemaphores[1] << ", " <<
                                                 submitInfo.pWaitSemaphores[2] << std::endl;
        std::cout << "\t signalSemaphoreCount: " << submitInfo.signalSemaphoreCount << std::endl;
        std::cout << "\t\t pSignalSemaphores: " << submitInfo.pSignalSemaphores[0] << ", " <<
                                             submitInfo.pSignalSemaphores[1] << ", " <<
                                             submitInfo.pSignalSemaphores[2] << std::endl << std::endl;
    }

    assert(VK_NOT_READY == VulkanDeviceContext::GetThe()->GetFenceStatus(*VulkanDeviceContext::GetThe(), videoDecodeCompleteFence));
    VkResult result = VulkanDeviceContext::GetThe()->MultiThreadedQueueSubmit(VulkanDeviceContext::DECODE, m_currentVideoQueueIndx,
                                                           1, &submitInfo, videoDecodeCompleteFence);
    assert(result == VK_SUCCESS);
    if (result != VK_SUCCESS) {
        return -1;
    }

    if (m_dumpDecodeData) {
        std::cout << "\t +++++++++++++++++++++++++++< " << currPicIdx << " >++++++++++++++++++++++++++++++" << std::endl;
        std::cout << "\t => Decode Submitted for CurrPicIdx: " << currPicIdx << std::endl
                  << "\t\tm_nPicNumInDecodeOrder: " << picNumInDecodeOrder << "\t\tframeCompleteFence " << videoDecodeCompleteFence
                  << "\t\tvideoDecodeCompleteSemaphore " << videoDecodeCompleteSemaphore << "\t\tdstImageView "
                  << pCurrFrameDecParams->decodeFrameInfo.dstPictureResource.imageViewBinding << std::endl;
    }

    const bool checkDecodeIdleSync = false; // For fence/sync/idle debugging
    if (checkDecodeIdleSync) { // For fence/sync debugging
        if (videoDecodeCompleteFence == VkFence()) {
            result = VulkanDeviceContext::GetThe()->MultiThreadedQueueWaitIdle(VulkanDeviceContext::DECODE, m_currentVideoQueueIndx);
            assert(result == VK_SUCCESS);
        } else {
            if (videoDecodeCompleteSemaphore == VkSemaphore()) {
                result = VulkanDeviceContext::GetThe()->WaitForFences(*VulkanDeviceContext::GetThe(), 1, &videoDecodeCompleteFence, true, gFenceTimeout);
                assert(result == VK_SUCCESS);
                result = VulkanDeviceContext::GetThe()->GetFenceStatus(*VulkanDeviceContext::GetThe(), videoDecodeCompleteFence);
                assert(result == VK_SUCCESS);
            }
        }
    }

    if (m_dumpDecodeData && (m_hwLoadBalancingTimelineSemaphore != VK_NULL_HANDLE)) { // For TL semaphore debug
       uint64_t  currSemValue = 0;
       VkResult semResult = VulkanDeviceContext::GetThe()->GetSemaphoreCounterValue(*VulkanDeviceContext::GetThe(), m_hwLoadBalancingTimelineSemaphore, &currSemValue);
       std::cout << "\t TL semaphore value ater submit: " << currSemValue << ", status: " << semResult << std::endl;

       const bool waitOnTlSemaphore = false;
       if (waitOnTlSemaphore) {
           uint64_t value = m_decodePicCount;
           VkSemaphoreWaitInfo waitInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO, nullptr, VK_SEMAPHORE_WAIT_ANY_BIT, 1,
                                        &m_hwLoadBalancingTimelineSemaphore, &value };
           std::cout << "\t TL semaphore wait for value: " << value << std::endl;
           semResult = VulkanDeviceContext::GetThe()->WaitSemaphores(*VulkanDeviceContext::GetThe(), &waitInfo, gLongTimeout);

           semResult = VulkanDeviceContext::GetThe()->GetSemaphoreCounterValue(*VulkanDeviceContext::GetThe(), m_hwLoadBalancingTimelineSemaphore, &currSemValue);
           std::cout << "\t TL semaphore value: " << currSemValue << ", status: " << semResult << std::endl;
       }
    }

    // For fence/sync debugging
    if (pDecodePictureInfo->flags.fieldPic) {
        result = VulkanDeviceContext::GetThe()->WaitForFences(*VulkanDeviceContext::GetThe(), 1, &videoDecodeCompleteFence, true, gFenceTimeout);
        assert(result == VK_SUCCESS);
        result = VulkanDeviceContext::GetThe()->GetFenceStatus(*VulkanDeviceContext::GetThe(), videoDecodeCompleteFence);
        assert(result == VK_SUCCESS);
    }

    const bool checkDecodeStatus = false; // Check the queries
    if (checkDecodeStatus && (frameSynchronizationInfo.queryPool != VK_NULL_HANDLE)) {
        VkQueryResultStatusKHR decodeStatus;
        result = VulkanDeviceContext::GetThe()->GetQueryPoolResults(*VulkanDeviceContext::GetThe(),
                                         frameSynchronizationInfo.queryPool,
                                         frameSynchronizationInfo.startQueryId,
                                         1,
                                         sizeof(decodeStatus),
                                         &decodeStatus,
                                         sizeof(decodeStatus),
                                         VK_QUERY_RESULT_WITH_STATUS_BIT_KHR | VK_QUERY_RESULT_WAIT_BIT);

        assert(result == VK_SUCCESS);
        assert(decodeStatus == VK_QUERY_RESULT_STATUS_COMPLETE_KHR);

        if (m_dumpDecodeData) {
            std::cout << "\t +++++++++++++++++++++++++++< " << currPicIdx << " >++++++++++++++++++++++++++++++" << std::endl;
            std::cout << "\t => Decode Status for CurrPicIdx: " << currPicIdx << std::endl
                      << "\t\tdecodeStatus: " << decodeStatus << std::endl;
        }
    }

    if (m_hwLoadBalancingTimelineSemaphore != VK_NULL_HANDLE) {
        m_currentVideoQueueIndx++;
        m_currentVideoQueueIndx %= VulkanDeviceContext::GetThe()->GetVideoDecodeNumQueues();
    }
    m_decodePicCount++;

    if (m_enableDecodeComputeFilter) {

        VkSharedBaseObj<VkImageResourceView> inputImageView;
        VkSharedBaseObj<VkImageResourceView> outputImageView;
        assert(m_imageSpecsIndex.filterIn != InvalidImageTypeIdx);
        int32_t index = m_videoFrameBuffer->GetCurrentImageResourceByIndex(currPicIdx, m_imageSpecsIndex.filterIn, inputImageView);
        assert(index == currPicIdx);
        assert(inputImageView);

        if ((index != currPicIdx) || !inputImageView) {
            return -1;
        }

        assert(m_imageSpecsIndex.filterOut != InvalidImageTypeIdx);
        index = m_videoFrameBuffer->GetCurrentImageResourceByIndex(currPicIdx, m_imageSpecsIndex.filterOut, outputImageView);

        assert(index == currPicIdx);
        assert(outputImageView);
        assert(inputImageView->GetImageView() != outputImageView->GetImageView());
        assert(inputImageView->GetPlaneImageView(0) != outputImageView->GetPlaneImageView(0));
        assert(inputImageView->GetPlaneImageView(1) != outputImageView->GetPlaneImageView(1));

        assert(pCurrFrameDecParams->decodeFrameInfo.dstPictureResource.imageViewBinding == inputImageView->GetImageView());

        result = m_yuvFilter->RecordCommandBuffer(currPicIdx,
                                                  inputImageView, &pCurrFrameDecParams->decodeFrameInfo.dstPictureResource,
                                                  outputImageView, nullptr, frameCompleteFence);
        assert(result == VK_SUCCESS);

        if (false) std::cout << currPicIdx << " : OUT view: " << outputImageView->GetImageView() << ", signalSem: " <<  frameCompleteSemaphore << std::endl << std::flush;
        assert(videoDecodeCompleteSemaphore != frameCompleteSemaphore);
        assert(VK_NOT_READY == VulkanDeviceContext::GetThe()->GetFenceStatus(*VulkanDeviceContext::GetThe(), frameCompleteFence));
        result = m_yuvFilter->SubmitCommandBuffer(currPicIdx,
                                                  1, &videoDecodeCompleteSemaphore,
                                                  1, &frameCompleteSemaphore,
                                                  frameCompleteFence);
        assert(result == VK_SUCCESS);
    }

    return currPicIdx;
}

VkDeviceSize VkVideoDecoder::GetBitstreamBuffer(VkDeviceSize size,
                                                VkDeviceSize minBitstreamBufferOffsetAlignment,
                                                VkDeviceSize minBitstreamBufferSizeAlignment,
                                                const uint8_t* pInitializeBufferMemory,
                                                VkDeviceSize initializeBufferMemorySize,
                                                VkSharedBaseObj<VulkanBitstreamBuffer>& bitstreamBuffer)
{
    assert(initializeBufferMemorySize <= size);
    // size_t newSize = 4 * 1024 * 1024;
    VkDeviceSize newSize = size;

    VkSharedBaseObj<VulkanBitstreamBufferImpl> newBitstreamBuffer;

    const bool enablePool = true;
    const bool debugBitstreamBufferDumpAlloc = false;
    int32_t availablePoolNode = -1;
    if (enablePool) {
        availablePoolNode = m_decodeFramesData.GetBitstreamBuffersQueue().GetAvailableNodeFromPool(newBitstreamBuffer);
    }
    if (!(availablePoolNode >= 0)) {
        VkResult result = VulkanBitstreamBufferImpl::Create(
                VulkanDeviceContext::GetThe()->GetVideoDecodeQueueFamilyIdx(),
                VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR,
                newSize, minBitstreamBufferOffsetAlignment,
                minBitstreamBufferSizeAlignment,
                pInitializeBufferMemory, initializeBufferMemorySize, newBitstreamBuffer);
        assert(result == VK_SUCCESS);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "\nERROR: VulkanBitstreamBufferImpl::Create() result: 0x%x\n", result);
            return 0;
        }
        if (debugBitstreamBufferDumpAlloc) {
            std::cout << "\tAllocated bitstream buffer with size " << newSize << " B, " <<
                             newSize/1024 << " KB, " << newSize/1024/1024 << " MB" << std::endl;
        }
        if (enablePool) {
            int32_t nodeAddedWithIndex = m_decodeFramesData.GetBitstreamBuffersQueue().AddNodeToPool(newBitstreamBuffer, true);
            if (nodeAddedWithIndex < 0) {
                assert("Could not add the new node to the pool");
            }
        }

    } else {

        assert(newBitstreamBuffer);
        newSize = newBitstreamBuffer->GetMaxSize();
        assert(initializeBufferMemorySize <= newSize);

        VkDeviceSize copySize = std::min<VkDeviceSize>(initializeBufferMemorySize, newSize);
        newBitstreamBuffer->CopyDataFromBuffer((const uint8_t*)pInitializeBufferMemory,
                                               0, // srcOffset
                                               0, // dstOffset
                                               copySize);

#ifdef CLEAR_BITSTREAM_BUFFERS_ON_CREATE
        newBitstreamBuffer->MemsetData(0x0, copySize, newSize - copySize);
#endif
        if (debugBitstreamBufferDumpAlloc) {
            std::cout << "\t\tFrom bitstream buffer pool with size " << newSize << " B, " <<
                             newSize/1024 << " KB, " << newSize/1024/1024 << " MB" << std::endl;

            std::cout << "\t\t\t FreeNodes " << m_decodeFramesData.GetBitstreamBuffersQueue().GetFreeNodesNumber();
            std::cout << " of MaxNodes " << m_decodeFramesData.GetBitstreamBuffersQueue().GetMaxNodes();
            std::cout << ", AvailableNodes " << m_decodeFramesData.GetBitstreamBuffersQueue().GetAvailableNodesNumber();
            std::cout << std::endl;
        }
    }
    bitstreamBuffer = newBitstreamBuffer;
    if (newSize > m_maxStreamBufferSize) {
        std::cout << "\tAllocated bitstream buffer with size " << newSize << " B, " <<
                             newSize/1024 << " KB, " << newSize/1024/1024 << " MB" << std::endl;
        m_maxStreamBufferSize = newSize;
    }
    return bitstreamBuffer->GetMaxSize();
}

VkResult VkVideoDecoder::Create(VkSharedBaseObj<VulkanVideoFrameBuffer>& videoFrameBuffer,
                                int32_t videoQueueIndx,
                                uint32_t enableDecoderFeatures,
                                VulkanFilterYuvCompute::FilterType filterType,
                                int32_t numDecodeImagesInFlight,
                                int32_t,
                                int32_t numBitstreamBuffersToPreallocate,
                                VkSharedBaseObj<VkVideoDecoder>& vkVideoDecoder)
{
    VkSharedBaseObj vkDecoder(new VkVideoDecoder(videoFrameBuffer,
                                                                 videoQueueIndx,
                                                                 enableDecoderFeatures,
                                                                 filterType,
                                                                 numDecodeImagesInFlight,
                                                                 numBitstreamBuffersToPreallocate));
    if (vkDecoder) {
        vkVideoDecoder = vkDecoder;
        return VK_SUCCESS;
    }

    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

void VkVideoDecoder::Deinitialize()
{
    if (VulkanDeviceContext::GetThe()->GetVideoDecodeNumQueues() > 1) {
        for (uint32_t queueId = 0; queueId <  (uint32_t)VulkanDeviceContext::GetThe()->GetVideoDecodeNumQueues(); queueId++) {
            VulkanDeviceContext::GetThe()->MultiThreadedQueueWaitIdle(VulkanDeviceContext::DECODE, queueId);
        }
    } else {
        VulkanDeviceContext::GetThe()->MultiThreadedQueueWaitIdle(VulkanDeviceContext::DECODE, m_currentVideoQueueIndx);
    }

    if (m_hwLoadBalancingTimelineSemaphore != VK_NULL_HANDLE) {
        VulkanDeviceContext::GetThe()->DestroySemaphore(*VulkanDeviceContext::GetThe(), m_hwLoadBalancingTimelineSemaphore, NULL);
        m_hwLoadBalancingTimelineSemaphore = VK_NULL_HANDLE;
    }

    m_videoFrameBuffer = nullptr;
    m_decodeFramesData.deinit();
    m_videoSession = nullptr;
    m_yuvFilter = nullptr;
}

VkVideoDecoder::~VkVideoDecoder()
{
    Deinitialize();
}

int32_t VkVideoDecoder::AddRef()
{
    return ++m_refCount;
}

int32_t VkVideoDecoder::Release()
{
    uint32_t ret;
    ret = --m_refCount;
    // Destroy the device if refcount reaches zero
    if (ret == 0) {
        delete this;
    }
    return ret;
}
