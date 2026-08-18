/*
 * Copyright 2023 NVIDIA Corporation.
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

#ifndef _VULKANAV1DECODER_H_
#define _VULKANAV1DECODER_H_

#include "VulkanVideoDecoder.h"
#include "vk_video/vulkan_video_codec_av1std.h"
#include "vk_video/vulkan_video_codec_av1std_decode.h"

#include <array>
#include <vector>

#define BUFFER_POOL_MAX_SIZE 10
#define ALIGN(value, n) (((value) + (n)-1) & ~((n)-1))
#define CLAMP(value, low, high) ((value) < (low) ? (low) : ((value) > (high) ? (high) : (value)))

#define BIT32_MAX (0xffffffff)
#define SINT16_MAX (0x7fff)
#define SINT16_MIN (-0x7fff - 1)

#define MAX_NUM_TEMPORAL_LAYERS 8
#define MAX_NUM_SPATIAL_LAYERS 4
#define MAX_NUM_OPERATING_POINTS MAX_NUM_TEMPORAL_LAYERS* MAX_NUM_SPATIAL_LAYERS

#define LEVEL_MAJOR_BITS 3
#define LEVEL_MINOR_BITS 2
#define LEVEL_BITS (LEVEL_MAJOR_BITS + LEVEL_MINOR_BITS)

#define LEVEL_MAJOR_MIN 2
#define LEVEL_MAJOR_MAX ((1 << LEVEL_MAJOR_BITS) - 1 + LEVEL_MAJOR_MIN)
#define LEVEL_MINOR_MIN 0
#define LEVEL_MINOR_MAX ((1 << LEVEL_MINOR_BITS) - 1)
#define OP_POINTS_CNT_MINUS_1_BITS 5
#define OP_POINTS_IDC_BITS 12

#define REF_FRAMES_BITS 3

#define GM_GLOBAL_MODELS_PER_FRAME 7
#define SUPERRES_NUM 8         // numerator for upscaling ratio
#define SUPERRES_DENOM_MIN 9   // smallest denominator for upscaling ratio
#define SUPERRES_DENOM_BITS 3  // number pf bits sent to specify denominator of upscaling ratio

// The minimum tile width or height is fixed at one superblock
#define MAX_TILE_WIDTH \
    (STD_VIDEO_AV1_MAX_TILE_COLS * STD_VIDEO_AV1_MAX_TILE_ROWS)  // maximum widht of a tile in units of luma samples
#define MAX_TILE_AREA (MAX_TILE_WIDTH * 2304)                    // maximum area of a tile in units of luma samples
#define MAX_TILES 512                                            // maximum number of tiles
#define MIN_TILE_SIZE_BYTES 1

// OBU types
typedef enum _AV1_OBU_TYPE {
    AV1_OBU_SEQUENCE_HEADER = 1,
    AV1_OBU_TEMPORAL_DELIMITER = 2,
    AV1_OBU_FRAME_HEADER = 3,
    AV1_OBU_TILE_GROUP = 4,
    AV1_OBU_METADATA = 5,
    AV1_OBU_FRAME = 6,
    AV1_OBU_REDUNDANT_FRAME_HEADER = 7,
    AV1_OBU_TILE_LIST = 8,
    AV1_OBU_PADDING = 15,
} AV1_OBU_TYPE;

// The Vulkan spec does not have definition for the xform types. Should it?
typedef enum _AV1_TRANSFORMATION_TYPE {
    IDENTITY = 0,     // identity transformation, 0-parameter
    TRANSLATION = 1,  // translational motion 2-parameter
    ROTZOOM = 2,      // simplified affine with rotation + zoom only, 4-parameter
    AFFINE = 3,       // affine, 6-parameter
    TRANS_TYPES,
} AV1_TRANSFORMATION_TYPE;

// The order of values in the wmmat matrix below is best described
// by the homography:
//      [x'     (m2 m3 m0   [x
//  z .  y'  =   m4 m5 m1 *  y
//       1]      m6 m7 1)    1]
struct AV1WarpedMotionParams {
    AV1_TRANSFORMATION_TYPE wmtype;
    int32_t wmmat[6];
    int8_t invalid;
};

#define WARPEDMODEL_PREC_BITS 16
static const AV1WarpedMotionParams default_warp_params = {
    IDENTITY,
    {0, 0, (1 << WARPEDMODEL_PREC_BITS), 0, 0, (1 << WARPEDMODEL_PREC_BITS)},
    0,
};

typedef struct _AV1ObuHeader {
    uint32_t header_size;
    uint32_t payload_size;
    AV1_OBU_TYPE type;
    bool has_size_field;
    bool has_extension;
    uint8_t reserved[2];
    // Below feilds exist if has_extension is set
    int32_t temporal_id;
    int32_t spatial_id;
} AV1ObuHeader;

// Sequence header structure.
struct av1_seq_param_s : public StdVideoPictureParametersSet, public StdVideoAV1SequenceHeader {
    static const char* m_refClassId;

    // Operating point info.
    int32_t operating_points_cnt_minus_1{};
    int32_t operating_point_idc[MAX_NUM_OPERATING_POINTS]{};  // specifies which spatial and temporal layers should be decoded
    bool display_model_info_present{};
    bool decoder_model_info_present{};
    StdVideoAV1Level level[MAX_NUM_OPERATING_POINTS]{};  // resolution, bitrate etc
    uint8_t tier[MAX_NUM_OPERATING_POINTS]{};

    StdVideoAV1ColorConfig color_config;
    StdVideoAV1TimingInfo timing_info;

    VkSharedBaseObj<VkVideoRefCountBase> client;

    bool isDifferentFrom(const av1_seq_param_s* other) {
#define CHECK_PTR(f, t)                                        \
    do {                                                       \
        if (memcmp(this->f, other->f, sizeof(t))) return true; \
    } while (0)

#define CHECK_FIELD(f)                        \
    do {                                      \
        if (this->f != other->f) return true; \
    } while (0)

        if (memcmp(&this->flags, &other->flags, sizeof(StdVideoAV1SequenceHeaderFlags))) return true;

        CHECK_FIELD(seq_profile);
        CHECK_FIELD(frame_width_bits_minus_1);
        CHECK_FIELD(frame_height_bits_minus_1);
        CHECK_FIELD(max_frame_width_minus_1);
        CHECK_FIELD(max_frame_height_minus_1);
        CHECK_FIELD(delta_frame_id_length_minus_2);
        CHECK_FIELD(additional_frame_id_length_minus_1);
        CHECK_FIELD(order_hint_bits_minus_1);
        CHECK_FIELD(seq_force_integer_mv);
        CHECK_FIELD(seq_force_screen_content_tools);
        CHECK_PTR(pColorConfig, StdVideoAV1ColorConfig);
        CHECK_PTR(pTimingInfo, StdVideoAV1TimingInfo);

        return false;
    }

    int32_t GetVpsId(bool& isVps) const override {
        isVps = false;
        return -1;
    }

    int32_t GetSpsId(bool& isSps) const override {
        isSps = false;
        return -1;
    }

    int32_t GetPpsId(bool& isPps) const override {
        isPps = false;
        return -1;
    }

    const StdVideoAV1SequenceHeader* GetStdAV1Sps() const override { return this; }

    const char* GetRefClassId() const override { return m_refClassId; }

    uint64_t SetSequenceCount(uint64_t updateSequenceCount) {
        assert(updateSequenceCount <= std::numeric_limits<uint32_t>::max());
        m_updateSequenceCount = (uint32_t)updateSequenceCount;
        return m_updateSequenceCount;
    }

    bool GetClientObject(VkSharedBaseObj<VkVideoRefCountBase>& clientObject) const override {
        clientObject = client;
        return !!clientObject;
    }

    explicit av1_seq_param_s(uint64_t updateSequenceCount)
        : StdVideoPictureParametersSet(TYPE_AV1_SPS, AV1_SPS_TYPE, m_refClassId, updateSequenceCount),
          StdVideoAV1SequenceHeader()

    {
        seq_profile = STD_VIDEO_AV1_PROFILE_MAIN;
    }

    ~av1_seq_param_s() override { client = nullptr; }

    static VkResult Create(uint64_t updateSequenceCount, VkSharedBaseObj<av1_seq_param_s>& spsAV1PictureParametersSet) {
        VkSharedBaseObj<av1_seq_param_s> av1PictureParametersSet(new av1_seq_param_s(updateSequenceCount));
        if (av1PictureParametersSet) {
            spsAV1PictureParametersSet = av1PictureParametersSet;
            return VK_SUCCESS;
        }
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
};

typedef struct _av1_timing_info_t {
    uint32_t num_units_in_display_tick;
    uint32_t time_scale;
    bool equal_picture_interval;
    uint32_t num_ticks_per_picture;
} av1_timing_info_t;

typedef struct _av1_dec_model_info {
    uint32_t num_units_in_decoding_tick;
    int32_t encoder_decoder_buffer_delay_length;
    int32_t buffer_removal_time_length;
    int32_t frame_presentation_time_length;
} av1_dec_model_info_t;

typedef struct _av1_dec_model_op_params {
    bool decoder_model_param_present;
    uint32_t bitrate;
    uint32_t buffer_size;
    int32_t cbr_flag;
    int32_t decoder_buffer_delay;
    int32_t encoder_buffer_delay;
    int32_t low_delay_mode_flag;
    int32_t display_model_param_present;
    int32_t initial_display_delay;
} av1_dec_model_op_params_t;

typedef struct _GlobalMotionParams {
    uint32_t wmtype;
    int32_t wmmat[6];
    int8_t invalid;
    int8_t reserved[3];
} GlobalMotionParams;

typedef struct _av1_ref_frames_s {
    VkPicIf* buffer;
    StdVideoAV1FrameType frame_type;
    StdVideoAV1FilmGrain film_grain_params;
    AV1WarpedMotionParams global_models[GM_GLOBAL_MODELS_PER_FRAME];
    int8_t lf_ref_delta[STD_VIDEO_AV1_NUM_REF_FRAMES];
    int8_t lf_mode_delta[2];
    bool showable_frame;
    struct {
        uint8_t FeatureEnabled[STD_VIDEO_AV1_MAX_SEGMENTS];
        int16_t FeatureData[STD_VIDEO_AV1_MAX_SEGMENTS][STD_VIDEO_AV1_SEG_LVL_MAX];
        int32_t last_active_id;
        uint8_t preskip_id;
        uint8_t reserved[3];
    } seg;

    // Temporary variables.
    uint32_t primary_ref_frame;  // if not 0 -- may not alloc a slot. Re-resolve this per frame per dpb index.
    uint32_t base_q_index;
    bool disable_frame_end_update_cdf;
    bool segmentation_enabled;

    int8_t RefFrameSignBias[STD_VIDEO_AV1_NUM_REF_FRAMES];
    uint8_t SavedOrderHints[STD_VIDEO_AV1_NUM_REF_FRAMES];
    uint8_t order_hint;
} av1_ref_frames_s;

// AV1 decoder class
class VulkanAV1Decoder : public VulkanVideoDecoder {
   protected:
    VkSharedBaseObj<av1_seq_param_s> m_sps;  // active sps
    VkParserAv1PictureData m_PicData;

    // common params
    int32_t temporal_id;
    int32_t spatial_id;
    bool m_bSPSReceived;
    bool m_bSPSChanged;
    bool m_obuAnnexB;
    uint8_t timing_info_present;
    av1_timing_info_t timing_info;
    av1_dec_model_info_t buffer_model;
    av1_dec_model_op_params_t op_params[MAX_NUM_OPERATING_POINTS + 1];
    uint32_t op_frame_timing[MAX_NUM_OPERATING_POINTS + 1];

    uint8_t delta_frame_id_length;
    uint8_t frame_id_length;
    uint8_t last_frame_type;
    uint8_t last_intra_only;
    uint8_t coded_lossless;
    uint8_t all_lossless : 1;

    // frame header
    uint16_t upscaled_width;
    uint16_t frame_width;
    uint16_t frame_height;
    int32_t render_width;
    int32_t render_height;

    uint32_t intra_only;
    int32_t showable_frame;
    int32_t last_show_frame;
    int32_t show_existing_frame;
    int32_t tu_presentation_delay;

    int32_t lossless[STD_VIDEO_AV1_MAX_SEGMENTS];

    uint8_t tile_size_bytes_minus_1;
    uint32_t log2_tile_cols;
    uint32_t log2_tile_rows;

    // global motion
    AV1WarpedMotionParams global_motions[GM_GLOBAL_MODELS_PER_FRAME];

    int32_t ref_frame_id[STD_VIDEO_AV1_NUM_REF_FRAMES];
    int32_t pic_idx[STD_VIDEO_AV1_NUM_REF_FRAMES];
    int32_t RefValid[STD_VIDEO_AV1_NUM_REF_FRAMES];
    int32_t ref_frame_idx[STD_VIDEO_AV1_REFS_PER_FRAME];

    // According to AV1 spec section - E.2. Decoder model definitions
    int32_t RefOrderHint[BUFFER_POOL_MAX_SIZE];
    av1_ref_frames_s m_pBuffers[BUFFER_POOL_MAX_SIZE];

    VkPicIf* m_pCurrPic;

    bool m_bOutputAllLayers;
    int32_t m_OperatingPointIDCActive;
    int m_numOutFrames;
    VkPicIf* m_pOutFrame[MAX_NUM_SPATIAL_LAYERS];
    bool m_showableFrame[MAX_NUM_SPATIAL_LAYERS];

   public:
    VulkanAV1Decoder(VkVideoCodecOperationFlagBitsKHR std, bool annexB = false);
    virtual ~VulkanAV1Decoder();

    bool ParseByteStream(const VkParserBitstreamPacket* pck, size_t* pParsedBytes) override;

   protected:
    bool IsPictureBoundary(int32_t) override { return true; };
    int32_t ParseNalUnit() override { return NALU_UNKNOWN; };
    bool DecodePicture(VkParserPictureData*) { return false; };
    bool end_of_picture(uint32_t frameSize);
    void InitParser() override;
    bool BeginPicture(VkParserPictureData* pnvpd) override;
    void lEndPicture(VkPicIf* pDispPic, bool bEvict);
    bool ParseOneFrame(const uint8_t* pdatain, int32_t datasize, const VkParserBitstreamPacket* pck, int* pParsedBytes);
    void EndOfStream() override;

    uint32_t read_u16_le(const void* vmem) {
        uint32_t val;
        const uint8_t* mem = (const uint8_t*)vmem;

        val = mem[1] << 8;
        val |= mem[0];
        return val;
    }

    uint32_t read_u24_le(const void* vmem) {
        uint32_t val;
        const uint8_t* mem = (const uint8_t*)vmem;

        val = mem[2] << 16;
        val |= mem[1] << 8;
        val |= mem[0];
        return val;
    }

    uint32_t read_u32_le(const void* vmem) {
        uint32_t val;
        const uint8_t* mem = (const uint8_t*)vmem;

        val = ((uint32_t)mem[3]) << 24;
        val |= mem[2] << 16;
        val |= mem[1] << 8;
        val |= mem[0];
        return val;
    }

    size_t le(int n) {
        size_t t = 0;
        for (int i = 0; i < n; i++) {
            uint8_t byte = u(8);
            t += (byte << (i * 8));
        }
        return t;
    }

    size_t read_tile_group_size(const uint8_t* src, int size) {
        switch (size) {
            case 1:
                return src[0];
            case 2:
                return read_u16_le(src);
            case 3:
                return read_u24_le(src);
            case 4:
                return read_u32_le(src);
            default:
                assert(0 && "Invalid size");
                return (size_t)(-1);
        }
    }

    bool ParseOBUHeaderAndSize(const uint8_t* pData, uint32_t datasize, AV1ObuHeader* hdr);
    bool ReadObuSize(const uint8_t* pData, uint32_t datasize, uint32_t* obu_size, uint32_t* length_feild_size);
    bool ReadObuHeader(const uint8_t* pData, uint32_t datasize, AV1ObuHeader* hdr);

    bool ParseObuTemporalDelimiter();
    bool ParseObuSequenceHeader();
    bool ParseObuFrameHeader();
    bool ParseObuTileGroup(const AV1ObuHeader&);
    bool ReadFilmGrainParams();

    void ReadTimingInfoHeader();
    void ReadDecoderModelInfo();
    uint32_t ReadUvlc();
    void SetupFrameSize(int32_t frame_size_override_flag);
    int32_t SetupFrameSizeWithRefs();

    bool DecodeTileInfo();
    inline int32_t ReadSignedBits(uint32_t bits);
    inline int32_t ReadDeltaQ(uint32_t bits);
    uint32_t SwGetUniform(uint32_t max_value);

    void DecodeQuantizationData();
    void DecodeSegmentationData();
    void DecodeLoopFilterdata();
    void DecodeCDEFdata();
    void DecodeLoopRestorationData();
    void SetFrameRefs(int32_t last_frame_idx, int32_t gold_frame_idx);
    int32_t GetRelativeDist1(int32_t a, int32_t b);
    int32_t IsSkipModeAllowed();
    uint32_t DecodeGlobalMotionParams();
    int32_t ReadGlobalMotionParams(AV1WarpedMotionParams* params, const AV1WarpedMotionParams* ref_params, int32_t allow_hp);

    int16_t Read_signed_primitive_refsubexpfin(uint16_t n, uint16_t k, int16_t ref);
    uint16_t Read_primitive_refsubexpfin(uint16_t n, uint16_t k, uint16_t ref);
    uint16_t Read_primitive_subexpfin(uint16_t n, uint16_t k);
    uint16_t Read_primitive_quniform(uint16_t n);
    void UpdateFramePointers(VkPicIf* currentPicture);
    bool IsFrameIntra() {
        return (m_PicData.std_info.frame_type == STD_VIDEO_AV1_FRAME_TYPE_INTRA_ONLY ||
                m_PicData.std_info.frame_type == STD_VIDEO_AV1_FRAME_TYPE_KEY);
    }
    int32_t ChooseOperatingPoint();
    bool AddBuffertoOutputQueue(VkPicIf* pDispPic, bool bShowableFrame);
    void AddBuffertoDispQueue(VkPicIf* pDispPic);

    void CreatePrivateContext() override {}
    void FreeContext() override {}
    int GetRelativeDist(int a, int b);
};

#endif  // _VULKANAV1DECODER_H_
