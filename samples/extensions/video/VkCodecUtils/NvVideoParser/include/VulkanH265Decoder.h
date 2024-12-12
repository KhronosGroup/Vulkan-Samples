/*
* Copyright 2021 - 2023 NVIDIA Corporation.
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

#ifndef _VULKANH265DECODER_H_
#define _VULKANH265DECODER_H_

#include "vkvideo_parser/StdVideoPictureParametersSet.h"
#include "VulkanVideoDecoder.h"
#include "nvVulkanh265ScalingList.h"
#include "VulkanH26xDecoder.h"
#include <memory>

#define MAX_NUM_VPS                 16
#define MAX_NUM_SPS                 16
#define MAX_NUM_PPS                 64
#define MAX_NUM_SUB_LAYERS          7
#define MAX_NUM_STRPS               64
#define MAX_NUM_LTRP                32
#define MAX_NUM_STRPS_ENTRIES       16
#define MAX_NUM_REF_PICS            16
#define MAX_NUM_TILE_COLUMNS        20
#define MAX_NUM_TILE_ROWS           22
#define HEVC_DPB_SIZE               16

#define MAX_VPS_LAYERS              64
#define MAX_NUM_LAYER_IDS           64
#define MAX_VPS_LAYER_SETS          1024
#define MAX_NUM_SCALABILITY_TYPES   16
#define MAX_VPS_OP_SETS_PLUS1       1024
#define MAX_VPS_OUTPUTLAYER_SETS    1024
#define MAX_SUB_LAYERS              7

#define VK_H265_SPS_VUI_FIELD(pStdVui, nvSpsIn, name) pStdVui->name = nvSpsIn->vui.name
#define VK_H265_SPS_VUI_FLAG(pStdVui, nvSpsIn, name) pStdVui->flags.name = nvSpsIn->vui.name

enum profile_e
{
    INVALID = 0,
    MAIN = 1,
    MAIN10 = 2,
    MAINSTILLPIC = 3,
    MAIN12 = 4,
    MAINMVC = 5,
};

enum nal_unit_type_e
{
    NUT_TRAIL_N = 0,
    NUT_TRAIL_R = 1,
    NUT_TSA_N = 2,
    NUT_TSA_R = 3,
    NUT_STSA_N = 4,
    NUT_STSA_R = 5,
    NUT_RADL_N = 6,
    NUT_RADL_R = 7,
    NUT_RASL_N = 8,
    NUT_RASL_R = 9,
    NUT_BLA_W_LP = 16,
    NUT_BLA_W_RADL = 17,
    NUT_BLA_N_LP = 18,
    NUT_IDR_W_RADL = 19,
    NUT_IDR_N_LP = 20,
    NUT_CRA_NUT = 21,
    NUT_VPS_NUT = 32,
    NUT_SPS_NUT = 33,
    NUT_PPS_NUT = 34,
    NUT_AUD_NUT = 35,
    NUT_EOS_NUT = 36,
    NUT_EOB_NUT = 37,
    NUT_FD_NUT = 38,
    NUT_PREFIX_SEI_NUT = 39,
    NUT_SUFFIX_SEI_NUT = 40,
};

enum hevc_slice_type_e
{
    SLICE_TYPE_B = 0,
    SLICE_TYPE_P = 1,
    SLICE_TYPE_I = 2,
};

struct hevc_video_hrd_param_s : public StdVideoH265HrdParameters
{

    hevc_video_hrd_param_s()
    : StdVideoH265HrdParameters(),
      maxNumSubLayers(),
      stdSubLayerHrdParametersNal{},
      stdSubLayerHrdParametersVcl{}
    {

    }

    void CopyObject(const hevc_video_hrd_param_s& other) {

        StdVideoH265HrdParameters::operator=(other);

        maxNumSubLayers = other.maxNumSubLayers;

        if (other.flags.nal_hrd_parameters_present_flag) {
            for (uint32_t i = 0; i < other.maxNumSubLayers; i++) {
                stdSubLayerHrdParametersNal[i] = other.stdSubLayerHrdParametersNal[i];
            }
            pSubLayerHrdParametersNal = stdSubLayerHrdParametersNal;
        }

        if (other.flags.vcl_hrd_parameters_present_flag) {
            for (uint32_t i = 0; i < other.maxNumSubLayers; i++) {
                stdSubLayerHrdParametersVcl[i] = other.stdSubLayerHrdParametersVcl[i];
            }
            pSubLayerHrdParametersVcl = stdSubLayerHrdParametersVcl;
        }
    }

    hevc_video_hrd_param_s(const hevc_video_hrd_param_s& other) // copy constructor
    {
        CopyObject(other);
    }

    hevc_video_hrd_param_s& operator=(const hevc_video_hrd_param_s& rhs)
    {
        if (this != &rhs) {
            CopyObject(rhs);
        }
        return *this;
    }

    void Reset() {

        StdVideoH265HrdParameters::operator=(StdVideoH265HrdParameters());
        maxNumSubLayers = 0;
        memset(&stdSubLayerHrdParametersNal, 0x00, sizeof(stdSubLayerHrdParametersNal));
        memset(&stdSubLayerHrdParametersVcl, 0x00, sizeof(stdSubLayerHrdParametersVcl));
    }

    void BindSubLayers(uint32_t maxNumSubLayersMinus1)
    {
        if (maxNumSubLayersMinus1) {
            if (flags.nal_hrd_parameters_present_flag) {
                pSubLayerHrdParametersNal = stdSubLayerHrdParametersNal;
            }

            if (flags.vcl_hrd_parameters_present_flag) {
                pSubLayerHrdParametersVcl = stdSubLayerHrdParametersVcl;
            }
        }
        maxNumSubLayers = maxNumSubLayersMinus1 + 1;
    }

    uint32_t                          maxNumSubLayers;
    StdVideoH265SubLayerHrdParameters stdSubLayerHrdParametersNal[STD_VIDEO_H265_SUBLAYERS_LIST_SIZE];
    StdVideoH265SubLayerHrdParameters stdSubLayerHrdParametersVcl[STD_VIDEO_H265_SUBLAYERS_LIST_SIZE];
};

struct short_term_ref_pic_set_s
{
    // low-level
    uint8_t NumNegativePics;
    uint8_t NumPositivePics;
    uint8_t inter_ref_pic_set_prediction_flag;
    uint8_t delta_idx_minus1;
    uint8_t UsedByCurrPicS0[MAX_NUM_STRPS_ENTRIES];
    uint8_t UsedByCurrPicS1[MAX_NUM_STRPS_ENTRIES];
    int32_t DeltaPocS0[MAX_NUM_STRPS_ENTRIES];
    int32_t DeltaPocS1[MAX_NUM_STRPS_ENTRIES];
};

struct hevc_seq_param_s : public StdVideoPictureParametersSet, public StdVideoH265SequenceParameterSet
{
    static const char*                   m_refClassId;
    StdVideoH265ProfileTierLevel         stdProfileTierLevel;
    StdVideoH265DecPicBufMgr             stdDecPicBufMgr;
    StdVideoH265ScalingLists             stdScalingLists;
    StdVideoH265ShortTermRefPicSet       stdShortTermRefPicSet[STD_VIDEO_H265_MAX_SHORT_TERM_REF_PIC_SETS];
    StdVideoH265LongTermRefPicsSps       stdLongTermRefPicsSps;
    StdVideoH265SequenceParameterSetVui  stdVui;
    hevc_video_hrd_param_s               stdHrdParameters;

    uint8_t max_dec_pic_buffering;
    uint8_t max_num_reorder_pics;
    uint8_t sps_rep_format_idx;

    scaling_list_s sps_scaling_list;

    short_term_ref_pic_set_s strpss[MAX_NUM_STRPS];

    VkSharedBaseObj<VkVideoRefCountBase> client;

    virtual int32_t GetVpsId(bool& isVps) const {
        isVps = false;
        return sps_video_parameter_set_id;
    }

    virtual int32_t GetSpsId(bool& isSps) const {
        isSps = true;
        return sps_seq_parameter_set_id;
    }

    virtual int32_t GetPpsId(bool& isPps) const {
        isPps = false;
        return -1;
    }

    virtual const StdVideoH265SequenceParameterSet* GetStdH265Sps() const { return this; }

    operator StdVideoH265SequenceParameterSet*() { return this; }
    operator const StdVideoH265SequenceParameterSet*() const { return this; }

    virtual const char* GetRefClassId() const { return m_refClassId; }

    uint64_t SetSequenceCount(uint64_t updateSequenceCount) {
        assert(updateSequenceCount <= std::numeric_limits<uint32_t>::max());
        m_updateSequenceCount = (uint32_t)updateSequenceCount;
        return m_updateSequenceCount;
    }

    virtual bool GetClientObject(VkSharedBaseObj<VkVideoRefCountBase>& clientObject) const
    {
        clientObject = client;
        return !!clientObject;
    }

    hevc_seq_param_s(uint64_t updateSequenceCount)
    : StdVideoPictureParametersSet(TYPE_H265_SPS, SPS_TYPE,
                                   m_refClassId, updateSequenceCount)
    , StdVideoH265SequenceParameterSet()
    , stdProfileTierLevel()
    , stdDecPicBufMgr()
    , stdScalingLists()
    , stdShortTermRefPicSet{}
    , stdLongTermRefPicsSps()
    , stdVui()
    , stdHrdParameters()
    , max_dec_pic_buffering()
    , max_num_reorder_pics()
    , sps_rep_format_idx()
    , sps_scaling_list()
    , strpss() { }

    virtual ~hevc_seq_param_s() {
        client = nullptr;
    }

    static VkResult Create(uint64_t updateSequenceCount,
                           VkSharedBaseObj<hevc_seq_param_s>& spsH265PictureParametersSet)
    {
        VkSharedBaseObj<hevc_seq_param_s> spsPictureParametersSet(
                new hevc_seq_param_s(updateSequenceCount));
        if (spsPictureParametersSet) {
            spsH265PictureParametersSet = spsPictureParametersSet;
            return VK_SUCCESS;
        }
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    static bool UpdateStdVui(const hevc_seq_param_s* pSps, StdVideoH265SequenceParameterSetVui* /*pStdVui*/)
    {
        if (pSps->flags.vui_parameters_present_flag) {

            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, aspect_ratio_idc);
            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, sar_width);
            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, sar_height);
            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, video_format);
            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, colour_primaries);
            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, transfer_characteristics);
            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, matrix_coeffs);
            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, chroma_sample_loc_type_top_field);
            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, chroma_sample_loc_type_bottom_field);
            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, def_disp_win_left_offset);
            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, def_disp_win_right_offset);
            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, def_disp_win_top_offset);
            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, def_disp_win_bottom_offset);
            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, vui_num_units_in_tick);
            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, vui_time_scale);
            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, vui_num_ticks_poc_diff_one_minus1);
            // StdVideoH265HrdParameters*  hrd_parameters;
            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, min_spatial_segmentation_idc);
            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, max_bytes_per_pic_denom);
            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, max_bits_per_min_cu_denom);
            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, log2_max_mv_length_horizontal);
            // VK_H265_SPS_VUI_FIELD(pStdVui, pSps, log2_max_mv_length_vertical);

            // VK_H265_SPS_VUI_FLAG(pStdVui, pSps, aspect_ratio_info_present_flag);
            // VK_H265_SPS_VUI_FLAG(pStdVui, pSps, overscan_info_present_flag);
            // VK_H265_SPS_VUI_FLAG(pStdVui, pSps, overscan_appropriate_flag);
            // VK_H265_SPS_VUI_FLAG(pStdVui, pSps, video_signal_type_present_flag);
            // VK_H265_SPS_VUI_FLAG(pStdVui, pSps, video_full_range_flag);
            // VK_H265_SPS_VUI_FLAG(pStdVui, pSps, colour_description_present_flag);
            // VK_H265_SPS_VUI_FLAG(pStdVui, pSps, chroma_loc_info_present_flag);
            // VK_H265_SPS_VUI_FLAG(pStdVui, pSps, neutral_chroma_indication_flag);
            // VK_H265_SPS_VUI_FLAG(pStdVui, pSps, field_seq_flag);
            // VK_H265_SPS_VUI_FLAG(pStdVui, pSps, frame_field_info_present_flag);
            // VK_H265_SPS_VUI_FLAG(pStdVui, pSps, default_display_window_flag);
            // VK_H265_SPS_VUI_FLAG(pStdVui, pSps, vui_timing_info_present_flag);
            // VK_H265_SPS_VUI_FLAG(pStdVui, pSps, vui_poc_proportional_to_timing_flag);
            // VK_H265_SPS_VUI_FLAG(pStdVui, pSps, vui_hrd_parameters_present_flag);
            // VK_H265_SPS_VUI_FLAG(pStdVui, pSps, bitstream_restriction_flag);
            // VK_H265_SPS_VUI_FLAG(pStdVui, pSps, tiles_fixed_structure_flag);
            // VK_H265_SPS_VUI_FLAG(pStdVui, pSps, motion_vectors_over_pic_boundaries_flag);
            // VK_H265_SPS_VUI_FLAG(pStdVui, pSps, restricted_ref_pic_lists_flag);

            return true;
        }

        return false;
    }

    static bool UpdateStdScalingList(const hevc_seq_param_s* pSps, StdVideoH265ScalingLists*  pStdScalingLists)
    {
        if (!pSps->flags.scaling_list_enabled_flag)
        {
            memset(&pStdScalingLists->ScalingList4x4,         16, sizeof(pStdScalingLists->ScalingList4x4));
            memset(&pStdScalingLists->ScalingList8x8,         16, sizeof(pStdScalingLists->ScalingList8x8));
            memset(&pStdScalingLists->ScalingList16x16,       16, sizeof(pStdScalingLists->ScalingList16x16));
            memset(&pStdScalingLists->ScalingList32x32,       16, sizeof(pStdScalingLists->ScalingList32x32));
            memset(&pStdScalingLists->ScalingListDCCoef16x16, 16, sizeof(pStdScalingLists->ScalingListDCCoef16x16));
            memset(&pStdScalingLists->ScalingListDCCoef32x32, 16, sizeof(pStdScalingLists->ScalingListDCCoef32x32));

        } else {

            const scaling_list_s *scl = &pSps->sps_scaling_list;
            Init4x4ScalingListsH265(&pStdScalingLists->ScalingList4x4[0][0], scl);
            Init8x8ScalingListsH265(&pStdScalingLists->ScalingList8x8[0][0], NULL, scl, 1);
            Init8x8ScalingListsH265(&pStdScalingLists->ScalingList16x16[0][0], pStdScalingLists->ScalingListDCCoef16x16, scl, 2);
            Init8x8ScalingListsH265(&pStdScalingLists->ScalingList32x32[0][0], pStdScalingLists->ScalingListDCCoef32x32, scl, 3);

            return true;
        }

        return false;
    }

};


struct hevc_pic_param_s : public StdVideoPictureParametersSet, public StdVideoH265PictureParameterSet
{
    hevc_pic_param_s(uint64_t updateSequenceCount)
    : StdVideoPictureParametersSet(TYPE_H265_PPS, PPS_TYPE,
                                   m_refClassId, updateSequenceCount)
    , StdVideoH265PictureParameterSet()
    , stdScalingLists()
    , pps_scaling_list() {}

    virtual ~hevc_pic_param_s()
    {
        client = nullptr;
    }

    static VkResult Create(uint64_t updateSequenceCount,
                           VkSharedBaseObj<hevc_pic_param_s>& ppsH265PictureParametersSet)
    {
        VkSharedBaseObj<hevc_pic_param_s> ppsPictureParametersSet(new hevc_pic_param_s(updateSequenceCount));
        if (ppsPictureParametersSet) {
            ppsH265PictureParametersSet = ppsPictureParametersSet;
            return VK_SUCCESS;
        }
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    virtual int32_t GetVpsId(bool& isVps) const {
        isVps = false;
        return sps_video_parameter_set_id;
    }

    virtual int32_t GetSpsId(bool& isSps) const {
        isSps = false;
        return pps_seq_parameter_set_id;
    }

    virtual int32_t GetPpsId(bool& isPps) const {
        isPps = true;
        return pps_pic_parameter_set_id;
    }

    virtual const StdVideoH265PictureParameterSet* GetStdH265Pps() const { return this; }

    virtual const char* GetRefClassId() const { return m_refClassId; }

    uint64_t SetSequenceCount(uint64_t updateSequenceCount) {
        assert(updateSequenceCount <= std::numeric_limits<uint32_t>::max());
        m_updateSequenceCount = (uint32_t)updateSequenceCount;
        return m_updateSequenceCount;
    }

    virtual bool GetClientObject(VkSharedBaseObj<VkVideoRefCountBase>& clientObject) const
    {
        clientObject = client;
        return !!clientObject;
    }

    void Reset() {
        StdVideoH265PictureParameterSet::operator=(StdVideoH265PictureParameterSet());
        pps_scaling_list = scaling_list_s();
        stdScalingLists = StdVideoH265ScalingLists();
        client = nullptr;
    }

    static bool UpdateStdScalingList(const hevc_pic_param_s* pPps, StdVideoH265ScalingLists*  pStdScalingLists)
    {
        if (!pPps->flags.pps_scaling_list_data_present_flag)
        {
            memset(&pStdScalingLists->ScalingList4x4,         16, sizeof(pStdScalingLists->ScalingList4x4));
            memset(&pStdScalingLists->ScalingList8x8,         16, sizeof(pStdScalingLists->ScalingList8x8));
            memset(&pStdScalingLists->ScalingList16x16,       16, sizeof(pStdScalingLists->ScalingList16x16));
            memset(&pStdScalingLists->ScalingList32x32,       16, sizeof(pStdScalingLists->ScalingList32x32));
            memset(&pStdScalingLists->ScalingListDCCoef16x16, 16, sizeof(pStdScalingLists->ScalingListDCCoef16x16));
            memset(&pStdScalingLists->ScalingListDCCoef32x32, 16, sizeof(pStdScalingLists->ScalingListDCCoef32x32));

        } else {

            const scaling_list_s *scl = &pPps->pps_scaling_list;
            Init4x4ScalingListsH265(&pStdScalingLists->ScalingList4x4[0][0], scl);
            Init8x8ScalingListsH265(&pStdScalingLists->ScalingList8x8[0][0], NULL, scl, 1);
            Init8x8ScalingListsH265(&pStdScalingLists->ScalingList16x16[0][0], pStdScalingLists->ScalingListDCCoef16x16, scl, 2);
            Init8x8ScalingListsH265(&pStdScalingLists->ScalingList32x32[0][0], pStdScalingLists->ScalingListDCCoef32x32, scl, 3);

            return true;
        }
        return false;
    }

    static const char*        m_refClassId;
    StdVideoH265ScalingLists  stdScalingLists;
    scaling_list_s pps_scaling_list;

    VkSharedBaseObj<VkVideoRefCountBase> client;
};

typedef struct _repFormat_t
{
    uint32_t chroma_and_bit_depth_vps_present_flag;
    uint32_t chroma_format_vps_idc;
    uint32_t separate_colour_plane_vps_flag;
    uint32_t pic_width_vps_in_luma_samples;
    uint32_t pic_height_vps_in_luma_samples;
    uint32_t bit_depth_vps_luma_minus8;
    uint32_t bit_depth_vps_chroma_minus8;
    uint32_t conformance_window_vps_flag;
    uint32_t conf_win_vps_left_offset;
    uint32_t conf_win_vps_right_offset;
    uint32_t conf_win_vps_top_offset;
    uint32_t conf_win_vps_bottom_offset;
}repFormat_t;

struct hevc_video_param_flags {
    uint32_t vps_base_layer_internal_flag : 1;
    uint32_t vps_base_layer_available_flag : 1;
    /* VPS Extension */
    uint32_t vps_extension_flag : 1;
    uint32_t splitting_flag : 1;
    uint32_t vps_nuh_layer_id_present_flag : 1;

    uint32_t vps_sub_layers_max_minus1_present_flag : 1;
    uint32_t max_tid_ref_present_flag : 1;
    uint32_t default_ref_layers_active_flag : 1;
    uint32_t rep_format_idx_present_flag : 1;

    uint32_t max_one_active_ref_layer_flag : 1;
    uint32_t vps_poc_lsb_aligned_flag : 1;
};

struct hevc_video_param_s : public StdVideoPictureParametersSet, public StdVideoH265VideoParameterSet
{

    hevc_video_param_s(uint64_t updateSequenceCount)
    : StdVideoPictureParametersSet(TYPE_H265_VPS, VPS_TYPE,
                                   m_refClassId, updateSequenceCount)
    , StdVideoH265VideoParameterSet()
    , stdDecPicBufMgr()
    , stdHrdParameters()
    , stdProfileTierLevel()
    , privFlags()
    {
      size_t clearSize = (size_t)&client - (size_t)&privFlags;
      memset(&privFlags, 0x00, clearSize);
    }

    virtual ~hevc_video_param_s() {
        client = nullptr;
    }

    static VkResult Create(uint64_t updateSequenceCount,
                           VkSharedBaseObj<hevc_video_param_s>& vpsH265PictureParametersSet)
    {
        VkSharedBaseObj<hevc_video_param_s> vpsPictureParametersSet(new hevc_video_param_s(updateSequenceCount));
        if (vpsPictureParametersSet) {
            vpsH265PictureParametersSet = vpsPictureParametersSet;
            return VK_SUCCESS;
        }
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    virtual int32_t GetVpsId(bool& isVps) const {
        isVps = true;
        return vps_video_parameter_set_id;
    }

    virtual int32_t GetSpsId(bool& isSps) const {
        isSps = false;
        return -1;
    }

    virtual int32_t GetPpsId(bool& isPps) const {
        isPps = false;
        return -1;
    }

    virtual const StdVideoH265VideoParameterSet* GetStdH265Vps() const { return this; }

    virtual const char* GetRefClassId() const { return m_refClassId; }

    uint64_t SetSequenceCount(uint64_t updateSequenceCount) {
        assert(updateSequenceCount <= std::numeric_limits<uint32_t>::max());
        m_updateSequenceCount = (uint32_t)updateSequenceCount;
        return m_updateSequenceCount;
    }

    virtual bool GetClientObject(VkSharedBaseObj<VkVideoRefCountBase>& clientObject) const
    {
        clientObject = client;
        return !!clientObject;
    }

    void Reset() {

        StdVideoH265VideoParameterSet::operator=(StdVideoH265VideoParameterSet());

        memset(&stdDecPicBufMgr,     0x00, sizeof(stdDecPicBufMgr));
        stdHrdParameters.reset();
        memset(&stdProfileTierLevel, 0x00, sizeof(stdProfileTierLevel));

        size_t clearSize = (size_t)&client - (size_t)&privFlags;
        memset(&privFlags, 0x00, clearSize);

        client = nullptr;
    }

    static const char*                           m_refClassId;
    StdVideoH265DecPicBufMgr                     stdDecPicBufMgr;
    std::shared_ptr<hevc_video_hrd_param_s[]>    stdHrdParameters;
    StdVideoH265ProfileTierLevel                 stdProfileTierLevel;

    hevc_video_param_flags privFlags;

    uint32_t vps_max_layers_minus1;
     // profile_t profile; // CMOD variable
    uint32_t vps_max_layer_id;
    uint32_t vps_num_layer_sets;
    uint8_t  layer_id_included_flag[MAX_VPS_LAYER_SETS][MAX_NUM_LAYER_IDS];
    uint32_t num_layers_in_id_list[MAX_VPS_LAYER_SETS];
    uint8_t  layer_set_layer_id_list[MAX_VPS_LAYER_SETS][MAX_NUM_LAYER_IDS];
    uint32_t vps_num_hrd_parameters; 
    // hrdParameters_t hrdParameters[MAX_VPS_LAYER_SETS]; // CMOD variable
    uint32_t hrd_layer_set_idx[MAX_VPS_LAYER_SETS];
    uint8_t  cprms_present_flag[MAX_VPS_LAYER_SETS];

    /* VPS Extension */
    uint8_t  scalability_mask_flag[MAX_NUM_SCALABILITY_TYPES];
    uint32_t numScalabilityTypes; 
    uint8_t  dimension_id_len[MAX_NUM_SCALABILITY_TYPES];

    uint8_t  layer_id_in_nuh[MAX_NUM_LAYER_IDS];
    uint8_t LayerIdxInVps[MAX_NUM_LAYER_IDS];
    uint8_t  dimension_id[MAX_NUM_LAYER_IDS][MAX_NUM_SCALABILITY_TYPES];
    uint32_t numViews;
    uint8_t  viewOrderIdx[MAX_NUM_LAYER_IDS];
    uint32_t view_id_len;
    uint8_t  view_id_val[MAX_NUM_LAYER_IDS];
    uint8_t  direct_dependency_flag[MAX_NUM_LAYER_IDS][MAX_NUM_LAYER_IDS];
    uint8_t  DependencyFlag[MAX_NUM_LAYER_IDS][MAX_NUM_LAYER_IDS];
    uint8_t  numDirectRefLayers[MAX_NUM_LAYER_IDS];
    uint8_t  idDirectRefLayer[MAX_NUM_LAYER_IDS][MAX_NUM_LAYER_IDS];
    uint8_t  numRefLayers[MAX_NUM_LAYER_IDS];
    uint8_t  idRefLayer[MAX_NUM_LAYER_IDS][MAX_NUM_LAYER_IDS];
    uint8_t  numPredictedLayers[MAX_NUM_LAYER_IDS];
    uint8_t  idPredictedLayer[MAX_NUM_LAYER_IDS][MAX_NUM_LAYER_IDS];

    uint8_t  layerIdInListFlag[MAX_NUM_LAYER_IDS];
    uint32_t numLayersInTreePartition[MAX_NUM_LAYER_IDS];
    uint8_t  treePartitionLayerIdList[MAX_NUM_LAYER_IDS][MAX_NUM_LAYER_IDS];
    uint32_t numIndependentLayers;
    uint32_t num_add_layer_sets;
    uint8_t  highest_layer_idx_plus1[MAX_VPS_LAYER_SETS][MAX_NUM_LAYER_IDS];

    uint8_t  sub_layers_vps_max_minus1[MAX_NUM_LAYER_IDS];
    uint8_t  max_tid_il_ref_pics_plus1[MAX_NUM_LAYER_IDS][MAX_NUM_LAYER_IDS];

    uint32_t vps_num_profile_tier_level_minus1;
    uint8_t  vps_profile_present_flag[MAX_VPS_OP_SETS_PLUS1];

    /* Operation Points */
    uint32_t num_add_olss;
    uint32_t numOutputLayerSets;
    uint32_t default_output_layer_idc;
    uint32_t layer_set_idx_for_ols_minus1[MAX_VPS_OUTPUTLAYER_SETS];
    uint32_t output_layer_flag[MAX_VPS_OUTPUTLAYER_SETS][MAX_NUM_LAYER_IDS];

    uint8_t  numNecessaryLayers[MAX_VPS_OUTPUTLAYER_SETS];
    uint8_t  necessaryLayerFlag[MAX_VPS_OUTPUTLAYER_SETS][MAX_NUM_LAYER_IDS];

    uint8_t  numOutputLayersInOutputLayerSet[MAX_VPS_OUTPUTLAYER_SETS];
    uint8_t  olsHighestOutputLayerId[MAX_VPS_OUTPUTLAYER_SETS];

    uint8_t  profile_tier_level_idx[MAX_VPS_OUTPUTLAYER_SETS][MAX_NUM_LAYER_IDS];

    /* Output Format */
    uint32_t vps_num_rep_formats_minus1;
    repFormat_t repFormat[MAX_NUM_LAYER_IDS];
    uint8_t  vps_rep_format_idx[MAX_NUM_LAYER_IDS];
    uint8_t  poc_lsb_not_present_flag[MAX_NUM_LAYER_IDS];

    /* DPB size */
    uint8_t  sub_layer_flag_info_present_flag[MAX_VPS_OUTPUTLAYER_SETS];
    uint8_t  sub_layer_dpb_info_present_flag[MAX_VPS_OUTPUTLAYER_SETS][MAX_SUB_LAYERS];
    uint8_t  max_vps_dec_pic_buffering_minus1[MAX_VPS_OUTPUTLAYER_SETS][MAX_SUB_LAYERS][MAX_NUM_LAYER_IDS];
    uint8_t  max_vps_num_reorder_pics[MAX_VPS_OUTPUTLAYER_SETS][MAX_SUB_LAYERS];
    uint8_t  max_vps_latency_increase_plus1[MAX_VPS_OUTPUTLAYER_SETS][MAX_SUB_LAYERS];

    /* VPS Extension 2 */
    uint32_t vps_extension2_flag;

    VkSharedBaseObj<VkVideoRefCountBase> client;
};

typedef struct _hevc_slice_header_s
{
    uint8_t nal_unit_type;
    uint8_t nuh_temporal_id_plus1;
    uint8_t pic_output_flag;
    uint8_t collocated_from_l0_flag;

    uint8_t first_slice_segment_in_pic_flag;
    uint8_t no_output_of_prior_pics_flag;
    uint8_t pic_parameter_set_id;
    uint8_t slice_type;

    uint32_t slice_segment_address;

    uint8_t colour_plane_id;
    uint8_t short_term_ref_pic_set_sps_flag;
    uint8_t short_term_ref_pic_set_idx;
    uint8_t num_long_term_sps;
    
    uint16_t pic_order_cnt_lsb;
    uint8_t num_long_term_pics;
    uint8_t reserved1;

    uint32_t NumBitsForShortTermRPSInSlice;
    uint32_t used_by_curr_pic_lt_flags;      // bitmask [MAX_NUM_REF_PICS]
    uint32_t delta_poc_msb_present_flags;    // bitmask [MAX_NUM_REF_PICS]

    uint8_t lt_idx_sps[MAX_NUM_REF_PICS];
    uint16_t poc_lsb_lt[MAX_NUM_REF_PICS];
    int32_t delta_poc_msb_cycle_lt[MAX_NUM_REF_PICS];

    uint8_t slice_temporal_mvp_enabled_flag;
    uint8_t inter_layer_pred_enabled_flag;
    uint8_t num_inter_layer_ref_pics_minus1;
    uint8_t numActiveRefLayerPics;

    uint8_t num_ref_idx_l0_active_minus1;
    uint8_t num_ref_idx_l1_active_minus1;
    uint8_t inter_layer_pred_layer_idc[MAX_VPS_LAYERS];
    uint8_t reserved2[2];

    short_term_ref_pic_set_s strps;
} hevc_slice_header_s;


typedef struct _hevc_dpb_entry_s
{
    int state;      // 0: empty, 1: in use
    int marking;    // 0: unused, 1: short-term, 2: long-term
    int output;     // 0: not needed for output, 1: needed for output
    int PicOrderCntVal;
    int LayerId;   // Has DPB of nuh_layer_id = LayerID
    VkPicIf *pPicBuf;
} hevc_dpb_entry_s;


typedef struct _mastering_display_colour_volume_s
{
    // H.265 Annex D.2.27
    uint16_t display_primaries_x[3];
    uint16_t display_primaries_y[3];
    uint16_t white_point_x;
    uint16_t white_point_y;
    uint32_t max_display_mastering_luminance;
    uint32_t min_display_mastering_luminance;
} mastering_display_colour_volume;


//
// H.265 decoder class
//
class VulkanH265Decoder: public VulkanVideoDecoder
{
public:
    VulkanH265Decoder(VkVideoCodecOperationFlagBitsKHR std);
    virtual ~VulkanH265Decoder();

public:
    // VulkanVideoDecodeParser
    virtual bool GetDisplayMasteringInfo(VkParserDisplayMasteringInfo *);

protected:

    struct H265ParserData
    {
        H265ParserData()
        : spsClientUpdateCount(),
          ppsClientUpdateCount(),
          vpsClientUpdateCount(),
          display() {}

        virtual ~H265ParserData() {

        }

        uint64_t spsClientUpdateCount[MAX_NUM_SPS];
        uint64_t ppsClientUpdateCount[MAX_NUM_PPS];
        uint64_t vpsClientUpdateCount[MAX_NUM_VPS];
        mastering_display_colour_volume display;
    };

    // VulkanVideoDecoder
    virtual void CreatePrivateContext();
    virtual void InitParser();
    virtual bool IsPictureBoundary(int32_t rbsp_size);
    virtual bool BeginPicture(VkParserPictureData *pnvpd);
    virtual void EndPicture();
    virtual void EndOfStream();
    virtual int32_t  ParseNalUnit();
    virtual void FreeContext();

protected:
    // Syntax
    void seq_parameter_set_rbsp();
    void pic_parameter_set_rbsp();
    void video_parameter_set_rbsp();
    void video_parameter_set_rbspExtension(hevc_video_param_s *pVideoParamSet);
    void deriveNecessaryLayerFlags(hevc_video_param_s *pVideoParamSet, uint32_t olsIdx );
    void setRefLayers(hevc_video_param_s *pVideoParamSet);
    uint32_t olsIdxToLsIdx(hevc_video_param_s *pVideoParamSet, uint32_t i );
    uint32_t inferoutput_layer_flag(hevc_video_param_s *pVideoParamSet, uint32_t i, uint32_t j);
    uint32_t xGetDimBitOffset(hevc_video_param_s *pVideoParamSet, uint32_t j);
    void initNumViews(hevc_video_param_s *pVideoParamSet);
    const StdVideoH265ProfileTierLevel* profile_tier_level(StdVideoH265ProfileTierLevel* pProfileTierLevel,
                                                          int MaxNumSubLayersMinus1,
                                                          uint8_t ProfilePresent = 1);
    bool scaling_list_data(scaling_list_s *scl);
    StdVideoH265ShortTermRefPicSet* short_term_ref_pic_set(StdVideoH265ShortTermRefPicSet* stdShortTermRefPicSet,
                                                           short_term_ref_pic_set_s *strps,
                                                           const short_term_ref_pic_set_s strpss[],
                                                           int idx, int num_short_term_ref_pic_sets);
    void vui_parameters(hevc_seq_param_s *sps, int sps_max_sub_layers_minus1);
    void hrd_parameters(hevc_video_hrd_param_s* pStdHrdParameters, bool commonInfPresentFlag, uint8_t maxNumSubLayersMinus1);
    void sub_layer_hrd_parameters(StdVideoH265SubLayerHrdParameters* pStdSubLayerHrdParameters, int subLayerId, int cpb_cnt_minus1, int sub_pic_cpb_params_present_flag);
    bool slice_header(int nal_unit_type, int nuh_temporal_id_plus1);
    uint32_t getNumRefLayerPics(const hevc_video_param_s* vps, hevc_slice_header_s *pSliceHeader);
    void getNumActiveRefLayerPics(const hevc_video_param_s *pVideoParamSet, hevc_slice_header_s *pSliceHeader);
    // DPB management
    bool dpb_sequence_start(VkSharedBaseObj<hevc_seq_param_s>& sps);
    void flush_decoded_picture_buffer(int NoOutputOfPriorPicsFlag = 0);
    int dpb_fullness();
    int dpb_reordering_delay();
    bool dpb_empty() { return (dpb_fullness() == 0); }
    bool dpb_bumping(int maxAllowedDpbSize);
    void output_picture(int nframe);
    void dpb_picture_start(VkSharedBaseObj<hevc_pic_param_s>& pps, hevc_slice_header_s *slh);
    void dpb_picture_end();
    int  picture_order_count(hevc_slice_header_s *slh);
    void reference_picture_set(hevc_slice_header_s *slh, int PicOrderCntVal);
    int  create_lost_ref_pic(int lostPOC, int layerID, int marking_flag);
    // SEI layer
    void sei_payload();

protected:
    H265ParserData *m_pParserData;
    int m_MaxDpbSize;
    int m_bPictureStarted;
    int m_prevPicOrderCntMsb;
    int m_prevPicOrderCntLsb;
    uint32_t m_intra_pic_flag : 1;
    uint32_t  NoRaslOutputFlag : 1;
    int m_NumBitsForShortTermRPSInSlice;
    int m_NumDeltaPocsOfRefRpsIdx;
    int m_NumPocTotalCurr;
    int m_NumPocStCurrBefore;
    int m_NumPocStCurrAfter;
    int m_NumPocLtCurr;
    int m_NumActiveRefLayerPics0;
    int m_NumActiveRefLayerPics1;
    int m_nuh_layer_id;
    int m_max_dec_pic_buffering;
    int8_t m_RefPicSetStCurrBefore[32];
    int8_t m_RefPicSetStCurrAfter[32];
    int8_t m_RefPicSetLtCurr[32];
    int8_t m_RefPicSetInterLayer0[32];
    int8_t m_RefPicSetInterLayer1[32];
    hevc_dpb_entry_s *m_dpb_cur;
    int8_t            m_current_dpb_id;
    hevc_dpb_entry_s m_dpb[HEVC_DPB_SIZE];
    hevc_slice_header_s m_slh;
    VkSharedBaseObj<hevc_seq_param_s> m_active_sps[MAX_VPS_LAYERS];
    VkSharedBaseObj<hevc_pic_param_s> m_active_pps[MAX_VPS_LAYERS];
    VkSharedBaseObj<hevc_video_param_s> m_active_vps;
    VkSharedBaseObj<hevc_seq_param_s> m_spss[MAX_NUM_SPS];
    VkSharedBaseObj<hevc_pic_param_s> m_ppss[MAX_NUM_PPS];
    VkSharedBaseObj<hevc_video_param_s> m_vpss[MAX_NUM_VPS];
    mastering_display_colour_volume *m_display;
};


#endif // _VULKANH265DECODER_H_
