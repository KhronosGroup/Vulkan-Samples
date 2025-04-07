/*
* Copyright 2017-2022 NVIDIA Corporation.  All rights reserved.
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

#include <iostream>
#include "VkDecoderUtils/VideoStreamDemuxer.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavcodec/avcodec.h>
#ifndef FF_API_OLD_BSF
#include <libavcodec/bsf.h>
#endif
}

inline bool check(int e, int iLine, const char *szFile) {
    if (e < 0) {
        std::cerr << "General error " << e << " at line " << iLine << " in file " << szFile;
        return false;
    }
    return true;
}

#define ck(call) check(call, __LINE__, __FILE__)

class FFmpegDemuxer : public VideoStreamDemuxer {

public:
    class DataProvider {
    public:
        virtual ~DataProvider() {}
        virtual int GetData(uint8_t *pBuf, int nBuf) = 0;
    };

private:
    // Initialize stream and demuxer

    int Initialize()
    {
    	int32_t defaultWidth = 1920;
	   int32_t defaultHeight = 1080;
	   int32_t defaultBitDepth = 12;
        if (!fmtc) {
            std::cerr << "No AVFormatContext provided.";
            return -1;
        }

        std::cout << "Media format: " << fmtc->iformat->long_name << " (" << fmtc->iformat->name << ")\n";

        ck(avformat_find_stream_info(fmtc, nullptr));
        videoStream = av_find_best_stream(fmtc, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (videoStream < 0) {
            std::cerr << "FFmpeg error: " << __FILE__ << " " << __LINE__ << " " << "Could not find stream in input file";
            return -1;
        }

        //fmtc->streams[iVideoStream]->need_parsing = AVSTREAM_PARSE_NONE;
        videoCodec = fmtc->streams[videoStream]->codecpar->codec_id;
        codedWidth = fmtc->streams[videoStream]->codecpar->width;
        codedHeight = fmtc->streams[videoStream]->codecpar->height;
        format = (AVPixelFormat)fmtc->streams[videoStream]->codecpar->format;
        codedLumaBitDepth = 8;
        codedChromaBitDepth = 8;
        if (fmtc->streams[videoStream]->codecpar->format == AV_PIX_FMT_YUV420P10LE) {
            codedLumaBitDepth = 10;
            codedChromaBitDepth = 10;
        } else if (fmtc->streams[videoStream]->codecpar->format == AV_PIX_FMT_YUV420P12LE) {
            codedLumaBitDepth = 12;
            codedChromaBitDepth = 12;
        }
        isStreamDemuxer = (!strcmp(fmtc->iformat->long_name, "QuickTime / MOV") ||
                 !strcmp(fmtc->iformat->long_name, "FLV (Flash Video)") ||
                 !strcmp(fmtc->iformat->long_name, "Matroska / WebM"));

        /**
         * Codec-specific bitstream restrictions that the stream conforms to.
         */
        profile = fmtc->streams[videoStream]->codecpar->profile;
        level = fmtc->streams[videoStream]->codecpar->level;

        /**
         * Video only. The aspect ratio (width / height) which a single pixel
         * should have when displayed.
         *
         * When the aspect ratio is unknown / undefined, the numerator should be
         * set to 0 (the denominator may have any value).
         */
        sample_aspect_ratio = fmtc->streams[videoStream]->codecpar->sample_aspect_ratio;

        /**
         * Video only. The order of the fields in interlaced video.
         */
        field_order = fmtc->streams[videoStream]->codecpar->field_order;

        /**
         * Video only. Additional colorspace characteristics.
         */
        colorRange = fmtc->streams[videoStream]->codecpar->color_range;
        colorPrimaries = fmtc->streams[videoStream]->codecpar->color_primaries;
        colorTransferCharacteristics = fmtc->streams[videoStream]->codecpar->color_trc;
        colorSpace = fmtc->streams[videoStream]->codecpar->color_space;
        chromaLocation = fmtc->streams[videoStream]->codecpar->chroma_location;

#if (LIBAVCODEC_VERSION_MAJOR < 58)
        pPkt = (AVPacket *)av_malloc(sizeof(AVPacket));
        av_init_packet(pPkt);
#else
        pPkt = av_packet_alloc();
#endif // (LIBAVCODEC_VERSION_MAJOR < 58)
        pPkt->data = NULL;
        pPkt->size = 0;
#if (LIBAVCODEC_VERSION_MAJOR < 58)
        pktFiltered = (AVPacket *)av_malloc(sizeof(AVPacket));
        av_init_packet(pktFiltered);
#else
        pktFiltered = av_packet_alloc();
#endif
        pktFiltered->data = NULL;
        pktFiltered->size = 0;

        if (isStreamDemuxer) {
            const AVBitStreamFilter *bsf = NULL;

            if (videoCodec == AV_CODEC_ID_H264) {
                bsf = av_bsf_get_by_name("h264_mp4toannexb");
            } else if (videoCodec == AV_CODEC_ID_HEVC) {
                bsf = av_bsf_get_by_name("hevc_mp4toannexb");
            } else if (videoCodec == AV_CODEC_ID_AV1) {
                bsf = av_bsf_get_by_name("av1_metadata");
            }

            if (!bsf) {
                std::cerr << "FFmpeg error: " << __FILE__ << " " << __LINE__ << " " << "av_bsf_get_by_name(): " << videoCodec << " failed";
                return -1;
            }
            ck(av_bsf_alloc(bsf, &bsfc));
            bsfc->par_in = fmtc->streams[videoStream]->codecpar;
            ck(av_bsf_init(bsfc));
        }

        if (fmtc->streams[videoStream]->codecpar->width == 0) {
            codedWidth = defaultWidth;
        }
        if (fmtc->streams[videoStream]->codecpar->height == 0) {
            codedHeight = defaultHeight;
        }
        if (fmtc->streams[videoStream]->codecpar->format == -1) {
            codedLumaBitDepth = defaultBitDepth;
        }

        return 0;
    }

    AVFormatContext *CreateFormatContext(DataProvider *pDataProvider) {

        AVFormatContext *ctx = nullptr;
        if (!((ctx = avformat_alloc_context()))) {
            std::cerr << "FFmpeg error: " << __FILE__ << " " << __LINE__;
            return nullptr;
        }

        uint8_t *avioc_buffer = nullptr;
        int avioc_buffer_size = 8 * 1024 * 1024;
        avioc_buffer = static_cast<uint8_t *>(av_malloc(avioc_buffer_size));
        if (!avioc_buffer) {
            std::cerr << "FFmpeg error: " << __FILE__ << " " << __LINE__;
            return nullptr;
        }
        avioc = avio_alloc_context(avioc_buffer, avioc_buffer_size,
            0, pDataProvider, &ReadPacket, nullptr, nullptr);
        if (!avioc) {
            std::cerr << "FFmpeg error: " << __FILE__ << " " << __LINE__;
            return nullptr;
        }
        ctx->pb = avioc;

        ck(avformat_open_input(&ctx, nullptr, nullptr, nullptr));
        return ctx;
    }

    static AVFormatContext *CreateFormatContext(const char *pFilePath, enum AVCodecID videoCodecId) {
        avformat_network_init();

        AVFormatContext *ctx = avformat_alloc_context();
        ctx->video_codec_id = videoCodecId;
        if (videoCodecId != AV_CODEC_ID_NONE) {
            ctx->video_codec = avcodec_find_decoder(videoCodecId);
        }
        ck(avformat_open_input(&ctx, pFilePath, nullptr, nullptr));
        return ctx;
    }

    explicit FFmpegDemuxer(const char *pFilePath,
	              AVCodecID video_codec_id = AV_CODEC_ID_NONE /* Forced video codec_id */)
        : pPkt()
        , pktFiltered()
        , videoStream()
        , isStreamDemuxer()
        , videoCodec()
        , codedWidth()
        , codedHeight()
        , codedLumaBitDepth()
        , codedChromaBitDepth()
        , format()
        , profile()
        , level()
        , sample_aspect_ratio()
        , field_order()
        , colorRange()
        , colorPrimaries()
        , colorTransferCharacteristics()
        , colorSpace()
        , chromaLocation()
        {
            fmtc = CreateFormatContext(pFilePath, video_codec_id);
        }

    ~FFmpegDemuxer() override {

        if (pPkt) {
#if (LIBAVCODEC_VERSION_MAJOR < 58)
            if (pPkt->data) {
                av_packet_unref(pPkt);
            }
            av_free(pPkt);
#else // (LIBAVCODEC_VERSION_MAJOR < 58)
            av_packet_free(&pPkt);
#endif // (LIBAVCODEC_VERSION_MAJOR < 58)
            pPkt = nullptr;
        }

        if (pktFiltered) {
#if (LIBAVCODEC_VERSION_MAJOR < 58)
            if (pktFiltered->data) {
                av_packet_unref(pktFiltered);
            }
            av_free(pktFiltered);
#else // (LIBAVCODEC_VERSION_MAJOR < 58)
            av_packet_free(&pktFiltered);
#endif // (LIBAVCODEC_VERSION_MAJOR < 58)
            pktFiltered = nullptr;
        }

        if (fmtc) {
            avformat_close_input(&fmtc);
            avformat_free_context(fmtc);
            fmtc = nullptr;
        }

        if (avioc) {
             av_freep(&avioc->buffer);
             av_freep(&avioc);
         }
    }

public:
    static VkResult Create(const char *pFilePath,
                           VkVideoCodecOperationFlagBitsKHR codecType,
                           bool,
                           int32_t,
                           int32_t,
                           int32_t,
                           VkSharedBaseObj<FFmpegDemuxer>& ffmpegDemuxer)
    {
		AVCodecID videoCodecId = AV_CODEC_ID_NONE;

        if (codecType != VK_VIDEO_CODEC_OPERATION_NONE_KHR) {
            if (codecType == VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR) {
                videoCodecId = AV_CODEC_ID_H264;
            } else if (codecType == VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR) {
                videoCodecId = AV_CODEC_ID_HEVC;
            }
        }

        VkSharedBaseObj demuxer(new FFmpegDemuxer(pFilePath, videoCodecId));

        if (demuxer && (demuxer->Initialize() >= 0)) {
            ffmpegDemuxer = demuxer;
            return VK_SUCCESS;
        }
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    static VkVideoCodecOperationFlagBitsKHR FFmpegToVkCodecOperation(const AVCodecID id) {
        switch (id) {
        	case AV_CODEC_ID_H264       : return VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR;
        	case AV_CODEC_ID_HEVC       : return VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR;
    #ifdef VK_EXT_video_decode_vp9
	        case AV_CODEC_ID_VP9        : return VK_VIDEO_CODEC_OPERATION_DECODE_VP9_BIT_KHR;
    #endif // VK_EXT_video_decode_vp9
    #ifdef vulkan_video_codec_av1std_decode
    	    case AV_CODEC_ID_AV1        : return VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR;
    #endif
        	case AV_CODEC_ID_VC1        :
	        case AV_CODEC_ID_MJPEG      :
        	case AV_CODEC_ID_MPEG1VIDEO:
			case AV_CODEC_ID_MPEG2VIDEO :
			case AV_CODEC_ID_VP8        :
			case AV_CODEC_ID_MPEG4      :
			default                     : assert(false); return VkVideoCodecOperationFlagBitsKHR(0);
        }
    }

    VkVideoCodecOperationFlagBitsKHR GetVideoCodec() const override
    {
        return FFmpegToVkCodecOperation(videoCodec);
    }

    VkVideoComponentBitDepthFlagsKHR GetLumaBitDepth() const override
    {
        switch (codedLumaBitDepth) {
        case 8:
            return VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
        case 10:
            return VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR;
        case 12:
            return VK_VIDEO_COMPONENT_BIT_DEPTH_12_BIT_KHR;
        default:
            assert(!"Unknown Luma Bit Depth!");
        }
        return VK_VIDEO_COMPONENT_BIT_DEPTH_INVALID_KHR;
    }

    VkVideoComponentBitDepthFlagsKHR GetChromaBitDepth() const override
    {
        switch (codedChromaBitDepth) {
        case 8:
            return VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
        case 10:
            return VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR;
        case 12:
            return VK_VIDEO_COMPONENT_BIT_DEPTH_12_BIT_KHR;
        default:
            assert(!"Unknown Chroma Bit Depth!");
        }
        return VK_VIDEO_COMPONENT_BIT_DEPTH_INVALID_KHR;
    }

    VkVideoChromaSubsamplingFlagsKHR GetChromaSubsampling() const override
    {
        switch (format) {
        case AV_PIX_FMT_YUVJ420P:    ///< planar YUV 4:2:0, 12bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV420P and setting color_range
        case AV_PIX_FMT_YUV420P:     ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
        case AV_PIX_FMT_YUV420P10LE: ///< planar YUV 4:2:0, 15bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian
        case AV_PIX_FMT_YUV420P16LE: ///< planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian
        case AV_PIX_FMT_YUV420P16BE: ///< planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian
            return VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR;
        case AV_PIX_FMT_YUYV422:     ///< packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
        case AV_PIX_FMT_YUV422P:     ///< planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
        case AV_PIX_FMT_YUV422P16LE: ///< planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian
        case AV_PIX_FMT_YUV422P16BE: ///< planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian
            return VK_VIDEO_CHROMA_SUBSAMPLING_422_BIT_KHR;
        case AV_PIX_FMT_YUV444P:     ///< planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
        case AV_PIX_FMT_YUV444P10BE: ///< planar YUV 4:4:4, 30bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian
        case AV_PIX_FMT_YUV444P10LE: ///< planar YUV 4:4:4, 30bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian
        case AV_PIX_FMT_YUV444P12BE: ///< planar YUV 4:4:4,36bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian
        case AV_PIX_FMT_YUV444P12LE: ///< planar YUV 4:4:4,36bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian
        case AV_PIX_FMT_YUV444P16LE: ///< planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian
        case AV_PIX_FMT_YUV444P16BE: ///< planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian
            return VK_VIDEO_CHROMA_SUBSAMPLING_444_BIT_KHR;
        default:
            break;
        }
        // assert(!"Unknown CHROMA_SUBSAMPLING!");
        std::cerr << "\nUnknown CHROMA_SUBSAMPLING from format: " << format << std::endl;
        return VK_VIDEO_CHROMA_SUBSAMPLING_444_BIT_KHR;
    }

    uint32_t GetProfileIdc() const override
    {
        switch (FFmpegToVkCodecOperation(videoCodec)) {
            case VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR:
            {
                switch(profile) {
                    case STD_VIDEO_H264_PROFILE_IDC_BASELINE:
                    case STD_VIDEO_H264_PROFILE_IDC_MAIN:
                    case STD_VIDEO_H264_PROFILE_IDC_HIGH:
                    case STD_VIDEO_H264_PROFILE_IDC_HIGH_444_PREDICTIVE:
                        break;
                    default:
                        std::cerr << "\nInvalid h.264 profile: " << profile << std::endl;
                }
            }
            break;
            case VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR:
            {
                switch(profile) {
                    case STD_VIDEO_H265_PROFILE_IDC_MAIN:
                    case STD_VIDEO_H265_PROFILE_IDC_MAIN_10:
                    case STD_VIDEO_H265_PROFILE_IDC_MAIN_STILL_PICTURE:
                    case STD_VIDEO_H265_PROFILE_IDC_FORMAT_RANGE_EXTENSIONS:
                    case STD_VIDEO_H265_PROFILE_IDC_SCC_EXTENSIONS:
                        break;
                    default:
                        std::cerr << "\nInvalid h.265 profile: " << profile << std::endl;
                }
            }
            break;
            case VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR:
            {
                switch(profile) {
                    case STD_VIDEO_AV1_PROFILE_MAIN:
                    case STD_VIDEO_AV1_PROFILE_HIGH:
                    case STD_VIDEO_AV1_PROFILE_PROFESSIONAL:
                        break;
                    default:
                        std::cerr << "\nInvalid AV1 profile: " << profile << std::endl;
                }
            }
            break;
            default:
                std::cerr << "\nInvalid codec type: " << FFmpegToVkCodecOperation(videoCodec) << std::endl;
        }
        return static_cast<uint32_t>(profile);
    }

    int GetWidth()  const override {
        return codedWidth;
    }
    int GetHeight()  const override {
        return codedHeight;
    }
    int GetBitDepth()  const override {
        return codedLumaBitDepth;
    }

    bool IsStreamDemuxerEnabled() const override { return isStreamDemuxer; }
    bool HasFramePreparser() const override { return true; }

    int64_t DemuxFrame(const uint8_t **ppVideo) override {

        if (!fmtc) {
            return -1;
        }

        if (pPkt->data) {
            av_packet_unref(pPkt);
        }

        int e = 0;
        while ((e = av_read_frame(fmtc, pPkt)) >= 0 && pPkt->stream_index != videoStream) {
            av_packet_unref(pPkt);
        }
        if (e < 0) {
            return e;
        }

        if (isStreamDemuxer)
		{
			if (pktFiltered->data)
			{
				av_packet_unref(pktFiltered);
			}
			ck(av_bsf_send_packet(bsfc, pPkt));
			ck(av_bsf_receive_packet(bsfc, pktFiltered));
			*ppVideo = pktFiltered->data;
			return pktFiltered->size;
		}
		*ppVideo = pPkt->data;
		return pPkt->size;
	}

    int64_t ReadBitstreamData(const uint8_t**, int64_t) override {
        return -1;
    }

    static int ReadPacket(void *opaque, uint8_t *pBuf, int nBuf) {
        return static_cast<DataProvider *>(opaque)->GetData(pBuf, nBuf);
    }

    void Rewind() override
    {
        av_seek_frame(fmtc, videoStream, 0, isStreamDemuxer ? AVSEEK_FLAG_ANY : AVSEEK_FLAG_BYTE);
    }

    void DumpStreamParameters() const override {

        std::cout << "Width: "    << codedWidth << std::endl;
        std::cout << "Height: "   << codedHeight <<  std::endl;
        std::cout << "BitDepth: " << codedLumaBitDepth << std::endl;
        std::cout << "Profile: "  << profile << std::endl;
        std::cout << "Level: "    << level << std::endl;
        std::cout << "Aspect Ration: "    << static_cast<float>(sample_aspect_ratio.num) / static_cast<float>(sample_aspect_ratio.den) << std::endl;

        static const char* FieldOrder[] = {
            "UNKNOWN",
            "PROGRESSIVE",
            "TT: Top coded_first, top displayed first",
            "BB: Bottom coded first, bottom displayed first",
            "TB: Top coded first, bottom displayed first",
            "BT: Bottom coded first, top displayed first",
        };
        std::cout << "Field Order: "    << FieldOrder[field_order] << std::endl;

        static const char* ColorRange[] = {
            "UNSPECIFIED",
            "MPEG: the normal 219*2^(n-8) MPEG YUV ranges",
            "JPEG: the normal     2^n-1   JPEG YUV ranges",
            "NB: Not part of ABI",
        };
        std::cout << "Color Range: "    << ColorRange[colorRange] << std::endl;

        static const char* ColorPrimaries[] = {
            "RESERVED0",
            "BT709: also ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP177 Annex B",
            "UNSPECIFIED",
            "RESERVED",
            "BT470M: also FCC Title 47 Code of Federal Regulations 73.682 (a)(20)",

            "BT470BG: also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM",
            "SMPTE170M: also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC",
            "SMPTE240M: also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC",
            "FILM: colour filters using Illuminant C",
            "BT2020: ITU-R BT2020",
            "SMPTE428: SMPTE ST 428-1 (CIE 1931 XYZ)",
            "SMPTE431: SMPTE ST 431-2 (2011) / DCI P3",
            "SMPTE432: SMPTE ST 432-1 (2010) / P3 D65 / Display P3",
            "JEDEC_P22: JEDEC P22 phosphors",
            "NB: Not part of ABI",
        };
        std::cout << "Color Primaries: "    << ColorPrimaries[colorPrimaries] << std::endl;

        static const char* ColorTransferCharacteristic[] = {
            "RESERVED0",
            "BT709: also ITU-R BT1361",
            "UNSPECIFIED",
            "RESERVED",
            "GAMMA22:  also ITU-R BT470M / ITU-R BT1700 625 PAL & SECAM",
            "GAMMA28:  also ITU-R BT470BG",
            "SMPTE170M:  also ITU-R BT601-6 525 or 625 / ITU-R BT1358 525 or 625 / ITU-R BT1700 NTSC",
            "SMPTE240M",
            "LINEAR:  Linear transfer characteristics",
            "LOG: Logarithmic transfer characteristic (100:1 range)",
            "LOG_SQRT: Logarithmic transfer characteristic (100 * Sqrt(10) : 1 range)",
            "IEC61966_2_4: IEC 61966-2-4",
            "BT1361_ECG: ITU-R BT1361 Extended Colour Gamut",
            "IEC61966_2_1: IEC 61966-2-1 (sRGB or sYCC)",
            "BT2020_10: ITU-R BT2020 for 10-bit system",
            "BT2020_12: ITU-R BT2020 for 12-bit system",
            "SMPTE2084: SMPTE ST 2084 for 10-, 12-, 14- and 16-bit systems",
            "SMPTE428:  SMPTE ST 428-1",
            "ARIB_STD_B67:  ARIB STD-B67, known as Hybrid log-gamma",
            "NB: Not part of ABI",
        };
        std::cout << "Color Transfer Characteristic: "    << ColorTransferCharacteristic[colorTransferCharacteristics] << std::endl;

        static const char* ColorSpace[] = {
            "RGB:   order of coefficients is actually GBR, also IEC 61966-2-1 (sRGB)",
            "BT709:   also ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / SMPTE RP177 Annex B",
            "UNSPECIFIED",
            "RESERVED",
            "FCC:  FCC Title 47 Code of Federal Regulations 73.682 (a)(20)",
            "BT470BG:  also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM / IEC 61966-2-4 xvYCC601",
            "SMPTE170M:  also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC",
            "SMPTE240M:  functionally identical to above",
            "YCGCO:  Used by Dirac / VC-2 and H.264 FRext, see ITU-T SG16",
            "BT2020_NCL:  ITU-R BT2020 non-constant luminance system",
            "BT2020_CL:  ITU-R BT2020 constant luminance system",
            "SMPTE2085:  SMPTE 2085, Y'D'zD'x",
            "CHROMA_DERIVED_NCL:  Chromaticity-derived non-constant luminance system",
            "CHROMA_DERIVED_CL:  Chromaticity-derived constant luminance system",
            "ICTCP:  ITU-R BT.2100-0, ICtCp",
            "NB:  Not part of ABI",
        };
        std::cout << "Color Space: "    << ColorSpace[colorSpace] << std::endl;

        static const char* ChromaLocation[] = {
            "UNSPECIFIED",
            "LEFT: MPEG-2/4 4:2:0, H.264 default for 4:2:0",
            "CENTER: MPEG-1 4:2:0, JPEG 4:2:0, H.263 4:2:0",
            "TOPLEFT: ITU-R 601, SMPTE 274M 296M S314M(DV 4:1:1), mpeg2 4:2:2",
            "TOP",
            "BOTTOMLEFT",
            "BOTTOM",
            "NB:Not part of ABI",
        };
        std::cout << "Chroma Location: "    << ChromaLocation[chromaLocation] << std::endl;
    }

private:
    AVFormatContext *fmtc = nullptr;
    AVIOContext *avioc = nullptr;
    AVPacket *pPkt, *pktFiltered;
    AVBSFContext *bsfc = nullptr;

    int videoStream;
    bool isStreamDemuxer;
    AVCodecID videoCodec;
    int codedWidth, codedHeight, codedLumaBitDepth, codedChromaBitDepth;

    AVPixelFormat format;
    /**
     * Codec-specific bitstream restrictions that the stream conforms to.
     */
    int profile;
    int level;

    /**
     * Video only. The aspect ratio (width / height) which a single pixel
     * should have when displayed.
     *
     * When the aspect ratio is unknown / undefined, the numerator should be
     * set to 0 (the denominator may have any value).
     */
    AVRational sample_aspect_ratio;

    /**
     * Video only. The order of the fields in interlaced video.
     */
    enum AVFieldOrder                  field_order;

    /**
     * Video only. Additional colorspace characteristics.
     */
    enum AVColorRange                  colorRange;
    enum AVColorPrimaries              colorPrimaries;
    enum AVColorTransferCharacteristic colorTransferCharacteristics;
    enum AVColorSpace                  colorSpace;
    enum AVChromaLocation              chromaLocation;

};

VkResult FFmpegDemuxerCreate(const char *pFilePath,
                             VkVideoCodecOperationFlagBitsKHR codecType,
                             bool requiresStreamDemuxing,
                             int32_t defaultWidth,
                             int32_t defaultHeight,
                             int32_t defaultBitDepth,
                             VkSharedBaseObj<VideoStreamDemuxer>& videoStreamDemuxer)
{
    VkSharedBaseObj<FFmpegDemuxer> ffmpegDemuxer;
    VkResult result = FFmpegDemuxer::Create(pFilePath,
                                            codecType,
                                            requiresStreamDemuxing,
                                            defaultWidth,
                                            defaultHeight,
                                            defaultBitDepth,
                                            ffmpegDemuxer);
    if (result == VK_SUCCESS) {
        videoStreamDemuxer = ffmpegDemuxer;
    }

    return result;
}
