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

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// H.265 elementary stream parser (picture & sequence layer)
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

#include <limits>
#include "vkvideo_parser/VulkanVideoParserIf.h"
#include "nvVulkanh265ScalingList.h"
#include "VulkanH265Decoder.h"
#include "nvVulkanVideoUtils.h"

static inline int CeilLog2(int n) { return (n > 0) ? Log2U31(n-1) : 0; }

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Construction/Destruction
//

VulkanH265Decoder::VulkanH265Decoder(VkVideoCodecOperationFlagBitsKHR std)
    : VulkanVideoDecoder(std), m_pParserData()
{
    m_lMinBytesForBoundaryDetection = 16;
    m_dpb_cur = NULL;
    m_current_dpb_id = -1;
    memset(&m_dpb, 0, sizeof(m_dpb));
    m_display = NULL;
}


VulkanH265Decoder::~VulkanH265Decoder()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Initialization
//

void VulkanH265Decoder::CreatePrivateContext()
{
    m_pParserData = new H265ParserData();
}

void VulkanH265Decoder::FreeContext()
{
    delete m_pParserData;
    m_pParserData = NULL;
}

void VulkanH265Decoder::InitParser()
{
    m_MaxDpbSize = 0;
    m_bPictureStarted = false;
    m_bEmulBytesPresent = true;
    m_bNoStartCodes = false;
    m_nuh_layer_id = 0;
    m_max_dec_pic_buffering = 0;
    EndOfStream();
}

void VulkanH265Decoder::EndOfStream()
{
    flush_decoded_picture_buffer();
    memset(&m_slh, 0, sizeof(m_slh));

    for (uint32_t i = 0; i < sizeof (m_vpss) / sizeof (m_vpss[0]); i++) {
        m_vpss[i] = nullptr;
    }

    for (uint32_t i = 0; i < sizeof (m_spss) / sizeof (m_spss[0]); i++) {
        m_spss[i] = nullptr;
    }

    for (uint32_t i = 0; i < sizeof (m_ppss) / sizeof (m_ppss[0]); i++) {
        m_ppss[i] = nullptr;
    }

    for (uint32_t i = 0; i < sizeof (m_active_sps) / sizeof (m_active_sps[0]); i++) {
        m_active_sps[i] = nullptr;
    }

    for (uint32_t i = 0; i < sizeof (m_active_pps) / sizeof (m_active_pps[0]); i++) {
       m_active_pps[i] = nullptr;
    }

    m_active_vps = nullptr;

    memset(&m_dpb, 0, sizeof(m_dpb));
    m_dpb_cur = NULL;
    m_current_dpb_id = -1;
    m_bPictureStarted = false;
    m_prevPicOrderCntMsb = 0;
    m_prevPicOrderCntLsb = -1;
    m_display = NULL;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Top-level parser
//

bool VulkanH265Decoder::BeginPicture(VkParserPictureData *pnvpd)
{
    hevc_dpb_entry_s *cur = m_dpb_cur;
    int8_t current_dpb_id = m_current_dpb_id;
    VkParserHevcPictureData * const hevc = &pnvpd->CodecSpecific.hevc;
    const hevc_seq_param_s * const sps = m_active_sps[m_nuh_layer_id];
    assert(sps);
    const hevc_pic_param_s * const pps = m_active_pps[m_nuh_layer_id];
    assert(pps);
    const hevc_video_param_s * const vps = m_active_vps;
    // It is possible VPS not to be available with some malformed video content
    // assert(vps);

    if ((!m_bPictureStarted) || (!cur))
    {
        return false;
    }
    // Common fields
    assert((current_dpb_id >= 0) && (current_dpb_id < HEVC_DPB_SIZE));

    pnvpd->PicWidthInMbs = (sps->pic_width_in_luma_samples + 0xf) >> 4;
    pnvpd->FrameHeightInMbs = (sps->pic_height_in_luma_samples + 0xf) >> 4;
    pnvpd->pCurrPic = cur->pPicBuf;
    pnvpd->current_dpb_id = current_dpb_id;
    pnvpd->field_pic_flag = 0;
    pnvpd->bottom_field_flag = 0;
    pnvpd->second_field = 0;
    pnvpd->progressive_frame = 1;
    pnvpd->top_field_first = 0;
    pnvpd->repeat_first_field = 0;
    pnvpd->ref_pic_flag = 1; // We enforce the ref_pic_flag setting in Vulkan Video..
    pnvpd->intra_pic_flag = m_intra_pic_flag;
    pnvpd->chroma_format = sps->chroma_format_idc;
    pnvpd->picture_order_count = cur->PicOrderCntVal << 1;

    hevc->ProfileLevel = sps->stdProfileTierLevel.general_profile_idc;
    hevc->ColorPrimaries = sps->stdVui.colour_primaries;

    // VPS
    hevc->pStdVps = vps;

    // SPS
    hevc->pStdSps = sps;

    // PPS
    assert(sps->sps_seq_parameter_set_id == pps->pps_seq_parameter_set_id);
    hevc->pStdPps = pps;

    hevc->pic_parameter_set_id       = m_slh.pic_parameter_set_id;      // PPS ID
    hevc->seq_parameter_set_id       = sps->sps_seq_parameter_set_id;   // SPS ID
    // It is possible VPS not to be available with some malformed video content
    hevc->vps_video_parameter_set_id = vps ? vps->vps_video_parameter_set_id : 0;  // VPS ID

    assert(hevc->pic_parameter_set_id == pps->pps_pic_parameter_set_id);
    assert(hevc->vps_video_parameter_set_id == pps->sps_video_parameter_set_id);
    assert(hevc->vps_video_parameter_set_id == sps->sps_video_parameter_set_id);

    hevc->IrapPicFlag = m_slh.nal_unit_type >= NUT_BLA_W_LP && m_slh.nal_unit_type <= NUT_CRA_NUT;
    hevc->IdrPicFlag = m_slh.nal_unit_type == NUT_IDR_W_RADL || m_slh.nal_unit_type == NUT_IDR_N_LP;
    hevc->short_term_ref_pic_set_sps_flag = m_slh.short_term_ref_pic_set_sps_flag;

    // ref pic sets
    hevc->CurrPicOrderCntVal = cur->PicOrderCntVal;
    hevc->NumBitsForShortTermRPSInSlice = m_NumBitsForShortTermRPSInSlice;
    hevc->NumDeltaPocsOfRefRpsIdx = m_NumDeltaPocsOfRefRpsIdx;
    hevc->NumPocTotalCurr = m_NumPocTotalCurr;
    hevc->NumPocStCurrBefore = m_NumPocStCurrBefore;
    hevc->NumPocStCurrAfter = m_NumPocStCurrAfter;
    hevc->NumPocLtCurr = m_NumPocLtCurr;
    hevc->NumActiveRefLayerPics0 = m_NumActiveRefLayerPics0;
    hevc->NumActiveRefLayerPics1 = m_NumActiveRefLayerPics1;

    for (int32_t i = 0; i < hevc->NumPocStCurrBefore; i++) {
        hevc->RefPicSetStCurrBefore[i] = m_RefPicSetStCurrBefore[i];
    }
    for (int32_t i = 0; i < hevc->NumPocStCurrAfter; i++) {
        hevc->RefPicSetStCurrAfter[i] = m_RefPicSetStCurrAfter[i];
    }
    for (int32_t i = 0; i < hevc->NumPocLtCurr; i++) {
        hevc->RefPicSetLtCurr[i] = m_RefPicSetLtCurr[i];
    }
    for (int32_t i = 0; i < hevc->NumActiveRefLayerPics0; i++) {
        hevc->RefPicSetInterLayer0[i] = m_RefPicSetInterLayer0[i];
    }
    for (int32_t i = 0; i < hevc->NumActiveRefLayerPics1; i++) {
        hevc->RefPicSetInterLayer1[i] = m_RefPicSetInterLayer1[i];
    }
    for (int32_t i = 0; i < m_MaxDpbSize; i++) {
        hevc->IsLongTerm[i] = (m_dpb[i].marking == 2);
        if (m_dpb[i].marking) {
            hevc->PicOrderCntVal[i] = m_dpb[i].PicOrderCntVal;
            hevc->RefPics[i] = m_dpb[i].pPicBuf;
        }
    }

    // MV-HEVC related fields
    if (m_nuh_layer_id > 0)
    {
        hevc->mv_hevc_enable = 1;
        hevc->nuh_layer_id = m_nuh_layer_id;
        hevc->default_ref_layers_active_flag = vps->privFlags.default_ref_layers_active_flag;
        hevc->NumDirectRefLayers = vps->numDirectRefLayers[m_nuh_layer_id];
        hevc->max_one_active_ref_layer_flag = vps->privFlags.max_one_active_ref_layer_flag;
        hevc->poc_lsb_not_present_flag = vps->poc_lsb_not_present_flag[vps->LayerIdxInVps[m_nuh_layer_id]];
    }

    return true;
}


void VulkanH265Decoder::EndPicture()
{
    dpb_picture_end();
}


bool VulkanH265Decoder::IsPictureBoundary(int32_t rbsp_size)
{
    int nal_unit_type, nuh_temporal_id_plus1;

    if (rbsp_size < 2) {
        return false;
    }
    nal_unit_type = u(1+6);         // forbidden_zero_bit, nal_unit_type
    u(6); // nuh_layer_id
    nuh_temporal_id_plus1 = u(3); // nuh_temporal_id_plus1
    // ignore invalid NALSs (TBD: maybe should be treated as picture boundaries ?)
    if ((nal_unit_type > 0x3f) || (nuh_temporal_id_plus1 > 0x7) || (nuh_temporal_id_plus1 <= 0))
        return false;
    // 7.4.1.4.3
    if (((nal_unit_type >= NUT_VPS_NUT) && (nal_unit_type <= NUT_EOB_NUT))
     || ((nal_unit_type >= 41) && (nal_unit_type <= 47)))    // NUT_RSV_NVCL41..47
        return true;
    // If we get a slice layer rbsp, return a boundary
    if ((nal_unit_type >= NUT_TRAIL_N && nal_unit_type <= NUT_RASL_R) || (nal_unit_type >= NUT_BLA_W_LP && nal_unit_type <= NUT_CRA_NUT))
    {
        if (m_bPictureStarted && (nal_unit_type != m_slh.nal_unit_type)) {
            return true;
        }
        // first_slice_in_pic
        return u(1) ? true : false;
    }
    // Currently always treat non-slice NALs as not picture boundaries (TBD: may not be the right thing to do)
    return false;
}


int32_t VulkanH265Decoder::ParseNalUnit()
{
    int retval = NALU_DISCARD;
    int nal_unit_type, nuh_temporal_id_plus1, nuh_layer_id;

    nal_unit_type  = u(1+6); // forbidden_zero_bit, nal_unit_type
    // The value of nuh_layer_id shall be the same for all VCL NAL units of a coded picture.
    nuh_layer_id = u(6);   // nuh_layer_id
    nuh_temporal_id_plus1 = u(3);
    if ((nal_unit_type > 0x3f) || (nuh_temporal_id_plus1 > 0x7) || (nuh_temporal_id_plus1 <= 0))
    {
        nvParserLog("Invalid NAL unit header\n");
        return NALU_DISCARD;
    }
    // Early exit for the reserved and unknown nal units types
    if ((nal_unit_type > NUT_RASL_R && nal_unit_type < NUT_BLA_W_LP) || 
        (nal_unit_type > NUT_CRA_NUT && nal_unit_type < NUT_VPS_NUT) || 
        (nal_unit_type > NUT_SUFFIX_SEI_NUT))
    {
        nvParserLog("Discarding NAL unit type %d\n", nal_unit_type);
        return NALU_DISCARD;
    }
    m_nuh_layer_id = nuh_layer_id;
    switch(nal_unit_type)
    {
    case NUT_SPS_NUT:
        seq_parameter_set_rbsp();
        break;
    case NUT_PPS_NUT:
        pic_parameter_set_rbsp();
        break;
    case NUT_VPS_NUT:
        video_parameter_set_rbsp();
        break;
    case NUT_PREFIX_SEI_NUT:
    case NUT_SUFFIX_SEI_NUT:
        sei_payload();
        break;
    default:
        if ((nal_unit_type >= NUT_TRAIL_N && nal_unit_type <= NUT_RASL_R) || (nal_unit_type >= NUT_BLA_W_LP && nal_unit_type <= NUT_CRA_NUT))
        {
            // slice_layer_rbsp
            if (slice_header(nal_unit_type, nuh_temporal_id_plus1))
            {
                if (!m_bPictureStarted) // 1st slice - can't rely on first_slice_segment_in_pic_flag if there are data drops
                {
                    int discontinuity = false;
                    bool isIrapPic = (nal_unit_type >= NUT_BLA_W_LP && nal_unit_type <= 23);
                    hevc_slice_header_s *slh = &m_slh;
                    VkSharedBaseObj<hevc_pic_param_s>& pps = m_ppss[slh->pic_parameter_set_id];
                    VkSharedBaseObj<hevc_seq_param_s>& sps = m_spss[pps->pps_seq_parameter_set_id];

                    if (m_vpss[sps->sps_video_parameter_set_id] != nullptr) {
                        m_active_vps = m_vpss[sps->sps_video_parameter_set_id];
                    }

                    if (isIrapPic) {
                        NoRaslOutputFlag = (nal_unit_type <= NUT_IDR_N_LP); // BLA or IDR
                    }

                    StdVideoH265SequenceParameterSet* p_active_sps(*m_active_sps[m_nuh_layer_id]);
                    if (!m_active_sps[m_nuh_layer_id] ||
                        (sps->pic_width_in_luma_samples != p_active_sps->pic_width_in_luma_samples) ||
                        (sps->pic_height_in_luma_samples != p_active_sps->pic_height_in_luma_samples) ||
                        (sps->chroma_format_idc != p_active_sps->chroma_format_idc) ||
                        (sps->bit_depth_luma_minus8 != p_active_sps->bit_depth_luma_minus8) ||
                        (sps->bit_depth_chroma_minus8 != p_active_sps->bit_depth_chroma_minus8)) {
                        NoRaslOutputFlag = 1; // first picture in sequence
                        discontinuity = true;
                    }

                    if ((isIrapPic && NoRaslOutputFlag) || (discontinuity) || (!m_MaxDpbSize)) {
                        int NoOutputOfPriorPicsFlag = (slh->nal_unit_type == NUT_CRA_NUT) ? 1 : slh->no_output_of_prior_pics_flag;
                        if (m_nuh_layer_id == 0) {
                            flush_decoded_picture_buffer(NoOutputOfPriorPicsFlag);
                        }
                        if (!dpb_sequence_start(sps)) {
                            return NALU_DISCARD;
                        }
                    }
                    else if (!m_active_sps[m_nuh_layer_id] || (pps->pps_seq_parameter_set_id != p_active_sps->sps_seq_parameter_set_id))
                    {
                        // TBD: Could this be legal if different SPS are compatible ?
                        nvParserLog("Invalid SPS change at non-IDR\n");
                        return NALU_DISCARD;
                    }
                    m_NumBitsForShortTermRPSInSlice = slh->NumBitsForShortTermRPSInSlice;
                    // When the VPS parameters are available the stdDecPicBufMgr.max_dec_pic_buffering_minus1[0]
                    // for the layers are not always set, based on h.265 spec version used in the content.
                    // Therefore set the m_max_dec_pic_buffering to be the max of the vps and sps buffering.
                    uint8_t vps_max_dec_pic_buffering = 1;
                    if (m_active_vps && m_vpss[sps->sps_video_parameter_set_id]) {
                        vps_max_dec_pic_buffering = (m_active_vps->vps_max_layers_minus1 + 1) * (m_active_vps->stdDecPicBufMgr.max_dec_pic_buffering_minus1[0] + 1);
                    }
                    m_max_dec_pic_buffering = std::max(sps->max_dec_pic_buffering, vps_max_dec_pic_buffering);

                    dpb_picture_start(pps, slh);
                    m_intra_pic_flag = 1; // updated further down
                }
                else
                {
                    if (!m_active_pps[m_nuh_layer_id] ||
                            (m_slh.pic_parameter_set_id != m_active_pps[m_nuh_layer_id]->pps_pic_parameter_set_id))
                    {
                        nvParserLog("Invalid PPS change at non-IDR\n");
                        return NALU_DISCARD;
                    }
                }
                m_intra_pic_flag &= (m_slh.slice_type == SLICE_TYPE_I);
                retval = NALU_SLICE;
            }
        }
        else
        {
            nvParserLog("Ignoring nal_unit_type=%d, nuh_temporal_id_plus1=%d\n", nal_unit_type, nuh_temporal_id_plus1);
        }
        break;
    }
    return retval;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Sequence / Picture layer
//

void VulkanH265Decoder::seq_parameter_set_rbsp()
{

    VkSharedBaseObj<hevc_seq_param_s> sps;
    VkResult result = hevc_seq_param_s::Create(0, sps);
    assert((result == VK_SUCCESS) && sps);
    if (result != VK_SUCCESS) {
        return;
    }

    sps->sps_video_parameter_set_id = (uint8_t)u(4);
    const hevc_video_param_s* vps = m_vpss[sps->sps_video_parameter_set_id];

    if ((m_nuh_layer_id > 0) && (vps == NULL)) {
        return;
    }

    bool MultiLayerExtSpsFlag = false;
    if (m_nuh_layer_id == 0) {
        sps->sps_max_sub_layers_minus1 = (uint8_t)u(3);
    } else {
        uint8_t tmp = (uint8_t)u(3);
        MultiLayerExtSpsFlag = (m_nuh_layer_id != 0) && (tmp == 7);
        sps->sps_max_sub_layers_minus1 = (tmp == 7 ? vps->vps_max_sub_layers_minus1 : tmp);
    }

    if (sps->sps_max_sub_layers_minus1 >= MAX_NUM_SUB_LAYERS) { // fatal
        assert(!"Too many layers");
        return;
    }

    if (!MultiLayerExtSpsFlag) {
        sps->flags.sps_temporal_id_nesting_flag = u(1);
        if ((!sps->sps_max_sub_layers_minus1) && (sps->flags.sps_temporal_id_nesting_flag != true)) {
            return;
        }
        sps->pProfileTierLevel = profile_tier_level(&sps->stdProfileTierLevel, sps->sps_max_sub_layers_minus1);
    }
    uint8_t seq_parameter_set_id = ue();
    bool sps_error = false;
    sps_error |= (seq_parameter_set_id >= MAX_NUM_SPS);
    sps->sps_seq_parameter_set_id = (uint8_t)seq_parameter_set_id;

    if (MultiLayerExtSpsFlag) {
        if (u(1)) { // update_rep_format_flag
            sps->sps_rep_format_idx = u(8);
        } else {
            sps->sps_rep_format_idx = vps->vps_rep_format_idx[vps->LayerIdxInVps[m_nuh_layer_id]];
        }
        if (sps->sps_rep_format_idx > 63) {
            return;
        }
        sps->chroma_format_idc          = (StdVideoH265ChromaFormatIdc)vps->repFormat[sps->sps_rep_format_idx].chroma_format_vps_idc; // PIERS: Is this cast okay?
        sps->pic_width_in_luma_samples  = vps->repFormat[sps->sps_rep_format_idx].pic_width_vps_in_luma_samples;
        sps->pic_height_in_luma_samples = vps->repFormat[sps->sps_rep_format_idx].pic_height_vps_in_luma_samples;
        sps->conf_win_left_offset       = vps->repFormat[sps->sps_rep_format_idx].conf_win_vps_left_offset;
        sps->conf_win_right_offset      = vps->repFormat[sps->sps_rep_format_idx].conf_win_vps_right_offset;
        sps->conf_win_top_offset        = vps->repFormat[sps->sps_rep_format_idx].conf_win_vps_top_offset;
        sps->conf_win_bottom_offset     = vps->repFormat[sps->sps_rep_format_idx].conf_win_vps_bottom_offset;
        sps->bit_depth_luma_minus8      = vps->repFormat[sps->sps_rep_format_idx].bit_depth_vps_luma_minus8;
        sps->bit_depth_chroma_minus8    = vps->repFormat[sps->sps_rep_format_idx].bit_depth_vps_chroma_minus8;
    } else {
        uint8_t chroma_format_idc = (uint8_t)ue();
        sps_error |= (chroma_format_idc > 3);
        sps->chroma_format_idc = (StdVideoH265ChromaFormatIdc)chroma_format_idc;
        if (sps->chroma_format_idc == 3) {
            sps->flags.separate_colour_plane_flag = u(1);
        }
        sps->pic_width_in_luma_samples = ue();
        sps->pic_height_in_luma_samples = ue();
        sps_error |= !!((sps->pic_width_in_luma_samples | sps->pic_height_in_luma_samples) & ((unsigned)~0 << 16));
        if (u(1)) { // pic_cropping_flag

            uint32_t conf_win_left_offset = ue();
            uint32_t conf_win_right_offset = ue();
            uint32_t conf_win_top_offset = ue();
            uint32_t conf_win_bottom_offset = ue();
            uint32_t ChromaArrayType = (sps->flags.separate_colour_plane_flag) ? 0 : sps->chroma_format_idc;
            uint32_t pic_width_in_crop_samples = sps->pic_width_in_luma_samples >> (((ChromaArrayType == 1) || (ChromaArrayType == 2)) ? 1:0);
            uint32_t pic_height_in_crop_samples = sps->pic_height_in_luma_samples >> ((ChromaArrayType == 1) ? 1:0);

            if (((conf_win_left_offset | conf_win_right_offset) < 256)
             && (conf_win_left_offset + conf_win_right_offset < pic_width_in_crop_samples)) {

                sps->conf_win_left_offset = (uint8_t)conf_win_left_offset;
                sps->conf_win_right_offset = (uint8_t)conf_win_right_offset;
            }
            if (((conf_win_top_offset | conf_win_bottom_offset) < 256)
             && (conf_win_top_offset + conf_win_bottom_offset < pic_height_in_crop_samples)) {
                sps->conf_win_top_offset = (uint8_t)conf_win_top_offset;
                sps->conf_win_bottom_offset = (uint8_t)conf_win_bottom_offset;
            }
        }
        sps->bit_depth_luma_minus8 = (uint8_t)ue();
        sps->bit_depth_chroma_minus8 = (uint8_t)ue();
    }

    sps_error |= (sps->bit_depth_luma_minus8 > 6) || (sps->bit_depth_chroma_minus8 > 6);
    sps->log2_max_pic_order_cnt_lsb_minus4 = (uint8_t)ue();
    if (sps->log2_max_pic_order_cnt_lsb_minus4 > 12) {
        nvParserLog("Invalid log2_max_pic_order_cnt_lsb_minus4 (%d)\n", sps->log2_max_pic_order_cnt_lsb_minus4);
        return;
    }

    if (!MultiLayerExtSpsFlag) {
        sps->pDecPicBufMgr = &sps->stdDecPicBufMgr;
        bool sps_sub_layer_ordering_info_present_flag = u(1);
        sps->max_dec_pic_buffering = 1;
        sps->max_num_reorder_pics = 0;
        // Below 3 fields may be used to optimize decode->display latency in parser
        for (int i = sps_sub_layer_ordering_info_present_flag ? 0 : sps->sps_max_sub_layers_minus1; i <= sps->sps_max_sub_layers_minus1; i++)
        {
            sps->stdDecPicBufMgr.max_dec_pic_buffering_minus1[i] = (uint8_t)ue();
            sps->stdDecPicBufMgr.max_num_reorder_pics[i]         = (uint8_t)ue();
            sps->stdDecPicBufMgr.max_latency_increase_plus1[i]   = (uint8_t)ue();
            if (sps->stdDecPicBufMgr.max_dec_pic_buffering_minus1[i] >= sps->max_dec_pic_buffering) {
                sps->max_dec_pic_buffering = sps->stdDecPicBufMgr.max_dec_pic_buffering_minus1[i] + 1;
            }
            if (sps->stdDecPicBufMgr.max_num_reorder_pics[i] > sps->max_num_reorder_pics) {
                sps->max_num_reorder_pics = sps->stdDecPicBufMgr.max_num_reorder_pics[i];
            }
        }
    } else {

        /* always output highest layer set */
        uint32_t targetOptLayerSetIdx = vps->vps_num_layer_sets - 1;
        uint32_t layerIdx = 0;
        while (layerIdx < vps->num_layers_in_id_list[targetOptLayerSetIdx]) {
            if (vps->layer_set_layer_id_list[targetOptLayerSetIdx][layerIdx] == m_nuh_layer_id) {
                break;
            }
            layerIdx++;
        }
        for (int i = 0; i <= sps->sps_max_sub_layers_minus1; i++) {
            sps->stdDecPicBufMgr.max_dec_pic_buffering_minus1[i] = vps->max_vps_dec_pic_buffering_minus1[targetOptLayerSetIdx][layerIdx][i];
            sps->stdDecPicBufMgr.max_num_reorder_pics[i]         = vps->max_vps_num_reorder_pics[targetOptLayerSetIdx][i];
            sps->stdDecPicBufMgr.max_latency_increase_plus1[i]   = vps->max_vps_latency_increase_plus1[targetOptLayerSetIdx][i];
            if (sps->stdDecPicBufMgr.max_dec_pic_buffering_minus1[i] >= sps->max_dec_pic_buffering)
            {
                sps->max_dec_pic_buffering = sps->stdDecPicBufMgr.max_dec_pic_buffering_minus1[i] + 1;
            }
            if (sps->stdDecPicBufMgr.max_num_reorder_pics[i] > sps->max_num_reorder_pics)
            {
                sps->max_num_reorder_pics = sps->stdDecPicBufMgr.max_num_reorder_pics[i];
            }
        }
    }
    sps->log2_min_luma_coding_block_size_minus3 = (uint8_t)ue();
    sps->log2_diff_max_min_luma_coding_block_size = (uint8_t)ue();
    sps->log2_min_luma_transform_block_size_minus2 = (uint8_t)ue();
    sps->log2_diff_max_min_luma_transform_block_size = (uint8_t)ue();
    sps->max_transform_hierarchy_depth_inter = (uint8_t)ue();
    sps->max_transform_hierarchy_depth_intra = (uint8_t)ue();
    sps->flags.scaling_list_enabled_flag = u(1);
    if (sps->flags.scaling_list_enabled_flag) {
        uint32_t sps_infer_scaling_list_flag = 0;
        if (MultiLayerExtSpsFlag) {
            sps_infer_scaling_list_flag = u(1);
        }
        if (sps_infer_scaling_list_flag) {
            u(6); // sps_scaling_list_ref_layer_id
        } else {
            sps->flags.sps_scaling_list_data_present_flag = u(1);
            if (sps->flags.sps_scaling_list_data_present_flag) {
                if (!scaling_list_data(&sps->sps_scaling_list))
                    return;
            }
        }
    }
    sps->flags.amp_enabled_flag = u(1);
    sps->flags.sample_adaptive_offset_enabled_flag = u(1);
    sps->flags.pcm_enabled_flag = u(1);
    if (sps->flags.pcm_enabled_flag) {

        sps->pcm_sample_bit_depth_luma_minus1 = (uint8_t)u(4);
        sps->pcm_sample_bit_depth_chroma_minus1 = (uint8_t)u(4);
        sps->log2_min_pcm_luma_coding_block_size_minus3 = (uint8_t)ue();
        sps->log2_diff_max_min_pcm_luma_coding_block_size = (uint8_t)ue();
        sps->flags.pcm_loop_filter_disabled_flag = u(1);
        if ((sps->pcm_sample_bit_depth_luma_minus1 + 1 > sps->bit_depth_luma_minus8 + 8)
         || (sps->pcm_sample_bit_depth_chroma_minus1 + 1 > sps->bit_depth_chroma_minus8 + 8)) {
            nvParserLog("Invalid pcm_sample_bit_depth_minus1 (y:%d, uv:%d)\n", sps->pcm_sample_bit_depth_luma_minus1, sps->pcm_sample_bit_depth_chroma_minus1);
            return;
        }
    }
    uint32_t num_short_term_ref_pic_sets = ue();
    sps->num_short_term_ref_pic_sets = (uint8_t)num_short_term_ref_pic_sets;
    if (sps->num_short_term_ref_pic_sets > MAX_NUM_STRPS) { // fatal

        nvParserLog("Invalid num_short_term_ref_pic_sets (%d)\n", num_short_term_ref_pic_sets);
        return;
    }
    for (uint32_t i = 0; i < num_short_term_ref_pic_sets; i++) {
        StdVideoH265ShortTermRefPicSet* pShortTermRefPicSet =
                short_term_ref_pic_set(&sps->stdShortTermRefPicSet[i],
                                       &sps->strpss[i], sps->strpss,
                                       i, num_short_term_ref_pic_sets);
        if (pShortTermRefPicSet == nullptr) {
            nvParserLog("Invalid short_term_ref_pic_set in SPS\n");
            return;
        }
    }
    if (num_short_term_ref_pic_sets) {
        sps->pShortTermRefPicSet = sps->stdShortTermRefPicSet;
    }
    sps->flags.long_term_ref_pics_present_flag = u(1);
    if (sps->flags.long_term_ref_pics_present_flag) {
        uint32_t num_long_term_ref_pics_sps = ue();
        sps->pLongTermRefPicsSps = &sps->stdLongTermRefPicsSps;
        sps->num_long_term_ref_pics_sps = (uint8_t)num_long_term_ref_pics_sps;
        sps->stdLongTermRefPicsSps.used_by_curr_pic_lt_sps_flag = 0;
        if (num_long_term_ref_pics_sps > MAX_NUM_LTRP) { // spec constraint

            nvParserLog("Invalid num_long_term_ref_pics_sps (%d)\n", num_long_term_ref_pics_sps);
            return;
        }
        for (uint32_t i = 0; i < num_long_term_ref_pics_sps; i++) {
            sps->stdLongTermRefPicsSps.lt_ref_pic_poc_lsb_sps[i] = (uint32_t)(uint16_t)u(sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
            const bool used_by_curr_pic_lt_sps_flag = u(1);
            if (used_by_curr_pic_lt_sps_flag) {
                sps->stdLongTermRefPicsSps.used_by_curr_pic_lt_sps_flag |= (1 << i);
            }
        }
    }
    sps->flags.sps_temporal_mvp_enabled_flag = u(1);
    sps->flags.strong_intra_smoothing_enabled_flag = u(1);
    sps->flags.vui_parameters_present_flag = u(1);
    if (sps->flags.vui_parameters_present_flag) { // vui_parameters_present_flag

        vui_parameters(sps, sps->sps_max_sub_layers_minus1);
    }
    sps->flags.sps_extension_present_flag = u(1);
    if (sps->flags.sps_extension_present_flag) { // sps_extension_present_flag

        sps->flags.sps_range_extension_flag = u(1);
        bool sps_multilayer_extension_flag = u(1);
        u(6); // sps_extension_6bits
        if (sps->flags.sps_range_extension_flag) {
            sps->flags.transform_skip_rotation_enabled_flag = u(1);
            sps->flags.transform_skip_context_enabled_flag = u(1);
            sps->flags.implicit_rdpcm_enabled_flag = u(1);
            sps->flags.explicit_rdpcm_enabled_flag = u(1);
            sps->flags.extended_precision_processing_flag = u(1);
            sps->flags.intra_smoothing_disabled_flag = u(1);
            sps->flags.high_precision_offsets_enabled_flag = u(1);
            sps->flags.persistent_rice_adaptation_enabled_flag = u(1);
            sps->flags.cabac_bypass_alignment_enabled_flag = u(1);
        }
        if (sps_multilayer_extension_flag) {
            u(1); // inter_view_mv_vert_constraint_flag
        }
    }

    // Currently ignoring rbsp_trailing bits

    // Basic validation
    int log2MinCbSize, log2CtbSizeY, log2MinTrafoSize, log2MaxTrafoSize;
    log2MinCbSize = sps->log2_min_luma_coding_block_size_minus3 + 3;
    if ((sps->log2_min_luma_coding_block_size_minus3 > 12)
     || ((sps->pic_width_in_luma_samples == 0) || (sps->pic_width_in_luma_samples & ((1 << log2MinCbSize) - 1)))
     || ((sps->pic_height_in_luma_samples == 0) || (sps->pic_height_in_luma_samples & ((1 << log2MinCbSize) - 1)))) {
        nvParserLog("Invalid picture size (%dx%d, log2MinCbSize=%d)\n", sps->pic_width_in_luma_samples, sps->pic_height_in_luma_samples, log2MinCbSize);
        sps_error = 1;
    }
    log2CtbSizeY = log2MinCbSize + sps->log2_diff_max_min_luma_coding_block_size;
    if ((log2CtbSizeY < 4) || (log2CtbSizeY > 6)) {
        // Restricted from 4..6 in all defined profiles (TBD: Doesn't really matter for the parser -> should we allow it ?)
        nvParserLog("Unsupported Log2CtbSizeY (%d)\n", log2CtbSizeY);
        sps_error = 1;
    }
    log2MinTrafoSize = sps->log2_min_luma_transform_block_size_minus2 + 2;
    log2MaxTrafoSize = log2MinTrafoSize + sps->log2_diff_max_min_luma_transform_block_size;
    if (log2MinTrafoSize >= log2MinCbSize) {
        nvParserLog("Invalid Log2MinTrafoSize (%d)\n", log2MinTrafoSize);
        sps_error = 1;
    }
    if (log2MaxTrafoSize > std::min<int32_t>(log2CtbSizeY, 5)) {
        nvParserLog("Invalid Log2MaxTrafoSize (%d)\n", log2MaxTrafoSize);
        sps_error = 1;
    }
    if ((sps->max_transform_hierarchy_depth_inter > (log2CtbSizeY - log2MinTrafoSize))
     || (sps->max_transform_hierarchy_depth_intra > (log2CtbSizeY - log2MinTrafoSize))) {
        nvParserLog("Invalid max_transform_hierarchy_depth (inter:%d, intra:%d)\n", sps->max_transform_hierarchy_depth_inter, sps->max_transform_hierarchy_depth_intra);
        sps_error = 1;
    }
    if (sps_error) {
        nvParserLog("Error parsing SPS (ignored)\n");
        return;
    }

    if (sps->UpdateStdScalingList(sps, &sps->stdScalingLists)) {
        sps->pScalingLists = &sps->stdScalingLists;
    } else {
        sps->pScalingLists = NULL;
    }

    if (sps->UpdateStdVui(sps, &sps->stdVui)) {
        sps->pSequenceParameterSetVui = &sps->stdVui;
    } else {
        sps->pSequenceParameterSetVui = NULL;
    }


    if (m_outOfBandPictureParameters && m_pClient) {

        sps->SetSequenceCount(m_pParserData->spsClientUpdateCount[seq_parameter_set_id]++);
        VkSharedBaseObj<StdVideoPictureParametersSet> picParamObj(sps);
        bool success = m_pClient->UpdatePictureParameters(picParamObj, sps->client);
        assert(success);
        if (success == false) {
            nvParserErrorLog("s", "\nError Updating the h.265 SPS parameters\n");
        }
    }

    m_spss[seq_parameter_set_id] = sps;
}


void VulkanH265Decoder::pic_parameter_set_rbsp()
{
    VkSharedBaseObj<hevc_pic_param_s> pps;
    VkResult result = hevc_pic_param_s::Create(0, pps);
    assert((result == VK_SUCCESS) && pps);
    if (result != VK_SUCCESS) {
        return;
    }

    pps->flags.uniform_spacing_flag = 1;

    uint32_t pic_parameter_set_id = ue();
    uint32_t seq_parameter_set_id = ue();
    if ((pic_parameter_set_id >= MAX_NUM_PPS) || (seq_parameter_set_id >= MAX_NUM_SPS))
    {
        nvParserLog("Invalid PPS (pps_id=%d, sps_id=%d)\n", pic_parameter_set_id, seq_parameter_set_id);
        return;
    }
    pps->pps_pic_parameter_set_id = (uint8_t)pic_parameter_set_id;
    pps->pps_seq_parameter_set_id = (uint8_t)seq_parameter_set_id;
    const hevc_seq_param_s* sps = m_spss[pps->pps_seq_parameter_set_id];

    // In case we receive pps before sps, m_spss[] will return sps as NULL.
    // Setting the sps_video_parameter_set_id as 0 for this case.
    // This also implies that with h.265 we need to cache the PPS/SPS data before we get a valid VPS at the client side.
    pps->sps_video_parameter_set_id = sps ? sps->sps_video_parameter_set_id : 0; // FIXME: pick the last SPS, instead.
    pps->flags.dependent_slice_segments_enabled_flag = u(1); // name differs from spec
    pps->flags.output_flag_present_flag = u(1);
    pps->num_extra_slice_header_bits = (uint8_t)u(3);
    pps->flags.sign_data_hiding_enabled_flag = u(1);
    pps->flags.cabac_init_present_flag = u(1);
    uint32_t num_ref_idx_l0_default_active_minus1 = ue();
    uint32_t num_ref_idx_l1_default_active_minus1 = ue();
    if ((num_ref_idx_l0_default_active_minus1 > 15) || (num_ref_idx_l1_default_active_minus1 > 15)) {
        nvParserLog("Invalid num_ref_idx_lx_default_active_minus1 (l0:%d, l1:%d)\n",
            num_ref_idx_l0_default_active_minus1, num_ref_idx_l1_default_active_minus1);
        return;
    }
    pps->num_ref_idx_l0_default_active_minus1 = (uint8_t)num_ref_idx_l0_default_active_minus1;
    pps->num_ref_idx_l1_default_active_minus1 = (uint8_t)num_ref_idx_l1_default_active_minus1;
    pps->init_qp_minus26 = (int8_t)se();
    uint8_t QpBdOffsetY = 0;
    if (sps != NULL) {
        QpBdOffsetY = 6 * sps->bit_depth_luma_minus8;
    }

    if (pps->init_qp_minus26 < -(26 + QpBdOffsetY) || pps->init_qp_minus26 > 25) {
        nvParserLog("Invalid init_qp_minus26 (%d)\n", pps->init_qp_minus26);
        return;
    }
    pps->flags.constrained_intra_pred_flag = u(1);
    pps->flags.transform_skip_enabled_flag = u(1);
    pps->flags.cu_qp_delta_enabled_flag = u(1);
    if (pps->flags.cu_qp_delta_enabled_flag) {
        pps->diff_cu_qp_delta_depth = (uint8_t)ue();
    }
    pps->pps_cb_qp_offset = (int8_t)se();
    pps->pps_cr_qp_offset = (int8_t)se();
    if ((pps->pps_cb_qp_offset < -12 || pps->pps_cb_qp_offset > 12)
     || (pps->pps_cr_qp_offset < -12 || pps->pps_cr_qp_offset > 12))
    {
        nvParserLog("Invalid pps_crcb_qp_offset (cb:%d,cr:%d)\n", pps->pps_cb_qp_offset, pps->pps_cr_qp_offset);
        return;
    }
    pps->flags.pps_slice_chroma_qp_offsets_present_flag = u(1); // name differs from spec
    pps->flags.weighted_pred_flag = u(1);
    pps->flags.weighted_bipred_flag = u(1);
    pps->flags.transquant_bypass_enabled_flag = u(1);
    pps->flags.tiles_enabled_flag = u(1);
    pps->flags.entropy_coding_sync_enabled_flag = u(1);
    pps->flags.loop_filter_across_tiles_enabled_flag = 1; // default as per spec, unless explicit
    if (pps->flags.tiles_enabled_flag)
    {
        uint32_t num_tile_columns_minus1 = ue();
        uint32_t num_tile_rows_minus1 = ue();
        if ((num_tile_columns_minus1 >= MAX_NUM_TILE_COLUMNS) || (num_tile_rows_minus1 >= MAX_NUM_TILE_ROWS))
        {
            assert(!"Unsupported number of tiles in PPS");
            nvParserLog("Unsupported number of tiles in PPS: %dx%d\n", num_tile_columns_minus1, num_tile_rows_minus1);
            return;
        }
        pps->num_tile_columns_minus1 = (uint8_t)num_tile_columns_minus1;
        pps->num_tile_rows_minus1 = (uint8_t)num_tile_rows_minus1;
        pps->flags.uniform_spacing_flag = u(1);
        if (!pps->flags.uniform_spacing_flag)
        {
            assert(pps->num_tile_columns_minus1 < sizeof(pps->column_width_minus1) / sizeof(pps->column_width_minus1[0]));
            for (int i = 0; i < pps->num_tile_columns_minus1; i++) {
                pps->column_width_minus1[i] = (uint16_t)ue();
            }
            assert(pps->num_tile_rows_minus1 < sizeof(pps->row_height_minus1) / sizeof(pps->row_height_minus1[0]));
            for (int i = 0; i < pps->num_tile_rows_minus1; i++) {
                pps->row_height_minus1[i] = (uint16_t)ue();
            }
        }
        pps->flags.loop_filter_across_tiles_enabled_flag = u(1);
    }
    pps->flags.pps_loop_filter_across_slices_enabled_flag = u(1);
    pps->flags.deblocking_filter_control_present_flag = u(1);
    if (pps->flags.deblocking_filter_control_present_flag)
    {
        pps->flags.deblocking_filter_override_enabled_flag = u(1);
        pps->flags.pps_deblocking_filter_disabled_flag = u(1); // slice_disable_deblocking_filter_flag LoopFilterDisable
        if (!pps->flags.pps_deblocking_filter_disabled_flag)
        {
            int beta_offset_div2 = se();
            int tc_offset_div2 = se();
            pps->pps_beta_offset_div2 = (int8_t)beta_offset_div2;
            pps->pps_tc_offset_div2 = (int8_t)tc_offset_div2;
            if ((beta_offset_div2 < -6) || (beta_offset_div2 > 6)
             || (tc_offset_div2 < -6) || (tc_offset_div2 > 6))
            {
                nvParserLog("Invalid deblocking filter control parameters (beta=%d, tc=%d)\n", beta_offset_div2, tc_offset_div2);
                return;
            }
        }
    }
    pps->flags.pps_scaling_list_data_present_flag = u(1);
    if (pps->flags.pps_scaling_list_data_present_flag) {
        scaling_list_data(&pps->pps_scaling_list);
    }
    pps->flags.lists_modification_present_flag = u(1);
    pps->log2_parallel_merge_level_minus2 = (uint8_t)ue();
    if (pps->log2_parallel_merge_level_minus2 > 12) // TBD: What's supported worst-case without using SPS ?
    {
        nvParserLog("Invalid log2_parallel_merge_level_minus2 (%d)\n", pps->log2_parallel_merge_level_minus2);
        return;
    }
    pps->flags.slice_segment_header_extension_present_flag = u(1);
    pps->flags.pps_extension_present_flag = u(1);
    if (pps->flags.pps_extension_present_flag) {
        pps->flags.pps_range_extension_flag = u(1);
        int pps_multilayer_extension_flag = u(1);
        int pps_extension_6bits = u(6);
        UNUSED_LOCAL_VAR(pps_extension_6bits);
        if (pps->flags.pps_range_extension_flag) {
            if (pps->flags.transform_skip_enabled_flag) {
                pps->log2_max_transform_skip_block_size_minus2 = (uint8_t)ue();
            }
            pps->flags.cross_component_prediction_enabled_flag = u(1);
            pps->flags.chroma_qp_offset_list_enabled_flag = u(1);
            if (pps->flags.chroma_qp_offset_list_enabled_flag) {
                pps->diff_cu_chroma_qp_offset_depth = (uint8_t)ue();
                pps->chroma_qp_offset_list_len_minus1 = (uint8_t)ue();
                if (pps->chroma_qp_offset_list_len_minus1 > 5) {
                    assert(!"Invalid pps range extension data");
                    nvParserLog("Invalid pps range extension data\n");
                    pps->flags.chroma_qp_offset_list_enabled_flag = false;
                    pps->chroma_qp_offset_list_len_minus1 = 0;
                    pps->diff_cu_chroma_qp_offset_depth = 0;
                }
                else
                {
                    for (int i = 0; (i <= pps->chroma_qp_offset_list_len_minus1) && (i < 6); i++) {
                        pps->cb_qp_offset_list[i] = (int8_t)se();
                        pps->cr_qp_offset_list[i] = (int8_t)se();
                    }
                }
            }
            pps->log2_sao_offset_scale_luma = (uint8_t)ue();
            pps->log2_sao_offset_scale_chroma = (uint8_t)ue();
        }
        if (pps_multilayer_extension_flag) {
            u(1); // poc_reset_info_present_flag
            if (u(1)) // infer_scaling_list_flag
            {
                u(6); // scaling_list_ref_layer_id
            }
            ue(); // num_ref_loc_offsets
        }
    }
    // Currently ignoring rbsp_trailing_bits

    if (pps->UpdateStdScalingList(pps, &pps->stdScalingLists)) {
        pps->pScalingLists = &pps->stdScalingLists;
    } else {
        pps->pScalingLists = NULL;
    }

    if (m_outOfBandPictureParameters && m_pClient) {

        pps->SetSequenceCount(m_pParserData->ppsClientUpdateCount[pic_parameter_set_id]++);
        VkSharedBaseObj<StdVideoPictureParametersSet> picParamObj(pps);
        bool success = m_pClient->UpdatePictureParameters(picParamObj, pps->client);
        assert(success);
        if (success == false) {
            nvParserErrorLog("s", "\nError Updating the h.265 PPS parameters\n");
        }
    }

    m_ppss[pic_parameter_set_id] = pps;
}

/* Decode video parameter set information from the stream. */
void VulkanH265Decoder::video_parameter_set_rbsp()
{
    uint32_t vps_video_parameter_set_id = u(4);
    if (vps_video_parameter_set_id >= MAX_NUM_VPS)
    {
        nvParserLog("Invalid VPS ID (vps_id = %d)\n", vps_video_parameter_set_id);
        return;
    }

    VkSharedBaseObj<hevc_video_param_s> vps;
    VkResult result = hevc_video_param_s::Create(0, vps);
    assert((result == VK_SUCCESS) && vps);
    if (result != VK_SUCCESS) {
        return;
    }

    // vps base
    vps->vps_video_parameter_set_id    = vps_video_parameter_set_id;
    vps->privFlags.vps_base_layer_internal_flag  = u(1);
    vps->privFlags.vps_base_layer_available_flag = u(1);
    uint32_t tmp = u(6);
    vps->vps_max_layers_minus1         = std::min<int32_t>(tmp, (uint32_t)(MAX_NUM_LAYER_IDS - 2));
    vps->vps_max_sub_layers_minus1     = u(3);
    vps->flags.vps_temporal_id_nesting_flag  = u(1);

    tmp = u(16);
    if (tmp != 0xFFFF)
    {
        nvParserLog("Invalid VPS (vps_id = %d)\n", vps_video_parameter_set_id);
        return;
    }

    vps->pProfileTierLevel = profile_tier_level(&vps->stdProfileTierLevel,
                                                           vps->vps_max_sub_layers_minus1);

    vps->flags.vps_sub_layer_ordering_info_present_flag =  u(1);
    for (uint32_t i = (vps->flags.vps_sub_layer_ordering_info_present_flag ? 0 : vps->vps_max_sub_layers_minus1);
            i <= vps->vps_max_sub_layers_minus1; i++)
    {
        vps->stdDecPicBufMgr.max_dec_pic_buffering_minus1[i] = ue();
        if (vps->stdDecPicBufMgr.max_dec_pic_buffering_minus1[i] >= HEVC_DPB_SIZE)
        {
            nvParserLog("Invalid vps parameter (vps_max_dec_pic_buffering_minus1=%d)\n", vps->stdDecPicBufMgr.max_dec_pic_buffering_minus1[i]);
            return;
        }

        vps->stdDecPicBufMgr.max_num_reorder_pics[i]         = ue();
        if (vps->stdDecPicBufMgr.max_num_reorder_pics[i] > vps->stdDecPicBufMgr.max_dec_pic_buffering_minus1[i])
        {
            nvParserLog("Invalid vps parameter (vps_max_num_reorder_pics=%d)\n", vps->stdDecPicBufMgr.max_num_reorder_pics[i]);
            return;
        }
        vps->stdDecPicBufMgr.max_latency_increase_plus1[i]   = ue();
    }

    if (vps->vps_max_sub_layers_minus1 != 0) {
        vps->pDecPicBufMgr = &vps->stdDecPicBufMgr;
    }

    vps->vps_max_layer_id = u(6);
    vps->vps_num_layer_sets = ue() + 1;
    if (vps->vps_num_layer_sets > 1024)
    {
        nvParserLog("Invalid Invalid vps parameter (vps_num_layer_sets=%d)\n", vps->vps_num_layer_sets);
        return;
    }

    for (uint32_t i = 1; i <= vps->vps_num_layer_sets - 1; i++)
    {
        for (uint32_t j = 0; j <= vps->vps_max_layer_id; j++) {
            vps->layer_id_included_flag[i][j] = u(1);
        }
    }
    for (uint32_t i = 1; i <= vps->vps_num_layer_sets - 1; i++)
    {
        uint32_t n = 0;
        for (uint32_t m = 0; m <= vps->vps_max_layer_id; m++)
        {
            if (vps->layer_id_included_flag[i][m])
                vps->layer_set_layer_id_list[i][n++] = m;
        }
        vps->num_layers_in_id_list[i] = n;
    }

    // HRD related
    vps->flags.vps_timing_info_present_flag = u(1);
    if (vps->flags.vps_timing_info_present_flag != 0)
    {
        vps->vps_num_units_in_tick = u(16);
        vps->vps_num_units_in_tick <<= 16;
        vps->vps_num_units_in_tick += u(16);
        vps->vps_time_scale = u(16);
        vps->vps_time_scale <<= 16;
        vps->vps_time_scale += u(16);

        vps->flags.vps_poc_proportional_to_timing_flag = u(1);
        if (vps->flags.vps_poc_proportional_to_timing_flag)
        {
            vps->vps_num_ticks_poc_diff_one_minus1 = ue();
        }
        else
        {
            vps->vps_num_ticks_poc_diff_one_minus1 = 0;
        }
        vps->vps_num_hrd_parameters = ue();
        if (vps->vps_num_hrd_parameters > vps->vps_num_layer_sets)
        {
            nvParserLog("Invalid Invalid vps parameter (vps_num_hrd_parameters=%d)\n", vps->vps_num_hrd_parameters);
            return;
        }

        hevc_video_hrd_param_s* pHrdParameters = nullptr;
        if (vps->vps_num_hrd_parameters) {
            // vps->stdHrdParameters.reset(new hevc_video_hrd_param_s[vps->vps_num_hrd_parameters]());
            auto deleter = [](hevc_video_hrd_param_s* p) { delete[] p; };
            vps->stdHrdParameters.reset(new hevc_video_hrd_param_s[vps->vps_num_hrd_parameters], deleter);
            if (vps->stdHrdParameters) {
                pHrdParameters = vps->stdHrdParameters.get();
                vps->pHrdParameters = pHrdParameters;
            } else {
                nvParserErrorLog("Couldn't allocate memory for stdHrdParameters[] with size %d\n", vps->vps_num_hrd_parameters);
                return;
            }
        }

        for (uint32_t i = 0; i < vps->vps_num_hrd_parameters; i++) { // pVideoParamSet->vps_num_hrd_parameters <= 1024

            assert(pHrdParameters);
            vps->hrd_layer_set_idx[i] = ue();

            if ((vps->hrd_layer_set_idx[i] >= vps->vps_num_layer_sets) ||
                (vps->hrd_layer_set_idx[i] < (uint32_t) (vps->privFlags.vps_base_layer_internal_flag ? 0 : 1))) {

                nvParserLog("Invalid Invalid vps parameter (hrd_layer_set_idx=%d)\n", vps->hrd_layer_set_idx[i]);
                return;
            }

            if (i > 0) {
                vps->cprms_present_flag[i] = u(1);
            }

            hrd_parameters(&pHrdParameters[i], vps->cprms_present_flag[i],
                            vps->vps_max_sub_layers_minus1);
        }
        // hevcDecodeHrdParameters(&pVideoParamSet->hrdParameters[i], 1); // CMOD function
    }

    uint32_t hevc_spec_version = 0;
    if (vps->vps_max_layers_minus1 == 0)
    {
        hevc_spec_version = 201304;
    }

    if (hevc_spec_version != 201304)
    {
        vps->privFlags.vps_extension_flag = u(1);
        if (vps->privFlags.vps_extension_flag != 0)
        {
            // vps_extension_alignment_bit_equal_to_one
            while (!byte_aligned())
                u(1); // vps_extension_alignment_bit_equal_to_one
            video_parameter_set_rbspExtension(vps);
        }
    }

    if (m_outOfBandPictureParameters && m_pClient) {

        vps->SetSequenceCount(m_pParserData->vpsClientUpdateCount[vps_video_parameter_set_id]++);
        VkSharedBaseObj<StdVideoPictureParametersSet> picParamObj(vps);
        bool success = m_pClient->UpdatePictureParameters(picParamObj, vps->client);
        assert(success);
        if (success == false) {
            nvParserErrorLog("s", "\nError Updating the h.265 VPS parameters\n");
        }
    }

    m_vpss[vps_video_parameter_set_id] = vps;

    return;
} // video_parameter_set_rbsp()


/* Decode video parameter set extension information from the stream. */
void VulkanH265Decoder::video_parameter_set_rbspExtension(hevc_video_param_s *pVideoParamSet)
{
    uint32_t i, j, k;

    if ((pVideoParamSet->vps_max_layers_minus1 > 0) &&
        (pVideoParamSet->privFlags.vps_base_layer_internal_flag != 0))
    {
        pVideoParamSet->pProfileTierLevel = profile_tier_level(&pVideoParamSet->stdProfileTierLevel,
                                                               pVideoParamSet->vps_max_sub_layers_minus1, 0);
    }

    pVideoParamSet->privFlags.splitting_flag = u(1);

    //////////////////////////////////////////////////////
    /* Layer and nuh_layer_id info */
    //////////////////////////////////////////////////////
    pVideoParamSet->numScalabilityTypes = 0;
    for (i = 0; i < MAX_NUM_SCALABILITY_TYPES; i++)
    {
        pVideoParamSet->scalability_mask_flag[i] = u(1);
        pVideoParamSet->numScalabilityTypes += pVideoParamSet->scalability_mask_flag[i];
    }

    for (i = 0; i < ( pVideoParamSet->numScalabilityTypes - pVideoParamSet->privFlags.splitting_flag); i++)
    {
        pVideoParamSet->dimension_id_len[i] = (uint8_t)(u(3) + 1);
    }
    if (pVideoParamSet->privFlags.splitting_flag)
    {
        /* infer last dimension ID Len */
        pVideoParamSet->dimension_id_len[pVideoParamSet->numScalabilityTypes - 1]
          = 5 - xGetDimBitOffset(pVideoParamSet, pVideoParamSet->numScalabilityTypes - 1);
    }

    pVideoParamSet->privFlags.vps_nuh_layer_id_present_flag = u(1);

    for (i = 1; i <= pVideoParamSet->vps_max_layers_minus1; i++)
    {
        if (pVideoParamSet->privFlags.vps_nuh_layer_id_present_flag)
        {
            pVideoParamSet->layer_id_in_nuh[i] = (uint8_t)u(6);
        }
        else
        {
            pVideoParamSet->layer_id_in_nuh[i] = (uint8_t)i;
        }

        if (!pVideoParamSet->privFlags.splitting_flag)
        {
            for (j = 0; j < pVideoParamSet->numScalabilityTypes; j++)
            {
                uint32_t codelength = pVideoParamSet->dimension_id_len[j];
                pVideoParamSet->dimension_id[i][j] = (uint8_t)u(codelength);
            }
        }
        else
        {
            for (j = 0; j < pVideoParamSet->numScalabilityTypes; j++)
            {
                pVideoParamSet->dimension_id[i][j] = (pVideoParamSet->layer_id_in_nuh[i]
                & ( (1 << xGetDimBitOffset(pVideoParamSet,  j + 1 ) ) - 1) ) >> xGetDimBitOffset(pVideoParamSet, j );
            }
        }
    }
    for (i = 1; i <= pVideoParamSet->vps_max_layers_minus1; i++)
    {
        pVideoParamSet->LayerIdxInVps[pVideoParamSet->layer_id_in_nuh[i]] = i;
    }

    initNumViews(pVideoParamSet);

    pVideoParamSet->view_id_len = u(4);
    if (pVideoParamSet->view_id_len > 0)
    {
        for (i = 0; i < pVideoParamSet->numViews; i++)
        {
            uint32_t codelength = pVideoParamSet->view_id_len;
            pVideoParamSet->view_id_val[i] = u(codelength);
        }
    }

    for (i = 1; i <= pVideoParamSet->vps_max_layers_minus1; i++)
    {
        for (j = 0; j < i; j++)
        {
            pVideoParamSet->direct_dependency_flag[i][j] = u(1);
        }
    }

    setRefLayers(pVideoParamSet);

    if (pVideoParamSet->numIndependentLayers > 1)
    {
        pVideoParamSet->num_add_layer_sets = ue();
        if (pVideoParamSet->num_add_layer_sets > 1023)
        {
            nvParserLog("Invalid Invalid vps parameter (num_add_layer_sets=%d)\n", pVideoParamSet->num_add_layer_sets);
            return;
        }
    }

    for (i = 0; i < pVideoParamSet->num_add_layer_sets; i++)
    {
        for (j = 1; j < pVideoParamSet->numIndependentLayers; j++)
        {
            uint32_t length = CeilLog2(pVideoParamSet->numLayersInTreePartition[j] + 1);
            pVideoParamSet->highest_layer_idx_plus1[i][j] = u(length);
        }

        uint32_t layerNum = 0;
        uint32_t lsIdx = pVideoParamSet->vps_num_layer_sets + i;
        for (uint32_t treeIdx = 1; treeIdx < pVideoParamSet->numIndependentLayers; treeIdx++)
        {
            for (uint32_t layerCnt = 0; layerCnt < pVideoParamSet->highest_layer_idx_plus1[i][treeIdx]; layerCnt++)
                pVideoParamSet->layer_set_layer_id_list[lsIdx][layerNum++] = pVideoParamSet->treePartitionLayerIdList[treeIdx][layerCnt];
        }
        pVideoParamSet->num_layers_in_id_list[lsIdx] = layerNum;
    }

    pVideoParamSet->privFlags.vps_sub_layers_max_minus1_present_flag = u(1);

    if (pVideoParamSet->privFlags.vps_sub_layers_max_minus1_present_flag)
    {
        for (i = 0; i <= pVideoParamSet->vps_max_layers_minus1; i++)
            pVideoParamSet->sub_layers_vps_max_minus1[i] = u(3);
    }
    else
    {
        for (i = 0; i <= pVideoParamSet->vps_max_layers_minus1; i++)
            pVideoParamSet->sub_layers_vps_max_minus1[i] = pVideoParamSet->vps_max_sub_layers_minus1;
    }

    pVideoParamSet->privFlags.max_tid_ref_present_flag = u(1);

    if (pVideoParamSet->privFlags.max_tid_ref_present_flag)
    {
        for (i = 0; i < pVideoParamSet->vps_max_layers_minus1; i++)
        {
            for (j = i+1; j <= pVideoParamSet->vps_max_layers_minus1; j++)
            {
                if (pVideoParamSet->direct_dependency_flag[j][i])
                    pVideoParamSet->max_tid_il_ref_pics_plus1[i][j] = u(3);
            }
        }
    }

    pVideoParamSet->privFlags.default_ref_layers_active_flag = u(1);

    pVideoParamSet->vps_num_profile_tier_level_minus1 = ue();
    if (pVideoParamSet->vps_num_profile_tier_level_minus1 > 63 ||
        (pVideoParamSet->vps_max_layers_minus1 > 0 && pVideoParamSet->vps_num_profile_tier_level_minus1 == 0))
    {
        nvParserLog("Invalid vps parameter (vps_num_profile_tier_level_minus1=%d)\n", pVideoParamSet->vps_num_profile_tier_level_minus1);
        return;
    }
    for (i = pVideoParamSet->privFlags.vps_base_layer_internal_flag ? 2 : 1; i <= pVideoParamSet->vps_num_profile_tier_level_minus1; i++)
    {
        pVideoParamSet->vps_profile_present_flag[i] = u(1);
        pVideoParamSet->pProfileTierLevel = profile_tier_level(&pVideoParamSet->stdProfileTierLevel, pVideoParamSet->vps_max_sub_layers_minus1, pVideoParamSet->vps_profile_present_flag[i]);
    }

    /* Operation Points */
    if (pVideoParamSet->vps_num_layer_sets + pVideoParamSet->num_add_layer_sets > 1)
    {
        pVideoParamSet->num_add_olss = ue();
        if (pVideoParamSet->num_add_olss > 1023)
        {
            pVideoParamSet->num_add_olss = 0;
        }
        pVideoParamSet->numOutputLayerSets =  pVideoParamSet->vps_num_layer_sets + pVideoParamSet->num_add_layer_sets + pVideoParamSet->num_add_olss;
        pVideoParamSet->default_output_layer_idc = u(2);
    }

    for (i = 1; i < pVideoParamSet->numOutputLayerSets; i++)
    {
        if (pVideoParamSet->vps_num_layer_sets + pVideoParamSet->num_add_layer_sets > 2
            && i >= pVideoParamSet->vps_num_layer_sets + pVideoParamSet->num_add_layer_sets)
        {
            uint32_t codelength = CeilLog2(pVideoParamSet->vps_num_layer_sets + pVideoParamSet->num_add_layer_sets);
            pVideoParamSet->layer_set_idx_for_ols_minus1[i] = u(codelength);
        }

        if (i > pVideoParamSet->vps_num_layer_sets - 1 || pVideoParamSet->default_output_layer_idc == 2)
        {
            for (j = 0; j < pVideoParamSet->num_layers_in_id_list[olsIdxToLsIdx(pVideoParamSet, i)]; j++)
                pVideoParamSet->output_layer_flag[i][j] = u(1);
        }
        else
        {
            for (j = 0; j < pVideoParamSet->num_layers_in_id_list[olsIdxToLsIdx(pVideoParamSet, i)]; j++)
                pVideoParamSet->output_layer_flag[i][j] = inferoutput_layer_flag(pVideoParamSet, i, j);
        }

        /* Derive Necessary Layer Flag */
        deriveNecessaryLayerFlags(pVideoParamSet, i);

        /* profile_tier_level_idx[i][j] */
        for (j = 0; j < pVideoParamSet->num_layers_in_id_list[olsIdxToLsIdx(pVideoParamSet, i)]; j++)
        {
            if (pVideoParamSet->necessaryLayerFlag[i][j] && pVideoParamSet->vps_num_profile_tier_level_minus1 > 0)
            {
                uint32_t codelength = CeilLog2(pVideoParamSet->vps_num_profile_tier_level_minus1 + 1);
                pVideoParamSet->profile_tier_level_idx[i][j] = u(codelength);
            }
        }

        /* alt_output_layer_flag */
        if (pVideoParamSet->numOutputLayersInOutputLayerSet[i] == 1
            && pVideoParamSet->numDirectRefLayers[pVideoParamSet->olsHighestOutputLayerId[i]] > 0)
            u(1);
    }

    pVideoParamSet->vps_num_rep_formats_minus1 = ue();
    if (pVideoParamSet->vps_num_rep_formats_minus1 > 15)
    {
        nvParserLog("Invalid vps parameter (vps_num_rep_formats_minus1=%d)\n", pVideoParamSet->vps_num_rep_formats_minus1);
        return;
    }

    for (i = 0; i <= pVideoParamSet->vps_num_rep_formats_minus1; i++)
    {
        pVideoParamSet->repFormat[i].pic_width_vps_in_luma_samples = u(16);
        pVideoParamSet->repFormat[i].pic_height_vps_in_luma_samples = u(16);

        pVideoParamSet->repFormat[i].chroma_and_bit_depth_vps_present_flag = u(1);

        if (pVideoParamSet->repFormat[i].chroma_and_bit_depth_vps_present_flag)
        {
            pVideoParamSet->repFormat[i].chroma_format_vps_idc = u(2);

            if (pVideoParamSet->repFormat[i].chroma_format_vps_idc == 3)
            {
                pVideoParamSet->repFormat[i].chroma_format_vps_idc = u(1);
            }
            pVideoParamSet->repFormat[i].bit_depth_vps_luma_minus8   = u(4);
            pVideoParamSet->repFormat[i].bit_depth_vps_chroma_minus8 = u(4);
            pVideoParamSet->repFormat[i].conformance_window_vps_flag = u(1);

            if (pVideoParamSet->repFormat[i].conformance_window_vps_flag)
            {
                pVideoParamSet->repFormat[i].conf_win_vps_left_offset   = ue();
                pVideoParamSet->repFormat[i].conf_win_vps_right_offset  = ue();
                pVideoParamSet->repFormat[i].conf_win_vps_top_offset    = ue();
                pVideoParamSet->repFormat[i].conf_win_vps_bottom_offset = ue();
            }
        }
    }

    if(pVideoParamSet->vps_num_rep_formats_minus1 > 0)
    {
        pVideoParamSet->privFlags.rep_format_idx_present_flag = u(1);
        if (pVideoParamSet->privFlags.rep_format_idx_present_flag)
        {
            for (i = pVideoParamSet->privFlags.vps_base_layer_internal_flag ? 1 : 0; i <= pVideoParamSet->vps_max_layers_minus1; i++)
            {
                uint32_t codelength = CeilLog2(pVideoParamSet->vps_num_rep_formats_minus1 + 1);
                pVideoParamSet->vps_rep_format_idx[i] = u(codelength);
            }
        }
    }

    pVideoParamSet->privFlags.max_one_active_ref_layer_flag = u(1);
    pVideoParamSet->privFlags.vps_poc_lsb_aligned_flag = u(1);

    for (i = 1; i <= pVideoParamSet->vps_max_layers_minus1; i++)
    {
        if (pVideoParamSet->numDirectRefLayers[pVideoParamSet->layer_id_in_nuh[i]] == 0)
        {
            pVideoParamSet->poc_lsb_not_present_flag[i] = u(1);
        }
    }

    // dpb_size
    for (i = 1; i < pVideoParamSet->numOutputLayerSets; i++)
    {
        uint32_t currLsIdx = olsIdxToLsIdx(pVideoParamSet, i);
        pVideoParamSet->sub_layer_flag_info_present_flag[i] = u(1);

        for (j = 0; j <= pVideoParamSet->sub_layers_vps_max_minus1[currLsIdx]; j++)
        {
            if (j > 0 && pVideoParamSet->sub_layer_flag_info_present_flag[i])
            {
                pVideoParamSet->sub_layer_dpb_info_present_flag[i][j] = u(1);
            }
            else if (j == 0)
            {
                pVideoParamSet->sub_layer_dpb_info_present_flag[i][j] = 1;
            }

            if (pVideoParamSet->sub_layer_dpb_info_present_flag[i][j])
            {
                for (k = 0; k < pVideoParamSet->num_layers_in_id_list[currLsIdx]; k++)
                {
                    if (pVideoParamSet->necessaryLayerFlag[i][k] &&
                    (pVideoParamSet->privFlags.vps_base_layer_internal_flag || (pVideoParamSet->layer_set_layer_id_list[currLsIdx][k] != 0) ))
                    {
                        pVideoParamSet->max_vps_dec_pic_buffering_minus1[i][k][j] = ue();
                    }
                }
                pVideoParamSet->max_vps_num_reorder_pics[i][j] = ue();
                pVideoParamSet->max_vps_latency_increase_plus1[i][j] = ue();
            }
        }
    }
    /* Others not used for MV-HEVC */

    return;
}

void VulkanH265Decoder::deriveNecessaryLayerFlags(hevc_video_param_s *pVideoParamSet, uint32_t olsIdx)
{
    uint32_t lsIdx = olsIdxToLsIdx(pVideoParamSet, olsIdx);
    for (uint32_t lsLayerIdx = 0; lsLayerIdx < pVideoParamSet->num_layers_in_id_list[lsIdx]; lsLayerIdx++)
    {
        pVideoParamSet->necessaryLayerFlag[olsIdx][lsLayerIdx] = 0;
    }
    for (uint32_t lsLayerIdx = 0; lsLayerIdx < pVideoParamSet->num_layers_in_id_list[lsIdx]; lsLayerIdx++)
    {
        if (pVideoParamSet->output_layer_flag[olsIdx][lsLayerIdx])
        {
            pVideoParamSet->necessaryLayerFlag[olsIdx][lsLayerIdx] = 1;
            uint32_t currLayerId = pVideoParamSet->layer_set_layer_id_list[lsIdx][lsLayerIdx];
            for (uint32_t rLsLayerIdx = 0; rLsLayerIdx < lsLayerIdx; rLsLayerIdx++ )
            {
                uint32_t refLayerId = pVideoParamSet->layer_set_layer_id_list[lsIdx][rLsLayerIdx];
                if (pVideoParamSet->DependencyFlag[pVideoParamSet->layer_id_in_nuh[currLayerId]][pVideoParamSet->layer_id_in_nuh[refLayerId]])
                {
                    pVideoParamSet->necessaryLayerFlag[olsIdx][rLsLayerIdx] = 1;
                }
            }
        }
    }
    pVideoParamSet->numNecessaryLayers[olsIdx] = 0;
    for (uint32_t lsLayerIdx = 0; lsLayerIdx < pVideoParamSet->num_layers_in_id_list[lsIdx]; lsLayerIdx++)
    {
        pVideoParamSet->numNecessaryLayers[olsIdx] += pVideoParamSet->necessaryLayerFlag[olsIdx][lsLayerIdx];
    }

    pVideoParamSet->numOutputLayersInOutputLayerSet[olsIdx] = 0;
    for (uint32_t j = 0; j < pVideoParamSet->num_layers_in_id_list[olsIdxToLsIdx(pVideoParamSet, olsIdx)]; j++)
    {
        pVideoParamSet->numOutputLayersInOutputLayerSet[olsIdx] += (pVideoParamSet->output_layer_flag[olsIdx][j]);
        if (pVideoParamSet->output_layer_flag[olsIdx][j])
        {
            pVideoParamSet->olsHighestOutputLayerId[olsIdx] = pVideoParamSet->layer_set_layer_id_list[olsIdxToLsIdx(pVideoParamSet, olsIdx)][j];
        }
    }
}

void VulkanH265Decoder::setRefLayers(hevc_video_param_s *pVideoParamSet)
{
    uint32_t i, j, k, h;
    uint32_t d, r, p;

   //DependencyFlag
    for (i = 0; i <= pVideoParamSet->vps_max_layers_minus1; i++ )
    {
        for (j = 0; j <= pVideoParamSet->vps_max_layers_minus1; j++)
        {
            pVideoParamSet->DependencyFlag[i][j] = pVideoParamSet->direct_dependency_flag[i][j];
            for (k = 0; k < i; k++)
            {
                if (pVideoParamSet->direct_dependency_flag[i][k] && pVideoParamSet->DependencyFlag[k][j])
                {
                    pVideoParamSet->DependencyFlag[i][j] = 1;
                }
            }
        }
    }

    //idDirectRefLayer, idRefLayer, idPredictedLayer
    for (i = 0; i <= pVideoParamSet->vps_max_layers_minus1; i++ )
    {
        uint32_t iNuhLId =  pVideoParamSet->layer_id_in_nuh[i];
        for (j = 0, d = 0, r = 0, p = 0; j <= pVideoParamSet->vps_max_layers_minus1; j++)
        {
            uint32_t jNuhLid =  pVideoParamSet->layer_id_in_nuh[j];
            if (pVideoParamSet->direct_dependency_flag[i][j])
            {
                pVideoParamSet->idDirectRefLayer[iNuhLId][d++] = jNuhLid;
            }
            if (pVideoParamSet->DependencyFlag[i][j])
            {
                pVideoParamSet->idRefLayer[iNuhLId][r++] = jNuhLid;
            }
            if (pVideoParamSet->DependencyFlag[j][i])
            {
                pVideoParamSet->idPredictedLayer[iNuhLId][p++] = jNuhLid;
            }
            pVideoParamSet->numDirectRefLayers[iNuhLId] = d;
            pVideoParamSet->numRefLayers[iNuhLId] = r;
            pVideoParamSet->numPredictedLayers[iNuhLId] = p;
        }
    }

    for (i = 0; i < MAX_NUM_LAYER_IDS; i++)
    {
        pVideoParamSet->layerIdInListFlag[i] = 0;
    }
    for (i = 0, k = 0; i <= pVideoParamSet->vps_max_layers_minus1; i++ )
    {
        uint32_t iNuhLId = pVideoParamSet->layer_id_in_nuh[i];
        if (pVideoParamSet->numDirectRefLayers[iNuhLId] == 0)
        {
            pVideoParamSet->treePartitionLayerIdList[k][0] = iNuhLId;
            for (j = 0, h = 1; j < pVideoParamSet->numPredictedLayers[iNuhLId]; j++)
            {
                uint32_t predLId = pVideoParamSet->idPredictedLayer[iNuhLId][j];
                if (!pVideoParamSet->layerIdInListFlag[predLId])
                {
                    pVideoParamSet->treePartitionLayerIdList[k][h++] = predLId;
                    pVideoParamSet->layerIdInListFlag[predLId] = 1;
                }
            }
            pVideoParamSet->numLayersInTreePartition[k++] = h;
        }
    }
    pVideoParamSet->numIndependentLayers = k;
}

void VulkanH265Decoder::initNumViews(hevc_video_param_s *pVideoParamSet)
{
    uint32_t NumViews = 1;
    uint32_t ScalabilityId[MAX_NUM_LAYER_IDS][MAX_NUM_SCALABILITY_TYPES];
    memset(ScalabilityId, 0, MAX_NUM_LAYER_IDS*MAX_NUM_SCALABILITY_TYPES*sizeof(uint32_t));
    for (uint32_t i = 0; i <= pVideoParamSet->vps_max_layers_minus1; i++ )
    {
        uint32_t lId = pVideoParamSet->layer_id_in_nuh[i];
        for (uint32_t smIdx = 0, j = 0; smIdx < MAX_NUM_SCALABILITY_TYPES; smIdx++)
        {
            if (pVideoParamSet->scalability_mask_flag[smIdx])
            {
                ScalabilityId[i][smIdx] = pVideoParamSet->dimension_id[i][j++];
            }
            else
            {
                ScalabilityId[i][smIdx] = 0;
            }
        }
        pVideoParamSet->viewOrderIdx[lId] = ScalabilityId[i][1];
        if (i > 0)
        {
            uint32_t newViewFlag = 1;
            for (uint32_t j = 0; j < i; j++)
            {
                if (pVideoParamSet->viewOrderIdx[lId] ==
                pVideoParamSet->viewOrderIdx[pVideoParamSet->layer_id_in_nuh[j]])
                {
                    newViewFlag = 0;
                }
            }
            NumViews += newViewFlag;
        }
    }
    pVideoParamSet->numViews = NumViews;
}

uint32_t VulkanH265Decoder::olsIdxToLsIdx(hevc_video_param_s *pVideoParamSet, uint32_t i)
{
    return (i < pVideoParamSet->vps_num_layer_sets + pVideoParamSet->num_add_layer_sets ) ? i : pVideoParamSet->layer_set_idx_for_ols_minus1[i] + 1 ;
}

uint32_t VulkanH265Decoder::inferoutput_layer_flag(hevc_video_param_s *pVideoParamSet, uint32_t i, uint32_t j)
{
    uint32_t output_layer_flag = 0;
    switch ( pVideoParamSet->default_output_layer_idc )
    {
        case 0:
            output_layer_flag = 1;
            break;
        case 1:
            output_layer_flag = ( j == pVideoParamSet->num_layers_in_id_list[olsIdxToLsIdx(pVideoParamSet, i )] - 1 );
            break;
        case 2:
            if (i == 0 && j == 0)
            {
                output_layer_flag = 1;  // This is a software only fix for a bug in the spec. In spec output_layer_flag is neither present nor inferred.
            }
            break;
        default:
            break;
  }
  return output_layer_flag;
}

uint32_t VulkanH265Decoder::xGetDimBitOffset(hevc_video_param_s *pVideoParamSet, uint32_t j)
{
    uint32_t dimBitOffset = 0;
    if (pVideoParamSet->privFlags.splitting_flag && j == pVideoParamSet->numScalabilityTypes)
    {
        dimBitOffset = 6;
    }
    else
    {
        for (uint32_t dimIdx = 0; dimIdx <= j-1; dimIdx++)
        {
            dimBitOffset += pVideoParamSet->dimension_id_len[dimIdx];
        }
    }
    return dimBitOffset;
}

static StdVideoH265LevelIdc generalLevelIdcToVulkanLevelIdcEnum(uint8_t general_level_idc)
{
    // general_level_idc and sub_layer_level_idc[ OpTid ] shall be set equal to a value of
    // 30 times the level number specified in Table A.4.
    // Table A.4 - General tier and level limits
    static const uint32_t H265_LEVEL_IDC_1_0 = (uint32_t)(1.0f * 30);
    static const uint32_t H265_LEVEL_IDC_2_0 = (uint32_t)(2.0f * 30);
    static const uint32_t H265_LEVEL_IDC_2_1 = (uint32_t)(2.1f * 30);
    static const uint32_t H265_LEVEL_IDC_3_0 = (uint32_t)(3.0f * 30);
    static const uint32_t H265_LEVEL_IDC_3_1 = (uint32_t)(3.1f * 30);
    static const uint32_t H265_LEVEL_IDC_4_0 = (uint32_t)(4.0f * 30);
    static const uint32_t H265_LEVEL_IDC_4_1 = (uint32_t)(4.1f * 30);
    static const uint32_t H265_LEVEL_IDC_5_0 = (uint32_t)(5.0f * 30);
    static const uint32_t H265_LEVEL_IDC_5_1 = (uint32_t)(5.1f * 30);
    static const uint32_t H265_LEVEL_IDC_5_2 = (uint32_t)(5.2f * 30);
    static const uint32_t H265_LEVEL_IDC_6_0 = (uint32_t)(6.0f * 30);
    static const uint32_t H265_LEVEL_IDC_6_1 = (uint32_t)(6.1f * 30);
    static const uint32_t H265_LEVEL_IDC_6_2 = (uint32_t)(6.2f * 30);

    switch (general_level_idc) {
        case H265_LEVEL_IDC_1_0: return STD_VIDEO_H265_LEVEL_IDC_1_0;
        case H265_LEVEL_IDC_2_0: return STD_VIDEO_H265_LEVEL_IDC_2_0;
        case H265_LEVEL_IDC_2_1: return STD_VIDEO_H265_LEVEL_IDC_2_1;
        case H265_LEVEL_IDC_3_0: return STD_VIDEO_H265_LEVEL_IDC_3_0;
        case H265_LEVEL_IDC_3_1: return STD_VIDEO_H265_LEVEL_IDC_3_1;
        case H265_LEVEL_IDC_4_0: return STD_VIDEO_H265_LEVEL_IDC_4_0;
        case H265_LEVEL_IDC_4_1: return STD_VIDEO_H265_LEVEL_IDC_4_1;
        case H265_LEVEL_IDC_5_0: return STD_VIDEO_H265_LEVEL_IDC_5_0;
        case H265_LEVEL_IDC_5_1: return STD_VIDEO_H265_LEVEL_IDC_5_1;
        case H265_LEVEL_IDC_5_2: return STD_VIDEO_H265_LEVEL_IDC_5_2;
        case H265_LEVEL_IDC_6_0: return STD_VIDEO_H265_LEVEL_IDC_6_0;
        case H265_LEVEL_IDC_6_1: return STD_VIDEO_H265_LEVEL_IDC_6_1;
        case H265_LEVEL_IDC_6_2: return STD_VIDEO_H265_LEVEL_IDC_6_2;
        default:
            nvParserErrorLog("\nError: Invalid h.265 IDC Level\n");
            return STD_VIDEO_H265_LEVEL_IDC_6_2;
    }
}

// TBD: Return data in a common profile struct
const StdVideoH265ProfileTierLevel* VulkanH265Decoder::profile_tier_level(StdVideoH265ProfileTierLevel* pProfileTierLevel,
                                                                          int MaxNumSubLayersMinus1, uint8_t ProfilePresent)
{
    pProfileTierLevel->general_profile_idc = (StdVideoH265ProfileIdc)0;
    if (ProfilePresent == 1)
    {
        u(2 + 1);   // general_profile_space, general_tier_flag
        pProfileTierLevel->general_profile_idc = (StdVideoH265ProfileIdc)u(5); // general_profile_idc
        u(16);      // general_profile_compatibility_flag_hi16
        u(16);      // general_profile_compatibility_flag_lo16
        u(24);      // general source/constraint flags(4), general_reserved_zero_44bits[0..19]
        u(24);      // general_reserved_zero_44bits[20..43]
    }
    uint8_t general_level_idc = u(8); // general_level_idc
    // Table A.4 - General tier and level limits
    pProfileTierLevel->general_level_idc = generalLevelIdcToVulkanLevelIdcEnum(general_level_idc);

    assert(MaxNumSubLayersMinus1 < MAX_NUM_SUB_LAYERS);
    if (MaxNumSubLayersMinus1 > 0)
    {
        uint32_t sub_layer_profile_level_present_flags = u(16);
        for (int i = 0; i < MaxNumSubLayersMinus1; i++)
        {
            int sub_layer_profile_present_flag = sub_layer_profile_level_present_flags >> (15 - (i * 2));
            int sub_layer_level_present_flag = sub_layer_profile_level_present_flags >> (14 - (i * 2));
            if (sub_layer_profile_present_flag)
            {
                u(2+1+5);   // sub_layer_profile_space, sub_layer_tier_flag, sub_layer_profile_idc
                u(16);      // sub_layer_profile_compatibility_flag_hi16
                u(16);      // sub_layer_profile_compatibility_flag_lo16
                u(24);
                u(24);
            }
            if (sub_layer_level_present_flag)
                u(8);   // sub_layer_level_idc
        }
    }
    // return (pProfileTierLevel->general_profile_idc << 8) | pProfileTierLevel->general_level_idc;
    return pProfileTierLevel;
}


bool VulkanH265Decoder::scaling_list_data(scaling_list_s *scl)
{
    for (int sizeId = 0; sizeId < 4; sizeId++)
    {
        for (int matrixId = 0; matrixId < ((sizeId == 3) ? 2 : 6); matrixId++)
        {
            scaling_list_entry_s *scle = &scl->entry[sizeId][matrixId];
            scle->scaling_list_pred_mode_flag = u(1);
            if (!scle->scaling_list_pred_mode_flag)
            {
                int scaling_list_pred_matrix_id_delta = ue();
                int refMatrixId = matrixId - scaling_list_pred_matrix_id_delta;
                scle->scaling_list_pred_matrix_id_delta = scaling_list_pred_matrix_id_delta;
                if (refMatrixId < 0)
                {
                    nvParserLog("Invalid scaling_list_pred_matrix_id_delta (refMatrixId = %d)\n", refMatrixId);
                    return false;
                }
            }
            else
            {
                int coefNum = std::min<int32_t>(64, (1 << (4 + (sizeId << 1))));
                int nextCoef = 8;
                if (sizeId > 1)
                {
                    int scaling_list_dc_coef_minus8 = se();
                    scle->scaling_list_dc_coef_minus8 = scaling_list_dc_coef_minus8;
                    if (scaling_list_dc_coef_minus8 < -7 || scaling_list_dc_coef_minus8 > 247)
                    {
                        nvParserLog("Invalid scaling_list_dc_coef_minus8 (%d)\n", scaling_list_dc_coef_minus8);
                        return false;
                    }
                    nextCoef = scle->scaling_list_dc_coef_minus8 + 8;
                }
                for (int i=0; i<coefNum; i++)
                {
                    int scaling_list_delta_coef = se();
                    scle->scaling_list_delta_coef[i] = (int8_t)scaling_list_delta_coef;
                    if (scaling_list_delta_coef < -128 || scaling_list_delta_coef > 127)
                    {
                        nvParserLog("Invalid scaling_list_delta_coef (%d)\n", scaling_list_delta_coef);
                        return false;
                    }
                    nextCoef = (nextCoef + scaling_list_delta_coef) & 0xff;
                    if (nextCoef == 0)
                    {
                        nvParserLog("Invalid scaling_list_delta_coef: zero ScalingList entry\n");
                        return false;
                    }
                }
            }
        }
    }
    return true;
}


StdVideoH265ShortTermRefPicSet* VulkanH265Decoder::short_term_ref_pic_set(StdVideoH265ShortTermRefPicSet* stdShortTermRefPicSet,
                                                                          short_term_ref_pic_set_s *strps,
                                                                          const short_term_ref_pic_set_s strpss[],
                                                                          int idx, int num_short_term_ref_pic_sets)
{
    int inter_ref_pic_set_prediction_flag = (idx != 0) ? u(1) : 0;
    strps->inter_ref_pic_set_prediction_flag = (uint8_t)inter_ref_pic_set_prediction_flag;
    stdShortTermRefPicSet->flags.inter_ref_pic_set_prediction_flag = (inter_ref_pic_set_prediction_flag != 0) ? 1 : 0;
    if (inter_ref_pic_set_prediction_flag)
    {
        uint8_t used_by_curr_pic_flag[MAX_NUM_STRPS_ENTRIES + 1];
        uint8_t use_delta_flag[MAX_NUM_STRPS_ENTRIES + 1];
        uint32_t delta_idx_minus1 = (idx == num_short_term_ref_pic_sets) ? ue() : 0;
        if (delta_idx_minus1 >= (uint32_t)idx)
        {
            nvParserLog("Invalid delta_idx_minus1 (%d > %d)\n", delta_idx_minus1, idx - 1);
            return nullptr;
        }
        strps->delta_idx_minus1 = (uint8_t)delta_idx_minus1;
        stdShortTermRefPicSet->delta_idx_minus1 = delta_idx_minus1;
        int delta_rps_sign = u(1);
        stdShortTermRefPicSet->flags.delta_rps_sign = (delta_rps_sign != 0) ? 1 : 0;
        int abs_delta_rps_minus1 = ue();
        stdShortTermRefPicSet->abs_delta_rps_minus1 = (uint32_t)abs_delta_rps_minus1;
        int DeltaRPS = (1 - 2 * delta_rps_sign) * (abs_delta_rps_minus1 + 1);
        int RIdx = idx - (delta_idx_minus1 + 1);
        assert(RIdx >= 0);
        const short_term_ref_pic_set_s *rstrps = &strpss[RIdx];
        for (int j = 0; j <= (rstrps->NumNegativePics + rstrps->NumPositivePics); j++)
        {
            assert(j < MAX_NUM_STRPS_ENTRIES + 1);
            used_by_curr_pic_flag[j] = (uint8_t)u(1);
            if (used_by_curr_pic_flag[j]) {
                stdShortTermRefPicSet->used_by_curr_pic_flag |= (1 << j);
            }
            use_delta_flag[j] = (!used_by_curr_pic_flag[j]) ? (uint8_t)u(1) : 1;
            if (use_delta_flag[j]) {
                stdShortTermRefPicSet->use_delta_flag |= 1 << j;
            }
        }

        {
            int i = 0;
            for (int j = rstrps->NumPositivePics - 1; j >= 0; j--)
            {
                int dPoc = rstrps->DeltaPocS1[j] + DeltaRPS;
                if ((dPoc < 0) && use_delta_flag[rstrps->NumNegativePics + j])
                {
                    assert(i < MAX_NUM_STRPS_ENTRIES);
                    strps->DeltaPocS0[i] = dPoc;
                    stdShortTermRefPicSet->delta_poc_s0_minus1[i] = (uint16_t)dPoc;
                    strps->UsedByCurrPicS0[i] = used_by_curr_pic_flag[rstrps->NumNegativePics + j];
                    if (strps->UsedByCurrPicS0[i] != 0) {
                        stdShortTermRefPicSet->used_by_curr_pic_s0_flag |= (1 << i);
                    }
                    i++;
                }
            }
            if ((DeltaRPS < 0) && use_delta_flag[rstrps->NumNegativePics+rstrps->NumPositivePics])
            {
                assert(i < MAX_NUM_STRPS_ENTRIES);
                strps->DeltaPocS0[i] = DeltaRPS;
                stdShortTermRefPicSet->delta_poc_s0_minus1[i] = (uint16_t)DeltaRPS;
                strps->UsedByCurrPicS0[i] = used_by_curr_pic_flag[rstrps->NumNegativePics+rstrps->NumPositivePics];
                if (strps->UsedByCurrPicS0[i] != 0) {
                    stdShortTermRefPicSet->used_by_curr_pic_s0_flag |= (1 << i);
                }
                i++;
            }
            for (int j = 0; j < rstrps->NumNegativePics; j++)
            {
                int dPoc = rstrps->DeltaPocS0[j] + DeltaRPS;
                if ((dPoc < 0) && use_delta_flag[j])
                {
                    assert(i < MAX_NUM_STRPS_ENTRIES);
                    strps->DeltaPocS0[i] = dPoc;
                    stdShortTermRefPicSet->delta_poc_s0_minus1[i] = (uint16_t)dPoc;
                    strps->UsedByCurrPicS0[i] = used_by_curr_pic_flag[j];
                    if (strps->UsedByCurrPicS0[i] != 0) {
                        stdShortTermRefPicSet->used_by_curr_pic_s0_flag |= (1 << i);
                    }
                    i++;
                }
            }
            strps->NumNegativePics = (uint8_t)i;
            stdShortTermRefPicSet->num_negative_pics = (uint32_t)i;
        }
        {
            int i = 0;
            for (int j = rstrps->NumNegativePics - 1; j >= 0; j--)
            {
                int dPoc = rstrps->DeltaPocS0[j] + DeltaRPS;
                if ((dPoc > 0) && use_delta_flag[j])
                {
                    assert(i < MAX_NUM_STRPS_ENTRIES);
                    strps->DeltaPocS1[i] = dPoc;
                    stdShortTermRefPicSet->delta_poc_s1_minus1[i] = dPoc;
                    strps->UsedByCurrPicS1[i] = used_by_curr_pic_flag[j];
                    if (strps->UsedByCurrPicS1[i] != 0) {
                        stdShortTermRefPicSet->used_by_curr_pic_s1_flag |= (1 << i);
                    }
                    i++;
                }
            }
            if ((DeltaRPS > 0) && use_delta_flag[rstrps->NumNegativePics+rstrps->NumPositivePics])
            {
                assert(i < MAX_NUM_STRPS_ENTRIES);
                strps->DeltaPocS1[i] = DeltaRPS;
                stdShortTermRefPicSet->delta_poc_s1_minus1[i] = DeltaRPS;
                strps->UsedByCurrPicS1[i] = used_by_curr_pic_flag[rstrps->NumNegativePics+rstrps->NumPositivePics];
                if (strps->UsedByCurrPicS1[i] != 0) {
                    stdShortTermRefPicSet->used_by_curr_pic_s1_flag |= (1 << i);
                }
                i++;
            }
            for (int j = 0; j < rstrps->NumPositivePics; j++)
            {
                int dPoc = rstrps->DeltaPocS1[j] + DeltaRPS;
                if ((dPoc > 0) && use_delta_flag[rstrps->NumNegativePics+j])
                {
                    assert(i < MAX_NUM_STRPS_ENTRIES);
                    strps->DeltaPocS1[i] = dPoc;
                    stdShortTermRefPicSet->delta_poc_s1_minus1[i] = dPoc;
                    strps->UsedByCurrPicS1[i] = used_by_curr_pic_flag[rstrps->NumNegativePics + j];
                    if (strps->UsedByCurrPicS1[i] != 0) {
                        stdShortTermRefPicSet->used_by_curr_pic_s1_flag |= (1 << i);
                    }
                    i++;
                }
            }
            strps->NumPositivePics = (uint8_t)i;
            stdShortTermRefPicSet->num_positive_pics = (uint32_t)i;
        }
        if (strps->NumNegativePics + strps->NumPositivePics > MAX_NUM_STRPS_ENTRIES)
        {
            nvParserLog("Invalid NumNegativePics+NumPositivePics (%d+%d)\n", strps->NumNegativePics, strps->NumPositivePics);
            return nullptr;
        }
    }
    else // (!inter_ref_pic_set_prediction_flag)
    {
        int16_t delta_poc_s0_minus1[MAX_NUM_STRPS_ENTRIES];
        int16_t delta_poc_s1_minus1[MAX_NUM_STRPS_ENTRIES];
        uint8_t used_by_curr_pic_s0_flag[MAX_NUM_STRPS_ENTRIES];
        uint8_t used_by_curr_pic_s1_flag[MAX_NUM_STRPS_ENTRIES];
        uint32_t num_negative_pics = ue();
        uint32_t num_positive_pics = ue();
        if ((num_negative_pics > MAX_NUM_STRPS_ENTRIES) || (num_positive_pics > MAX_NUM_STRPS_ENTRIES)
         || (num_negative_pics + num_positive_pics > MAX_NUM_STRPS_ENTRIES))
        {
            nvParserLog("Invalid num_negative_pics+num_positive_pics (%d+%d)\n", num_negative_pics, num_positive_pics);
            return nullptr;
        }
        for (uint32_t i = 0; i < num_negative_pics; i++)
        {
            delta_poc_s0_minus1[i] = (int16_t)ue();
            used_by_curr_pic_s0_flag[i] = (uint8_t)u(1);
        }
        for (uint32_t i = 0; i < num_positive_pics; i++)
        {
            delta_poc_s1_minus1[i] = (int16_t)ue();
            used_by_curr_pic_s1_flag[i] = (uint8_t)u(1);
        }
        strps->NumNegativePics = (uint8_t)num_negative_pics;
        stdShortTermRefPicSet->num_negative_pics = num_negative_pics;
        strps->NumPositivePics = (uint8_t)num_positive_pics;
        stdShortTermRefPicSet->num_positive_pics = num_positive_pics;
        for (uint32_t i = 0; i < num_negative_pics; i++)
        {
            strps->DeltaPocS0[i] = ((i == 0) ? 0 : strps->DeltaPocS0[i - 1]) - (delta_poc_s0_minus1[i] + 1);
            stdShortTermRefPicSet->delta_poc_s0_minus1[i] = (uint16_t)strps->DeltaPocS0[i];
            strps->UsedByCurrPicS0[i] = used_by_curr_pic_s0_flag[i];
            if (strps->UsedByCurrPicS0[i] != 0) {
                stdShortTermRefPicSet->used_by_curr_pic_s0_flag |= (1 << i);
            }
        }
        for (uint32_t i = 0; i < num_positive_pics; i++)
        {
            strps->DeltaPocS1[i] = ((i == 0) ? 0 : strps->DeltaPocS1[i - 1]) + (delta_poc_s1_minus1[i] + 1);
            stdShortTermRefPicSet->delta_poc_s1_minus1[i] = (uint16_t)strps->DeltaPocS1[i];
            strps->UsedByCurrPicS1[i] = used_by_curr_pic_s1_flag[i];
            if (strps->UsedByCurrPicS1[i] != 0) {
                stdShortTermRefPicSet->used_by_curr_pic_s1_flag |= (1 << i);
            }
        }
    }
    return stdShortTermRefPicSet;
}


void VulkanH265Decoder::vui_parameters(hevc_seq_param_s *sps, int sps_max_sub_layers_minus1)
{
    StdVideoH265SequenceParameterSetVui *vui = &sps->stdVui;
    vui->aspect_ratio_idc = STD_VIDEO_H265_ASPECT_RATIO_IDC_UNSPECIFIED;
    vui->flags.aspect_ratio_info_present_flag = u(1);
    if (vui->flags.aspect_ratio_info_present_flag) {
        vui->aspect_ratio_idc = (StdVideoH265AspectRatioIdc)u(8);
    }
    // Table E-1
    switch(vui->aspect_ratio_idc)
    {
    case 1:        vui->sar_width = 1; vui->sar_height = 1; break;
    case 2:        vui->sar_width = 12; vui->sar_height = 11; break;
    case 3:        vui->sar_width = 10; vui->sar_height = 11; break;
    case 4:        vui->sar_width = 16; vui->sar_height = 11; break;
    case 5:        vui->sar_width = 40; vui->sar_height = 33; break;
    case 6:        vui->sar_width = 24; vui->sar_height = 11; break;
    case 7:        vui->sar_width = 20; vui->sar_height = 11; break;
    case 8:        vui->sar_width = 32; vui->sar_height = 11; break;
    case 9:        vui->sar_width = 80; vui->sar_height = 33; break;
    case 10:       vui->sar_width = 18; vui->sar_height = 11; break;
    case 11:       vui->sar_width = 15; vui->sar_height = 11; break;
    case 12:       vui->sar_width = 64; vui->sar_height = 33; break;
    case 13:       vui->sar_width = 160; vui->sar_height = 99; break;
    case 14:       vui->sar_width = 4; vui->sar_height = 3; break;
    case 15:       vui->sar_width = 3; vui->sar_height = 2; break;
    case 16:       vui->sar_width = 2; vui->sar_height = 1; break;
    case 255: // Extended_SAR
        vui->sar_width = (uint16_t)u(16);
        vui->sar_height = (uint16_t)u(16);
        break;
    default:    // Default to square pixels for everything else
        vui->sar_width = 1;
        vui->sar_height = 1;
        break;
    }
    vui->flags.overscan_info_present_flag = u(1);
    if (vui->flags.overscan_info_present_flag) {
        vui->flags.overscan_appropriate_flag = u(1);
    }
    vui->flags.video_signal_type_present_flag = (uint8_t)u(1);
    if (vui->flags.video_signal_type_present_flag)
    {
        vui->video_format = (uint8_t)u(3);
        vui->flags.video_full_range_flag = (uint8_t)u(1);
        vui->flags.colour_description_present_flag = (uint8_t)u(1);
        if (vui->flags.colour_description_present_flag)
        {
            vui->colour_primaries = (uint8_t)u(8);
            vui->transfer_characteristics = (uint8_t)u(8);
            vui->matrix_coeffs = (uint8_t)u(8);
        }
    }
    vui->flags.chroma_loc_info_present_flag = u(1);
    if (vui->flags.chroma_loc_info_present_flag) {
        vui->chroma_sample_loc_type_top_field = ue();
        vui->chroma_sample_loc_type_bottom_field = ue();
    }
    vui->flags.neutral_chroma_indication_flag = u(1);
    vui->flags.field_seq_flag = (uint8_t)u(1);
    vui->flags.frame_field_info_present_flag = u(1);
    vui->flags.default_display_window_flag = u(1);
    if (vui->flags.default_display_window_flag) {
        vui->def_disp_win_left_offset = ue();
        vui->def_disp_win_right_offset = ue();
        vui->def_disp_win_top_offset = ue();
        vui->def_disp_win_bottom_offset = ue();
    }
    vui->flags.vui_timing_info_present_flag = (uint8_t)u(1);
    if (vui->flags.vui_timing_info_present_flag)
    {
        vui->vui_num_units_in_tick = u(32);
        vui->vui_time_scale = u(32);
        vui->flags.vui_poc_proportional_to_timing_flag = u(1);
        if (vui->flags.vui_poc_proportional_to_timing_flag) {
            vui->vui_num_ticks_poc_diff_one_minus1 = ue();
        }
        vui->flags.vui_hrd_parameters_present_flag = u(1);
        if (vui->flags.vui_hrd_parameters_present_flag) {
            hrd_parameters(&sps->stdHrdParameters, 1, sps_max_sub_layers_minus1);
            sps->stdVui.pHrdParameters = &sps->stdHrdParameters;
        }
    }
    vui->flags.bitstream_restriction_flag = u(1);
    if (vui->flags.bitstream_restriction_flag) {
        vui->flags.tiles_fixed_structure_flag = u(1);
        vui->flags.motion_vectors_over_pic_boundaries_flag = u(1);
        vui->flags.restricted_ref_pic_lists_flag = u(1);
        vui->min_spatial_segmentation_idc = ue();
        vui->max_bytes_per_pic_denom = ue();
        vui->max_bits_per_min_cu_denom = ue();
        vui->log2_max_mv_length_horizontal = ue();
        vui->log2_max_mv_length_vertical = ue();
    }
}


void VulkanH265Decoder::sub_layer_hrd_parameters(StdVideoH265SubLayerHrdParameters* pStdSubLayerHrdParameters,
        int /*subLayerId*/, int cpb_cnt_minus1, int sub_pic_hrd_params_present_flag)
{
    int CpbCnt = cpb_cnt_minus1 /*[subLayerId]*/;
    for (int i = 0; i <= CpbCnt; i++)
    {
        pStdSubLayerHrdParameters->bit_rate_value_minus1[i] = ue();
        pStdSubLayerHrdParameters->cpb_size_value_minus1[i] = ue();
        if (sub_pic_hrd_params_present_flag)
        {
            pStdSubLayerHrdParameters->cpb_size_du_value_minus1[i] = ue();
            pStdSubLayerHrdParameters->bit_rate_du_value_minus1[i] = ue();
        }
        bool cbr_flag = u(1);
        if (cbr_flag) {
            pStdSubLayerHrdParameters->cbr_flag |= (1 << i);
        }
    }
}

// Annex E
void VulkanH265Decoder::hrd_parameters(hevc_video_hrd_param_s* pStdHrdParameters, bool commonInfPresentFlag, uint8_t maxNumSubLayersMinus1)
{
    if (commonInfPresentFlag)
    {
        pStdHrdParameters->flags.nal_hrd_parameters_present_flag = u(1);
        pStdHrdParameters->flags.vcl_hrd_parameters_present_flag = u(1);
        if (pStdHrdParameters->flags.nal_hrd_parameters_present_flag || pStdHrdParameters->flags.vcl_hrd_parameters_present_flag)
        {
            pStdHrdParameters->flags.sub_pic_hrd_params_present_flag = u(1);
            if (pStdHrdParameters->flags.sub_pic_hrd_params_present_flag)
            {
                pStdHrdParameters->tick_divisor_minus2 = u(8);
                pStdHrdParameters->du_cpb_removal_delay_increment_length_minus1 = u(5);
                pStdHrdParameters->flags.sub_pic_cpb_params_in_pic_timing_sei_flag = u(1);
                pStdHrdParameters->dpb_output_delay_du_length_minus1 = u(5);
            }
            pStdHrdParameters->bit_rate_scale = u(4);
            pStdHrdParameters->cpb_size_scale = u(4);
            if (pStdHrdParameters->flags.sub_pic_hrd_params_present_flag) {
                pStdHrdParameters->cpb_size_du_scale = u(4);
            }
            pStdHrdParameters->initial_cpb_removal_delay_length_minus1 = u(5);
            pStdHrdParameters->au_cpb_removal_delay_length_minus1 = u(5);
            pStdHrdParameters->dpb_output_delay_length_minus1 = u(5);
        }
    }
    assert(maxNumSubLayersMinus1 < STD_VIDEO_H265_SUBLAYERS_LIST_SIZE);
    for (uint32_t i = 0; i <= maxNumSubLayersMinus1; i++)
    {
        bool fixed_pic_rate_within_cvs_flag = false;
        bool fixed_pic_rate_general_flag = u(1);
        if (!fixed_pic_rate_general_flag) {
            fixed_pic_rate_within_cvs_flag = u(1);
        } else {
            pStdHrdParameters->flags.fixed_pic_rate_general_flag |= (1 << i);
            fixed_pic_rate_within_cvs_flag = true;
        }
        if (fixed_pic_rate_within_cvs_flag) {
            pStdHrdParameters->flags.fixed_pic_rate_within_cvs_flag |= (1 << i);
        }

        bool low_delay_hrd_flag = false;
        if (fixed_pic_rate_within_cvs_flag) {
            pStdHrdParameters->elemental_duration_in_tc_minus1[i] = (uint16_t)ue();
            pStdHrdParameters->flags.low_delay_hrd_flag &= ~(1 << i);
        } else {
            low_delay_hrd_flag = u(1);
            if (low_delay_hrd_flag) {
                pStdHrdParameters->flags.low_delay_hrd_flag |= (1 << i);
            }
        }
        if (!low_delay_hrd_flag) {
            pStdHrdParameters->cpb_cnt_minus1[i] = (uint8_t)ue();
        } else {
            pStdHrdParameters->cpb_cnt_minus1[i] = 0;
        }
        if (pStdHrdParameters->flags.nal_hrd_parameters_present_flag) {
            sub_layer_hrd_parameters(pStdHrdParameters->stdSubLayerHrdParametersNal, i,
                                     pStdHrdParameters->cpb_cnt_minus1[i],
                                     pStdHrdParameters->flags.sub_pic_hrd_params_present_flag);
        }
        if (pStdHrdParameters->flags.vcl_hrd_parameters_present_flag) {
            sub_layer_hrd_parameters(pStdHrdParameters->stdSubLayerHrdParametersVcl, i,
                                     pStdHrdParameters->cpb_cnt_minus1[i],
                                     pStdHrdParameters->flags.sub_pic_hrd_params_present_flag);
        }
    }

    pStdHrdParameters->BindSubLayers(maxNumSubLayersMinus1);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Slice layer
//

bool VulkanH265Decoder::slice_header(int nal_unit_type, int nuh_temporal_id_plus1)
{

    const bool RapPicFlag = (nal_unit_type >= NUT_BLA_W_LP) && (nal_unit_type <= NUT_CRA_NUT); // spec includes 2 reserved values (<=23)
    const bool IdrPicFlag = (nal_unit_type == NUT_IDR_W_RADL) || (nal_unit_type == NUT_IDR_N_LP);

    // defaults
    hevc_slice_header_s slhtmp;
    hevc_slice_header_s *slh = &slhtmp;
    memset(slh, 0, sizeof(hevc_slice_header_s));
    slh->nal_unit_type = (uint8_t) nal_unit_type;
    slh->nuh_temporal_id_plus1 = (uint8_t)nuh_temporal_id_plus1;
    slh->pic_output_flag = 1;
    slh->collocated_from_l0_flag = 1;

    slh->first_slice_segment_in_pic_flag = (uint8_t)u(1);
    if (RapPicFlag) {
        slh->no_output_of_prior_pics_flag = (uint8_t)u(1);
    }
    uint32_t pic_parameter_set_id = ue();
    slh->pic_parameter_set_id = (uint8_t)pic_parameter_set_id;

    if (pic_parameter_set_id >= MAX_NUM_PPS) {
         nvParserLog("Invalid pic_parameter_set_id id in slice header (pps_id=%d)\n", pic_parameter_set_id);
         return false;
     }

    const hevc_pic_param_s *pps = m_ppss[pic_parameter_set_id];
    if (pps == nullptr) {
        nvParserLog("Invalid PPS slot id in slice header (pps_id=%d)\n", pic_parameter_set_id);
        return false;
    }

    const hevc_seq_param_s* sps = m_spss[pps->pps_seq_parameter_set_id];
    if (sps == nullptr) {
        nvParserLog("Invalid SPS slot id in slice header (pps_id=%d)\n", pic_parameter_set_id);
        return false;
    }

    const hevc_video_param_s* vps = m_vpss[sps->sps_video_parameter_set_id];
    if ((m_nuh_layer_id > 0) && (vps == NULL)) {
        nvParserLog("Invalid value of HEVC video parameters\n");
        return false;
    }
    int Log2CtbSizeY = sps->log2_min_luma_coding_block_size_minus3 + 3 + sps->log2_diff_max_min_luma_coding_block_size;
    int PicWidthInCtbsY  = (sps->pic_width_in_luma_samples  + (1 << Log2CtbSizeY) - 1) / (1 << Log2CtbSizeY);
    int PicHeightInCtbsY = (sps->pic_height_in_luma_samples + (1 << Log2CtbSizeY) - 1) / (1 << Log2CtbSizeY);
    uint32_t PicSizeInCtbsY = PicWidthInCtbsY * PicHeightInCtbsY;
    if (PicSizeInCtbsY > (1 << 24)) {
        nvParserLog("Unsupported sequence (PicSizeInCtbsY=%d)\n", PicSizeInCtbsY);
        return false;
    }

    bool dependent_slice_segment_flag = false;
    if (!slh->first_slice_segment_in_pic_flag) {
        if (pps->flags.dependent_slice_segments_enabled_flag) {
            dependent_slice_segment_flag = u(1);
        }
        slh->slice_segment_address = u(CeilLog2(PicSizeInCtbsY));
        if ((slh->slice_segment_address < 1) || (slh->slice_segment_address >= PicSizeInCtbsY)) {
            nvParserLog("Invalid slice segment address (%d)\n", slh->slice_segment_address);
            return false;
        }
    }
    if (dependent_slice_segment_flag) {
        const hevc_slice_header_s *slhold = &m_slh;
        uint32_t slice_segment_address = slh->slice_segment_address;
        if ((slh->nal_unit_type != slhold->nal_unit_type)
             || (slh->no_output_of_prior_pics_flag != slhold->no_output_of_prior_pics_flag)
             || (slh->pic_parameter_set_id != slhold->pic_parameter_set_id)) {

            nvParserLog("Missing first slice!\n");
            return false;
        }
        *slh = *slhold;
        slh->first_slice_segment_in_pic_flag = 0;
        slh->slice_segment_address = slice_segment_address;
    } else { // (!dependent_slice_segment_flag)

        uint32_t slice_type;

        if (pps->num_extra_slice_header_bits)
            u(pps->num_extra_slice_header_bits);
        slice_type = ue();
        if (slice_type > 2)
        {
            nvParserLog("Invalid slice_type (%d)\n", slice_type);
            return false;
        }
        slh->slice_type = (uint8_t)slice_type;
        if (pps->flags.output_flag_present_flag) {
            slh->pic_output_flag = (uint8_t)u(1);
        }
        if (sps->flags.separate_colour_plane_flag) {
            slh->colour_plane_id = (uint8_t)u(2);
            if (slh->colour_plane_id > 2) {
                nvParserLog("Invalid colour_plane_id (%d)\n", slh->colour_plane_id);
                return false;
            }
        }
        if (((m_nuh_layer_id > 0) && !vps->poc_lsb_not_present_flag[vps->LayerIdxInVps[m_nuh_layer_id]]) || !IdrPicFlag) {
            slh->pic_order_cnt_lsb = (uint16_t)u(sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
        }

        m_NumPocTotalCurr = 0;
        if (!IdrPicFlag) {
            slh->short_term_ref_pic_set_sps_flag = (uint8_t)u(1);
            if (!slh->short_term_ref_pic_set_sps_flag) {
                int bitcnt = consumed_bits();
                StdVideoH265ShortTermRefPicSet stdShortTermRefPicSet;
                if (!short_term_ref_pic_set(&stdShortTermRefPicSet,
                                            &slh->strps, sps->strpss,
                                            sps->num_short_term_ref_pic_sets, sps->num_short_term_ref_pic_sets)) {
                    return false;
                }
                slh->NumBitsForShortTermRPSInSlice = consumed_bits() - bitcnt;
            } else {
                if (sps->num_short_term_ref_pic_sets > 1) {
                    int v = CeilLog2(sps->num_short_term_ref_pic_sets);
                    slh->short_term_ref_pic_set_idx = (uint8_t)u(v);
                }
                if (slh->short_term_ref_pic_set_idx >= sps->num_short_term_ref_pic_sets) {
                    nvParserLog("Invalid short_term_ref_pic_set_idx (%d/%d)\n", slh->short_term_ref_pic_set_idx, sps->num_short_term_ref_pic_sets);
                    return false;
                }
            }
            if (sps->flags.long_term_ref_pics_present_flag) {
                assert(sps->pLongTermRefPicsSps);
                if (sps->num_long_term_ref_pics_sps) {
                    uint32_t num_long_term_sps = ue();
                    if (slh->num_long_term_sps > sps->num_long_term_ref_pics_sps) {
                        nvParserLog("Invalid num_long_term_sps (%d/%d)\n", num_long_term_sps, sps->num_long_term_ref_pics_sps);
                        return false;
                    }
                    slh->num_long_term_sps = (uint8_t)num_long_term_sps;
                }
                slh->num_long_term_pics = (uint8_t)ue();
                if ((slh->num_long_term_pics > MAX_NUM_REF_PICS)
                 || (slh->num_long_term_sps + slh->num_long_term_pics > MAX_NUM_REF_PICS)) {
                    nvParserLog("Invalid num_long_term_sps + num_long_term_pics (%d + %d)\n", slh->num_long_term_sps, slh->num_long_term_pics);
                    return false;
                }
                for (uint32_t i = 0; i < (uint32_t)(slh->num_long_term_sps + slh->num_long_term_pics); i++) {
                    if (i < slh->num_long_term_sps) {
                        if (sps->num_long_term_ref_pics_sps > 1) {
                            int v = CeilLog2(sps->num_long_term_ref_pics_sps);
                            slh->lt_idx_sps[i] = (uint8_t)u(v);
                        }
                    } else {
                        slh->poc_lsb_lt[i] = (uint16_t)u(sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
                        slh->used_by_curr_pic_lt_flags |= u(1) << i;
                    }
                    if (u(1)) { // delta_poc_msb_present_flag[i]

                        slh->delta_poc_msb_present_flags |= 1 << i;
                        slh->delta_poc_msb_cycle_lt[i] = ue();
                    }
                }
            }
            if (sps->flags.sps_temporal_mvp_enabled_flag) {
                slh->slice_temporal_mvp_enabled_flag = (uint8_t) u(1);
            }

            const short_term_ref_pic_set_s *strps = !slh->short_term_ref_pic_set_sps_flag ? &slh->strps : &sps->strpss[slh->short_term_ref_pic_set_idx];
            for (uint32_t i = 0; i < strps->NumNegativePics; i++) {
                m_NumPocTotalCurr += strps->UsedByCurrPicS0[i];
            }
            for (uint32_t  i = 0; i < strps->NumPositivePics; i++) {
                m_NumPocTotalCurr += strps->UsedByCurrPicS1[i];
            }
            for (uint32_t i = 0; i < (uint32_t )(slh->num_long_term_sps + slh->num_long_term_pics); i++) {
                int UsedByCurrPicLt =
                    (i < slh->num_long_term_sps) ? ((sps->stdLongTermRefPicsSps.used_by_curr_pic_lt_sps_flag >> slh->lt_idx_sps[i]) & 1)
                                                 : ((slh->used_by_curr_pic_lt_flags >> i) & 1);
                m_NumPocTotalCurr += UsedByCurrPicLt;
            }
        }
    }

    if (m_nuh_layer_id > 0 && !vps->privFlags.default_ref_layers_active_flag &&
          vps->numDirectRefLayers[m_nuh_layer_id] > 0) {
        slh->inter_layer_pred_enabled_flag = (uint8_t) u(1);

        if (slh->inter_layer_pred_enabled_flag && vps->numDirectRefLayers[m_nuh_layer_id] > 1) {
            if (!vps->privFlags.max_one_active_ref_layer_flag) {
                uint32_t codelength = CeilLog2(vps->numDirectRefLayers[m_nuh_layer_id]);
                slh->num_inter_layer_ref_pics_minus1 = (uint8_t) u(codelength);

                getNumActiveRefLayerPics(vps, slh);

                if (slh->numActiveRefLayerPics != vps->numDirectRefLayers[m_nuh_layer_id]) {
                    for (uint32_t i = 0; i < slh->numActiveRefLayerPics; i++) {
                        codelength = CeilLog2(vps->numDirectRefLayers[m_nuh_layer_id]);
                        slh->inter_layer_pred_layer_idc[i] = (uint8_t) u(codelength);
                    }
                }
            }
        }
    }

    if (m_nuh_layer_id > 0) {
        getNumActiveRefLayerPics(vps, slh);
    }
    if (sps->flags.sample_adaptive_offset_enabled_flag) {
        u(1 + 1); // slice_sao_luma_flag, slice_sao_chroma_flag
    }
    if (slh->slice_type == SLICE_TYPE_P || slh->slice_type == SLICE_TYPE_B) {
        if (u(1)) { // num_ref_idx_active_override_flag
            slh->num_ref_idx_l0_active_minus1 = (uint8_t) ue();
            if(slh->slice_type == SLICE_TYPE_B)
                slh->num_ref_idx_l1_active_minus1 = (uint8_t) ue();
        } else {
            slh->num_ref_idx_l0_active_minus1 = pps->num_ref_idx_l0_default_active_minus1;
            slh->num_ref_idx_l1_active_minus1 = pps->num_ref_idx_l1_default_active_minus1;
        }
        if (slh->slice_type != SLICE_TYPE_B) {
            slh->num_ref_idx_l1_active_minus1 = 0;
        }
    }

    m_slh = *slh;
    return true;
}

uint32_t VulkanH265Decoder::getNumRefLayerPics(const hevc_video_param_s* vps, hevc_slice_header_s *pSliceHeader)
{
    uint32_t numRefLayerPics = 0;
    uint32_t i, j;
    for (i = 0, j = 0; i < vps->numDirectRefLayers[m_nuh_layer_id]; i++)
    {
        uint32_t refLayerIdx = vps->LayerIdxInVps[vps->idDirectRefLayer[m_nuh_layer_id][i]];
        if (vps->sub_layers_vps_max_minus1[refLayerIdx] >= (pSliceHeader->nuh_temporal_id_plus1 -1) &&
               ((pSliceHeader->nuh_temporal_id_plus1 - 1) == 0 || vps->max_tid_il_ref_pics_plus1[refLayerIdx][vps->LayerIdxInVps[m_nuh_layer_id]]))
        {
            j++;
        }
    }
    numRefLayerPics = j;
    return numRefLayerPics;
}

void VulkanH265Decoder::getNumActiveRefLayerPics(const hevc_video_param_s* vps, hevc_slice_header_s *pSliceHeader)
{
    if (m_nuh_layer_id == 0 ||  getNumRefLayerPics(vps, pSliceHeader) == 0)
    {
        pSliceHeader->numActiveRefLayerPics = 0;
    }
    else if (vps->privFlags.default_ref_layers_active_flag)
    {
        pSliceHeader->numActiveRefLayerPics = getNumRefLayerPics(vps, pSliceHeader);
    }
    else if (!pSliceHeader->inter_layer_pred_enabled_flag)
    {
        pSliceHeader->numActiveRefLayerPics = 0;
    }
    else if (vps->privFlags.max_one_active_ref_layer_flag || (vps->numDirectRefLayers[m_nuh_layer_id] == 1))
    {
        pSliceHeader->numActiveRefLayerPics = 1;
    }
    else
    {
        pSliceHeader->numActiveRefLayerPics = pSliceHeader->num_inter_layer_ref_pics_minus1 + 1 ;
    }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// DPB management
//

static int32_t GetMaxDpbSize(VkSharedBaseObj<hevc_seq_param_s>& sps)
{
    int32_t MaxLumaPS = 0;
    switch (sps->stdProfileTierLevel.general_level_idc) { // Table A.8 – General tier and level limits
        case STD_VIDEO_H265_LEVEL_IDC_1_0: MaxLumaPS =    36864; break;
        case STD_VIDEO_H265_LEVEL_IDC_2_0: MaxLumaPS =   122880; break;
        case STD_VIDEO_H265_LEVEL_IDC_2_1: MaxLumaPS =   245760; break;
        case STD_VIDEO_H265_LEVEL_IDC_3_0: MaxLumaPS =   552960; break;
        case STD_VIDEO_H265_LEVEL_IDC_3_1: MaxLumaPS =   983040; break;
        case STD_VIDEO_H265_LEVEL_IDC_4_0: MaxLumaPS =  2228224; break;
        case STD_VIDEO_H265_LEVEL_IDC_4_1: MaxLumaPS =  2228224; break;
        case STD_VIDEO_H265_LEVEL_IDC_5_0: MaxLumaPS =  8912896; break;
        case STD_VIDEO_H265_LEVEL_IDC_5_1: MaxLumaPS =  8912896; break;
        case STD_VIDEO_H265_LEVEL_IDC_5_2: MaxLumaPS =  8912896; break;
        case STD_VIDEO_H265_LEVEL_IDC_6_0: MaxLumaPS = 35651584; break;
        case STD_VIDEO_H265_LEVEL_IDC_6_1: MaxLumaPS = 35651584; break;
        case STD_VIDEO_H265_LEVEL_IDC_6_2: MaxLumaPS = 35651584; break;
        case STD_VIDEO_H265_LEVEL_IDC_MAX_ENUM:
        default:
            MaxLumaPS = 35651584;
            break;
    }

    // From A.4.1 General tier and level limits
    const int32_t PicSizeInSamplesY = sps->pic_width_in_luma_samples * sps->pic_height_in_luma_samples;
    const int32_t MaxDpbPicBuf = 6;
    int MaxDpbSize = 0;

    if (PicSizeInSamplesY <= (MaxLumaPS >> 2)) {
        MaxDpbSize = MaxDpbPicBuf * 4;
    } else if (PicSizeInSamplesY <= (MaxLumaPS >> 1)) {
        MaxDpbSize = MaxDpbPicBuf * 2;
    } else if (PicSizeInSamplesY <= ((3 * MaxLumaPS) >> 2)) {
        MaxDpbSize = (MaxDpbPicBuf * 4) / 3;
    } else {
        MaxDpbSize = MaxDpbPicBuf;
    }
    return std::min<int32_t>(MaxDpbSize, HEVC_DPB_SIZE);
}

bool VulkanH265Decoder::dpb_sequence_start(VkSharedBaseObj<hevc_seq_param_s>& sps)
{
    VkParserSequenceInfo nvsi;
    uint32_t pic_width_in_luma_samples, pic_height_in_luma_samples;
    uint32_t Log2SubWidthC, Log2SubHeightC;

    m_active_sps[m_nuh_layer_id] = sps;
    memset(&nvsi, 0, sizeof(nvsi));
    pic_width_in_luma_samples = sps->pic_width_in_luma_samples;
    pic_height_in_luma_samples = sps->pic_height_in_luma_samples;
    nvsi.eCodec = VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR;
    nvsi.frameRate = NV_FRAME_RATE_UNKNOWN;
    if ((sps->stdVui.flags.vui_timing_info_present_flag)
     && (sps->stdVui.vui_num_units_in_tick > 0)
     && (sps->stdVui.vui_time_scale > sps->stdVui.vui_num_units_in_tick)) // >= 1Hz
    {
        nvsi.frameRate = PackFrameRate(sps->stdVui.vui_time_scale, sps->stdVui.vui_num_units_in_tick);
    }
    nvsi.bProgSeq = 1; //!sps->vui.field_seq_flag;
    nvsi.nCodedWidth = (pic_width_in_luma_samples + 0xf) & ~0xf;
    nvsi.nCodedHeight = (pic_height_in_luma_samples + 0xf) & ~0xf;
    Log2SubWidthC = (sps->chroma_format_idc == 1) || (sps->chroma_format_idc == 2);
    Log2SubHeightC = (sps->chroma_format_idc == 1);
    nvsi.nDisplayWidth = pic_width_in_luma_samples - (sps->conf_win_right_offset << Log2SubWidthC);
    nvsi.nDisplayHeight = pic_height_in_luma_samples - (sps->conf_win_bottom_offset << Log2SubHeightC);
    nvsi.nChromaFormat = sps->chroma_format_idc;
    nvsi.uBitDepthLumaMinus8 = sps->bit_depth_luma_minus8;
    nvsi.uBitDepthChromaMinus8 = sps->bit_depth_chroma_minus8;
    nvsi.lDARWidth = nvsi.nDisplayWidth;
    nvsi.lDARHeight = nvsi.nDisplayHeight;
    nvsi.lVideoFormat = VideoFormatUnspecified;
    nvsi.lColorPrimaries = ColorPrimariesUnspecified;
    nvsi.lTransferCharacteristics = TransferCharacteristicsUnspecified;
    nvsi.lMatrixCoefficients = MatrixCoefficientsUnspecified;
    /*
      In case of reconfigure we could have m_max_dec_pic_buffering containing old value. Use max val to be safe.
      We need +3 to accomodate all cases (streams with too small m_max_dec_pic_buffering, error streams).
    */
    nvsi.nMinNumDecodeSurfaces = std::max<int32_t>(m_max_dec_pic_buffering + 3, sps->max_dec_pic_buffering + 3);
    if ((sps->stdVui.sar_width > 0) && (sps->stdVui.sar_height > 0))
    {
        nvsi.lDARWidth = sps->stdVui.sar_width * nvsi.nDisplayWidth;
        nvsi.lDARHeight = sps->stdVui.sar_height * nvsi.nDisplayHeight;
    }
    if (sps->stdVui.flags.video_signal_type_present_flag)
    {
        nvsi.lVideoFormat = sps->stdVui.video_format;
        nvsi.uVideoFullRange = sps->stdVui.flags.video_full_range_flag;
        if (sps->stdVui.flags.colour_description_present_flag)
        {
            nvsi.lColorPrimaries = sps->stdVui.colour_primaries;
            nvsi.lTransferCharacteristics = sps->stdVui.transfer_characteristics;
            nvsi.lMatrixCoefficients = sps->stdVui.matrix_coeffs;
        }
    }
    SimplifyAspectRatio(&nvsi.lDARWidth, &nvsi.lDARHeight);

    // From A.4.1 General tier and level limits
    const int32_t MaxDpbSize = GetMaxDpbSize(sps);
    nvsi.nMinNumDpbSlots = MaxDpbSize;
    nvsi.codecProfile = sps->pProfileTierLevel->general_profile_idc;

    // Update codecProfile in case the general_profile_idc is found to be
    // zero in bitstream. The general_profile_idc = 0 means that the bitstream
    // conforms to one of the valid profiles as per H.265 standard.
    if (sps->pProfileTierLevel->general_profile_idc == 0) {
        if (sps->bit_depth_luma_minus8 == 0) {
            nvsi.codecProfile = STD_VIDEO_H265_PROFILE_IDC_MAIN;
        } else if (sps->bit_depth_luma_minus8 == 2 || sps->bit_depth_luma_minus8 == 4) {
            nvsi.codecProfile = STD_VIDEO_H265_PROFILE_IDC_MAIN_10;
        }
    }

    if (!init_sequence(&nvsi)) {
        return false;
    }

    if (m_MaxFrameBuffers > 0)
    {
        m_MaxDpbSize = std::min<int32_t>(m_MaxFrameBuffers, MaxDpbSize);
    }
    return true;
}


void VulkanH265Decoder::flush_decoded_picture_buffer(int NoOutputOfPriorPicsFlag)
{
    // mark all reference pictures as "unused for reference"
    // empty frame buffers marked as "not needed for output" and "unused for reference"
    for (int i = 0; i < HEVC_DPB_SIZE; i++)
    {
        m_dpb[i].marking = 0;
        if (NoOutputOfPriorPicsFlag)
        {
            m_dpb[i].output = 0;
        }
        if (m_dpb[i].state == 1 && m_dpb[i].output == 0 && m_dpb[i].marking == 0)
        {
            m_dpb[i].state = 0; // empty
            if (m_dpb[i].pPicBuf)
            {
                m_dpb[i].pPicBuf->Release();
                m_dpb[i].pPicBuf = NULL;
            }
        }
    }

    while (!dpb_empty())
    {
        bool ret = dpb_bumping(0);
        if (ret == false)
            break;
    }

    // Release all frame buffers (mostly redundant with the above, but includes the current entry in case something went wrong)
    for (int i = 0; i < HEVC_DPB_SIZE; i++)
    {
        m_dpb[i].state = 0;
        m_dpb[i].marking = 0;
        if (m_dpb[i].pPicBuf)
        {
            m_dpb[i].pPicBuf->Release();
            m_dpb[i].pPicBuf = NULL;
        }
    }
}


int VulkanH265Decoder::dpb_fullness()
{
    int numDpbPictures = 0, i;
    for (i = 0; i < HEVC_DPB_SIZE; i++)
    {
        if (m_dpb[i].state == 1)
            numDpbPictures++;
    }
    return numDpbPictures;
}

int VulkanH265Decoder::dpb_reordering_delay()
{
    int numReorderPics = 0, i;
    for (i = 0; i < HEVC_DPB_SIZE; i++)
    {
        if ((m_dpb[i].LayerId == m_nuh_layer_id) &&
                (m_dpb[i].state == 1) && (m_dpb[i].output != 0)) {
            numReorderPics++;
        }
    }
    return numReorderPics;
}


bool VulkanH265Decoder::dpb_bumping(int maxAllowedDpbSize)
{
    int iMin = -1, iMin2 = -1;
    int pocMin = 0;
    int i;

    for (i = 0; i < HEVC_DPB_SIZE; i++)
    {
        if (m_dpb[i].state == 1)
        {
            if (m_dpb[i].output && (iMin < 0 || (m_dpb[i].PicOrderCntVal < pocMin) ||
                    ((m_dpb[i].PicOrderCntVal == pocMin) && (m_dpb[i].LayerId < m_dpb[iMin].LayerId))))
            {
                pocMin = m_dpb[i].PicOrderCntVal;
                iMin = i;
            }
            else if ((iMin2 < 0) || (m_dpb[i].PicOrderCntVal < m_dpb[iMin2].PicOrderCntVal))
            {
                iMin2 = i;
            }
        }
    }

    if (iMin < 0)
    {
        iMin = iMin2;
        /* Allow exceeding DPB size up to m_MaxDpbSize -1. This allows decoding of non-compliant streams
         * which may have too small value of max_dec_pic_buffering. This is for new picture start
         * For flushing DPB and deinit, this value is 0, thereby removing the entries as intended. */
        if (dpb_fullness() < maxAllowedDpbSize)
            return false;
        if (iMin < 0)   // Was called with an empty DPB
            return false;
        m_dpb[iMin].marking = 0;    // force removal
        nvParserLog("WARNING: DPB overflow\n");
    }

    if (m_dpb[iMin].output) // Always true except for DPB overflow
    {
        output_picture(iMin);
        m_dpb[iMin].output = 0;
    }

    if (m_dpb[iMin].marking == 0)
    {
        m_dpb[iMin].state = 0;
        if (m_dpb[iMin].pPicBuf)
        {
            m_dpb[iMin].pPicBuf->Release();
            m_dpb[iMin].pPicBuf = NULL;
        }
    }
    return true;
}


void VulkanH265Decoder::output_picture(int nframe)
{
    if (m_dpb[nframe].pPicBuf)
    {
        display_picture(m_dpb[nframe].pPicBuf);
    }
}


void VulkanH265Decoder::dpb_picture_start(VkSharedBaseObj<hevc_pic_param_s>& pps, hevc_slice_header_s *slh)
{
    hevc_dpb_entry_s *cur;
    int i, iCur, dpbSize;
    bool ret;

    m_active_pps[m_nuh_layer_id] = pps;
    m_bPictureStarted = true;
    m_NumDeltaPocsOfRefRpsIdx = 0;
    if (slh->strps.inter_ref_pic_set_prediction_flag && m_active_sps[m_nuh_layer_id])
    {
        StdVideoH265SequenceParameterSet* p_active_sps(*m_active_sps[m_nuh_layer_id]);
        int RIdx = p_active_sps->num_short_term_ref_pic_sets - (slh->strps.delta_idx_minus1 + 1);
        if (RIdx >= 0) {
            m_NumDeltaPocsOfRefRpsIdx = m_active_sps[m_nuh_layer_id]->strpss[RIdx].NumNegativePics +
                                        m_active_sps[m_nuh_layer_id]->strpss[RIdx].NumPositivePics;
        }
    }

    bool isIrapPic = slh->nal_unit_type >= NUT_BLA_W_LP && slh->nal_unit_type <= 23;

    int PicOrderCntVal = picture_order_count(slh);
    reference_picture_set(slh, PicOrderCntVal);
    int PicOutputFlag = (((slh->nal_unit_type == NUT_RASL_N) || (slh->nal_unit_type == NUT_RASL_R)) && NoRaslOutputFlag) ? 0 : slh->pic_output_flag;
    if (isIrapPic && NoRaslOutputFlag)
    {
        int NoOutputOfPriorPicsFlag = (slh->nal_unit_type == NUT_CRA_NUT) ? 1 : slh->no_output_of_prior_pics_flag;
        if (NoOutputOfPriorPicsFlag)
        {
            for (i = 0; i < HEVC_DPB_SIZE; i++)
            {
                if (m_dpb[i].LayerId == m_nuh_layer_id)
                {
                    m_dpb[i].state = 0;
                    m_dpb[i].marking = 0;
                    m_dpb[i].output = 0;
                }
            }
        }
    }
    for (i = 0; i < HEVC_DPB_SIZE; i++)
    {
        if (m_dpb[i].marking == 0 && m_dpb[i].output == 0)
        {
            m_dpb[i].state = 0;
            if (m_dpb[i].pPicBuf)
            {
                m_dpb[i].pPicBuf->Release();
                m_dpb[i].pPicBuf = NULL;
            }
        }
    }
    // Make room in DPB
    dpbSize = std::min<int32_t>(m_max_dec_pic_buffering, m_MaxDpbSize);
    if (dpbSize <= 0) {
        dpbSize = 1;
    } if (dpbSize > HEVC_DPB_SIZE) {
        dpbSize = HEVC_DPB_SIZE;
    }
    while (dpb_fullness() >= dpbSize)
    {
        ret = dpb_bumping(m_MaxDpbSize - 1);
        if (ret == false)
            break;
    }

    // select decoded picture buffer
    for (iCur = 0; iCur < HEVC_DPB_SIZE; iCur++)
    {
        if (m_dpb[iCur].state == 0)
            break;
    }
    // initialize DPB frame buffer
    cur = &m_dpb[iCur];
    cur->PicOrderCntVal = PicOrderCntVal;
    cur->LayerId = m_nuh_layer_id;
    cur->output = PicOutputFlag;
    if (!cur->pPicBuf)
    {
        if (!m_pClient->AllocPictureBuffer(&cur->pPicBuf))
        {
            nvParserLog("WARNING: Failed to allocate frame buffer picture\n");
        }
    }
    m_dpb_cur = cur;
    m_current_dpb_id = iCur;
}


void VulkanH265Decoder::dpb_picture_end()
{
    hevc_dpb_entry_s * const cur = m_dpb_cur;
    bool ret;

    if (!m_bPictureStarted || !cur)
        return;
    m_bPictureStarted = false;

    cur->state = 1;
    cur->marking = 1;
    // Apply max reordering delay now to minimize decode->display latency
    assert(m_active_sps[m_nuh_layer_id]);
    while (dpb_reordering_delay() > m_active_sps[m_nuh_layer_id]->max_num_reorder_pics)
    {
        // NOTE: This should never actually evict any references from the dpb (just output for display)
        ret = dpb_bumping(m_MaxDpbSize - 1);
        if (ret == false)
            break;
    }
}


// 8.3.1 Decoding process for picture order count
int VulkanH265Decoder::picture_order_count(hevc_slice_header_s *slh)
{
    const hevc_seq_param_s *sps = m_active_sps[m_nuh_layer_id];
    assert(sps);
    bool isIrapPic = slh->nal_unit_type >= NUT_BLA_W_LP && slh->nal_unit_type <= 23;
    int PicOrderCntMsb;

    if (isIrapPic && NoRaslOutputFlag)
    {
        PicOrderCntMsb = 0;
    }
    else
    {
        int MaxPicOrderCntLsb = 1 << (sps->log2_max_pic_order_cnt_lsb_minus4 + 4);

        if ((slh->pic_order_cnt_lsb < m_prevPicOrderCntLsb) && ((m_prevPicOrderCntLsb - slh->pic_order_cnt_lsb) >= (MaxPicOrderCntLsb / 2)))
            PicOrderCntMsb = m_prevPicOrderCntMsb + MaxPicOrderCntLsb;
        else if ((slh->pic_order_cnt_lsb > m_prevPicOrderCntLsb) && ((slh->pic_order_cnt_lsb - m_prevPicOrderCntLsb) > (MaxPicOrderCntLsb / 2)))
            PicOrderCntMsb = m_prevPicOrderCntMsb - MaxPicOrderCntLsb;
        else
            PicOrderCntMsb = m_prevPicOrderCntMsb;
    }

    int PicOrderCntVal = PicOrderCntMsb + slh->pic_order_cnt_lsb;

    int TemporalId = slh->nuh_temporal_id_plus1 - 1;
    bool isSubLayerNonRef =
            (slh->nal_unit_type == NUT_TRAIL_N) ||
            (slh->nal_unit_type == NUT_TSA_N) ||
            (slh->nal_unit_type == NUT_STSA_N) ||
            (slh->nal_unit_type == NUT_RADL_N) ||
            (slh->nal_unit_type == NUT_RASL_N) ||
            (slh->nal_unit_type == 10) ||
            (slh->nal_unit_type == 12) ||
            (slh->nal_unit_type == 14); // (= all even values < 16)
    if ((TemporalId == 0)
         && !(slh->nal_unit_type >= NUT_RADL_N && slh->nal_unit_type <= NUT_RASL_R)
         && !isSubLayerNonRef)
    {
        m_prevPicOrderCntLsb = slh->pic_order_cnt_lsb;
        m_prevPicOrderCntMsb = PicOrderCntMsb;
    }
    return PicOrderCntVal;
}

// 8.3.2 Decoding process for reference picture set
void VulkanH265Decoder::reference_picture_set(hevc_slice_header_s *slh, int PicOrderCntVal)
{
    int PocStCurrBefore[16], PocStCurrAfter[16], PocStFoll[16], PocLtCurr[16], PocLtFoll[16];
    int NumPocStCurrBefore, NumPocStCurrAfter, NumPocLtCurr;
    int NumPocStFoll, NumPocLtFoll;
    int numActiveRefLayerPics0 = 0, numActiveRefLayerPics1 = 0;
    int CurrDeltaPocMsbPresentFlag[16], FollDeltaPocMsbPresentFlag[16];
    const hevc_seq_param_s *sps = m_active_sps[m_nuh_layer_id];
    assert(sps);
    const hevc_video_param_s *vps = m_active_vps;
    hevc_dpb_entry_s * const dpb = m_dpb;
    int MaxPicOrderCntLsb = 1 << (sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
    bool isIrapPic = slh->nal_unit_type >= NUT_BLA_W_LP && slh->nal_unit_type <= 23;

    if (isIrapPic && NoRaslOutputFlag)
    {
        for (int i = 0; i < HEVC_DPB_SIZE; i++)
            if (dpb[i].LayerId == m_nuh_layer_id)
                dpb[i].marking = 0;
    }

    if (slh->nal_unit_type == NUT_IDR_W_RADL || slh->nal_unit_type == NUT_IDR_N_LP) // current picture is an IDR picture
    {
        NumPocStCurrBefore = 0;
        NumPocStCurrAfter  = 0;
        NumPocStFoll       = 0;
        NumPocLtCurr       = 0;
        NumPocLtFoll       = 0;
    }
    else
    {
        const short_term_ref_pic_set_s *strps = !slh->short_term_ref_pic_set_sps_flag ? &slh->strps : &sps->strpss[slh->short_term_ref_pic_set_idx];
        int j = 0;
        int k = 0;
        for (int i = 0; i < strps->NumNegativePics; i++)
        {
            if (strps->UsedByCurrPicS0[i])
                PocStCurrBefore[j++] = PicOrderCntVal + strps->DeltaPocS0[i];
            else
                PocStFoll[k++] = PicOrderCntVal + strps->DeltaPocS0[i];
        }
        NumPocStCurrBefore = j;

        j = 0;
        for (int i = 0; i < strps->NumPositivePics; i++)
        {
            if (strps->UsedByCurrPicS1[i])
                PocStCurrAfter[j++] = PicOrderCntVal + strps->DeltaPocS1[i];
            else
                PocStFoll[k++] = PicOrderCntVal + strps->DeltaPocS1[i];
        }
        NumPocStCurrAfter = j;
        NumPocStFoll = k;

        int PocLsbLt[16] = {0};
        bool UsedByCurrPicLt[16] = { false };
        int DeltaPocMSBCycleLt[16] = {0};

        for (int i = 0; i < slh->num_long_term_sps + slh->num_long_term_pics; i++)
        {
            if (i < slh->num_long_term_sps)
            {
                PocLsbLt[i] = sps->stdLongTermRefPicsSps.lt_ref_pic_poc_lsb_sps[slh->lt_idx_sps[i]];
                UsedByCurrPicLt[i] = ((sps->stdLongTermRefPicsSps.used_by_curr_pic_lt_sps_flag >> slh->lt_idx_sps[i]) & 1);
            }
            else
            {
                PocLsbLt[i] = slh->poc_lsb_lt[i];
                UsedByCurrPicLt[i] = (slh->used_by_curr_pic_lt_flags >> i) & 1;
            }
            if (i == 0 || i == slh->num_long_term_sps)
                DeltaPocMSBCycleLt[i] = slh->delta_poc_msb_cycle_lt[i];
            else
                DeltaPocMSBCycleLt[i] = slh->delta_poc_msb_cycle_lt[i] + DeltaPocMSBCycleLt[i-1];
        }
        j = 0;
        k = 0;
        for (int i = 0; i < slh->num_long_term_sps + slh->num_long_term_pics; i++)
        {
            int pocLt = PocLsbLt[i];
            if (slh->delta_poc_msb_present_flags & (1<<i))
                pocLt += PicOrderCntVal - DeltaPocMSBCycleLt[i] * MaxPicOrderCntLsb - slh->pic_order_cnt_lsb;
            if (UsedByCurrPicLt[i])
            {
                PocLtCurr[j] = pocLt;
                CurrDeltaPocMsbPresentFlag[j++] = (slh->delta_poc_msb_present_flags >> i) & 1;
            }
            else
            {
                PocLtFoll[k] = pocLt;
                FollDeltaPocMsbPresentFlag[k++] = (slh->delta_poc_msb_present_flags >> i) & 1;
            }
        }
        NumPocLtCurr = j;
        NumPocLtFoll = k;
    }
    int8_t RefPicSetStFoll[16], RefPicSetLtFoll[16];

    for (int i = 0; i < 16; i++)
    {
        // set all entries to "no reference picture"
        m_RefPicSetStCurrBefore[i] = -1;
        m_RefPicSetStCurrAfter[i]  = -1;
        RefPicSetStFoll[i]         = -1;
        m_RefPicSetLtCurr[i]       = -1;
        RefPicSetLtFoll[i]         = -1;
        m_RefPicSetInterLayer0[i]  = -1;
        m_RefPicSetInterLayer1[i]  = -1;
    }
    m_NumPocStCurrBefore = NumPocStCurrBefore;
    m_NumPocStCurrAfter = NumPocStCurrAfter;
    m_NumPocLtCurr = NumPocLtCurr;

    for (int i = 0; i < NumPocLtCurr; i++)
    {
        int mask = !CurrDeltaPocMsbPresentFlag[i] ? MaxPicOrderCntLsb-1 : ~0;
        // if there is a reference picture picX in the DPB with slice_pic_order_cnt_lsb equal to PocLtCurr[i]
        // if there is a reference picture picX in the DPB with PicOrderCntVal equal to PocLtCurr[i]
        for (int j = 0; j < HEVC_DPB_SIZE; j++)
        {
            if ((dpb[j].LayerId == m_nuh_layer_id) && (dpb[j].state == 1) && (dpb[j].marking != 0) && ((dpb[j].PicOrderCntVal & mask) == PocLtCurr[i]))
            {
                m_RefPicSetLtCurr[i] = (int8_t)j;
                break;
            }
        }
        if (m_RefPicSetLtCurr[i] < 0)
        {
            nvParserLog("long-term reference picture not available (POC=%d)\n", PocLtCurr[i]);
        }
    }

    for (int i = 0; i < NumPocLtFoll; i++)
    {
        int mask = !FollDeltaPocMsbPresentFlag[i] ? MaxPicOrderCntLsb-1 : ~0;
        // if there is a reference picture picX in the DPB with slice_pic_order_cnt_lsb equal to PocLtFoll[i]
        // if there is a reference picture picX in the DPB with PicOrderCntVal to PocLtFoll[i]
        for (int j = 0; j < HEVC_DPB_SIZE; j++)
        {
            if ((dpb[j].LayerId == m_nuh_layer_id) && (dpb[j].state == 1) && (dpb[j].marking != 0) && ((dpb[j].PicOrderCntVal & mask) == PocLtFoll[i]))
            {
                RefPicSetLtFoll[i] = (int8_t)j;
                break;
            }
        }
    }

    for (int i = 0; i < NumPocLtCurr; i++)
    {
        if (m_RefPicSetLtCurr[i] != -1)
            dpb[m_RefPicSetLtCurr[i]].marking = 2;
    }

    for (int i = 0; i < NumPocLtFoll; i++)
    {
        if (RefPicSetLtFoll[i] != -1)
            dpb[RefPicSetLtFoll[i]].marking = 2;
    }

    for (int i = 0; i < NumPocStCurrBefore; i++)
    {
        // if there is a short-term reference picture picX in the DPB with PicOrderCntVal equal to PocStCurrBefore[i]
        for (int j = 0; j < HEVC_DPB_SIZE; j++)
        {
            if ((dpb[j].LayerId == m_nuh_layer_id) && (dpb[j].state == 1) && (dpb[j].marking == 1) && (dpb[j].PicOrderCntVal == PocStCurrBefore[i]))
            {
                m_RefPicSetStCurrBefore[i] = (int8_t)j;
                break;
            }
        }
        if (m_RefPicSetStCurrBefore[i] < 0)
        {
            nvParserLog("short-term reference picture not available (POC=%d)\n", PocStCurrBefore[i]);
            m_RefPicSetStCurrBefore[i] = create_lost_ref_pic(PocStCurrBefore[i], m_nuh_layer_id, 1);
        }
    }

    for (int i = 0; i < NumPocStCurrAfter; i++)
    {
        // if there is a short-term reference picture picX in the DPB with PicOrderCntVal equal to PocStCurrAfter[i]
        for (int j = 0; j < HEVC_DPB_SIZE; j++)
        {
            if ((dpb[j].LayerId == m_nuh_layer_id) && (dpb[j].state == 1) && (dpb[j].marking == 1) && (dpb[j].PicOrderCntVal == PocStCurrAfter[i]))
            {
                m_RefPicSetStCurrAfter[i] = (int8_t)j;
                break;
            }
        }
        if (m_RefPicSetStCurrAfter[i] < 0)
        {
            nvParserLog("short-term reference picture not available (POC=%d)\n", PocStCurrAfter[i]);
            m_RefPicSetStCurrAfter[i] = create_lost_ref_pic(PocStCurrAfter[i], m_nuh_layer_id, 1);
        }
    }

    for (int i = 0; i < NumPocStFoll; i++)
    {
        // if there is a short-term reference picture picX in the DPB with PicOrderCntVal equal to PocStFoll[i]
        for (int j = 0; j < HEVC_DPB_SIZE; j++)
        {
            if ((dpb[j].LayerId == m_nuh_layer_id) && (dpb[j].state == 1) && (dpb[j].marking == 1) && (dpb[j].PicOrderCntVal == PocStFoll[i]))
            {
                RefPicSetStFoll[i] = (int8_t)j;
                break;
            }
        }
    }

    // Enhance layer
    if (m_nuh_layer_id > 0)
    {
        /* traversing layer prediction list */
        for (int i = 0; i < slh->numActiveRefLayerPics; i++)
        {
            unsigned int layerIdRef = slh->inter_layer_pred_layer_idc[i];
            unsigned int viewIdCur  = vps->view_id_val[m_nuh_layer_id];
            unsigned int viewIdZero = vps->view_id_val[0];
            unsigned int viewIdRef  = vps->view_id_val[layerIdRef];
            int j;
            // there is a picture picX in the DPB that is in the same access unit as the current picture and has nuh_layer_id equal to RefPicLayerId
            for (j = 0; j < 16; j++) {
                if (((unsigned int)m_dpb[j].LayerId == layerIdRef) && (m_dpb[j].state == 1) && m_dpb[j].marking && (m_dpb[j].PicOrderCntVal == PicOrderCntVal)) {
                    break;
                }
            }
            if (j < 16)
            {
                if ((viewIdCur <= viewIdZero && viewIdCur <= viewIdRef) || (viewIdCur >= viewIdZero && viewIdCur >= viewIdRef))
                {
                    m_RefPicSetInterLayer0[numActiveRefLayerPics0++] = (int8_t)j;
                }
                else
                {
                    m_RefPicSetInterLayer1[numActiveRefLayerPics1++] = (int8_t)j;
                }
            }
            else
            {
                nvParserLog("InterLayer reference picture not available (POC=%d)\n", PicOrderCntVal);
            }
        }
    }
    m_NumActiveRefLayerPics0 = numActiveRefLayerPics0;
    m_NumActiveRefLayerPics1 = numActiveRefLayerPics1;

    // All reference pictures in the decoded picture buffer that are not included in RefPicSetLtCurr, RefPicSetLtFoll, RefPicSetStCurrBefore, RefPicSetStCurrAfter or RefPicSetStFoll are marked as "unused for reference".
    uint32_t inUseMask = 0;

    for (int i = 0; i < NumPocLtCurr; i++) {
        if (m_RefPicSetLtCurr[i] >= 0) {
            inUseMask |= 1 << m_RefPicSetLtCurr[i];
        }
    }
    for (int i = 0; i < NumPocLtFoll; i++) {
        if (RefPicSetLtFoll[i] >= 0) {
            inUseMask |= 1 << RefPicSetLtFoll[i];
        }
    }
    for (int i = 0; i < NumPocStCurrBefore; i++) {
        if (m_RefPicSetStCurrBefore[i] >= 0) {
            inUseMask |= 1 << m_RefPicSetStCurrBefore[i];
        }
    }
    for (int i = 0; i < NumPocStCurrAfter; i++) {
        if (m_RefPicSetStCurrAfter[i] >= 0) {
            inUseMask |= 1 << m_RefPicSetStCurrAfter[i];
        }
    }
    for (int i = 0; i < NumPocStFoll; i++) {
        if (RefPicSetStFoll[i] >= 0) {
            inUseMask |= 1 << RefPicSetStFoll[i];
        }
    }
    for (int i = 0; i < HEVC_DPB_SIZE; i++) {
        if ((m_dpb[i].LayerId == m_nuh_layer_id) && !(inUseMask & 1)) {
            m_dpb[i].marking = 0;
        }
        inUseMask >>= 1;
    }
}


int VulkanH265Decoder::create_lost_ref_pic(int lostPOC, int layerID, int marking_flag)
{
    int returnDPBPos = -1;
    int closestPOC = INT_MAX;
    for (int i = 0; i < HEVC_DPB_SIZE; i++)
    {
        if ((m_dpb[i].LayerId == layerID) && m_dpb[i].state &&
                (m_dpb[i].marking == marking_flag) &&
                (abs((m_dpb[i].PicOrderCntVal - lostPOC)) < closestPOC) &&
                (abs((m_dpb[i].PicOrderCntVal - lostPOC)) != 0))
        {
            closestPOC = abs(m_dpb[i].PicOrderCntVal - lostPOC);
            returnDPBPos = i;
        }
    }
    if (returnDPBPos != -1)
    {
        nvParserLog("Generating reference picture %d instead of picture %d\n",m_dpb[returnDPBPos].PicOrderCntVal,lostPOC);
    }
    return returnDPBPos;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// SEI payload (D.2)
//

void VulkanH265Decoder::sei_payload()
{
    while (available_bits() >= 3 * 8)
    {
        int payloadType = 0, payloadSize = 0, bitsUsed, skip;

        while ((next_bits(8) == 0xff) && (available_bits() >= 8))
        {
            f(8, 0xff); // ff_byte
            payloadType += 255;
        }
        payloadType += u(8); // last_payload_type_byte
        while ((next_bits(8) == 0xff) && (available_bits() >= 8))
        {
            f(8, 0xff); // ff_byte
            payloadSize += 255;
        }
        payloadSize += u(8); // last_payload_size_byte
        if (available_bits() < payloadSize * 8)
        {
            nvParserLog("ignoring truncated SEI message (%d/%d)\n", payloadSize, available_bits() / 8);
            break;
        }
        bitsUsed = consumed_bits();

        switch (payloadType)
        {
        case 137: // mastering_display_colour_volume
            {
                mastering_display_colour_volume _display;
                mastering_display_colour_volume *display = &_display;

                for (uint8_t i = 0; i < 3; i++)
                {
                    display->display_primaries_x[i] = u(16);
                    display->display_primaries_y[i] = u(16);
                }
                display->white_point_x = u(16);
                display->white_point_y = u(16);
                display->max_display_mastering_luminance = u(32);
                display->min_display_mastering_luminance = u(32);

                if (!m_display)
                    nvParserLog("  Mastering Display Color Volume SEI luminance is [%.4f, %.4f]\n  R: %.5f %.5f, G: %.5f %.5f, B: %.5f %.5f\n  White Point: %.5f %.5f\n",
                        display->min_display_mastering_luminance * 0.0001, display->max_display_mastering_luminance * 0.0001,
                        display->display_primaries_x[2] * 0.00002,         display->display_primaries_y[2] * 0.00002,
                        display->display_primaries_x[0] * 0.00002,         display->display_primaries_y[0] * 0.00002,
                        display->display_primaries_x[1] * 0.00002,         display->display_primaries_y[1] * 0.00002,
                        display->white_point_x * 0.00002,                  display->white_point_y * 0.00002
                        );

                m_pParserData->display = *display;
                m_display = &m_pParserData->display;
            }
            break;
        default:
            nvParserVerboseLog("SEI(%d): %d bytes (0x%06X)\n", payloadType, payloadSize, next_bits(24));
            break;
        }

        // Skip over unknown payloads (NOTE: assumes that emulation prevention bytes are not present)
        skip = payloadSize * 8 - (consumed_bits() - bitsUsed);
        if (skip > 0)
            skip_bits(skip);
    }
}

bool VulkanH265Decoder::GetDisplayMasteringInfo(VkParserDisplayMasteringInfo *pdisp)
{
    if (m_display)
    {
        memcpy(pdisp, m_display, sizeof(VkParserDisplayMasteringInfo));
        return true;
    }

    return false;
}

const char* hevc_video_param_s::m_refClassId = "h265VpsVideoPictureParametersSet";
const char* hevc_seq_param_s::m_refClassId   = "h265SpsVideoPictureParametersSet";
const char* hevc_pic_param_s::m_refClassId   = "h265PpsVideoPictureParametersSet";
