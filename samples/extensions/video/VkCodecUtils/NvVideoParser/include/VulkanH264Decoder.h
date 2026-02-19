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

#ifndef _VULKANH264DECODER_H_
#define _VULKANH264DECODER_H_

#include "vkvideo_parser/StdVideoPictureParametersSet.h"
#include "VulkanVideoDecoder.h"
#include "VulkanH26xDecoder.h"
#include "nvVulkanh264ScalingList.h"

#define VK_H264_SPS_VUI_FIELD(pStdVui, nvSpsIn, name) pStdVui->name = nvSpsIn->vui.name
#define SET_VK_H264_SPS_VUI_FIELD(pStdVui, name, value) pStdVui->name = value
#define VK_H264_SPS_VUI_FLAG(pStdVui, nvSpsIn, name) pStdVui->flags.name = nvSpsIn->vui.name

#define MAX_REFS         32  // maximum size of reference picture lists (number of pictures)
#define MAX_DPB_SIZE     16  // maximum size of decoded picture buffer (number of frames)
#define MAX_DPB_SVC_SIZE 17  // maximum size of decoded picture buffer (number of frames) + reff buffer
#define MAX_MMCOS        72  // maximum number of mmco's

struct hrd_parameters_s
{
    uint8_t  bit_rate_scale;
    uint8_t  cpb_size_scale;
    uint8_t  cpb_cnt_minus1;
    unsigned int bit_rate;
    unsigned int cbp_size;
    uint32_t time_offset_length;
};

struct vui_parameters_s
{
    unsigned char aspect_ratio_idc;
    int sar_width;
    int sar_height;
    int video_format;
    int colour_primaries;
    int transfer_characteristics;
    int matrix_coefficients;
    int num_units_in_tick;
    int time_scale;
    int initial_cpb_removal_delay_length;
    int cpb_removal_delay_length_minus1;
    int dpb_output_delay_length_minus1;
    int max_num_reorder_frames;
    int max_dec_frame_buffering;
    unsigned int aspect_ratio_info_present_flag:1;
    unsigned int video_signal_type_present_flag:1;
    unsigned int overscan_info_present_flag:1;
    unsigned int overscan_appropriate_flag:1;
    unsigned int video_full_range_flag:1;
    unsigned int color_description_present_flag:1;
    unsigned int nal_hrd_parameters_present_flag:1;
    unsigned int vcl_hrd_parameters_present_flag:1;
    unsigned int chroma_loc_info_present_flag:1;
    unsigned int timing_info_present_flag:1;
    unsigned int fixed_frame_rate_flag:1;
    unsigned int pic_struct_present_flag:1;
    unsigned int bitstream_restriction_flag:1;
    hrd_parameters_s nal_hrd;
    hrd_parameters_s vcl_hrd;
};

struct seq_parameter_set_svc_extension_s
{
    int inter_layer_deblocking_filter_control_present_flag;
    int extended_spatial_scalability_idc;
    int chroma_phase_x_plus1_flag;
    int chroma_phase_y_plus1;
    int seq_ref_layer_chroma_phase_x_plus1_flag;
    int seq_ref_layer_chroma_phase_y_plus1;
    int seq_scaled_ref_layer_left_offset;
    int seq_scaled_ref_layer_top_offset;
    int seq_scaled_ref_layer_right_offset;
    int seq_scaled_ref_layer_bottom_offset;
    int seq_tcoeff_level_prediction_flag;
    int adaptive_tcoeff_level_prediction_flag;
    int slice_header_restriction_flag;
};

struct seq_parameter_set_s : public StdVideoPictureParametersSet, public StdVideoH264SequenceParameterSet
{
    static const char*                   m_refClassId;
    StdVideoH264ScalingLists             stdScalingLists;
    int32_t                              offset_for_ref_frame[255];
    StdVideoH264SequenceParameterSetVui  stdVui;
    StdVideoH264HrdParameters            stdHrdParameters;
    // private interface
    NvScalingListH264 seqScalingList;
    vui_parameters_s vui;
    // SVC
    seq_parameter_set_svc_extension_s svc;

    int constraint_set_flags;

    VkSharedBaseObj<VkVideoRefCountBase> client;

    seq_parameter_set_s(uint64_t updateSequenceCount)
    : StdVideoPictureParametersSet(TYPE_H264_SPS, SPS_TYPE,
                                   m_refClassId, updateSequenceCount)
    , StdVideoH264SequenceParameterSet()
    , stdScalingLists()
    , offset_for_ref_frame{0}
    , stdVui()
    , stdHrdParameters()
    , seqScalingList()
    , vui()
    , svc()
    , constraint_set_flags()
      {
          pOffsetForRefFrame = offset_for_ref_frame;
      }

    virtual ~seq_parameter_set_s() {
        client = nullptr;
    }

    static VkResult Create(uint64_t updateSequenceCount,
                           VkSharedBaseObj<seq_parameter_set_s>& spsH265PictureParametersSet)
    {
        VkSharedBaseObj<seq_parameter_set_s> spsPictureParametersSet(new seq_parameter_set_s(updateSequenceCount));
        if (spsPictureParametersSet) {
            spsH265PictureParametersSet = spsPictureParametersSet;
            return VK_SUCCESS;
        }
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    virtual int32_t GetVpsId(bool& isVps) const {
        isVps = false;
        return 0;
    }

    virtual int32_t GetSpsId(bool& isSps) const {
        isSps = true;
        return seq_parameter_set_id;
    }

    virtual int32_t GetPpsId(bool& isPps) const {
        isPps = false;
        return -1;
    }

    virtual const char* GetRefClassId() const { return m_refClassId; }

    uint64_t SetSequenceCount(uint64_t updateSequenceCount) {
        assert(updateSequenceCount <= std::numeric_limits<uint32_t>::max());
        m_updateSequenceCount = (uint32_t)updateSequenceCount;
        return m_updateSequenceCount;
    }

    virtual const StdVideoH264SequenceParameterSet* GetStdH264Sps() const { return this; }

    virtual bool GetClientObject(VkSharedBaseObj<VkVideoRefCountBase>& clientObject) const
    {
        clientObject = client;
        return !!clientObject;
    }

    void Reset() {

        StdVideoH264SequenceParameterSet::operator=(StdVideoH264SequenceParameterSet());
        stdScalingLists = StdVideoH264ScalingLists();
        stdVui = StdVideoH264SequenceParameterSetVui();
        seqScalingList = NvScalingListH264();
        vui = vui_parameters_s();
        svc = seq_parameter_set_svc_extension_s();
        constraint_set_flags = 0;

        client = nullptr;
    }

    static bool UpdateStdScalingList(const seq_parameter_set_s* pSps, StdVideoH264ScalingLists* pStdScalingLists)
    {
        if (pStdScalingLists && pSps->seqScalingList.scaling_matrix_present_flag) {

            // see SetSeqPicScalingListsH264()
            memcpy(&pStdScalingLists->ScalingList4x4, &pSps->seqScalingList.ScalingList4x4,
                    sizeof(pSps->seqScalingList.ScalingList4x4));
            memcpy(&pStdScalingLists->ScalingList8x8, &pSps->seqScalingList.ScalingList8x8,
                    sizeof(pSps->seqScalingList.ScalingList8x8));

            for (uint32_t i = 0; i < 8; i++) {

                if (pSps->seqScalingList.scaling_list_type[i] == SCALING_LIST_PRESENT) {
                    pStdScalingLists->scaling_list_present_mask |= (1 << i);
                }

                if (pSps->seqScalingList.scaling_list_type[i] == SCALING_LIST_USE_DEFAULT) {
                    pStdScalingLists->use_default_scaling_matrix_mask |= (1 << i);
                }
            }

            // see SetSeqPicScalingListsH264()
            return true;
        }

        return false;
    }

    static bool UpdateStdVui(const seq_parameter_set_s* pSps,
                             StdVideoH264SequenceParameterSetVui* pStdVui,
                             StdVideoH264HrdParameters*           pStdHrdParameters)
    {
        if (pSps->flags.vui_parameters_present_flag) {

            SET_VK_H264_SPS_VUI_FIELD(pStdVui, aspect_ratio_idc,
                    (StdVideoH264AspectRatioIdc)pSps->vui.aspect_ratio_idc);
            VK_H264_SPS_VUI_FIELD(pStdVui, pSps, sar_width);
            VK_H264_SPS_VUI_FIELD(pStdVui, pSps, sar_height);
            VK_H264_SPS_VUI_FIELD(pStdVui, pSps, video_format);
            VK_H264_SPS_VUI_FIELD(pStdVui, pSps, colour_primaries);
            VK_H264_SPS_VUI_FIELD(pStdVui, pSps, transfer_characteristics);
            VK_H264_SPS_VUI_FIELD(pStdVui, pSps, matrix_coefficients);
            VK_H264_SPS_VUI_FIELD(pStdVui, pSps, num_units_in_tick);
            VK_H264_SPS_VUI_FIELD(pStdVui, pSps, time_scale);

            VK_H264_SPS_VUI_FIELD(pStdVui, pSps, max_num_reorder_frames);
            VK_H264_SPS_VUI_FIELD(pStdVui, pSps, max_dec_frame_buffering);

            // StdVideoH264HrdParameters   hrd_parameters);
            pStdHrdParameters->cpb_cnt_minus1 = pSps->vui.nal_hrd.cpb_cnt_minus1;
            pStdHrdParameters->bit_rate_scale = pSps->vui.nal_hrd.bit_rate;
            pStdHrdParameters->cpb_size_scale = pSps->vui.nal_hrd.cbp_size;
            // stdVui.pHrdParameters->bit_rate_value_minus1[32];
            // stdVui.pHrdParameters->cpb_size_value_minus1[32];
            // stdVui.pHrdParameters->cbr_flag[32];
            pStdHrdParameters->initial_cpb_removal_delay_length_minus1 = pSps->vui.initial_cpb_removal_delay_length - 1;
            pStdHrdParameters->cpb_removal_delay_length_minus1 = pSps->vui.cpb_removal_delay_length_minus1;
            pStdHrdParameters->dpb_output_delay_length_minus1 = (uint32_t)pSps->vui.dpb_output_delay_length_minus1;
            pStdHrdParameters->time_offset_length = pSps->vui.nal_hrd.time_offset_length;

            pStdVui->pHrdParameters = pStdHrdParameters;

            // StdVideoH264SpsVuiFlags     flags;
            VK_H264_SPS_VUI_FLAG(pStdVui, pSps, aspect_ratio_info_present_flag);
            VK_H264_SPS_VUI_FLAG(pStdVui, pSps, overscan_info_present_flag);
            VK_H264_SPS_VUI_FLAG(pStdVui, pSps, overscan_appropriate_flag);
            VK_H264_SPS_VUI_FLAG(pStdVui, pSps, video_signal_type_present_flag);
            VK_H264_SPS_VUI_FLAG(pStdVui, pSps, video_full_range_flag);
            VK_H264_SPS_VUI_FLAG(pStdVui, pSps, color_description_present_flag);
            VK_H264_SPS_VUI_FLAG(pStdVui, pSps, chroma_loc_info_present_flag);
            VK_H264_SPS_VUI_FLAG(pStdVui, pSps, timing_info_present_flag);
            VK_H264_SPS_VUI_FLAG(pStdVui, pSps, fixed_frame_rate_flag);
            VK_H264_SPS_VUI_FLAG(pStdVui, pSps, bitstream_restriction_flag);
            VK_H264_SPS_VUI_FLAG(pStdVui, pSps, nal_hrd_parameters_present_flag);
            VK_H264_SPS_VUI_FLAG(pStdVui, pSps, vcl_hrd_parameters_present_flag);

            return true;
        } else {
            pStdVui->pHrdParameters = nullptr;
        }

        return false;
    }
};

struct seq_parameter_set_mvc_extension_s
{
    seq_parameter_set_mvc_extension_s() :
        num_views_minus1(0),
        view_id(NULL),
        num_anchor_refs_l0(NULL),
        anchor_ref_l0(NULL),
        num_anchor_refs_l1(NULL),
        anchor_ref_l1(NULL),
        num_non_anchor_refs_l0(NULL),
        non_anchor_ref_l0(NULL),
        num_non_anchor_refs_l1(NULL),
        non_anchor_ref_l1(NULL),
        num_level_values_signalled_minus1(0),
        level_idc(NULL),
        num_applicable_ops_minus1(NULL),
        applicable_op_temporal_id(NULL),
        applicable_op_num_target_views_minus1(NULL),
        applicable_op_target_view_id(NULL),
        applicable_op_num_views_minus1(NULL) {};

    void release() {
        if(num_level_values_signalled_minus1)
        {
            for(int i = 0; i <= num_level_values_signalled_minus1; i++)
            {
                if(num_applicable_ops_minus1[i])
                {
                    for(int j = 0; j <= num_applicable_ops_minus1[i]; j++)
                    {
                        if(applicable_op_target_view_id[i][j])
                            delete[] applicable_op_target_view_id[i][j];
                    }
                    if(applicable_op_temporal_id[i])
                        delete[] applicable_op_temporal_id[i];
                    if(applicable_op_num_target_views_minus1[i])
                        delete[] applicable_op_num_target_views_minus1[i];
                    if(applicable_op_target_view_id[i])
                        delete[] applicable_op_target_view_id[i];
                    if(applicable_op_num_views_minus1[i])
                        delete[] applicable_op_num_views_minus1[i];
                }
            }
        }

        num_level_values_signalled_minus1 = 0;
        if(level_idc)
            delete[] level_idc;
        if(num_applicable_ops_minus1)
            delete[] num_applicable_ops_minus1;
        if(applicable_op_temporal_id)
            delete[] applicable_op_temporal_id;
        if(applicable_op_num_target_views_minus1)
            delete[] applicable_op_num_target_views_minus1;
        if(applicable_op_target_view_id)
            delete[] applicable_op_target_view_id;
        if(applicable_op_num_views_minus1)
            delete[] applicable_op_num_views_minus1;

        if(num_views_minus1)
        {
            for(int i = 1; i <= num_views_minus1; i++)
            {
                if(non_anchor_ref_l0[i])
                    delete[] non_anchor_ref_l0[i];
                if(non_anchor_ref_l1[i])
                    delete[] non_anchor_ref_l1[i];
            }
        }
        if(num_non_anchor_refs_l0)
            delete[] num_non_anchor_refs_l0;
        if(num_non_anchor_refs_l1)
            delete[] num_non_anchor_refs_l1;
        if(non_anchor_ref_l0)
            delete[] non_anchor_ref_l0;
        if(non_anchor_ref_l1)
            delete[] non_anchor_ref_l1;

        if(num_views_minus1)
        {
            for(int i = 1; i <= num_views_minus1; i++)
            {
                if(anchor_ref_l0[i])
                    delete[] anchor_ref_l0[i];
                if(anchor_ref_l1[i])
                    delete[] anchor_ref_l1[i];
            }
        }

        if(num_anchor_refs_l0)
            delete[] num_anchor_refs_l0;
        if(num_anchor_refs_l1)
            delete[] num_anchor_refs_l1;
        if(anchor_ref_l0)
            delete[] anchor_ref_l0;
        if(anchor_ref_l1)
            delete[] anchor_ref_l1;

        if(view_id)
            delete[] view_id;
        num_views_minus1 = 0;
    };
    int num_views_minus1;
    int *view_id;
    int *num_anchor_refs_l0;
    int **anchor_ref_l0;
    int *num_anchor_refs_l1;
    int **anchor_ref_l1;
    int *num_non_anchor_refs_l0;
    int **non_anchor_ref_l0;
    int *num_non_anchor_refs_l1;
    int **non_anchor_ref_l1;

    int num_level_values_signalled_minus1;
    int *level_idc;
    int *num_applicable_ops_minus1;
    int **applicable_op_temporal_id;
    int **applicable_op_num_target_views_minus1;
    int ***applicable_op_target_view_id;
    int **applicable_op_num_views_minus1;
};

struct pic_parameter_set_s : public StdVideoPictureParametersSet, public StdVideoH264PictureParameterSet
{

    pic_parameter_set_s(uint64_t updateSequenceCount)
    : StdVideoPictureParametersSet(TYPE_H264_PPS, PPS_TYPE,
                                   m_refClassId, updateSequenceCount)
    , StdVideoH264PictureParameterSet()
    , stdScalingLists()
    , num_slice_groups_minus1()
    , picScalinList() {}

    virtual ~pic_parameter_set_s()
    {
        client = nullptr;
    }

    static VkResult Create(uint64_t updateSequenceCount,
                           VkSharedBaseObj<pic_parameter_set_s>& ppsH264PictureParametersSet)
    {
        VkSharedBaseObj<pic_parameter_set_s> ppsPictureParametersSet(new pic_parameter_set_s(updateSequenceCount));
        if (ppsPictureParametersSet) {
            ppsH264PictureParametersSet = ppsPictureParametersSet;
            return VK_SUCCESS;
        }
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    virtual int32_t GetVpsId(bool& isVps) const {
        isVps = false;
        return 0;
    }

    virtual int32_t GetSpsId(bool& isSps) const {
        isSps = false;
        return seq_parameter_set_id;
    }

    virtual int32_t GetPpsId(bool& isPps) const {
        isPps = true;
        return pic_parameter_set_id;
    }

    virtual const StdVideoH264PictureParameterSet* GetStdH264Pps() const { return this; }

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
        StdVideoH264PictureParameterSet::operator=(StdVideoH264PictureParameterSet());
        picScalinList = NvScalingListH264();
        stdScalingLists = StdVideoH264ScalingLists();
        client = nullptr;
    }

    static bool UpdateStdScalingList(const pic_parameter_set_s* pPps, StdVideoH264ScalingLists* pStdScalingLists)
    {
        if (pStdScalingLists && pPps->picScalinList.scaling_matrix_present_flag) {

            // see SetSeqPicScalingListsH264()
            memcpy(&pStdScalingLists->ScalingList4x4, &pPps->picScalinList.ScalingList4x4,
                    sizeof(pPps->picScalinList.ScalingList4x4));
            memcpy(&pStdScalingLists->ScalingList8x8, &pPps->picScalinList.ScalingList8x8,
                    sizeof(pPps->picScalinList.ScalingList8x8));

            for (uint32_t i = 0; i < 8; i++) {

                if (pPps->picScalinList.scaling_list_type[i] == SCALING_LIST_PRESENT) {
                    pStdScalingLists->scaling_list_present_mask |= (1 << i);
                }

                if (pPps->picScalinList.scaling_list_type[i] == SCALING_LIST_USE_DEFAULT) {
                    pStdScalingLists->use_default_scaling_matrix_mask |= (1 << i);
                }
            }
            return true;
        }
        return false;
    }

    static const char*        m_refClassId;
    StdVideoH264ScalingLists  stdScalingLists;
    unsigned char num_slice_groups_minus1;
    NvScalingListH264 picScalinList;

    VkSharedBaseObj<VkVideoRefCountBase> client;
};

struct memory_management_control_operation_s
{
    int memory_management_control_operation;
    int difference_of_pic_nums_minus1;
    int long_term_frame_idx; // also used for long_term_pic_num and max_long_term_frame_idx_plus1
};

struct memory_management_base_control_operation_s
{
    int memory_management_base_control_operation;
    int difference_of_base_pic_nums_minus1;
    int long_term_base_pic_num;
};

struct ref_pic_list_reordering_s
{
    int reordering_of_pic_nums_idc;
    int PicNumIdx; // abs_diff_pic_num_minus1 or long_term_pic_num depending on reordering_of_pic_nums_idc
};

typedef struct _nalu_header_extension
{
    int svc_extension_flag;
    union
    {
        struct
        {
            // SVC
            int idr_flag;
            int priority_id;
            int no_inter_layer_pred_flag;
            int dependency_id;
            int quality_id;
            int temporal_id;
            int use_ref_base_pic_flag;
            int discardable_flag;
            int output_flag;
        } svc;
        struct
        {
            // MVC
            unsigned char non_idr_flag;
            int priority_id;
            int view_id;
            int temporal_id;
            unsigned char anchor_pic_flag;
            unsigned char inter_view_flag;
        } mvc;
    };
} nalu_header_extension_u;

struct slice_header_s
{
    int first_mb_in_slice;
    int slice_type_raw;
    int slice_type;
    int pic_parameter_set_id;
    int colour_plane_id;
    int frame_num;
    int idr_pic_id;
    int pic_order_cnt_lsb;
    int delta_pic_order_cnt_bottom;
    int delta_pic_order_cnt[2];
    int redundant_pic_cnt;
    int num_ref_idx_l0_active_minus1;
    int num_ref_idx_l1_active_minus1;
    // dec_ref_pic_marking
    uint32_t direct_spatial_mv_pred_flag : 1;
    uint32_t field_pic_flag : 1;
    uint32_t bottom_field_flag  : 1;
    uint32_t no_output_of_prior_pics_flag : 1;
    uint32_t long_term_reference_flag : 1;
    uint32_t adaptive_ref_pic_marking_mode_flag : 1;
    uint32_t mmco5  : 1; // derived value that indicates presence of an mmco equal to 5
    uint32_t IdrPicFlag : 1;
    memory_management_control_operation_s mmco[MAX_MMCOS];
    // ref_pic_list_reordering
    uint32_t nal_ref_idc : 8;        // extracted from NAL start code
    uint32_t nal_unit_type : 8;      // extracted from NAL start code
    uint32_t ref_pic_list_reordering_flag_l0 : 1;
    uint32_t ref_pic_list_reordering_flag_l1 : 1;
    ref_pic_list_reordering_s ref_pic_list_reordering_l0[MAX_REFS];
    ref_pic_list_reordering_s ref_pic_list_reordering_l1[MAX_REFS];
    // pred_weight_table
    int luma_log2_weight_denom;
    int chroma_log2_weight_denom;
    int weights_out_of_range; // 1 if out-of-range weights
    int16_t luma_weight[2][MAX_REFS]; // [LX][refIdx]
    int16_t luma_offset[2][MAX_REFS]; // [LX][refIdx]
    int16_t chroma_weight[2][MAX_REFS][2]; // [LX][refIdx][iCbCr]
    int16_t chroma_offset[2][MAX_REFS][2]; // [LX][refIdx][iCbCr]
    // access_unit_delimiter
    int primary_pic_type;
    // pic_timing
    int sei_pic_struct;
    int view_id;
    // FMO
    unsigned int slice_group_change_cycle;
    // SVC
    int base_pred_weight_table_flag;
    int store_ref_base_pic_flag;
    int adaptive_ref_base_pic_marking_mode_flag;
    memory_management_base_control_operation_s mmbco[MAX_MMCOS];
    int ref_layer_dq_id;
    int disable_inter_layer_deblocking_filter_idc;
    int inter_layer_slice_alpha_c0_offset_div2;
    int inter_layer_slice_beta_offset_div2;
    int constrained_intra_resampling_flag;
    int ref_layer_chroma_phase_x_plus1_flag;
    int ref_layer_chroma_phase_y_plus1;
    int scaled_ref_layer_left_offset;
    int scaled_ref_layer_top_offset;
    int scaled_ref_layer_right_offset;
    int scaled_ref_layer_bottom_offset;
    int slice_skip_flag;
    int num_mbs_in_slice_minus1;
    int adaptive_base_mode_flag;
    int default_base_mode_flag;
    int adaptive_motion_prediction_flag;
    int default_motion_prediction_flag;
    int adaptive_residual_prediction_flag;
    int default_residual_prediction_flag;
    int tcoeff_level_prediction_flag;
    nalu_header_extension_u nhe;
};

struct layer_data_s
{
    int available;
    int used;
    int MaxRefLayerDQId;
    int dqid_next;
    VkSharedBaseObj<seq_parameter_set_s> sps;
    VkSharedBaseObj<pic_parameter_set_s> pps;
    slice_header_s      slh;
    int slice_count;
};

struct dpb_entry_s
{
    // DPB attributes
    int state; // empty (0), top (1), bottom (2), top and bottom (3) used
    VkPicIf *pPicBuf;
    bool top_needed_for_output;
    bool bottom_needed_for_output;
    bool reference_picture;
    bool complementary_field_pair;
    // reference frame attributes
    int top_field_marking; // unused, short-term, long-term
    int bottom_field_marking; // unused, short-term, long-term
    bool not_existing; // set if in frame_num gap
    int FrameNum;
    int LongTermFrameIdx;
    int TopFieldOrderCnt;
    int BottomFieldOrderCnt;
    int PicOrderCnt;
    //
    int FrameNumWrap;
    int TopPicNum;
    int BottomPicNum;
    int TopLongTermPicNum;
    int BottomLongTermPicNum;
    // MVC
    int view_id;
    int VOIdx;
    int inter_view_flag;
};

struct svc_dpb_entry_s
{
    VkPicIf *pPicBuf;
    VkPicIf *pPicBufRefBase;
    bool complementary_field_pair;
    int PicOrderCnt;
    int ref;  // 0=unused for reference, 1=used for short-term reference, 2=used for long-term reference
    int output; // 0=not needed for output, 1=needed for output
    int TopFieldOrderCnt;
    int BottomFieldOrderCnt;
    int base; // 0=reference picture, 1=reference base picture
    int non_existing; // 0=not marked as "non-existing", 1=marked as "non-existing"
    int FrameNum;
    int FrameNumWrap;
    int PicNum;
    int LongTermFrameIdx;
    int LongTermPicNum;
};

struct prefix_nal_unit_svc_s
{
    nalu_header_extension_u nalu;
    int store_ref_base_pic_flag;
    // dec_ref_base_pic_marking
    int adaptive_ref_base_pic_marking_mode_flag;
    memory_management_base_control_operation_s mmbco[MAX_MMCOS];
};

struct dependency_state_s
{
    int MaxLongTermFrameIdx;
    int prevPicOrderCntMsb;
    int prevPicOrderCntLsb;
    int prevFrameNum;
    int prevFrameNumOffset;
    int PrevRefFrameNum;
    int dpb_entry_id; // next id
    svc_dpb_entry_s dpb_entry[16+1]; // 1 temporary entry for current picture
};

struct dependency_data_s
{
    int used;
    VkSharedBaseObj<seq_parameter_set_s> sps;
    seq_parameter_set_svc_extension_s    sps_svc;
    slice_header_s   slh;
    int MaxDpbFrames;
};

typedef struct _slice_group_map_s
{
    // This version of the parser does not support
    // slice_group_map
    // Reduced version just to make sure the bitstream
    // is correctly handled.
    uint16_t slice_group_map_type;
    int16_t  slice_group_change_rate_minus1;
} slice_group_map_s;

typedef struct _frame_packing_arrangement_s
{
    unsigned int frame_packing_arrangement_type;
    unsigned int content_interpretation_flag;
} frame_packing_arrangement_s;

typedef struct _DPBPicNum
{
    int TopPicNum;
    int BottomPicNum;
    int PicOrderCnt;
} DPBPicNum;

typedef bool (*PFNSORTCHECK)(int, int *, const DPBPicNum *, const VkParserPictureData *);


//
// H.264 decoder class
//
class VulkanH264Decoder: public VulkanVideoDecoder
{
public:
    enum { MAX_NUM_SPS = 32 };
    enum { MAX_NUM_PPS = 256 };

public:
    VulkanH264Decoder(VkVideoCodecOperationFlagBitsKHR std);
    virtual ~VulkanH264Decoder();

protected:

    struct H264ParserData
    {

        H264ParserData()
        : slh(),
          spssClientUpdateCount(),
          spsmes(),
          spsmesClientUpdateCount(),
          spssvcsClientUpdateCount(),
          ppssClientUpdateCount(),
          nhe()
        {

        }

        virtual ~H264ParserData() {

        }

        slice_header_s slh;    // first slice of the picture (for dpb management)
        uint64_t  spssClientUpdateCount[MAX_NUM_SPS];
        seq_parameter_set_mvc_extension_s spsmes[MAX_NUM_SPS];
        uint64_t spsmesClientUpdateCount[MAX_NUM_SPS];
        uint64_t spssvcsClientUpdateCount[MAX_NUM_SPS];
        uint64_t ppssClientUpdateCount[MAX_NUM_PPS];
        nalu_header_extension_u nhe;
    };

    // CNvVideoDecoder
    virtual void CreatePrivateContext();
    virtual void InitParser();
    virtual bool IsPictureBoundary(int32_t rbsp_size);
    virtual bool BeginPicture(VkParserPictureData *pnvpd);
    virtual void EndPicture();
    virtual void EndOfStream();
    virtual int32_t  ParseNalUnit();
    virtual void FreeContext();

private:
    // Header parsing
    enum SpsNalUnitTarget {
       SPS_NAL_UNIT_TARGET_SPS = 0,
       SPS_NAL_UNIT_TARGET_SPS_MVC,
       SPS_NAL_UNIT_TARGET_SPS_SVC,
    };

    int32_t seq_parameter_set_rbsp(SpsNalUnitTarget spsNalUnitTarget = SPS_NAL_UNIT_TARGET_SPS,
                                   seq_parameter_set_s *spssvc = NULL);
    bool seq_parameter_set_mvc_extension_rbsp(int32_t sps_id);
    void vui_parameters(vui_parameters_s *vui);
    void hrd_parameters(vui_parameters_s *vui, hrd_parameters_s *hrd);
    int scaling_list(unsigned char scalingList[], int sizeOfScalingList); // returns scaling_list_type
    bool pic_parameter_set_rbsp();
    bool slice_header(slice_header_s *slh, int nal_ref_idc, int nal_unit_type);
    bool ref_pic_list_reordering(slice_header_s *slh);
    bool pred_weight_table(slice_header_s *slh, int chromaArrayType);
    void dec_ref_pic_marking(slice_header_s *slh);
    void nal_unit_header_extension();

    // DPB management
    bool dpb_sequence_start(slice_header_s *slh);
    void dpb_picture_start(pic_parameter_set_s *pps, slice_header_s *slh);
    void dpb_picture_end();
    VkPicIf *alloc_picture();
    void output_picture(int nframe, int picture_structure);
    void picture_order_count(const seq_parameter_set_s *sps, const slice_header_s *slh);
    void picture_order_count_type_0(const seq_parameter_set_s *sps, const slice_header_s *slh);
    void picture_order_count_type_1(const seq_parameter_set_s *sps, const slice_header_s *slh);
    void picture_order_count_type_2(const seq_parameter_set_s *sps, const slice_header_s *slh);
    void picture_numbers(const slice_header_s *slh, int MaxFrameNum);
    void decoded_reference_picture_marking(slice_header_s *slh, uint32_t num_ref_frames);
    void gaps_in_frame_num();
    void sliding_window_decoded_reference_picture_marking(uint32_t num_ref_frames);
    void adaptive_memory_control_decoded_reference_picture_marking(slice_header_s *slh, int num_ref_frames);
    int dpb_fullness();
    bool dpb_full() { int fullness = dpb_fullness(); return ((fullness > 0) && (fullness >= m_MaxDpbSize)); }
    bool dpb_empty() { return (dpb_fullness() == 0); }
    void dpb_bumping(int DpbSize=MAX_DPB_SIZE);
    int dpb_reordering_delay();
    void display_bumping();
    void flush_decoded_picture_buffer();
    bool is_comp_field_pair(dpb_entry_s *dpb, slice_header_s *slh);
    bool find_comp_field_pair(slice_header_s *slh, int *iCur);
    uint8_t  derive_MaxDpbFrames(const seq_parameter_set_s *sps);
    // SEI layer
    void sei_payload(int payloadType, int payloadSize);
    // MVC
    int get_view_output_index(int view_id);
    // SVC
    bool prefix_nal_unit_svc(int nal_ref_idc);
    int dec_ref_base_pic_marking(memory_management_base_control_operation_s mmbco[MAX_MMCOS]);
    void update_layer_info(seq_parameter_set_s *sps, pic_parameter_set_s *pps, slice_header_s *slh);
    bool seq_parameter_set_svc_extension_rbsp();
    bool IsLayerBoundary_SVC(slice_header_s *slhold, slice_header_s *slhnew);
    bool BeginPicture_SVC(VkParserPictureData *pnvpd);
    void EndPicture_SVC();
    void gaps_in_frame_num_SVC();
    void picture_numbers_SVC(dependency_data_s *dd, dependency_state_s *ds);
    void decoded_reference_picture_marking_SVC(dependency_data_s *dd, dependency_state_s *ds);
    void sliding_window_ref_pic_marking(dependency_data_s *dd, dependency_state_s *ds);
    void adaptive_ref_base_pic_marking(dependency_data_s *dd, dependency_state_s *ds);
    void adaptive_ref_pic_marking(dependency_data_s *dd, dependency_state_s *ds);
    // DPB for SVC
    int dpb_fullness_SVC(dependency_state_s *ds);
    bool dpb_full_SVC(dependency_data_s *dd, dependency_state_s *ds);
    bool dpb_empty_SVC(dependency_state_s *ds) { return (dpb_fullness_SVC(ds) == 0); }
    void dpb_bumping_SVC(dependency_state_s *ds);
    void flush_dpb_SVC(dependency_state_s *ds);
    void output_order_dpb_SVC(bool is_target_dep, dependency_data_s *dd, dependency_state_s *ds);
    void picture_order_count_SVC(dependency_data_s *dd, dependency_state_s *ds);
    void picture_order_count_type_0_SVC(dependency_data_s *dd, dependency_state_s *ds);
    void picture_order_count_type_1_SVC(dependency_data_s *dd, dependency_state_s *ds);
    void picture_order_count_type_2_SVC(dependency_data_s *dd, dependency_state_s *ds);
    void output_picture_SVC(VkPicIf *, int);
    bool init_sequence_svc(const seq_parameter_set_s* sps);

protected:
    // Decoder
    void reference_picture_list_initialization_P_frame(int8_t *RefPicList0, const DPBPicNum *picnums, const VkParserPictureData *pd);
    void reference_picture_list_initialization_P_field(int8_t *RefPicList0, const DPBPicNum *picnums, const VkParserPictureData *pd);
    void reference_picture_list_initialization_B_frame(int8_t *RefPicList0, int8_t *RefPicList1, const DPBPicNum *picnums, const VkParserPictureData *pd);
    void reference_picture_list_initialization_B_field(int8_t *RefPicList0, int8_t *RefPicList1, const DPBPicNum *picnums, const VkParserPictureData *pd);
    int reference_picture_list_initialization_field(int8_t *refFrameListXShortTerm, int8_t *refFrameListLongTerm, int ksmax, int klmax, int8_t *RefPicListX, const VkParserPictureData *pd);
    int reference_picture_list_initialization_field_ListX(int8_t *refFrameListX, int kfmax, int kmin, int8_t *RefPicListX, const VkParserPictureData *pd);
    int sort_list_descending(int8_t *RefPicListX, int kmin, int n, const DPBPicNum *picnums, const VkParserPictureData *pd, PFNSORTCHECK sort_check);
    int sort_list_ascending(int8_t *RefPicListX, int kmin, int n, const DPBPicNum *picnums, const VkParserPictureData *pd, PFNSORTCHECK sort_check);
    void reference_picture_list_reordering_LX(int8_t *RefPicListX, int num_ref_idx_minus1, const ref_pic_list_reordering_s *rpl, const DPBPicNum *picnums, const VkParserPictureData *pd);

private:
    // Parser state (unused by slice-level decoding)
    H264ParserData *m_pParserData;
    int32_t m_MaxDpbSize;
    int MaxLongTermFrameIdx;
    int PrevRefFrameNum;
    int prevPicOrderCntMsb, prevPicOrderCntLsb;
    int prevFrameNumOffset, prevFrameNum;
    int iCur;
    int picture_started;
    uint32_t m_intra_pic_flag : 1;
    uint32_t m_idr_found_flag : 1;   // true in steady-state once we found an IDR picture
    uint32_t m_aso : 1;                 // true if ASO detected in current picture
    uint32_t m_prefix_nalu_valid : 1;
    int m_last_sps_id;
    int m_last_sei_pic_struct;
    int m_last_primary_pic_type;
    int m_first_mb_in_slice;
    dpb_entry_s *cur;
    dpb_entry_s dpb[MAX_DPB_SIZE+1];
    slice_header_s m_slh;       // 1st slice header of current picture
    VkSharedBaseObj<seq_parameter_set_s> m_sps;  // active sps
    VkSharedBaseObj<pic_parameter_set_s> m_pps;  // active pps
    seq_parameter_set_mvc_extension_s *m_spsme; // active spsme
    VkSharedBaseObj<seq_parameter_set_s> m_spss[MAX_NUM_SPS];
    seq_parameter_set_mvc_extension_s *m_spsmes[MAX_NUM_SPS];
    VkSharedBaseObj<seq_parameter_set_s> m_spssvcs[MAX_NUM_SPS];
    VkSharedBaseObj<pic_parameter_set_s> m_ppss[MAX_NUM_PPS];
    frame_packing_arrangement_s m_fpa; // Stereo SEI
    nalu_header_extension_u m_nhe;  // current nal ubit header extension
    // use MVC decoder
    bool m_bUseMVC;
    int m_prevPicOrderCntMsb, m_prevPicOrderCntLsb;
    int m_prevFrameNumOffset, m_prevFrameNum;
    int m_PrevRefFrameNum, m_PrevViewId;
    int m_MaxRefFramesPerView;
    VkPicIf *CurrFrmViewPic[1024];    // frame buffer for all views of the current frame
    // use SVC decoder
    bool m_bUseSVC;
    bool m_bLayerFirstSlice;
    uint32_t m_iDQIdMax;
    prefix_nal_unit_svc_s m_prefix_nal_unit_svc;
    slice_header_s m_slh_prev;
    layer_data_s m_layer_data[8 * 16];
    dependency_state_s m_dependency_state[8];
    dependency_state_s *m_ds; // current dependency state
    dependency_data_s m_dependency_data[8];
    dependency_data_s *m_dd; // current dependency data
    slice_group_map_s* m_slice_group_map; // [pps_id] (base layer only)
};

#endif // _VULKANH264DECODER_H_
