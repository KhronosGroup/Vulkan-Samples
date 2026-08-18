/*
 * Copyright 2021 NVIDIA Corporation.
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

#ifndef _VULKANVIDEOPARSER_IF_H_
#define _VULKANVIDEOPARSER_IF_H_

#include "vk_video/vulkan_video_codecs_common.h"

#include "vkvideo_parser/PictureBufferBase.h"
#include "VkCodecUtils/VkVideoRefCountBase.h"
#include "vkvideo_parser/StdVideoPictureParametersSet.h"
#include "VkCodecUtils/VulkanBitstreamBuffer.h"

static const uint32_t NV_VULKAN_VIDEO_PARSER_API_VERSION = VK_MAKE_VIDEO_STD_VERSION(0, 9, 9);

typedef uint32_t FrameRate;  // Packed 18-bit numerator & 14-bit denominator

// Definitions for video_format
enum {
    VideoFormatComponent = 0,
    VideoFormatPAL,
    VideoFormatNTSC,
    VideoFormatSECAM,
    VideoFormatMAC,
    VideoFormatUnspecified,
    VideoFormatReserved6,
    VideoFormatReserved7
};

// Definitions for color_primaries
enum {
    ColorPrimariesForbidden = 0,
    ColorPrimariesBT709 = 1,
    ColorPrimariesUnspecified = 2,
    ColorPrimariesReserved = 3,
    ColorPrimariesBT470M = 4,
    ColorPrimariesBT470BG = 5,
    ColorPrimariesSMPTE170M = 6,  // Also, ITU-R BT.601
    ColorPrimariesSMPTE240M = 7,
    ColorPrimariesGenericFilm = 8,
    ColorPrimariesBT2020 = 9,
    // below are defined in AOM standard
    ColorPrimariesXYZ = 10,        /**< SMPTE 428 (CIE 1921 XYZ) */
    ColorPrimariesSMPTE431 = 11,   /**< SMPTE RP 431-2 */
    ColorPrimariesSMPTE432 = 12,   /**< SMPTE EG 432-1  */
    ColorPrimariesRESERVED13 = 13, /**< For future use (values 13 - 21)  */
    ColorPrimariesEBU3213 = 22,    /**< EBU Tech. 3213-E  */
    ColorPrimariesRESERVED23 = 23  /**< For future use (values 23 - 255)  */
};

// Definitions for transfer_characteristics
enum {
    TransferCharacteristicsForbidden = 0,
    TransferCharacteristicsBT709 = 1,
    TransferCharacteristicsUnspecified = 2,
    TransferCharacteristicsReserved = 3,
    TransferCharacteristicsBT470M = 4,
    TransferCharacteristicsBT470BG = 5,
    TransferCharacteristicsSMPTE170M = 6,
    TransferCharacteristicsSMPTE240M = 7,
    TransferCharacteristicsLinear = 8,
    TransferCharacteristicsLog100 = 9,
    TransferCharacteristicsLog316 = 10,
    TransferCharacteristicsIEC61966_2_4 = 11,
    TransferCharacteristicsBT1361 = 12,
    TransferCharacteristicsIEC61966_2_1 = 13,
    TransferCharacteristicsBT2020 = 14,
    TransferCharacteristicsBT2020_2 = 15,
    TransferCharacteristicsST2084 = 16,
    TransferCharacteristicsST428_1 = 17,
    // below are defined in AOM standard
    TransferCharacteristicsHLG = 18,       /**< BT.2100 HLG, ARIB STD-B67 */
    TransferCharacteristicsRESERVED19 = 19 /**< For future use (values 19-255) */
};

// Definitions for matrix_coefficients
enum {
    MatrixCoefficientsForbidden = 0,
    MatrixCoefficientsBT709 = 1,
    MatrixCoefficientsUnspecified = 2,
    MatrixCoefficientsReserved = 3,
    MatrixCoefficientsFCC = 4,
    MatrixCoefficientsBT470BG = 5,
    MatrixCoefficientsSMPTE170M = 6,
    MatrixCoefficientsSMPTE240M = 7,
    MatrixCoefficientsYCgCo = 8,
    MatrixCoefficientsBT2020_NCL = 9,  // Non-constant luminance
    MatrixCoefficientsBT2020_CL = 10,  // Constant luminance
    // below are defined in AOM standard
    MatrixCoefficientsSMPTE2085 = 11,   /**< SMPTE ST 2085 YDzDx */
    MatrixCoefficientsCHROMAT_NCL = 12, /**< Chromaticity-derived non-constant luminance */
    MatrixCoefficientsCHROMAT_CL = 13,  /**< Chromaticity-derived constant luminance */
    MatrixCoefficientsICTCP = 14,       /**< BT.2100 ICtCp */
    MatrixCoefficientsRESERVED15 = 15
};

// Maximum raw sequence header length (all codecs)
#define VK_MAX_SEQ_HDR_LEN (1024)  // 1024 bytes

typedef struct VkParserH264DpbEntry {
    VkPicIf* pPicBuf;      // ptr to reference frame
    int32_t FrameIdx;      // frame_num(short-term) or LongTermFrameIdx(long-term)
    int32_t is_long_term;  // 0=short term reference, 1=long term reference
    int32_t not_existing;  // non-existing reference frame (corresponding PicIdx
                           // should be set to -1)
    int32_t used_for_reference;  // 0=unused, 1=top_field, 2=bottom_field,
                                 // 3=both_fields
    int32_t FieldOrderCnt[2];  // field order count of top and bottom fields
} VkParserH264DpbEntry;

typedef struct VkParserH264PictureData {
    // SPS
    const StdVideoPictureParametersSet* pStdSps;
    // PPS
    const StdVideoPictureParametersSet* pStdPps;
    uint8_t pic_parameter_set_id;  // PPS ID
    uint8_t seq_parameter_set_id;  // SPS ID
    int32_t num_ref_idx_l0_active_minus1;
    int32_t num_ref_idx_l1_active_minus1;
    int32_t weighted_pred_flag;
    int32_t weighted_bipred_idc;
    int32_t pic_init_qp_minus26;
    int32_t redundant_pic_cnt_present_flag;
    uint8_t deblocking_filter_control_present_flag;
    uint8_t transform_8x8_mode_flag;
    uint8_t MbaffFrameFlag;
    uint8_t constrained_intra_pred_flag;
    uint8_t entropy_coding_mode_flag;
    uint8_t pic_order_present_flag;
    int8_t chroma_qp_index_offset;
    int8_t second_chroma_qp_index_offset;
    int32_t frame_num;
    int32_t CurrFieldOrderCnt[2];
    uint8_t fmo_aso_enable;
    uint8_t num_slice_groups_minus1;
    uint8_t slice_group_map_type;
    int8_t pic_init_qs_minus26;
    uint32_t slice_group_change_rate_minus1;
    // DPB
    VkParserH264DpbEntry dpb[16 + 1];  // List of reference frames within the DPB

    // Quantization Matrices (raster-order)
    union {
        // MVC extension
        struct {
            int32_t num_views_minus1;
            int32_t view_id;
            uint8_t inter_view_flag;
            uint8_t num_inter_view_refs_l0;
            uint8_t num_inter_view_refs_l1;
            uint8_t MVCReserved8Bits;
            int32_t InterViewRefsL0[16];
            int32_t InterViewRefsL1[16];
        } mvcext;
        // SVC extension
        struct {
            uint8_t profile_idc;
            uint8_t level_idc;
            uint8_t DQId;
            uint8_t DQIdMax;
            uint8_t disable_inter_layer_deblocking_filter_idc;
            uint8_t ref_layer_chroma_phase_y_plus1;
            int8_t inter_layer_slice_alpha_c0_offset_div2;
            int8_t inter_layer_slice_beta_offset_div2;
            uint16_t DPBEntryValidFlag;

            union {
                struct {
                    uint8_t inter_layer_deblocking_filter_control_present_flag : 1;
                    uint8_t extended_spatial_scalability_idc : 2;
                    uint8_t adaptive_tcoeff_level_prediction_flag : 1;
                    uint8_t slice_header_restriction_flag : 1;
                    uint8_t chroma_phase_x_plus1_flag : 1;
                    uint8_t chroma_phase_y_plus1 : 2;
                    uint8_t tcoeff_level_prediction_flag : 1;
                    uint8_t constrained_intra_resampling_flag : 1;
                    uint8_t ref_layer_chroma_phase_x_plus1_flag : 1;
                    uint8_t store_ref_base_pic_flag : 1;
                    uint8_t Reserved : 4;
                } f;
                uint8_t ucBitFields[2];
            };

            union {
                int16_t seq_scaled_ref_layer_left_offset;
                int16_t scaled_ref_layer_left_offset;
            };
            union {
                int16_t seq_scaled_ref_layer_top_offset;
                int16_t scaled_ref_layer_top_offset;
            };
            union {
                int16_t seq_scaled_ref_layer_right_offset;
                int16_t scaled_ref_layer_right_offset;
            };
            union {
                int16_t seq_scaled_ref_layer_bottom_offset;
                int16_t scaled_ref_layer_bottom_offset;
            };
        } svcext;
    };
} VkParserH264PictureData;

typedef struct VkParserHevcPictureData {
    // VPS
    const StdVideoPictureParametersSet* pStdVps;
    // SPS
    const StdVideoPictureParametersSet* pStdSps;
    // PPS
    const StdVideoPictureParametersSet* pStdPps;

    uint8_t pic_parameter_set_id;        // PPS ID
    uint8_t seq_parameter_set_id;        // SPS ID
    uint8_t vps_video_parameter_set_id;  // VPS ID

    uint8_t IrapPicFlag;
    uint8_t IdrPicFlag;
    uint8_t short_term_ref_pic_set_sps_flag;

    // RefPicSets
    int32_t NumBitsForShortTermRPSInSlice;
    int32_t NumDeltaPocsOfRefRpsIdx;
    int32_t NumPocTotalCurr;
    int32_t NumPocStCurrBefore;
    int32_t NumPocStCurrAfter;
    int32_t NumPocLtCurr;
    int32_t CurrPicOrderCntVal;
    VkPicIf* RefPics[16];
    int32_t PicOrderCntVal[16];
    uint8_t IsLongTerm[16];  // 1=long-term reference
    int8_t RefPicSetStCurrBefore[8];
    int8_t RefPicSetStCurrAfter[8];
    int8_t RefPicSetLtCurr[8];

    // various profile related
    // 0 = invalid, 1 = Main, 2 = Main10, 3 = still picture, 4 = Main 12,
    // 5 = MV-HEVC Main8
    uint8_t ProfileLevel;
    uint8_t ColorPrimaries;  // ColorPrimariesBTXXXX enum
    uint8_t bit_depth_luma_minus8;
    uint8_t bit_depth_chroma_minus8;

    // MV-HEVC related fields
    uint8_t mv_hevc_enable;
    uint8_t nuh_layer_id;
    uint8_t default_ref_layers_active_flag;
    uint8_t NumDirectRefLayers;
    uint8_t max_one_active_ref_layer_flag;
    uint8_t poc_lsb_not_present_flag;
    uint8_t pad0[2];

    int32_t NumActiveRefLayerPics0;
    int32_t NumActiveRefLayerPics1;
    int8_t RefPicSetInterLayer0[8];
    int8_t RefPicSetInterLayer1[8];

} VkParserHevcPictureData;

typedef struct VkParserVp9PictureData {
    uint32_t width;
    uint32_t height;

    // Frame Indexes
    VkPicIf* pLastRef;
    VkPicIf* pGoldenRef;
    VkPicIf* pAltRef;

    uint32_t keyFrame;
    uint32_t version;
    uint32_t showFrame;
    uint32_t errorResilient;
    uint32_t bit_depth_minus8;
    uint32_t colorSpace;
    uint32_t subsamplingX;
    uint32_t subsamplingY;
    uint32_t activeRefIdx[3];
    uint32_t intraOnly;
    uint32_t resetFrameContext;
    uint32_t frameParallelDecoding;
    uint32_t refreshFrameFlags;
    uint8_t refFrameSignBias[4];
    uint32_t frameContextIdx;
    uint32_t allow_high_precision_mv;
    uint32_t mcomp_filter_type;
    uint32_t loopFilterLevel;
    uint32_t loopFilterSharpness;
    uint32_t log2_tile_columns;
    uint32_t log2_tile_rows;
    int32_t mbRefLfDelta[4];
    int32_t mbModeLfDelta[2];
    int32_t segmentMapTemporalUpdate;
    uint8_t segmentFeatureEnable[8][4];
    uint8_t mb_segment_tree_probs[7];
    uint8_t segment_pred_probs[3];
    int16_t segmentFeatureData[8][4];
    uint32_t scaledWidth;
    uint32_t scaledHeight;
    uint32_t scalingActive;
    uint32_t segmentEnabled;
    uint32_t prevIsKeyFrame;
    uint32_t PrevShowFrame;
    uint32_t modeRefLfEnabled;
    int32_t qpYAc;
    int32_t qpYDc;
    int32_t qpChDc;
    int32_t qpChAc;
    uint32_t segmentMapUpdate;
    uint32_t segmentFeatureMode;
    uint32_t refreshEntropyProbs;
    uint32_t frameTagSize;
    uint32_t offsetToDctParts;
} VkParserVp9PictureData;

struct VkParserAv1PictureData {
    // The picture info structure is mostly pointing at other
    // structures defining the coding tool parameters. Those
    // additional structures live below this one.
    VkVideoDecodeAV1PictureInfoKHR khr_info;
    // --- The following fields are referred to by pointer from pKHR  ---
    StdVideoDecodeAV1PictureInfo std_info;
    uint8_t SkipModeFrame[STD_VIDEO_AV1_SKIP_MODE_FRAMES];
    uint8_t OrderHints[STD_VIDEO_AV1_NUM_REF_FRAMES];
    uint32_t expectedFrameId[STD_VIDEO_AV1_NUM_REF_FRAMES];

    StdVideoAV1TileInfo tileInfo;
    // --- The following fields are referred to by pointer from tileInfo ---
    uint16_t MiColStarts[64];
    uint16_t MiRowStarts[64];
    uint16_t width_in_sbs_minus_1[64];
    uint16_t height_in_sbs_minus_1[64];
    // --- End of tileInfo data ---

    StdVideoAV1Quantization quantization;
    StdVideoAV1Segmentation segmentation;
    StdVideoAV1LoopFilter loopFilter;
    StdVideoAV1CDEF CDEF;
    StdVideoAV1LoopRestoration loopRestoration;
    StdVideoAV1GlobalMotion globalMotion;
    StdVideoAV1FilmGrain filmGrain;
    uint32_t tileOffsets[64];  // TODO: Hack until the interfaces get cleaned up (all the cached parameters should be ref-counted
                               // etc, AV1_MAX_COLS*AV1_MAX_ROWS is larger than 64, switch to dynamic structure. Cheat while the CTS
                               // uses simpler bitstreams)
    uint32_t tileSizes[64];
    // --- End of pKHR data ---

    const StdVideoPictureParametersSet* pStdSps;
    bool needsSessionReset;  // true: new session detected
    bool showFrame;

    uint8_t ref_frame_idx[STD_VIDEO_AV1_REFS_PER_FRAME];
    int32_t pic_idx[STD_VIDEO_AV1_NUM_REF_FRAMES];

    VkVideoDecodeAV1DpbSlotInfoKHR setupSlot;
    StdVideoDecodeAV1ReferenceInfo setupSlotInfo;
    VkVideoDecodeAV1DpbSlotInfoKHR dpbSlots[STD_VIDEO_AV1_NUM_REF_FRAMES];
    StdVideoDecodeAV1ReferenceInfo dpbSlotInfos[STD_VIDEO_AV1_NUM_REF_FRAMES];

    uint32_t upscaled_width;
    uint32_t frame_width;
    uint32_t frame_height;
};

typedef struct VkParserPictureData {
    int32_t PicWidthInMbs;            // Coded Frame Size
    int32_t FrameHeightInMbs;         // Coded Frame Height
    VkPicIf* pCurrPic;                // Current picture (output)
    uint32_t field_pic_flag : 1;      // 0=frame picture, 1=field picture
    uint32_t bottom_field_flag : 1;   // 0=top field, 1=bottom field (ignored if
                                      // field_pic_flag=0)
    uint32_t second_field : 1;        // Second field of a complementary field pair
    uint32_t progressive_frame : 1;   // Frame is progressive
    uint32_t top_field_first : 1;     // Frame pictures only
    uint32_t repeat_first_field : 3;  // For 3:2 pulldown (number of additional fields,
                                      // 2 = frame doubling, 4 = frame tripling)
    uint32_t ref_pic_flag : 1;        // Frame is a reference frame
    uint32_t intra_pic_flag : 1;      // Frame is entirely intra coded (no temporal
                                      // dependencies)
    int32_t chroma_format;            // Chroma Format (should match sequence info)
    int32_t picture_order_count;      // picture order count (if known)

    // Codec-specific data
    union {
        VkParserH264PictureData h264;
        VkParserHevcPictureData hevc;
        VkParserAv1PictureData av1;
        VkParserVp9PictureData vp9;
    } CodecSpecific;

    // Dpb Id for the setup (current picture to be reference) slot
    int8_t current_dpb_id;
    // Bitstream data
    uint32_t firstSliceIndex;
    uint32_t numSlices;
    size_t bitstreamDataOffset;                            // bitstream data offset in bitstreamData buffer
    size_t bitstreamDataLen;                               // Number of bytes in bitstream data buffer
    VkSharedBaseObj<VulkanBitstreamBuffer> bitstreamData;  // bitstream data for this picture (slice-layer)
} VkParserPictureData;

// Packet input for parsing
typedef struct VkParserBitstreamPacket {
    const uint8_t* pByteStream;    // Ptr to byte stream data decode/display event
    size_t nDataLength;            // Data length for this packet
    int64_t llPTS;                 // Presentation Time Stamp for this packet (clock rate specified at initialization)
    uint32_t bEOS : 1;             // true if this is an End-Of-Stream packet (flush everything)
    uint32_t bPTSValid : 1;        // true if llPTS is valid (also used to detect frame boundaries for VC1 SP/MP)
    uint32_t bDiscontinuity : 1;   // true if DecMFT is signalling a discontinuity
    uint32_t bPartialParsing : 1;  // 0: parse entire packet, 1: parse until next
    uint32_t bEOP : 1;             // true if the packet in pByteStream is exactly one frame
    uint8_t* pbSideData;           // Auxiliary encryption information
    int32_t nSideDataLength;       // Auxiliary encrypton information length
} VkParserBitstreamPacket;

typedef struct VkParserOperatingPointInfo {
    VkVideoCodecOperationFlagBitsKHR eCodec;
    union {
        struct {
            uint8_t operating_points_cnt;
            uint8_t reserved24_bits[3];
            uint16_t operating_points_idc[32];
        } av1;
        uint8_t CodecReserved[1024];
    };
} VkParserOperatingPointInfo;

// Sequence information
typedef struct VkParserSequenceInfo {
    VkVideoCodecOperationFlagBitsKHR eCodec;  // Compression Standard
    bool isSVC;                               // h.264 SVC
    FrameRate frameRate;                      // Frame Rate stored in the bitstream
    int32_t bProgSeq;                         // Progressive Sequence
    int32_t nDisplayWidth;                    // Displayed Horizontal Size
    int32_t nDisplayHeight;                   // Displayed Vertical Size
    int32_t nCodedWidth;                      // Coded Picture Width
    int32_t nCodedHeight;                     // Coded Picture Height
    int32_t nMaxWidth;                        // Max width within sequence
    int32_t nMaxHeight;                       // Max height within sequence
    uint8_t nChromaFormat;                    // Chroma Format (0=4:0:0, 1=4:2:0, 2=4:2:2, 3=4:4:4)
    uint8_t uBitDepthLumaMinus8;              // Luma bit depth (0=8bit)
    uint8_t uBitDepthChromaMinus8;            // Chroma bit depth (0=8bit)
    uint8_t uVideoFullRange;                  // 0=16-235, 1=0-255
    int32_t lBitrate;                         // Video bitrate (bps)
    int32_t lDARWidth,
        lDARHeight;                                  // Display Aspect Ratio = lDARWidth : lDARHeight
    int32_t lVideoFormat;                            // Video Format (VideoFormatXXX)
    int32_t lColorPrimaries;                         // Colour Primaries (ColorPrimariesXXX)
    int32_t lTransferCharacteristics;                // Transfer Characteristics
    int32_t lMatrixCoefficients;                     // Matrix Coefficients
    int32_t cbSequenceHeader;                        // Number of bytes in SequenceHeaderData
    int32_t nMinNumDpbSlots;                         // Minimum number of DPB slots for correct decoding
    int32_t nMinNumDecodeSurfaces;                   // Minimum number of decode surfaces for correct decoding
    uint8_t SequenceHeaderData[VK_MAX_SEQ_HDR_LEN];  // Raw sequence header data
                                                     // (codec-specific)
    uint32_t codecProfile;
    bool hasFilmGrain;  // AV1 specific filmgrain.
} VkParserSequenceInfo;

enum {
    VK_PARSER_CAPS_MVC = 0x01,
    VK_PARSER_CAPS_SVC = 0x02,
};

typedef struct VkParserDisplayMasteringInfo {
    // H.265 Annex D.2.27
    uint16_t display_primaries_x[3];
    uint16_t display_primaries_y[3];
    uint16_t white_point_x;
    uint16_t white_point_y;
    uint32_t max_display_mastering_luminance;
    uint32_t min_display_mastering_luminance;
} VkParserDisplayMasteringInfo;

// Interface to allow decoder to communicate with the client
class VkParserVideoDecodeClient {
   public:
    virtual int32_t BeginSequence(const VkParserSequenceInfo* pnvsi) = 0;  // Returns max number of reference frames
    // (always at least 2 for MPEG-2)
    virtual bool AllocPictureBuffer(VkPicIf** ppPicBuf) = 0;                // Returns a new VkPicIf interface
    virtual bool DecodePicture(VkParserPictureData* pParserPictureData) = 0;  // Called when a picture is
    // ready to be decoded
    virtual bool UpdatePictureParameters(VkSharedBaseObj<StdVideoPictureParametersSet>& pictureParametersObject,  // IN
                                         VkSharedBaseObj<VkVideoRefCountBase>& client)                            // OUT
        = 0;  // Called when a picture is
    // ready to be decoded
    virtual bool DisplayPicture(VkPicIf* pPicBuf,
                                int64_t llPTS) = 0;                        // Called when a picture is ready to be displayed
    virtual void UnhandledNALU(const uint8_t* pbData, size_t cbData) = 0;  // Called for custom NAL parsing (not required)
    virtual uint32_t GetDecodeCaps() { return 0; }                         // NVD_CAPS_XXX
    // called from sequence header of av1 scalable video streams
    virtual VkDeviceSize GetBitstreamBuffer(VkDeviceSize size, VkDeviceSize minBitstreamBufferOffsetAlignment,
                                            VkDeviceSize minBitstreamBufferSizeAlignment, const uint8_t* pInitializeBufferMemory,
                                            VkDeviceSize initializeBufferMemorySize,
                                            VkSharedBaseObj<VulkanBitstreamBuffer>& bitstreamBuffer) = 0;

   protected:
    virtual ~VkParserVideoDecodeClient() {}
};

// Initialization parameters for decoder class
typedef struct VkParserInitDecodeParameters {
    uint32_t interfaceVersion;
    VkParserVideoDecodeClient* pClient;  // should always be present if using parsing functionality
    uint32_t defaultMinBufferSize;
    uint32_t bufferOffsetAlignment;
    uint32_t bufferSizeAlignment;
    uint64_t referenceClockRate;  // ticks per second of PTS clock
                                  // (0 = default = 10000000 = 10Mhz)
    int32_t errorThreshold;                  // threshold for deciding to bypass of picture
                                             // (0 = do not decode, 100 = always decode)
    VkParserSequenceInfo* pExternalSeqInfo;  // optional external sequence header
                                             // data from system layer

    // If set, Picture Parameters are going to be provided via UpdatePictureParameters callback
    bool outOfBandPictureParameters;
} VkParserInitDecodeParameters;

// High-level interface to video decoder (Note that parsing and decoding
// functionality are decoupled from each other)
class VulkanVideoDecodeParser : public virtual VkVideoRefCountBase {
   public:
    virtual VkResult Initialize(const VkParserInitDecodeParameters* pParserPictureData) = 0;
    virtual bool ParseByteStream(const VkParserBitstreamPacket* pck, size_t* pParsedBytes = NULL) = 0;
    virtual bool GetDisplayMasteringInfo(VkParserDisplayMasteringInfo* pdisp) = 0;
};

/////////////////////////////////////////////////////////////////////////////////////////

#endif  // _VULKANVIDEOPARSER_IF_H_
