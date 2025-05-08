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

#include <cassert>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <cinttypes>

#include "VkCodecUtils/Helpers.h"
#include "VkCodecUtils/VulkanDeviceContext.h"
#include "VkVideoCore/VulkanVideoCapabilities.h"
#include "VulkanVideoProcessor.h"
#include "vulkan_interfaces.h"
#include "nvidia_utils/vulkan/ycbcrvkinfo.h"
#include "crcgenerator.h"

inline void CheckInputFile(const char* szInFilePath)
{
    std::ifstream fpIn(szInFilePath, std::ios::in | std::ios::binary);
    if (fpIn.fail()) {
        std::ostringstream err;
        err << "Unable to open input file: " << szInFilePath << std::endl;
        throw std::invalid_argument(err.str());
    }
}

int32_t VulkanVideoProcessor::Initialize(ProgramConfig& programConfig, VkQueue decodeQueue, VkQueue encodeQueue, VkQueue transferQueue, VkQueue presentQueue)
{

    const char* filePath = programConfig.videoFileName.c_str();
    const char* outputFileName = programConfig.outputFileName.empty() ?
            nullptr : programConfig.outputFileName.c_str();
    const VkVideoCodecOperationFlagBitsKHR forceCodecType = programConfig.forceParserType;
    const bool enableStreamDemuxing = (programConfig.enableStreamDemuxing == 1);
    const int32_t defaultWidth = programConfig.initialWidth;
    const int32_t defaultHeight = programConfig.initialHeight;
    const int32_t defaultBitDepth = programConfig.initialBitdepth;
    const uint32_t                            loopCount                        = programConfig.loopCount;
	constexpr uint32_t startFrame = 0;
    const int32_t  maxFrameCount = programConfig.maxFrameCount;
    const int32_t numDecodeImagesInFlight = std::max(programConfig.numDecodeImagesInFlight, 4);
    const int32_t numDecodeImagesToPreallocate = programConfig.numDecodeImagesToPreallocate;
    const int32_t numBitstreamBuffersToPreallocate = std::max(programConfig.numBitstreamBuffersToPreallocate, 4);
    const bool enableHwLoadBalancing = programConfig.enableHwLoadBalancing;
    const bool enablePostProcessFilter = (programConfig.enablePostProcessFilter >= 0);
    const bool enableDisplayPresent = (programConfig.noPresent == 0);
    const  VulkanFilterYuvCompute::FilterType postProcessFilterType = enablePostProcessFilter ?
            static_cast<VulkanFilterYuvCompute::FilterType>(programConfig.enablePostProcessFilter) :
                                                      VulkanFilterYuvCompute::YCBCRCOPY;

    Deinit();

    CheckInputFile(filePath);

    VkResult result = VideoStreamDemuxer::Create(filePath,
                                                 forceCodecType,
                                                 enableStreamDemuxing,
                                                 defaultWidth,
                                                 defaultHeight,
                                                 defaultBitDepth,
                                                 m_videoStreamDemuxer);

    if (result != VK_SUCCESS) {
        return -result;
    }

    m_usesStreamDemuxer = m_videoStreamDemuxer->IsStreamDemuxerEnabled();
    m_usesFramePreparser = m_videoStreamDemuxer->HasFramePreparser();

    result =  VulkanVideoFrameBuffer::Create(m_vkVideoFrameBuffer);
    assert(result == VK_SUCCESS);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "\nERROR: Create VulkanVideoFrameBuffer result: 0x%x\n", result);
    }

    FILE* outFile = m_frameToFile.AttachFile(outputFileName);
    if ((outputFileName != nullptr) && (outFile == nullptr)) {
        fprintf( stderr, "Error opening the output file %s", outputFileName);
        return -1;
    }

    uint32_t enableDecoderFeatures = 0;
    if (outFile != nullptr) {
        enableDecoderFeatures |= VkVideoDecoder::ENABLE_LINEAR_OUTPUT;
    }

    if (enableHwLoadBalancing) {
        enableDecoderFeatures |= VkVideoDecoder::ENABLE_HW_LOAD_BALANCING;
    }

    if (enablePostProcessFilter) {
        enableDecoderFeatures |= VkVideoDecoder::ENABLE_POST_PROCESS_FILTER;
    }

    if (enableDisplayPresent) {
        enableDecoderFeatures |= VkVideoDecoder::ENABLE_GRAPHICS_TEXTURE_SAMPLING;
    }

    result = VkVideoDecoder::Create(
                                    m_vkVideoFrameBuffer,
                                    0,
                                    enableDecoderFeatures,
                                    postProcessFilterType,
                                    numDecodeImagesInFlight,
                                    numDecodeImagesToPreallocate,
                                    numBitstreamBuffersToPreallocate,
                                    m_vkVideoDecoder);
    assert(result == VK_SUCCESS);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "\nERROR: Create VkVideoDecoder result: 0x%x\n", result);
    }

    VkVideoCoreProfile videoProfile(m_videoStreamDemuxer->GetVideoCodec(),
                                    m_videoStreamDemuxer->GetChromaSubsampling(),
                                    m_videoStreamDemuxer->GetLumaBitDepth(),
                                    m_videoStreamDemuxer->GetChromaBitDepth(),
                                    m_videoStreamDemuxer->GetProfileIdc());

    if (!VulkanVideoCapabilities::IsCodecTypeSupported(VulkanDeviceContext::GetThe(),
                                                       VulkanDeviceContext::GetThe()->GetVideoDecodeQueueFamilyIdx(),
                                                       m_videoStreamDemuxer->GetVideoCodec())) {
        std::cout << "*** The video codec " << VkVideoCoreProfile::CodecToName(m_videoStreamDemuxer->GetVideoCodec()) << " is not supported! ***" << std::endl;
        assert(!"The video codec is not supported");
        return -1;
    }

    VkVideoCapabilitiesKHR videoCapabilities;
    VkVideoDecodeCapabilitiesKHR videoDecodeCapabilities;
    result = VulkanVideoCapabilities::GetVideoDecodeCapabilities(VulkanDeviceContext::GetThe(), videoProfile,
                                                                 videoCapabilities,
                                                                 videoDecodeCapabilities);

    if (result != VK_SUCCESS)
	{
		std::cout << "*** Could not get Video Capabilities :" << result << " ***" << std::endl;
		assert(!"Could not get Video Capabilities!");
		return -result;
	}

	constexpr uint32_t defaultMinBufferSize = 2 * 1024 * 1024; // 2MB
    result = CreateParser(filePath,
                          m_videoStreamDemuxer->GetVideoCodec(),
                          defaultMinBufferSize,
                          static_cast<uint32_t>(videoCapabilities.minBitstreamBufferOffsetAlignment),
                          static_cast<uint32_t>(videoCapabilities.minBitstreamBufferSizeAlignment));
    assert(result == VK_SUCCESS);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "\nERROR: CreateParser() result: 0x%x\n", result);
    }

    m_loopCount = static_cast<int32_t>(loopCount);
    m_startFrame = startFrame;
    m_maxFrameCount = maxFrameCount;

    return 0;
}

VkResult VulkanVideoProcessor::Create(const ProgramConfig                   &settings,
                                      VkSharedBaseObj<VulkanVideoProcessor> &vulkanVideoProcessor)
{
    VkSharedBaseObj videoProcessor(new VulkanVideoProcessor(settings));

    if (videoProcessor) {
        vulkanVideoProcessor = videoProcessor;
        return VK_SUCCESS;
    }
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

VkFormat VulkanVideoProcessor::GetFrameImageFormat(int32_t* pWidth, int32_t* pHeight, int32_t* pBitDepth)  const
{
    VkFormat frameImageFormat = VK_FORMAT_UNDEFINED;
    if (m_videoStreamDemuxer) {
        if (m_videoStreamDemuxer->GetBitDepth() == 8) {
            frameImageFormat = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
        } else if (m_videoStreamDemuxer->GetBitDepth() == 10) {
            frameImageFormat = VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16;
        } else if (m_videoStreamDemuxer->GetBitDepth() == 12) {
            frameImageFormat = VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16;
        } else {
            assert(0);
        }

        if (pWidth) {
            *pWidth = GetWidth();
        }

        if (pHeight) {
            *pHeight = GetHeight();
        }

        if (pBitDepth) {
            *pBitDepth = GetBitDepth();
        }
    }

    return frameImageFormat;
}

int32_t VulkanVideoProcessor::GetWidth() const
{
    return m_videoStreamDemuxer->GetWidth();
}

int32_t VulkanVideoProcessor::GetHeight()  const
{
    return m_videoStreamDemuxer->GetHeight();
}

int32_t VulkanVideoProcessor::GetBitDepth()  const
{
    return m_videoStreamDemuxer->GetBitDepth();
}

void VulkanVideoProcessor::Deinit()
{

    m_vkParser = nullptr;
    m_vkVideoDecoder = nullptr;
    m_vkVideoFrameBuffer = nullptr;
    m_videoStreamDemuxer = nullptr;
}

void VulkanVideoProcessor::DumpVideoFormat(const VkParserDetectedVideoFormat* videoFormat, bool dumpData)
{
    if (dumpData) {
        std::cout << "Display Area : " << std::endl
                  << "\tLeft : " << videoFormat->display_area.left << std::endl
                  << "\tRight : " << videoFormat->display_area.right << std::endl
                  << "\tTop : " << videoFormat->display_area.top << std::endl
                  << "\tBottom : " << videoFormat->display_area.bottom << std::endl;
    }

    if (dumpData) {
        std::cout << "Geometry  : " << std::endl
                  << "\tCoded Width : " << videoFormat->coded_width << std::endl
                  << "\tDisplayed Width : " << videoFormat->display_area.right - videoFormat->display_area.left << std::endl
                  << "\tCoded Height : " << videoFormat->coded_height << std::endl
                  << "\tDisplayed Height : " << videoFormat->display_area.bottom - videoFormat->display_area.top << std::endl;
    }

    const char* pCodec = VkVideoCoreProfile::CodecToName(videoFormat->codec);
    if (dumpData) {
        std::cout << "Codec : " << pCodec << std::endl;
    }

    /* These below token numbers are based on "chroma_format_idc" from the spec. */
    /* Also, mind the separate_colour_plane_flag, as well. */
    static const char* nvVideoChromaFormat[] = {
        nullptr,
        "Monochrome",
        "420",
        nullptr,
        "422",
        nullptr,
        nullptr,
        nullptr,
        "444",
    };
    assert(videoFormat->chromaSubsampling < sizeof(nvVideoChromaFormat)/sizeof(nvVideoChromaFormat[0]));
    assert(nvVideoChromaFormat[videoFormat->chromaSubsampling] != nullptr);
    const char* pVideoChromaFormat = nvVideoChromaFormat[videoFormat->chromaSubsampling];
    if (dumpData) {
        std::cout << "VideoChromaFormat : " << pVideoChromaFormat << std::endl;
    }

    static const char* VideoFormat[] = {
        // Definitions for video_format

        "Component",
        "PAL",
        "NTSC",
        "SECAM",
        "MAC",
        "Unspecified",
        "Reserved6",
        "Reserved7",
    };
    assert(videoFormat->video_signal_description.video_format < sizeof(VideoFormat)/sizeof(VideoFormat[0]));
    const char* pVideoFormat = VideoFormat[videoFormat->video_signal_description.video_format];
    if (dumpData) {
        std::cout << "VideoFormat : " << pVideoFormat << std::endl;
    }

    const char* ColorPrimaries[] = {
        // Definitions for color_primaries

        "Forbidden",
        "BT709",
        "Unspecified",
        "Reserved",
        "BT470M",
        "BT470BG",
        "SMPTE170M",
        "SMPTE240M",
        "GenericFilm",
        "BT2020",
    };
    assert(videoFormat->video_signal_description.color_primaries < sizeof(ColorPrimaries)/sizeof(ColorPrimaries[0]));
    const char* pColorPrimaries = ColorPrimaries[videoFormat->video_signal_description.color_primaries];
    if (dumpData) {
        std::cout << "ColorPrimaries : " << pColorPrimaries << std::endl;
    }

    const char* TransferCharacteristics[] = {
        "Forbidden",
        "BT709",
        "Unspecified",
        "Reserved",
        "BT470M",
        "BT470BG",
        "SMPTE170M",
        "SMPTE240M",
        "Linear",
        "Log100",
        "Log316",
        "IEC61966_2_4",
        "BT1361",
        "IEC61966_2_1",
        "BT2020",
        "BT2020_2",
        "ST2084",
        "ST428_1",
    };
    assert(videoFormat->video_signal_description.transfer_characteristics < sizeof(TransferCharacteristics)/sizeof(TransferCharacteristics[0]));
    const char* pTransferCharacteristics = TransferCharacteristics[videoFormat->video_signal_description.transfer_characteristics];
    if (dumpData) {
        std::cout << "TransferCharacteristics : " << pTransferCharacteristics << std::endl;
    }

    const char* MatrixCoefficients[] = {
        "Forbidden",
        "BT709",
        "Unspecified",
        "Reserved",
        "FCC",
        "BT470BG",
        "SMPTE170M",
        "SMPTE240M",
        "YCgCo",
        "BT2020_NCL",
        "BT2020_CL",
    };
    assert(videoFormat->video_signal_description.matrix_coefficients < sizeof(MatrixCoefficients)/sizeof(MatrixCoefficients[0]));
    const char* pMatrixCoefficients = MatrixCoefficients[videoFormat->video_signal_description.matrix_coefficients];
    if (dumpData) {
        std::cout << "MatrixCoefficients : " << pMatrixCoefficients << std::endl;
    }
}

size_t ConvertFrameToNv12(const VulkanDeviceContext *vkDevCtx, int32_t frameWidth, int32_t frameHeight,
                                                    VkSharedBaseObj<VkImageResource>& imageResource,
                                                    uint8_t* pOutBuffer, const VkMpFormatInfo* mpInfo)
{
    size_t outputBufferSize = 0;
    VkDevice device   = imageResource->GetDevice();
    VkImage  srcImage = imageResource->GetImage ();
    VkSharedBaseObj<VulkanDeviceMemoryImpl> srcImageDeviceMemory(imageResource->GetMemory());

    // Map the image and read the image data.
    VkDeviceSize imageOffset = imageResource->GetImageDeviceMemoryOffset();
    VkDeviceSize maxSize = 0;
    const uint8_t* readImagePtr = srcImageDeviceMemory->GetReadOnlyDataPtr(imageOffset, maxSize);
    assert(readImagePtr != nullptr);

    int32_t secondaryPlaneHeight = frameHeight;
    int32_t imageHeight = frameHeight;
    bool isUnnormalizedRgba = false;
    if (mpInfo && (mpInfo->planesLayout.layout == YCBCR_SINGLE_PLANE_UNNORMALIZED) && !(mpInfo->planesLayout.disjoint)) {
        isUnnormalizedRgba = true;
    }

    if (mpInfo && mpInfo->planesLayout.secondaryPlaneSubsampledY) {
        secondaryPlaneHeight /= 2;
    }

    VkImageSubresource subResource = {};
    VkSubresourceLayout layouts[3];
    memset(layouts, 0x00, sizeof(layouts));

    if (mpInfo && !isUnnormalizedRgba) {
        switch (mpInfo->planesLayout.layout) {
            case YCBCR_SINGLE_PLANE_UNNORMALIZED:
            case YCBCR_SINGLE_PLANE_INTERLEAVED:
                subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
                vkDevCtx->GetImageSubresourceLayout(device, srcImage, &subResource, &layouts[0]);
                break;
            case YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED:
                subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
                vkDevCtx->GetImageSubresourceLayout(device, srcImage, &subResource, &layouts[0]);
                subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
                vkDevCtx->GetImageSubresourceLayout(device, srcImage, &subResource, &layouts[1]);
                break;
            case YCBCR_PLANAR_CBCR_STRIDE_INTERLEAVED:
            case YCBCR_PLANAR_CBCR_BLOCK_JOINED:
            case YCBCR_PLANAR_STRIDE_PADDED:
                subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
                vkDevCtx->GetImageSubresourceLayout(device, srcImage, &subResource, &layouts[0]);
                subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
                vkDevCtx->GetImageSubresourceLayout(device, srcImage, &subResource, &layouts[1]);
                subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_2_BIT;
                vkDevCtx->GetImageSubresourceLayout(device, srcImage, &subResource, &layouts[2]);
                break;
            default:
                assert(0);
        }

    } else {
        vkDevCtx->GetImageSubresourceLayout(device, srcImage, &subResource, &layouts[0]);
    }

    // Treat all non 8bpp formats as 16bpp for output to prevent any loss.
    uint32_t bytesPerPixel = 1;
    if (mpInfo->planesLayout.bpp != YCBCRA_8BPP) {
        bytesPerPixel = 2;
    }

    uint32_t numPlanes = 3;
    VkSubresourceLayout yuvPlaneLayouts[3] = {};
    yuvPlaneLayouts[0].offset = 0;
    yuvPlaneLayouts[0].rowPitch = frameWidth * bytesPerPixel;
    yuvPlaneLayouts[1].offset = yuvPlaneLayouts[0].rowPitch * frameHeight;
    yuvPlaneLayouts[1].rowPitch = frameWidth * bytesPerPixel;
    if (mpInfo && mpInfo->planesLayout.secondaryPlaneSubsampledX) {
        yuvPlaneLayouts[1].rowPitch /= 2;
    }
    yuvPlaneLayouts[2].offset = yuvPlaneLayouts[1].offset + (yuvPlaneLayouts[1].rowPitch * secondaryPlaneHeight);
    yuvPlaneLayouts[2].rowPitch = frameWidth * bytesPerPixel;
    if (mpInfo && mpInfo->planesLayout.secondaryPlaneSubsampledX) {
        yuvPlaneLayouts[2].rowPitch /= 2;
    }

    // Copy the luma plane, always assume the 422 or 444 formats and src CbCr always is interleaved (shares the same plane).
    uint32_t numCompatiblePlanes = 1;
    for (uint32_t plane = 0; plane < numCompatiblePlanes; plane++) {
        const uint8_t* pSrc = readImagePtr + layouts[plane].offset;
        uint8_t* pDst = pOutBuffer + yuvPlaneLayouts[plane].offset;
        for (int height = 0; height < imageHeight; height++) {
            memcpy(pDst, pSrc, (size_t)yuvPlaneLayouts[plane].rowPitch);
            pDst += (size_t)yuvPlaneLayouts[plane].rowPitch;
            pSrc += (size_t)layouts[plane].rowPitch;
        }
    }

	if (mpInfo)
	{
		// 9+ bpp is output as 16bpp yuv.
		for (uint32_t plane = numCompatiblePlanes; plane < numPlanes; plane++) {
			uint32_t srcPlane = std::min(plane, mpInfo->planesLayout.numberOfExtraPlanes);
			uint8_t* pDst = pOutBuffer + yuvPlaneLayouts[plane].offset;
			for (int height = 0; height < secondaryPlaneHeight; height++) {
				const uint8_t* pSrc;
				if (srcPlane != plane) {
					pSrc = readImagePtr + layouts[srcPlane].offset + ((plane - 1) * bytesPerPixel) + (layouts[srcPlane].rowPitch * height);

				} else {
					pSrc = readImagePtr + layouts[srcPlane].offset + (layouts[srcPlane].rowPitch * height);
				}

				for (VkDeviceSize width = 0; width < (yuvPlaneLayouts[plane].rowPitch / bytesPerPixel); width++) {
					memcpy(pDst, pSrc, bytesPerPixel);
					pDst += bytesPerPixel;
					pSrc += 2 * bytesPerPixel;
				}
			}
		}
	}

    outputBufferSize += ((size_t)yuvPlaneLayouts[0].rowPitch * imageHeight);
    if (mpInfo->planesLayout.numberOfExtraPlanes >= 1) {
        outputBufferSize += ((size_t)yuvPlaneLayouts[1].rowPitch * secondaryPlaneHeight);
        outputBufferSize += ((size_t)yuvPlaneLayouts[2].rowPitch * secondaryPlaneHeight);
    }

    return outputBufferSize;
}

size_t VulkanVideoProcessor::OutputFrameToFile(VulkanDecodedFrame* pFrame)
{
    if (!m_frameToFile) {
        return (size_t)-1;
    }

    assert(pFrame != nullptr);

    VkSharedBaseObj<VkImageResourceView> imageResourceView;
    pFrame->imageViews[VulkanDecodedFrame::IMAGE_VIEW_TYPE_LINEAR].GetImageResourceView(imageResourceView);
    assert(!!imageResourceView);
    assert(pFrame->pictureIndex != -1);

    VkSharedBaseObj<VkImageResource> imageResource = imageResourceView->GetImageResource();
    uint8_t* pOutputBuffer = m_frameToFile.EnsureAllocation(VulkanDeviceContext::GetThe(), imageResource);
    assert(pOutputBuffer != nullptr);

    // Needed allocation size can shrink, but may never grow. Frames will be allocated for maximum resolution upfront.
    assert((pFrame->displayWidth >= 0) && (pFrame->displayHeight >= 0));

    // Wait for decoder and copy engine to be done with this frame.
	vk::WaitAndGetStatus(VulkanDeviceContext::GetThe()->getDevice(),
                     pFrame->frameCompleteFence,
                     pFrame->queryPool,
                     pFrame->startQueryId,
                     pFrame->pictureIndex, false, "frameCompleteFence");

    // Convert frame to linear image format and write it to file.
    VkFormat format = imageResource->GetImageCreateInfo().format;
    const VkMpFormatInfo* mpInfo = YcbcrVkFormatInfo(format);
    size_t usedBufferSize = ConvertFrameToNv12(VulkanDeviceContext::GetThe(), pFrame->displayWidth, pFrame->displayHeight, imageResource, pOutputBuffer, mpInfo);

    // Output a crc for this frame.
    if (m_settings.outputcrcPerFrame != 0) {
        fprintf(m_settings.crcOutputFile, "CRC Frame[%" PRId64 "]:", pFrame->displayOrder);
        size_t crcCount = m_settings.crcInitValue.size();
        for (size_t i = 0; i < crcCount; i += 1) {
            uint32_t frameCrc = m_settings.crcInitValue[i];
            getCRC(&frameCrc, pOutputBuffer, usedBufferSize, Crc32Table);
            fprintf(m_settings.crcOutputFile, "0x%08X ", frameCrc);
        }
        fprintf(m_settings.crcOutputFile, "\n");
        if (m_settings.crcOutputFile != stdout) {
            fflush(m_settings.crcOutputFile);
        }
    }

    if ((m_settings.outputcrc != 0) && (m_settings.crcOutput != nullptr)) {
        size_t crcCount = m_settings.crcInitValue.size();
        for (size_t i = 0; i < crcCount; i += 1) {
            getCRC(&(m_settings.crcOutput[i]), pOutputBuffer, usedBufferSize, Crc32Table);
        }
    }

    // Write image to file.
    if (m_settings.outputy4m != 0) {
        return m_frameToFile.WriteFrameToFileY4M(0, usedBufferSize, pFrame->displayWidth, pFrame->displayHeight, mpInfo);
    } else {
        return m_frameToFile.WriteDataToFile(0, usedBufferSize);
    }
}

void VulkanVideoProcessor::Restart()
{
    m_videoStreamDemuxer->Rewind();
    m_videoFrameNum = false;
    m_currentBitstreamOffset = 0;
}

bool VulkanVideoProcessor::StreamCompleted()
{
    if (--m_loopCount > 0)
	{
		std::cout << "Restarting video stream with loop number " << (m_loopCount + 1) << std::endl;
		// Reload the file stream
		Restart();
		return false;
	}
	std::cout << "End of Video Stream with status  " << VK_SUCCESS << std::endl;
	return true;
}

int32_t VulkanVideoProcessor::ParserProcessNextDataChunk()
{
    if (m_videoStreamsCompleted) {
        return -1;
    }

    int32_t retValue = 0;
    int64_t bitstreamChunkSize = 0;
    size_t  bitstreamBytesConsumed = 0;
    const uint8_t* pBitstreamData = nullptr;
    bool requiresPartialParsing = false;
    if (m_usesFramePreparser || m_usesStreamDemuxer) {
        bitstreamChunkSize = m_videoStreamDemuxer->DemuxFrame(&pBitstreamData);
        assert(bitstreamBytesConsumed <= (size_t)std::numeric_limits<int32_t>::max());
        retValue = (int32_t)bitstreamChunkSize;
    } else {
        bitstreamChunkSize = m_videoStreamDemuxer->ReadBitstreamData(&pBitstreamData, m_currentBitstreamOffset);
        requiresPartialParsing = true;
    }
	if ((bitstreamChunkSize > 0) && pBitstreamData != nullptr) {
        assert(static_cast<uint64_t>(bitstreamChunkSize) < std::numeric_limits<size_t>::max());
		const VkResult parserStatus = ParseVideoStreamData(pBitstreamData, static_cast<size_t>(bitstreamChunkSize),
                                                     &bitstreamBytesConsumed,
                                                     requiresPartialParsing);
        if (parserStatus != VK_SUCCESS) {
            m_videoStreamsCompleted = true;
            std::cerr << "Parser: end of Video Stream with status  " << parserStatus << std::endl;
            retValue = -1;
        } else {
            retValue = static_cast<int32_t>(bitstreamBytesConsumed);
        }
        assert(bitstreamBytesConsumed <= static_cast<size_t>(std::numeric_limits<int32_t>::max()));
        m_currentBitstreamOffset += static_cast<int64_t>(bitstreamBytesConsumed);
    } else {
        // Call the parser one last time with zero buffer to flush the display queue.
        ParseVideoStreamData(nullptr, 0, &bitstreamBytesConsumed, requiresPartialParsing);
        m_videoStreamsCompleted = StreamCompleted();
        retValue = 0;
    }

    return retValue;
}

int32_t VulkanVideoProcessor::GetNextFrame(VulkanDecodedFrame* pFrame, bool* endOfStream)
{
    // The below call to DequeueDecodedPicture allows returning the next frame without parsing of the stream.
    // Parsing is only done when there are no more frames in the queue.
    int32_t framesInQueue = m_vkVideoFrameBuffer->DequeueDecodedPicture(pFrame);

    // Loop until a frame (or more) is parsed and added to the queue.
    while ((framesInQueue == 0) && !m_videoStreamsCompleted) {

        ParserProcessNextDataChunk();

        framesInQueue = m_vkVideoFrameBuffer->DequeueDecodedPicture(pFrame);
    }

    if (framesInQueue) {

        if (m_videoFrameNum == 0) {
            DumpVideoFormat(m_vkVideoDecoder->GetVideoFormatInfo(), true);
        }

        if (m_frameToFile) {
            OutputFrameToFile(pFrame);
        }

        m_videoFrameNum++;
    }

    if ((m_maxFrameCount != -1) && (m_videoFrameNum >= (uint32_t)m_maxFrameCount)) {
        // Tell the FrameProcessor we're done after this frame is drawn.
        std::cout << "Number of video frames " << m_videoFrameNum
                  << " of max frame number " << m_maxFrameCount << std::endl;
        m_videoStreamsCompleted = StreamCompleted();
        *endOfStream = m_videoStreamsCompleted;
        return -1;
    }

    *endOfStream = m_videoStreamsCompleted;

    if ((framesInQueue == 0) && m_videoStreamsCompleted) {
        return -1;
    }

    return 1;
}

int32_t VulkanVideoProcessor::ReleaseFrame(VulkanDecodedFrame* pDisplayedFrame)
{
    if (pDisplayedFrame->pictureIndex != -1) {
        DecodedFrameRelease decodedFramesRelease = { pDisplayedFrame->pictureIndex };
        DecodedFrameRelease* decodedFramesReleasePtr = &decodedFramesRelease;

        pDisplayedFrame->pictureIndex = -1;

        decodedFramesRelease.decodeOrder = pDisplayedFrame->decodeOrder;
        decodedFramesRelease.displayOrder = pDisplayedFrame->displayOrder;

        decodedFramesRelease.hasConsummerSignalFence = pDisplayedFrame->hasConsummerSignalFence;
        decodedFramesRelease.hasConsummerSignalSemaphore = pDisplayedFrame->hasConsummerSignalSemaphore;
        decodedFramesRelease.timestamp = 0;

        return m_vkVideoFrameBuffer->ReleaseDisplayedPicture(&decodedFramesReleasePtr, 1);
    }

    return -1;
}

VkResult VulkanVideoProcessor::CreateParser(const char*,
                                            VkVideoCodecOperationFlagBitsKHR vkCodecType,
                                            uint32_t defaultMinBufferSize,
                                            uint32_t bufferOffsetAlignment,
                                            uint32_t bufferSizeAlignment)
{
    static constexpr VkExtensionProperties h264StdExtensionVersion = { VK_STD_VULKAN_VIDEO_CODEC_H264_DECODE_EXTENSION_NAME, VK_STD_VULKAN_VIDEO_CODEC_H264_DECODE_SPEC_VERSION };
    static constexpr VkExtensionProperties h265StdExtensionVersion = { VK_STD_VULKAN_VIDEO_CODEC_H265_DECODE_EXTENSION_NAME, VK_STD_VULKAN_VIDEO_CODEC_H265_DECODE_SPEC_VERSION };
    static constexpr VkExtensionProperties av1StdExtensionVersion = { VK_STD_VULKAN_VIDEO_CODEC_AV1_DECODE_EXTENSION_NAME, VK_STD_VULKAN_VIDEO_CODEC_AV1_DECODE_SPEC_VERSION };

    const VkExtensionProperties* pStdExtensionVersion = nullptr;
    if (vkCodecType == VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR) {
        pStdExtensionVersion = &h264StdExtensionVersion;
    } else if (vkCodecType == VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR) {
        pStdExtensionVersion = &h265StdExtensionVersion;
    } else if (vkCodecType == VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR) {
        pStdExtensionVersion = &av1StdExtensionVersion;
    } else {
        assert(!"Unsupported Codec Type");
        return VK_ERROR_FORMAT_NOT_SUPPORTED;
    }

    VkSharedBaseObj<IVulkanVideoDecoderHandler> decoderHandler(m_vkVideoDecoder);
    VkSharedBaseObj<IVulkanVideoFrameBufferParserCb> videoFrameBufferCb(m_vkVideoFrameBuffer);
    return vulkanCreateVideoParser(decoderHandler,
                                   videoFrameBufferCb,
                                   vkCodecType,
                                   pStdExtensionVersion,
                                   1, // maxNumDecodeSurfaces - currently ignored
                                   1, // maxNumDpbSurfaces - currently ignored
                                   defaultMinBufferSize,
                                   bufferOffsetAlignment,
                                   bufferSizeAlignment,
                                   0, // clockRate - default 0 = 10Mhz
                                   m_vkParser);
}

VkResult VulkanVideoProcessor::ParseVideoStreamData(const uint8_t* pData, size_t size,
                                                    size_t *pnVideoBytes, bool doPartialParsing,
                                                    uint32_t flags, int64_t timestamp) const
{
    if (!m_vkParser) {
        assert(!"Parser not initialized!");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkParserSourceDataPacket packet = { 0 };
    packet.payload = pData;
    packet.payload_size = size;
    packet.flags = flags;
    if (timestamp) {
        packet.flags |= VK_PARSER_PKT_TIMESTAMP;
    }
    packet.timestamp = timestamp;
    if (!pData || size == 0) {
        packet.flags |= VK_PARSER_PKT_ENDOFSTREAM;
    }

    return m_vkParser->ParseVideoData(&packet, pnVideoBytes, doPartialParsing);
}
