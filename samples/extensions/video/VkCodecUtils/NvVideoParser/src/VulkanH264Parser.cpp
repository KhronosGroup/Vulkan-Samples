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
// H.264 elementary stream parser (picture & sequence layer)
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "vkvideo_parser/VulkanVideoParserIf.h"
#include "VulkanH264Decoder.h"
#include "nvVulkanVideoUtils.h"
#include "nvVulkanh264ScalingList.h"

#define MARKING_UNUSED 0 // unused for reference
#define MARKING_SHORT  1 // used for short-term reference
#define MARKING_LONG   2 // used for long-term reference
#define INF_MAX        (0x7fffffff)
#ifndef ARRAYSIZE
#define ARRAYSIZE(a) ((sizeof(a) / sizeof(a[0])))
#endif

enum Nal_Unit_Type
{
  NAL_UNIT_EXTERNAL                 = 0,
  NAL_UNIT_CODED_SLICE              = 1,
  NAL_UNIT_CODED_SLICE_DATAPART_A   = 2,
  NAL_UNIT_CODED_SLICE_DATAPART_B   = 3,
  NAL_UNIT_CODED_SLICE_DATAPART_C   = 4,
  NAL_UNIT_CODED_SLICE_IDR          = 5,
  NAL_UNIT_SEI                      = 6,
  NAL_UNIT_SPS                      = 7,
  NAL_UNIT_PPS                      = 8,
  NAL_UNIT_ACCESS_UNIT_DELIMITER    = 9,
  NAL_UNIT_END_OF_SEQUENCE          = 10,
  NAL_UNIT_END_OF_STREAM            = 11,
  NAL_UNIT_FILLER_DATA              = 12,
  NAL_UNIT_SUBSET_SPS               = 15,

  NAL_UNIT_CODED_SLICE_PREFIX       = 14,

  NAL_UNIT_CODED_SLICE_SCALABLE     = 20,
  NAL_UNIT_CODED_SLICE_IDR_SCALABLE = 21
};

static inline int Min(int x, int y) { return (x <= y) ? x : y; } // (5-11)
static inline int Max(int x, int y) { return (x >= y) ? x : y; } // (5-12)

VulkanH264Decoder::VulkanH264Decoder(VkVideoCodecOperationFlagBitsKHR std)
  : VulkanVideoDecoder(std),
    m_pParserData(NULL),
    m_MaxDpbSize(0),
    m_prefix_nalu_valid(false),
    m_spsme(NULL),
    m_bUseMVC(false),
    m_bUseSVC(false),
    m_slice_group_map()
{
    memset(m_spsmes, 0, sizeof(m_spsmes));
    memset(&m_nhe, 0, sizeof(nalu_header_extension_u));
}

VulkanH264Decoder::~VulkanH264Decoder()
{
    EndOfStream();

    if (m_slice_group_map) {
        delete[] m_slice_group_map;
        m_slice_group_map = nullptr;
    }

}


void VulkanH264Decoder::CreatePrivateContext()
{
    m_pParserData = new H264ParserData();
}

void VulkanH264Decoder::FreeContext()
{
    delete m_pParserData;
    m_pParserData = NULL;
}

void VulkanH264Decoder::InitParser()
{
    uint32_t decoder_caps;

    memset(dpb, 0, sizeof(dpb));
    PrevRefFrameNum = 0;
    prevPicOrderCntMsb = prevPicOrderCntLsb = 0;
    prevFrameNumOffset = prevFrameNum = 0;
    iCur = 0;
    cur = &dpb[iCur];
    picture_started = false;
    EndOfStream();
    m_bEmulBytesPresent = true;
    m_MaxDpbSize = 0;
    decoder_caps = (m_pClient) ? m_pClient->GetDecodeCaps() : 0;
    m_bUseMVC = !!(decoder_caps & VK_PARSER_CAPS_MVC);
    m_bUseSVC = !!(decoder_caps & VK_PARSER_CAPS_SVC);
    m_aso = false;
}


void VulkanH264Decoder::EndOfStream()
{
    if (!m_bUseSVC)
    {
        flush_decoded_picture_buffer();
        for (int i = 0; i <= MAX_DPB_SIZE; i++)
        {
            if (dpb[i].pPicBuf)
            {
                dpb[i].pPicBuf->Release();
                dpb[i].pPicBuf = NULL;
            }
        }
    }
    else
    {
        for (int did = 0; did < 8; did++)
        {
            dependency_state_s *ds = &m_dependency_state[did];
            dependency_data_s *dd = &m_dependency_data[did];
            if (dd->used)
            {
                flush_dpb_SVC(ds);
                for (int i = 0; i < MAX_DPB_SVC_SIZE; i++)
                {
                    if (ds->dpb_entry[i].pPicBuf)
                    {
                        ds->dpb_entry[i].pPicBuf->Release();
                        ds->dpb_entry[i].pPicBuf = NULL;
                    }
                    if (ds->dpb_entry[i].pPicBufRefBase)
                    {
                        ds->dpb_entry[i].pPicBufRefBase->Release();
                        ds->dpb_entry[i].pPicBufRefBase = NULL;
                    }
                }
            }
        }
    }
    PrevRefFrameNum = 0;
    prevPicOrderCntMsb = prevPicOrderCntLsb = 0;
    prevFrameNumOffset = prevFrameNum = 0;
    iCur = 0;
    cur = &dpb[iCur];
    picture_started = false;
    memset(&m_slh, 0, sizeof(m_slh));
    m_sps  = nullptr;
    m_pps  = nullptr;
    memset(&m_fpa, 0, sizeof(m_fpa));
    m_last_sps_id = 0;
    m_last_sei_pic_struct = -1;
    m_last_primary_pic_type = -1;
    m_idr_found_flag = false;
    m_MaxDpbSize = 0;
    m_MaxRefFramesPerView = 0;

    for (uint32_t i = 0; i < sizeof (m_spss) / sizeof (m_spss[0]); i++) {
        m_spss[i] = nullptr;
    }

    for (uint32_t i = 0; i < sizeof (m_ppss) / sizeof (m_ppss[0]); i++) {
        m_ppss[i] = nullptr;
    }

    // svc
    for (uint32_t i = 0; i < sizeof (m_layer_data) / sizeof (m_layer_data[0]); i++) {
        m_layer_data[i] = layer_data_s();
    }

    for (uint32_t i = 0; i < sizeof (m_spssvcs) / sizeof (m_spssvcs[0]); i++) {
        m_spssvcs[i] = nullptr;
    }

    memset(&m_slh_prev, 0, sizeof(m_slh_prev));
    memset(&m_prefix_nal_unit_svc, 0, sizeof(m_prefix_nal_unit_svc));
    memset(&m_dependency_data, 0, sizeof(m_dependency_data));
    memset(&m_dependency_state, 0, sizeof(m_dependency_state));
}


bool VulkanH264Decoder::BeginPicture(VkParserPictureData *pnvpd)
{
    if (m_bUseSVC) //prepare layer data
    {
        return BeginPicture_SVC(pnvpd);
    }
    else
    {
        if (!picture_started) {
            return false;
        }
        const slice_header_s * const slh = &m_slh;
        const seq_parameter_set_s * const sps = m_sps;
        const pic_parameter_set_s * const pps = m_pps;
        VkParserH264PictureData * const h264 = &pnvpd->CodecSpecific.h264;

        pnvpd->PicWidthInMbs = sps->pic_width_in_mbs_minus1 + 1;
        pnvpd->FrameHeightInMbs = (2 - sps->flags.frame_mbs_only_flag) * (sps->pic_height_in_map_units_minus1 + 1);
        pnvpd->pCurrPic = dpb[iCur].pPicBuf;
        pnvpd->current_dpb_id = iCur;
        pnvpd->field_pic_flag = slh->field_pic_flag;
        pnvpd->bottom_field_flag = slh->bottom_field_flag;
        pnvpd->second_field = (slh->field_pic_flag) && (dpb[iCur].complementary_field_pair);
        if (slh->field_pic_flag) {
            pnvpd->top_field_first = (pnvpd->second_field == pnvpd->bottom_field_flag);
        } else {
            pnvpd->top_field_first = (dpb[iCur].TopFieldOrderCnt < dpb[iCur].BottomFieldOrderCnt);
        }
        pnvpd->progressive_frame = (!slh->field_pic_flag) && (dpb[iCur].TopFieldOrderCnt == dpb[iCur].BottomFieldOrderCnt);
        pnvpd->ref_pic_flag = (slh->nal_ref_idc != 0);
        pnvpd->intra_pic_flag = m_intra_pic_flag;
        pnvpd->repeat_first_field = 0;
        pnvpd->picture_order_count = dpb[iCur].PicOrderCnt;
        if (!slh->field_pic_flag)
        {
            // Hack for x264 mbaff bug: delta_pic_order_cnt_bottom unspecified for interlaced content
            if ((!sps->flags.frame_mbs_only_flag) && (sps->flags.mb_adaptive_frame_field_flag)
             && (sps->pic_order_cnt_type == 0) && (!pps->flags.bottom_field_pic_order_in_frame_present_flag)
             && (pnvpd->progressive_frame))
            {
                pnvpd->progressive_frame = 0;
                pnvpd->top_field_first = 1;
            }
            // Use pic_struct to override field order
            if ((slh->sei_pic_struct >= 3) && (slh->sei_pic_struct <= 6))
            {
                pnvpd->top_field_first = slh->sei_pic_struct & 1;
            }
            // Use SEI to determine the number of fields
            switch(slh->sei_pic_struct) // Table D-1
            {
            case 5:
            case 6:
                pnvpd->repeat_first_field = 1;
                break;
            case 7:
                pnvpd->repeat_first_field = 2;  // frame doubling
                break;
            case 8:
                pnvpd->repeat_first_field = 4;  // frame tripling
                break;
            }
        }
        pnvpd->chroma_format = sps->chroma_format_idc;

        h264->pic_parameter_set_id = slh->pic_parameter_set_id; // PPS ID
        h264->seq_parameter_set_id = sps->seq_parameter_set_id; // SPS ID

        assert(pps->pic_parameter_set_id == h264->pic_parameter_set_id);
        assert(pps->seq_parameter_set_id == h264->seq_parameter_set_id);

        // SPS
        h264->pStdSps = sps;
        // PPS
        h264->pStdPps = pps;

        h264->fmo_aso_enable = m_aso;
        if (pps->num_slice_groups_minus1)
        {
            // slice_group_map_s is not ssupported with this version of the parser
            cur->not_existing = true;
        }
        // DPB
        h264->frame_num = dpb[iCur].FrameNum;
        h264->CurrFieldOrderCnt[0] = dpb[iCur].TopFieldOrderCnt;
        h264->CurrFieldOrderCnt[1] = dpb[iCur].BottomFieldOrderCnt;

        // MVC ext
        h264->mvcext.num_views_minus1 = m_spsme ? m_spsme->num_views_minus1 : 0;
        h264->mvcext.view_id = m_nhe.mvc.view_id;
        h264->mvcext.inter_view_flag = m_nhe.mvc.inter_view_flag;
        h264->mvcext.MVCReserved8Bits = 0;
        if(m_spsme) {
            int32_t VOIdx = get_view_output_index(m_nhe.mvc.view_id);
            if (m_nhe.mvc.anchor_pic_flag)
            {
                if(m_spsme->num_anchor_refs_l0)
                {
                    h264->mvcext.num_inter_view_refs_l0 = (unsigned char) m_spsme->num_anchor_refs_l0[VOIdx];
                    for(int32_t i = 0; i < m_spsme->num_anchor_refs_l0[VOIdx]; i++)
                    {
                        h264->mvcext.InterViewRefsL0[i] = m_spsme->anchor_ref_l0[VOIdx][i];
                    }
                }
                if(m_spsme->num_anchor_refs_l1)
                {
                    h264->mvcext.num_inter_view_refs_l1 = (unsigned char) m_spsme->num_anchor_refs_l1[VOIdx];
                    for(int32_t i = 0; i < m_spsme->num_anchor_refs_l1[VOIdx]; i++)
                    {
                        h264->mvcext.InterViewRefsL1[i] = m_spsme->anchor_ref_l1[VOIdx][i];
                    }
                }
            }
            else
            {
                if(m_spsme->num_non_anchor_refs_l0)
                {
                    h264->mvcext.num_inter_view_refs_l0 = (unsigned char) m_spsme->num_non_anchor_refs_l0[VOIdx];
                    for(int32_t i = 0; i < m_spsme->num_non_anchor_refs_l0[VOIdx]; i++)
                    {
                        h264->mvcext.InterViewRefsL0[i] = m_spsme->non_anchor_ref_l0[VOIdx][i];
                    }
                }
                if(m_spsme->num_non_anchor_refs_l1)
                {
                    h264->mvcext.num_inter_view_refs_l1 = (unsigned char) m_spsme->num_non_anchor_refs_l1[VOIdx];
                    for(int32_t i = 0; i < m_spsme->num_non_anchor_refs_l1[VOIdx]; i++)
                    {
                        h264->mvcext.InterViewRefsL1[i] = m_spsme->non_anchor_ref_l1[VOIdx][i];
                    }
                }
            }
        }
        memset(&h264->dpb, 0, sizeof(h264->dpb));
        for (int32_t i = 0; i < MAX_DPB_SIZE; i++) {
            // Check dpb consistency
            assert((dpb[i].state & 1) || (dpb[i].top_field_marking == 0));
            assert((dpb[i].state & 2) || (dpb[i].bottom_field_marking == 0));
            assert((dpb[i].state != 3) || (dpb[i].top_field_marking == 0) || (dpb[i].bottom_field_marking == 0) || (dpb[i].top_field_marking == dpb[i].bottom_field_marking));
            if (dpb[i].top_field_marking != 0 || dpb[i].bottom_field_marking != 0 || 
                (dpb[i].inter_view_flag && dpb[i].view_id != m_nhe.mvc.view_id))
            {
                h264->dpb[i].pPicBuf = dpb[i].pPicBuf;
                h264->dpb[i].used_for_reference = ((dpb[i].bottom_field_marking != 0) << 1) | (dpb[i].top_field_marking != 0);
                if (dpb[i].inter_view_flag && dpb[i].view_id != m_nhe.mvc.view_id)
                    h264->dpb[i].used_for_reference |= 3;
                h264->dpb[i].is_long_term = dpb[i].top_field_marking == 2 || dpb[i].bottom_field_marking == 2;
                h264->dpb[i].not_existing = dpb[i].not_existing;
                h264->dpb[i].FrameIdx = h264->dpb[i].is_long_term ? dpb[i].LongTermFrameIdx : dpb[i].FrameNum;
                h264->dpb[i].FieldOrderCnt[0] = dpb[i].TopFieldOrderCnt;
                h264->dpb[i].FieldOrderCnt[1] = dpb[i].BottomFieldOrderCnt;
            }
        }
        m_idr_found_flag |= (slh->nal_unit_type == 5) || (slh->nal_unit_type == 20 && !(slh->nhe.mvc.non_idr_flag));

        if ((pps->flags.weighted_pred_flag) && (slh->weights_out_of_range > 0) && (slh->slice_type != I) && (m_lErrorThreshold < 30))
        {
            nvParserLog("Dropping picture due to out-of-range prediction weights (%d)\n", slh->weights_out_of_range);
            cur->not_existing = true;
        }

        if (cur->not_existing)
        {
            dpb_picture_end();  // EndPicture will not be called if BeginPicture() fails
            return false;
        }
        return true;
    }
}


// Called back after EndOfPicture
void VulkanH264Decoder::EndPicture()
{
    if (m_bUseSVC)
    {
        EndPicture_SVC();
    }
    else
    {
        dpb_picture_end();
    }
}

bool VulkanH264Decoder::init_sequence_svc(const seq_parameter_set_s* sps)
{
    VkParserSequenceInfo nvsi;
    int PicWidthInMbs, FrameHeightInMbs;

    PicWidthInMbs    = sps->pic_width_in_mbs_minus1 + 1;
    FrameHeightInMbs = (2 - sps->flags.frame_mbs_only_flag) * (sps->pic_height_in_map_units_minus1 + 1);
    memset(&nvsi, 0, sizeof(nvsi));
    nvsi.eCodec = VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR;
    nvsi.frameRate = NV_FRAME_RATE_UNKNOWN;
    nvsi.bProgSeq = sps->flags.frame_mbs_only_flag;
    nvsi.nCodedWidth = PicWidthInMbs * 16;
    nvsi.nCodedHeight = FrameHeightInMbs * 16;
    nvsi.nDisplayWidth = nvsi.nCodedWidth;
    nvsi.nDisplayHeight = nvsi.nCodedHeight;
    if (sps->flags.frame_cropping_flag)
    {
        int crop_right = sps->frame_crop_right_offset*2;
        int crop_bottom = sps->frame_crop_bottom_offset * 2 * ( 2 - sps->flags.frame_mbs_only_flag);
        if ((crop_right >= 0) && (crop_right < nvsi.nCodedWidth/2)
         && (crop_bottom >= 0) && (crop_bottom < nvsi.nCodedHeight/2))
        {
            nvsi.nDisplayWidth -= crop_right;
            nvsi.nDisplayHeight -= crop_bottom;
        }
    }
    nvsi.nChromaFormat = (uint8_t)sps->chroma_format_idc;
    nvsi.uBitDepthLumaMinus8 = (uint8_t)sps->bit_depth_luma_minus8;
    nvsi.uBitDepthChromaMinus8 = (uint8_t)sps->bit_depth_chroma_minus8;
    nvsi.lDARWidth = nvsi.nDisplayWidth;
    nvsi.lDARHeight = nvsi.nDisplayHeight;
    nvsi.lVideoFormat = VideoFormatUnspecified;
    nvsi.lColorPrimaries = ColorPrimariesUnspecified;
    nvsi.lTransferCharacteristics = TransferCharacteristicsUnspecified;
    nvsi.lMatrixCoefficients = MatrixCoefficientsUnspecified;
    if (sps->flags.vui_parameters_present_flag)
    {
        if ((sps->vui.sar_width > 0) && (sps->vui.sar_height > 0))
        {
            nvsi.lDARWidth = sps->vui.sar_width * nvsi.nDisplayWidth;
            nvsi.lDARHeight = sps->vui.sar_height * nvsi.nDisplayHeight;
        }
        if (sps->vui.video_signal_type_present_flag)
        {
            nvsi.lVideoFormat = sps->vui.video_format;
            if (sps->vui.color_description_present_flag)
            {
                nvsi.lColorPrimaries = sps->vui.colour_primaries;
                nvsi.lTransferCharacteristics = sps->vui.transfer_characteristics;
                nvsi.lMatrixCoefficients = sps->vui.matrix_coefficients;
            }
        }
        if (sps->vui.timing_info_present_flag)
        {
            uint32_t lNum = sps->vui.time_scale;   // lNum/lDenom = field rate in Hz
            uint32_t lDenom = sps->vui.num_units_in_tick;

            if ((lDenom > 0) && (lNum > lDenom)) // > 1Hz
            {
                nvsi.frameRate = PackFrameRate((lNum + 1) >> 1, lDenom);
            }
        }
        nvsi.lBitrate = sps->vui.nal_hrd.bit_rate;
    }
    SimplifyAspectRatio(&nvsi.lDARWidth, &nvsi.lDARHeight);

    nvsi.nMinNumDpbSlots = std::min<int32_t>((nvsi.nMinNumDecodeSurfaces - 3), MAX_DPB_SIZE);
    nvsi.codecProfile = sps->profile_idc;

    if (!init_sequence(&nvsi)) {
        return false;
    }
    return true;
}

bool VulkanH264Decoder::BeginPicture_SVC(VkParserPictureData *pnvpd)
{
    {
        // Reset m_dependency_data array
        for (size_t i = 0; i < sizeof(m_dependency_data) / sizeof(m_dependency_data[0]); i++) {
            m_dependency_data[i] = dependency_data_s();
        }
    }

    // determine target layer
    int32_t DQIdMax = 0;
    for (DQIdMax = 127; DQIdMax >= 0; DQIdMax--) {
        if (m_layer_data[DQIdMax].available)
            break;
    }

    if (DQIdMax < 0) {
        nvParserLog("Access unit is empty\n");
        return false;
    }

    m_iDQIdMax = DQIdMax;
    int dependencyIdMax = DQIdMax >> 4; // dependency_id of target dependency representation

    if (!init_sequence_svc(m_layer_data[DQIdMax].sps))
        return false;

    // layer and dependency representations required for decoding (G.8.1.1)
    int dqid_next = -1;
    for (int dqid = DQIdMax; dqid >= 0; dqid = m_layer_data[dqid].MaxRefLayerDQId)
    {
        nvParserLog("  DQId = %d (0x%x) max:%d\n", dqid, dqid, m_layer_data[dqid].MaxRefLayerDQId);
        if (dqid_next >= 0 && !(dqid < dqid_next)) // has to be strictly monotonically decreasing (prevents infinite loop)
        {
            nvParserLog("ref_layer_dq_id > DQId - 1");
            return false;
        }
        if (!m_layer_data[dqid].available)
        {
            nvParserLog("invalid ref_layer_dq_id: %d, reference layer representation not available", dqid);
            return false;
        }

        m_dependency_data[dqid >> 4].used = 1;
        m_layer_data[dqid].used = 1;
        m_layer_data[dqid].dqid_next = dqid_next;
        dqid_next = dqid;
    }

    for (int did = 0; did <= dependencyIdMax; did++)
    {
        m_dd = &m_dependency_data[did];
        if (m_dd->used)
        {
            if (!m_layer_data[16 * did + 0].used) {
                nvParserLog("quality_id == 0 not used\n");
            }
            m_dd->sps = m_layer_data[16 * did + 0].sps;
            m_dd->sps_svc = m_layer_data[16 * did + 0].sps->svc;
            m_dd->slh = m_layer_data[16 * did + 0].slh;
            m_dd->MaxDpbFrames = derive_MaxDpbFrames(m_dd->sps);
            if (did == dependencyIdMax) {
                m_dd->MaxDpbFrames = std::min<int32_t>(m_MaxFrameBuffers, m_dd->MaxDpbFrames);
            }
            if (m_dd->sps->max_num_ref_frames > m_dd->MaxDpbFrames) {
                nvParserLog("max_num_ref_frames > MaxDpbFrames");
            }
            if (m_dd->slh.IdrPicFlag) {
                flush_dpb_SVC(&m_dependency_state[did]);
            }
        }
    }

    VkParserH264PictureData *h264 = &pnvpd->CodecSpecific.h264;
    const slice_header_s * slh = &m_slh;

    for (int did = 0; did <= dependencyIdMax; did++)
    {
        m_ds = &m_dependency_state[did];
        m_dd = &m_dependency_data[did];
        if (m_dd->used)
        {
            gaps_in_frame_num_SVC();
            // initialize current picture
            m_ds->dpb_entry[16].base     = false;
            m_ds->dpb_entry[16].FrameNum = m_dd->slh.frame_num;
            m_ds->dpb_entry[16].ref      = 0;
            m_ds->dpb_entry[16].non_existing = false;
            picture_order_count_SVC(m_dd, m_ds); // stores result in ds->dpb_entry[16]
            for (uint32_t qid = 0; qid < 16; qid++)
            {
                uint32_t DQId = 16 * did + qid;
                layer_data_s *ld = &m_layer_data[DQId];
                if (!ld->used) { // used layers are always consecutive starting with qid=0 (i.e. no qid gaps)
                    break;
                }
                // frame buffer management
                if (DQId == (m_iDQIdMax & ~15)) // target dependency layer
                {
                    if (m_ds->dpb_entry[16].pPicBuf)
                    {
                        m_ds->dpb_entry[16].pPicBuf->Release();
                        m_ds->dpb_entry[16].pPicBuf = NULL;
                    }
                    if (m_ds->dpb_entry[16].pPicBufRefBase)
                    {
                        m_ds->dpb_entry[16].pPicBufRefBase->Release();
                        m_ds->dpb_entry[16].pPicBufRefBase = NULL;
                    }
                    // allocate buffer for current frame
                    m_ds->dpb_entry[16].pPicBuf = alloc_picture();
                    if (!m_ds->dpb_entry[16].pPicBuf)
                    {
                        nvParserLog("%s : Failed to allocate buffer for current picture\n", __FUNCTION__);
                        assert(0);
                    }
                    // allocate buffer for current base reference frame
                    if (m_dd->slh.store_ref_base_pic_flag && (m_iDQIdMax & 15)) // only if reference base layer and target layer differ
                    {
                        m_ds->dpb_entry[16].pPicBufRefBase = alloc_picture();
                        if (!m_ds->dpb_entry[16].pPicBufRefBase)
                        {
                            nvParserLog("%s : Failed to allocate buffer for ref base picture\n", __FUNCTION__);
                            assert(0);
                        }
                    }
                }
            }
        }
    }

    size_t end_offset = m_pVkPictureData->bitstreamDataLen;
    uint32_t maxCount = 0;
    const uint32_t* pSliceOffsets = m_pVkPictureData->bitstreamData->GetStreamMarkersPtr(0, maxCount);
    uint32_t TotalSliceCnt = 0;
    uint32_t nNumSlices = m_pVkPictureData->numSlices;
    assert(maxCount == nNumSlices);

    uint32_t PicLayer = 0;
    for(uint32_t layer = 0; layer < 128; layer++)
    {
        TotalSliceCnt += m_layer_data[layer].slice_count;
        int CurrentSliceCnt = m_layer_data[layer].slice_count;
        if (!m_layer_data[layer].used) {
            continue;
        }
        // slice calculation
        uint32_t firstSlice = TotalSliceCnt - CurrentSliceCnt;
        uint32_t startoffset = pSliceOffsets[firstSlice];
        (pnvpd + PicLayer)->bitstreamData = m_pVkPictureData->bitstreamData;
        (pnvpd + PicLayer)->bitstreamDataOffset = startoffset;
        (pnvpd + PicLayer)->numSlices = CurrentSliceCnt;
        (pnvpd + PicLayer)->bitstreamDataLen = ((TotalSliceCnt == nNumSlices) ? end_offset : pSliceOffsets[TotalSliceCnt]) - startoffset;
        // When processing layers, the decoder must consider the firstSliceIndex so that offsets
        // within a layer starts at 0
        (pnvpd + PicLayer)->firstSliceIndex = firstSlice;

        const seq_parameter_set_s* sps = m_layer_data[layer].sps;
        const pic_parameter_set_s* pps = m_layer_data[layer].pps;
        slh = &m_layer_data[layer].slh;
        h264 = &(pnvpd + PicLayer)->CodecSpecific.h264;
        svc_dpb_entry_s *dpb_entry = m_dependency_state[layer >> 4].dpb_entry;

        (pnvpd + PicLayer)->PicWidthInMbs = sps->pic_width_in_mbs_minus1 + 1;
        (pnvpd + PicLayer)->FrameHeightInMbs = (2 - sps->flags.frame_mbs_only_flag) * (sps->pic_height_in_map_units_minus1 + 1);
        (pnvpd + PicLayer)->pCurrPic = (slh->store_ref_base_pic_flag && (layer != m_iDQIdMax) && (layer == (m_iDQIdMax & ~15)))
                                     ? dpb_entry[16].pPicBufRefBase : dpb_entry[16].pPicBuf;
        (pnvpd + PicLayer)->field_pic_flag = slh->field_pic_flag;
        (pnvpd + PicLayer)->bottom_field_flag = slh->bottom_field_flag;
        (pnvpd + PicLayer)->second_field = (slh->field_pic_flag) && (dpb_entry[16].complementary_field_pair);
        if (slh->field_pic_flag)
            (pnvpd + PicLayer)->top_field_first = ((pnvpd + layer)->second_field == (pnvpd + PicLayer)->bottom_field_flag);
        else
            (pnvpd + PicLayer)->top_field_first = (dpb_entry[16].TopFieldOrderCnt < dpb_entry[16].BottomFieldOrderCnt);
        (pnvpd + PicLayer)->progressive_frame = (!slh->field_pic_flag) && (dpb_entry[16].TopFieldOrderCnt == dpb_entry[16].BottomFieldOrderCnt);
        (pnvpd + PicLayer)->ref_pic_flag = (slh->nal_ref_idc != 0);
        (pnvpd + PicLayer)->intra_pic_flag =  m_intra_pic_flag;
        (pnvpd + PicLayer)->repeat_first_field = 0;
        (pnvpd + PicLayer)->picture_order_count = dpb_entry[16].PicOrderCnt;
        if (!slh->field_pic_flag)
        {
            // Hack for x264 mbaff bug: delta_pic_order_cnt_bottom unspecified for interlaced content
            if ((!sps->flags.frame_mbs_only_flag) && (sps->flags.mb_adaptive_frame_field_flag)
            && (sps->pic_order_cnt_type == 0) && (!pps->flags.bottom_field_pic_order_in_frame_present_flag)
            && ((pnvpd + PicLayer)->progressive_frame))
            {
                (pnvpd + PicLayer)->progressive_frame = 0;
                (pnvpd + PicLayer)->top_field_first = 1;
            }
            // Use pic_struct to override field order
            if ((slh->sei_pic_struct >= 3) && (slh->sei_pic_struct <= 6))
            {
                (pnvpd + PicLayer)->top_field_first = slh->sei_pic_struct & 1;
            }
            // Use SEI to determine the number of fields
            switch(slh->sei_pic_struct) // Table D-1
            {
            case 5:
            case 6:
                (pnvpd + PicLayer)->repeat_first_field = 1;
                break;
            case 7:
                (pnvpd + PicLayer)->repeat_first_field = 2;  // frame doubling
                break;
            case 8:
                (pnvpd + PicLayer)->repeat_first_field = 4;  // frame tripling
                break;
            }
        }
        (pnvpd + PicLayer)->chroma_format = sps->chroma_format_idc;

        // SPS
        h264->pStdSps = sps;

        // m_bUseSVC

        // PPS
        h264->pStdPps = pps;

        h264->frame_num = dpb_entry[16].FrameNum;
        h264->CurrFieldOrderCnt[0] = dpb_entry[16].TopFieldOrderCnt;
        h264->CurrFieldOrderCnt[1] = dpb_entry[16].BottomFieldOrderCnt;

        // DPB mgmt
        unsigned short DPBEntryValidFlag = 0;
        memset(&h264->dpb, 0, sizeof(h264->dpb));
        for (int k = 0; k < MAX_DPB_SIZE; k++)
        {
            // skip entries that are not available for reference picture list construction
            if (!dpb_entry[k].ref) // not a reference picture
                continue;
            bool bRef = true;
            if (!slh->nhe.svc.use_ref_base_pic_flag) // don't use reference base pictures
            {
                if (dpb_entry[k].base) // reference base picture
                    bRef = false;
            }
            else // don't use reference pictures if a correspdonding reference base picture is available
            {
                if (!dpb_entry[k].base)
                {
                    // is there a ref base pic with the same FrameNum / LongTermFrameIdx?
                    int k1;
                    for (k1 = 0; k1 < MAX_DPB_SIZE; k1++)
                    {
                        if (dpb_entry[k].ref == MARKING_SHORT && dpb_entry[k1].ref == MARKING_SHORT && dpb_entry[k1].base && dpb_entry[k].FrameNum == dpb_entry[k1].FrameNum)
                            break;
                        if (dpb_entry[k].ref == MARKING_LONG && dpb_entry[k1].ref == MARKING_LONG && dpb_entry[k1].base && dpb_entry[k].LongTermFrameIdx == dpb_entry[k1].LongTermFrameIdx)
                            break;
                    }
                    if (k1 < MAX_DPB_SIZE) // found a ref base pic with the same FrameNum / LongTermFrameIdx
                        bRef = false;
                }
            }
            h264->dpb[k].pPicBuf = dpb_entry[k].pPicBuf;
            h264->dpb[k].used_for_reference = bRef ? 3 : 0;
            h264->dpb[k].is_long_term   = (dpb_entry[k].ref == MARKING_LONG);
            h264->dpb[k].not_existing   = !!dpb_entry[k].non_existing;
            h264->dpb[k].FrameIdx       = (dpb_entry[k].ref == MARKING_LONG) ? dpb_entry[k].LongTermFrameIdx : dpb_entry[k].FrameNum;
            h264->dpb[k].FieldOrderCnt[0] = dpb_entry[k].TopFieldOrderCnt;
            h264->dpb[k].FieldOrderCnt[1] = dpb_entry[k].BottomFieldOrderCnt;

            DPBEntryValidFlag |= (1 << k);
        }

        nvParserLog("DPBEntryValidFlag %x layer:%d\n", DPBEntryValidFlag, layer);

        h264->svcext.DPBEntryValidFlag = DPBEntryValidFlag;
        h264->svcext.profile_idc = (unsigned char) sps->profile_idc;
        h264->svcext.level_idc = (unsigned char)sps->level_idc;
        h264->svcext.DQId = (unsigned char)((slh->nhe.svc.dependency_id << 4) + slh->nhe.svc.quality_id);
        h264->svcext.DQIdMax = (unsigned char) m_iDQIdMax;
        h264->svcext.disable_inter_layer_deblocking_filter_idc = (unsigned char) slh->disable_inter_layer_deblocking_filter_idc;
        h264->svcext.ref_layer_chroma_phase_y_plus1 = (unsigned char) slh->ref_layer_chroma_phase_y_plus1;
        h264->svcext.inter_layer_slice_alpha_c0_offset_div2 = (unsigned char) slh->inter_layer_slice_alpha_c0_offset_div2;
        h264->svcext.inter_layer_slice_beta_offset_div2 = (unsigned char) slh->inter_layer_slice_beta_offset_div2;

        h264->svcext.f.inter_layer_deblocking_filter_control_present_flag = sps->svc.inter_layer_deblocking_filter_control_present_flag;
        h264->svcext.f.extended_spatial_scalability_idc = sps->svc.extended_spatial_scalability_idc;
        h264->svcext.f.adaptive_tcoeff_level_prediction_flag = sps->svc.adaptive_tcoeff_level_prediction_flag;
        h264->svcext.f.slice_header_restriction_flag = sps->svc.slice_header_restriction_flag;
        h264->svcext.f.chroma_phase_x_plus1_flag = sps->svc.chroma_phase_x_plus1_flag;
        h264->svcext.f.chroma_phase_y_plus1 = sps->svc.chroma_phase_y_plus1;
        h264->svcext.f.tcoeff_level_prediction_flag = slh->tcoeff_level_prediction_flag;
        h264->svcext.f.constrained_intra_resampling_flag = slh->constrained_intra_resampling_flag;
        h264->svcext.f.ref_layer_chroma_phase_x_plus1_flag = slh->ref_layer_chroma_phase_x_plus1_flag;
        h264->svcext.f.store_ref_base_pic_flag = slh->store_ref_base_pic_flag;

        h264->svcext.scaled_ref_layer_left_offset   = (short) slh->scaled_ref_layer_left_offset;
        h264->svcext.scaled_ref_layer_top_offset    = (short) slh->scaled_ref_layer_top_offset;
        h264->svcext.scaled_ref_layer_right_offset   = (short) slh->scaled_ref_layer_right_offset;
        h264->svcext.scaled_ref_layer_bottom_offset = (short) slh->scaled_ref_layer_bottom_offset;

        nvParserLog(" Layer %d: id:%d (0x%x) Size:%dx%d\n", PicLayer, h264->svcext.DQId, h264->svcext.DQId,
            (pnvpd + PicLayer)->PicWidthInMbs*16,  (pnvpd + PicLayer)->FrameHeightInMbs*16);

        // increment pic count
        PicLayer++;
    }
    assert(nNumSlices == TotalSliceCnt);
    m_iTargetLayer = PicLayer-1;

    return true;
}

void VulkanH264Decoder::EndPicture_SVC()
{
    int dependencyIdMax = m_iDQIdMax >> 4; // dependency_id of target dependency representation
    for (int did=0; did<=dependencyIdMax; did++)
    {
        m_ds = &m_dependency_state[did];
        m_dd = &m_dependency_data[did];
        if (m_dd->used)
        {
            if (m_dd->slh.nal_ref_idc > 0)
                decoded_reference_picture_marking_SVC(m_dd, m_ds);
            output_order_dpb_SVC(did==dependencyIdMax, m_dd, m_ds);
        }
    }
    // clear SVC layer data
    {
        for (size_t i = 0; i < sizeof(m_layer_data) / sizeof(m_layer_data[0]); i++) {
            m_layer_data[i] = layer_data_s();
        }
    }

}

// Operation of the output order DPB
void VulkanH264Decoder::output_order_dpb_SVC(bool is_target_dep, dependency_data_s *dd, dependency_state_s *ds)
{
    // Removal of pictures from the DPB before possible insertion of the current picture
    if (dd->slh.IdrPicFlag)
    {
        if (dd->slh.no_output_of_prior_pics_flag)
        {
            for (int k = 0; k < MAX_DPB_SIZE; k++)
            {
                ds->dpb_entry[k].output = false;
            }
        }
    }

    // empty frame buffers marked as "not needed for output" and "unused for reference"
    for (int k=0; k<MAX_DPB_SIZE; k++)
    {
        if (!ds->dpb_entry[k].output && ds->dpb_entry[k].ref == MARKING_UNUSED)
        {
            if (ds->dpb_entry[k].pPicBuf)
            {
                ds->dpb_entry[k].pPicBuf->Release();
                ds->dpb_entry[k].pPicBuf = NULL;
            }
        }
    }

    if (dd->slh.mmco5 || dd->slh.IdrPicFlag) // && !dd->no_output_of_prior_pics_flag
    {
        flush_dpb_SVC(ds);
    }

    if (dd->slh.nal_ref_idc != 0)
    {
        while (dpb_full_SVC(dd, ds))
            dpb_bumping_SVC(ds);
        for (int k=0; k<MAX_DPB_SIZE; k++)
        {
            if (!ds->dpb_entry[k].ref && !ds->dpb_entry[k].output)
            {
                if (ds->dpb_entry[k].pPicBuf)
                {
                    ds->dpb_entry[k].pPicBuf->Release();
                    ds->dpb_entry[k].pPicBuf = NULL;
                }
                ds->dpb_entry[k] = ds->dpb_entry[16];
                ds->dpb_entry[k].output = is_target_dep && dd->slh.nhe.svc.output_flag;
                ds->dpb_entry[k].pPicBufRefBase = NULL;
                if (ds->dpb_entry[k].pPicBuf)
                {
                    ds->dpb_entry[k].pPicBuf->AddRef();
                }
                break;
            }
        }
        if (dd->slh.store_ref_base_pic_flag && (m_iDQIdMax & 15)) // only if reference base layer and target layer differ
        {
            while (dpb_full_SVC(dd, ds))
                dpb_bumping_SVC(ds);
            for (int k=0; k<MAX_DPB_SIZE; k++)
            {
                if (!ds->dpb_entry[k].ref && !ds->dpb_entry[k].output)
                {
                    if (ds->dpb_entry[k].pPicBuf)
                    {
                        ds->dpb_entry[k].pPicBuf->Release();
                        ds->dpb_entry[k].pPicBuf = NULL;
                    }
                    ds->dpb_entry[k] = ds->dpb_entry[16];
                    ds->dpb_entry[k].output = false;
                    ds->dpb_entry[k].base = 1;
                    ds->dpb_entry[k].pPicBuf = ds->dpb_entry[k].pPicBufRefBase;
                    ds->dpb_entry[k].pPicBufRefBase = NULL;
                    if (ds->dpb_entry[k].pPicBuf)
                    {
                        ds->dpb_entry[k].pPicBuf->AddRef();
                    }
                    break;
                }
            }
        }
    }
    else if (is_target_dep && dd->slh.nhe.svc.output_flag)
    {
        for (;;)
        {
            if (dpb_full_SVC(dd, ds))
            {
                int curPOC = Min(ds->dpb_entry[16].TopFieldOrderCnt, ds->dpb_entry[16].BottomFieldOrderCnt);
                int k;
                for (k=0; k<MAX_DPB_SIZE; k++)
                {
                    if (ds->dpb_entry[k].output && curPOC > Min(ds->dpb_entry[k].TopFieldOrderCnt, ds->dpb_entry[k].BottomFieldOrderCnt))
                        break;
                }
                if (k < MAX_DPB_SIZE) // not smallest POC
                {
                    dpb_bumping_SVC(ds);
                }
                else
                {
                    output_picture_SVC(ds->dpb_entry[16].pPicBuf, 3);// current ds->dpb_entry[16].id
                    break;
                }
            }
            else
            {
                for (int k=0; k<MAX_DPB_SIZE; k++)
                {
                    if (!ds->dpb_entry[k].ref && !ds->dpb_entry[k].output)
                    {
                        if (ds->dpb_entry[k].pPicBuf)
                        {
                            ds->dpb_entry[k].pPicBuf->Release();
                            ds->dpb_entry[k].pPicBuf = NULL;
                        }
                        ds->dpb_entry[k] = ds->dpb_entry[16];
                        ds->dpb_entry[k].ref = MARKING_UNUSED;
                        ds->dpb_entry[k].output = true;
                        if (ds->dpb_entry[k].pPicBuf)
                        {
                            ds->dpb_entry[k].pPicBuf->AddRef();
                        }
                        break;
                    }
                }
                break;
            }
        }
    }
}

void VulkanH264Decoder::flush_dpb_SVC(dependency_state_s *ds)
{
    nvParserLog(" flush_dpb_SVC\n");
    for (int k=0; k<MAX_DPB_SIZE; k++)
        ds->dpb_entry[k].ref = MARKING_UNUSED;

    while (!dpb_empty_SVC(ds))
        dpb_bumping_SVC(ds);
}

bool VulkanH264Decoder::dpb_full_SVC(dependency_data_s *dd, dependency_state_s *ds)
{
    return (dpb_fullness_SVC(ds) >= dd->MaxDpbFrames);
}

int VulkanH264Decoder::dpb_fullness_SVC(dependency_state_s *ds)
{
    int n = 0;
    for (int k=0; k<MAX_DPB_SIZE; k++)
        if (ds->dpb_entry[k].ref || ds->dpb_entry[k].output)
            n++;
    return n;
}

uint8_t VulkanH264Decoder::derive_MaxDpbFrames(const seq_parameter_set_s *sps)
{
    static const struct max_dpb_mbs_limit_s {
        StdVideoH264LevelIdc level;
        int MaxDPBMbs;
    } mbs_level_limits[] = {
        // level, MaxDpbMbs
        {STD_VIDEO_H264_LEVEL_IDC_1_0, 396   },
        {STD_VIDEO_H264_LEVEL_IDC_1_1, 900   },
        {STD_VIDEO_H264_LEVEL_IDC_1_2, 2376  },
        {STD_VIDEO_H264_LEVEL_IDC_1_3, 2376  },
        {STD_VIDEO_H264_LEVEL_IDC_2_0, 2376  },
        {STD_VIDEO_H264_LEVEL_IDC_2_1, 4752  },
        {STD_VIDEO_H264_LEVEL_IDC_2_2, 8100  },
        {STD_VIDEO_H264_LEVEL_IDC_3_0, 8100  },
        {STD_VIDEO_H264_LEVEL_IDC_3_1, 18000 },
        {STD_VIDEO_H264_LEVEL_IDC_3_2, 20480 },
        {STD_VIDEO_H264_LEVEL_IDC_4_0, 32768 },
        {STD_VIDEO_H264_LEVEL_IDC_4_1, 32768 },
        {STD_VIDEO_H264_LEVEL_IDC_4_2, 34816 },
        {STD_VIDEO_H264_LEVEL_IDC_5_0, 110400},
        {STD_VIDEO_H264_LEVEL_IDC_5_1, 184320},
        {STD_VIDEO_H264_LEVEL_IDC_5_2, 184320},
        {STD_VIDEO_H264_LEVEL_IDC_6_0, 696320},
        {STD_VIDEO_H264_LEVEL_IDC_6_1, 696320},
        {STD_VIDEO_H264_LEVEL_IDC_6_2, 696320}
    };

    int PicWidthInMbs        =  sps->pic_width_in_mbs_minus1 + 1;
    int FrameHeightInMbs     = (sps->pic_height_in_map_units_minus1 + 1) << (sps->flags.frame_mbs_only_flag ? 0 : 1);
    int constraint_set3_flag = sps->constraint_set_flags >> 4 & 1;

    // The following logic maps the H264 level to level 1b based on certain conditions.
    // Although in Vulkan video we do not support this level, still the only difference
    // this level 1b creates is in computation of max number of mbs supported if these
    // conditions are true. In the earlier logic there was a separate entry for this level
    // which is no longer required as the max number of mbs supported for level 1b is same
    // as the max number of mbs supported for STD_VIDEO_H264_LEVEL_1_0. Therefore, the additional
    // entry is removed from the above table and corresponding mapping is created in the below
    // logic.
    int level = (sps->level_idc == STD_VIDEO_H264_LEVEL_IDC_1_1 &&
                 constraint_set3_flag &&
                 (sps->profile_idc == 66 ||
                  sps->profile_idc == 77 ||
                  sps->profile_idc == 88)) ? STD_VIDEO_H264_LEVEL_IDC_1_0 // level 1b
                                         : sps->level_idc;

    uint8_t MaxDpbFrames = MAX_DPB_SIZE; // default
    for (uint32_t i = 0; i < ARRAYSIZE(mbs_level_limits); i++)
    {
        if (level == mbs_level_limits[i].level)
        {
            MaxDpbFrames = Min(mbs_level_limits[i].MaxDPBMbs / (PicWidthInMbs * FrameHeightInMbs), 16);
            break;
        }
    }

    return MaxDpbFrames;
}

// 7.4.1.2.4
bool VulkanH264Decoder::IsPictureBoundary(int32_t rbsp_size)
{
    const slice_header_s * const slhold = &m_slh_prev;
    VkSharedBaseObj<seq_parameter_set_s>* spss = &m_spss[0];
    int nal_ref_idc, nal_unit_type, first_mb_in_slice;
    int pps_id, sps_id, idr_pic_id = 0;
    int colour_plane_id = 0;
    int frame_num, field_pic_flag, bottom_field_flag;
    bool base_layer = true, svc_extension_flag = false, idr_flag = false, non_idr_flag = false;
    int IdrPicFlag = 0;
    int view_id;

    if (rbsp_size < 2)
        return false;
    f(1, 0); // forbidden_zero_bit
    nal_ref_idc = u(2);
    nal_unit_type = u(5);
    if (m_bUseMVC || m_bUseSVC)
    {
        if (nal_unit_type == 14 || nal_unit_type == 20)
        {
            svc_extension_flag = !!u(1);
            if (svc_extension_flag)
            {
                int dependency_id, quality_id;
                idr_flag = !!u(1);   // idr_flag
                u(6);   // priority_id
                u(1);   // no_inter_layer_pred_flag
                dependency_id = u(3);   // dependency_id
                quality_id = u(4);   // quality_id

                if (m_slh_prev.nhe.svc.dependency_id > dependency_id)
                    return true;
                if (m_slh_prev.nhe.svc.dependency_id != dependency_id)
                    return false;
                if (m_slh_prev.nhe.svc.quality_id > quality_id)
                    return true;
                if (m_slh_prev.nhe.svc.quality_id != quality_id)
                    return false;
                u(3);
                u(1);
                u(1);
                u(1);
                f(2, 3);
            }
            else // MVC
            {
                non_idr_flag = !!u(1); // non_idr_flag
                u(6); // priority_id
                view_id = u(10);
                if (slhold->nhe.mvc.view_id != view_id)
                    return true;
                u(3); // temporal_id
                u(1); // anchor_pic_flag
                u(1); // inter_view_flag
                f(1, 1); // reserved_one_bit
            }
        }    
        if ((nal_unit_type != 1) && (nal_unit_type != 5) && (nal_unit_type != 20) && (nal_unit_type != 21))
            return (nal_unit_type == 9);    // access_unit_delimiter
    }
    else
    {
        if ((nal_unit_type != 1) && (nal_unit_type != 5))
            return (nal_unit_type == 9);    // access_unit_delimiter
    }
    if ((slhold == NULL) || (m_bitstreamData.GetStreamMarkersCount() == 0))
        return true;
    if (slhold->nal_ref_idc != nal_ref_idc && (slhold->nal_ref_idc == 0 || nal_ref_idc == 0))
        return true;
    if (slhold->nal_unit_type != nal_unit_type &&
        (slhold->nal_unit_type == 5 || nal_unit_type == 5))
        return true;
    first_mb_in_slice = ue();   // first_mb_in_slice
    ue();   // slice_type_raw
    pps_id = ue();

    if (svc_extension_flag)
    {
        base_layer = (nal_unit_type == 1 || nal_unit_type == 5);
    }
    if ((pps_id < 0) || (pps_id >= MAX_NUM_PPS) || (!m_ppss[pps_id])) {
        return false;
    }
    sps_id = m_ppss[pps_id]->seq_parameter_set_id;
    spss = base_layer ? &m_spss[0] : &m_spssvcs[0];
    
    if ((slhold->pic_parameter_set_id != pps_id) || (!spss[sps_id]))
        return true;
    if (spss[sps_id]->flags.separate_colour_plane_flag)
        colour_plane_id = u(2);
    frame_num = u(spss[sps_id]->log2_max_frame_num_minus4 + 4);
    if (slhold->frame_num != frame_num)
        return true;
    field_pic_flag = 0;
    bottom_field_flag = 0;
    if (!spss[sps_id]->flags.frame_mbs_only_flag)
    {
        field_pic_flag = u(1);
        if (field_pic_flag)
            bottom_field_flag = u(1);
    }
    if ((slhold->field_pic_flag != field_pic_flag)
     || (slhold->bottom_field_flag != bottom_field_flag))
        return true;

    if (nal_unit_type == 20)
    {
        if (svc_extension_flag)
        {
            IdrPicFlag = idr_flag;
        } 
        else
        {
            // MVC
            IdrPicFlag = !non_idr_flag;
        }
    } else
    {
        IdrPicFlag = (nal_unit_type == 5);
    }

    if (IdrPicFlag)
    {
        idr_pic_id = ue();
    }
    if (spss[sps_id]->pic_order_cnt_type == 0)
    {
        int pic_order_cnt_lsb = u(spss[sps_id]->log2_max_pic_order_cnt_lsb_minus4 + 4);
        int delta_pic_order_cnt_bottom = 0;
        if (m_ppss[pps_id]->flags.bottom_field_pic_order_in_frame_present_flag && !field_pic_flag) {
            delta_pic_order_cnt_bottom = se();
        }
        if ((slhold->pic_order_cnt_lsb != pic_order_cnt_lsb)
             || (slhold->delta_pic_order_cnt_bottom != delta_pic_order_cnt_bottom)) {
            return true;
        }

    } else if (spss[sps_id]->pic_order_cnt_type == 1) {

        int delta_pic_order_cnt[2] = {0,0};
        if (!spss[sps_id]->flags.delta_pic_order_always_zero_flag)
        {
            delta_pic_order_cnt[0] = se();
            if (m_ppss[pps_id]->flags.bottom_field_pic_order_in_frame_present_flag && !field_pic_flag)
                delta_pic_order_cnt[1] = se();
        }
        if ((slhold->delta_pic_order_cnt[0] != delta_pic_order_cnt[0])
             || (slhold->delta_pic_order_cnt[1] != delta_pic_order_cnt[1])) {
            return true;
        }
    }

    if (slhold->IdrPicFlag != IdrPicFlag) {
        return true;
    }

    if (slhold->IdrPicFlag && IdrPicFlag &&
        ((slhold->idr_pic_id != idr_pic_id)
            || ((first_mb_in_slice == slhold->first_mb_in_slice) && (slhold->colour_plane_id == colour_plane_id))
            || ((first_mb_in_slice < slhold->first_mb_in_slice) && (spss[sps_id]->profile_idc != 66)) )) {
        return true;
    }

    return false;
}


int32_t VulkanH264Decoder::ParseNalUnit()
{
    slice_header_s slh;
    int nal_ref_idc, nal_unit_type, picture_boundary;
    int retval = NALU_DISCARD;

    picture_boundary = (m_nalu.start_offset == 0);
    f(1, 0);    // forbidden_zero_bit
    nal_ref_idc = u(2);
    nal_unit_type = u(5);
    if (nal_unit_type == NAL_UNIT_CODED_SLICE_PREFIX || nal_unit_type == NAL_UNIT_CODED_SLICE_SCALABLE)
    {
        if(m_bUseMVC || m_bUseSVC)
        {
            nal_unit_header_extension();
        }
    }
    switch (nal_unit_type)
    {
    case NAL_UNIT_CODED_SLICE:
    case NAL_UNIT_CODED_SLICE_IDR:
        if (slice_header(&slh, nal_ref_idc, nal_unit_type))
        {
            if (picture_boundary)
            {
                const seq_parameter_set_s *sps = m_spss[m_ppss[slh.pic_parameter_set_id]->seq_parameter_set_id];
                if ((slh.nal_unit_type == 5) || (!m_MaxDpbSize) // IDR picture (or first non-IDR picture after a sequence header)
                 || (sps->pic_width_in_mbs_minus1 != m_sps->pic_width_in_mbs_minus1)
                 || (sps->pic_height_in_map_units_minus1 != m_sps->pic_height_in_map_units_minus1)
                 || (sps->log2_max_frame_num_minus4 != m_sps->log2_max_frame_num_minus4) )
                {
                    if (!dpb_sequence_start(&slh)) {
                        return NALU_UNKNOWN;
                    }
                }
                slh.sei_pic_struct = m_last_sei_pic_struct;
                slh.primary_pic_type = m_last_primary_pic_type;
                m_last_sei_pic_struct = -1;
                m_last_primary_pic_type = -1;
                if (!m_bUseSVC) // for SVC, it is handled inside BeginPicture_SVC
                    dpb_picture_start(m_ppss[slh.pic_parameter_set_id], &slh);
                m_intra_pic_flag = 1;
                m_aso = false; //((sps->profile_idc == 66) && (slh.first_mb_in_slice != 0));
            }
            else
            {
                if (m_sps->profile_idc == 66) // fmo/aso only allowed in baseline
                {
                    if (slh.first_mb_in_slice < m_first_mb_in_slice)
                        m_aso = true;
                }
            }
            m_first_mb_in_slice = slh.first_mb_in_slice;
            if ((slh.slice_type != I) && (slh.slice_type != SI))
                m_intra_pic_flag = 0;
            retval = NALU_SLICE;
        }
        break;
    case NAL_UNIT_CODED_SLICE_SCALABLE:
    case NAL_UNIT_CODED_SLICE_IDR_SCALABLE:
        if ((m_bUseMVC || m_bUseSVC) && (slice_header(&slh, nal_ref_idc, nal_unit_type)))
        {
            if (picture_boundary)
            {
                const seq_parameter_set_s *sps;
                if (m_bUseSVC) {
                    sps = m_spssvcs[m_ppss[slh.pic_parameter_set_id]->seq_parameter_set_id];
                } else {
                    sps = m_spss[m_ppss[slh.pic_parameter_set_id]->seq_parameter_set_id];
                }

                if ((slh.nal_unit_type == 5) || (!m_MaxDpbSize) // IDR picture (or first non-IDR picture after a sequence header)
                    || (sps->pic_width_in_mbs_minus1 != m_sps->pic_width_in_mbs_minus1)
                    || (sps->pic_height_in_map_units_minus1 != m_sps->pic_height_in_map_units_minus1)
                    || (sps->log2_max_frame_num_minus4 != m_sps->log2_max_frame_num_minus4) )
                {
                    if (!dpb_sequence_start(&slh)) {
                        return NALU_UNKNOWN;
                    }
                }
                slh.sei_pic_struct = m_last_sei_pic_struct;
                slh.primary_pic_type = m_last_primary_pic_type;
                m_last_sei_pic_struct = -1;
                m_last_primary_pic_type = -1;
                if (!m_bUseSVC) // for SVC, it is handled inside BeginPicture_SVC
                    dpb_picture_start(m_ppss[slh.pic_parameter_set_id], &slh);
                m_intra_pic_flag = 1;
            }
            if ((slh.slice_type != I) && (slh.slice_type != SI))
                m_intra_pic_flag = 0;
            retval = NALU_SLICE;
        }
        break;
    case NAL_UNIT_SEI:
        // sei_rbsp, sei_message (7.3.2.3)
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
                nvParserLog("ignoring truncated SEI message (%d/%d)\n", payloadSize, available_bits()/8);
                break;
            }
            bitsUsed = consumed_bits();
            sei_payload(payloadType, payloadSize);
            // Skip over unknown payloads (NOTE: assumes that emulation prevention bytes are not present)
            skip = payloadSize * 8 - (consumed_bits() - bitsUsed);
            if (skip > 0) {
                skip_bits(skip);
            }
        }
        break;
    case NAL_UNIT_SPS:
        seq_parameter_set_rbsp();
        break;
    case NAL_UNIT_SUBSET_SPS:
        if(m_bUseMVC)
        {
            int32_t sps_id = seq_parameter_set_rbsp(SPS_NAL_UNIT_TARGET_SPS_MVC);
            seq_parameter_set_mvc_extension_rbsp(sps_id);
        }
        else if(m_bUseSVC)
        {
            seq_parameter_set_svc_extension_rbsp();
        }
        break;
    case NAL_UNIT_PPS:
        pic_parameter_set_rbsp();
        break;
    case NAL_UNIT_ACCESS_UNIT_DELIMITER:
        m_last_primary_pic_type = u(3);
        break;
    case NAL_UNIT_CODED_SLICE_PREFIX:
        if (m_bUseSVC)
        {
            if (m_nhe.svc_extension_flag) 
            {
                m_prefix_nalu_valid = true;
                prefix_nal_unit_svc(nal_ref_idc); // prefix NAL unit
            }
        }
        else if (m_bUseMVC)
        {
            if (!m_nhe.svc_extension_flag)
            {
                m_prefix_nalu_valid = true;
            }
        }
        else
        {
            retval = NALU_UNKNOWN;
        }
        break;
    default:
        retval = NALU_UNKNOWN;  // Let the client know about unsupported NAL units
    }
    switch(nal_unit_type)
    {
    case NAL_UNIT_CODED_SLICE:
    case NAL_UNIT_CODED_SLICE_IDR:
    case NAL_UNIT_CODED_SLICE_PREFIX:
        break;
    default:
        m_prefix_nalu_valid = false;
    }
    return retval;
}

// G.7.3.2.12.1
bool VulkanH264Decoder::prefix_nal_unit_svc(int nal_ref_idc)
{
    int additional_prefix_nal_unit_extension_flag, additional_prefix_nal_unit_extension_data_flag = false;
    memset(&m_prefix_nal_unit_svc, 0, sizeof(m_prefix_nal_unit_svc));

    m_prefix_nal_unit_svc.nalu = m_nhe; 
    if (nal_ref_idc != 0)
    {
        m_prefix_nal_unit_svc.store_ref_base_pic_flag = u(1);
        if ((m_nhe.svc.use_ref_base_pic_flag || m_prefix_nal_unit_svc.store_ref_base_pic_flag) && !m_nhe.svc.idr_flag)
            m_prefix_nal_unit_svc.adaptive_ref_base_pic_marking_mode_flag = (unsigned char)dec_ref_base_pic_marking(m_prefix_nal_unit_svc.mmbco);

        additional_prefix_nal_unit_extension_flag = u(1);
        if (additional_prefix_nal_unit_extension_flag == 1)
            while (more_rbsp_data())
                additional_prefix_nal_unit_extension_data_flag = u(1);
        rbsp_trailing_bits();
    }
    else if (more_rbsp_data())
    {
        while (more_rbsp_data())
            additional_prefix_nal_unit_extension_data_flag = u(1);
        rbsp_trailing_bits();
    }

    return additional_prefix_nal_unit_extension_data_flag;
}

// G.7.3.3.5
int VulkanH264Decoder::dec_ref_base_pic_marking(memory_management_base_control_operation_s mmbco[MAX_MMCOS])
{
    int adaptive_ref_base_pic_marking_mode_flag = u(1);
    if (adaptive_ref_base_pic_marking_mode_flag)
    {
        int i = 0;
        do
        {
            if (i >= MAX_MMCOS) 
            {
                nvParserLog("Too many memory_management_base_control_operation\n");
                break;
            }
            mmbco[i].memory_management_base_control_operation = ue();
            if (mmbco[i].memory_management_base_control_operation == 1)
                mmbco[i].difference_of_base_pic_nums_minus1 = ue();
            if (mmbco[i].memory_management_base_control_operation == 2)
                mmbco[i].long_term_base_pic_num = ue();
        }
        while (mmbco[i++].memory_management_base_control_operation != 0);
    }
    return adaptive_ref_base_pic_marking_mode_flag;
}

VkPicIf *VulkanH264Decoder::alloc_picture()
{
    VkPicIf *p = NULL;
    if (m_pClient)
        m_pClient->AllocPictureBuffer(&p);
    return p;
}


void VulkanH264Decoder::output_picture(int nframe, int)
{
    if (!dpb[nframe].not_existing)
        display_picture(dpb[nframe].pPicBuf);
}

void VulkanH264Decoder::output_picture_SVC(VkPicIf *pPicBuf, int)
{
    //assert(m_ds == &dependency_state[m_iDQIdMax >> 4]);
    //if (m_ds != &dependency_state[m_iDQIdMax >> 4])
    //    nvParserLog("output of picture that is not in target dependency representation\n");
    display_picture(pPicBuf);    
}

static StdVideoH264LevelIdc levelIdcToVulkanLevelIdcEnum(uint8_t level_idc, bool constraint_set3_flag)
{
    // If level_idc is equal to 9 or 11 and constraint_set3_flag is equal to 1,
    // the indicated level is level 1b.
    // Otherwise (level_idc is not equal to 11 or constraint_set3_flag is not equal to 1),
    // level_idc is equal to a value of ten times the level number (of the indicated level)
    // specified in Table A-1.

    static const uint32_t H264_LEVEL_IDC_1_0 = (uint32_t)(1.0 * 10);
    static const uint32_t H264_LEVEL_IDC_1_1 = (uint32_t)(1.1 * 10);
    static const uint32_t H264_LEVEL_IDC_1_2 = (uint32_t)(1.2 * 10);
    static const uint32_t H264_LEVEL_IDC_1_3 = (uint32_t)(1.3 * 10);
    static const uint32_t H264_LEVEL_IDC_2_0 = (uint32_t)(2.0 * 10);
    static const uint32_t H264_LEVEL_IDC_2_1 = (uint32_t)(2.1 * 10);
    static const uint32_t H264_LEVEL_IDC_2_2 = (uint32_t)(2.2 * 10);
    static const uint32_t H264_LEVEL_IDC_3_0 = (uint32_t)(3.0 * 10);
    static const uint32_t H264_LEVEL_IDC_3_1 = (uint32_t)(3.1 * 10);
    static const uint32_t H264_LEVEL_IDC_3_2 = (uint32_t)(3.2 * 10);
    static const uint32_t H264_LEVEL_IDC_4_0 = (uint32_t)(4.0 * 10);
    static const uint32_t H264_LEVEL_IDC_4_1 = (uint32_t)(4.1 * 10);
    static const uint32_t H264_LEVEL_IDC_4_2 = (uint32_t)(4.2 * 10);
    static const uint32_t H264_LEVEL_IDC_5_0 = (uint32_t)(5.0 * 10);
    static const uint32_t H264_LEVEL_IDC_5_1 = (uint32_t)(5.1 * 10);
    static const uint32_t H264_LEVEL_IDC_5_2 = (uint32_t)(5.2 * 10);
    static const uint32_t H264_LEVEL_IDC_6_0 = (uint32_t)(6.0 * 10);
    static const uint32_t H264_LEVEL_IDC_6_1 = (uint32_t)(6.1 * 10);
    static const uint32_t H264_LEVEL_IDC_6_2 = (uint32_t)(6.2 * 10);

    if ((level_idc == 9) || ((level_idc == 11) && constraint_set3_flag)) {
        // We don't have an enum in Vulkan for 1b profile,
        // so use the next level 1.1
        return STD_VIDEO_H264_LEVEL_IDC_1_1;
    }

    switch (level_idc) {
        case H264_LEVEL_IDC_1_0: return STD_VIDEO_H264_LEVEL_IDC_1_0;
        case H264_LEVEL_IDC_1_1: return STD_VIDEO_H264_LEVEL_IDC_1_1;
        case H264_LEVEL_IDC_1_2: return STD_VIDEO_H264_LEVEL_IDC_1_2;
        case H264_LEVEL_IDC_1_3: return STD_VIDEO_H264_LEVEL_IDC_1_3;
        case H264_LEVEL_IDC_2_0: return STD_VIDEO_H264_LEVEL_IDC_2_0;
        case H264_LEVEL_IDC_2_1: return STD_VIDEO_H264_LEVEL_IDC_2_1;
        case H264_LEVEL_IDC_2_2: return STD_VIDEO_H264_LEVEL_IDC_2_2;
        case H264_LEVEL_IDC_3_0: return STD_VIDEO_H264_LEVEL_IDC_3_0;
        case H264_LEVEL_IDC_3_1: return STD_VIDEO_H264_LEVEL_IDC_3_1;
        case H264_LEVEL_IDC_3_2: return STD_VIDEO_H264_LEVEL_IDC_3_2;
        case H264_LEVEL_IDC_4_0: return STD_VIDEO_H264_LEVEL_IDC_4_0;
        case H264_LEVEL_IDC_4_1: return STD_VIDEO_H264_LEVEL_IDC_4_1;
        case H264_LEVEL_IDC_4_2: return STD_VIDEO_H264_LEVEL_IDC_4_2;
        case H264_LEVEL_IDC_5_0: return STD_VIDEO_H264_LEVEL_IDC_5_0;
        case H264_LEVEL_IDC_5_1: return STD_VIDEO_H264_LEVEL_IDC_5_1;
        case H264_LEVEL_IDC_5_2: return STD_VIDEO_H264_LEVEL_IDC_5_2;
        case H264_LEVEL_IDC_6_0: return STD_VIDEO_H264_LEVEL_IDC_6_0;
        case H264_LEVEL_IDC_6_1: return STD_VIDEO_H264_LEVEL_IDC_6_1;
        case H264_LEVEL_IDC_6_2: return STD_VIDEO_H264_LEVEL_IDC_6_2;
        default:
            return STD_VIDEO_H264_LEVEL_IDC_6_2;
    }
}

int32_t VulkanH264Decoder::seq_parameter_set_rbsp(SpsNalUnitTarget spsNalUnitTarget,
                                                  seq_parameter_set_s *spssvc)
{
    uint8_t profile_idc = u(8);
    uint8_t constraint_set_flags = u(8);
    uint8_t level_idc = u(8);
    int sps_id = ue();
    if ((sps_id < 0) || (sps_id >= MAX_NUM_SPS))
    {
        nvParserLog("Invalid SPS id (%d)\n", sps_id);
        return -1;
    }
    m_last_sps_id = sps_id;

    VkSharedBaseObj<seq_parameter_set_s> sps(spssvc);
    if (spssvc == nullptr) {
        VkResult result = seq_parameter_set_s::Create(0, sps);
        assert((result == VK_SUCCESS) && sps);
        if (result != VK_SUCCESS) {
            return false;
        }
    }

    // non-zero defaults
    sps->seq_parameter_set_id = sps_id;
    sps->chroma_format_idc = (StdVideoH264ChromaFormatIdc)1;
    sps->svc.slice_header_restriction_flag = 1;

    sps->profile_idc = (StdVideoH264ProfileIdc)profile_idc;
    sps->constraint_set_flags = constraint_set_flags;

    sps->flags.constraint_set0_flag = (constraint_set_flags >> 0) & 1;
    sps->flags.constraint_set1_flag = (constraint_set_flags >> 1) & 1;
    sps->flags.constraint_set2_flag = (constraint_set_flags >> 2) & 1;
    sps->flags.constraint_set3_flag = (constraint_set_flags >> 3) & 1;
    sps->flags.constraint_set4_flag = (constraint_set_flags >> 4) & 1;
    sps->flags.constraint_set5_flag = (constraint_set_flags >> 5) & 1;

    // Table A-1 Level limits
    sps->level_idc = levelIdcToVulkanLevelIdcEnum(level_idc, sps->flags.constraint_set3_flag);
    if ((profile_idc == 100) || (profile_idc == 110) || (profile_idc == 122) || (profile_idc == 244) ||
        (profile_idc == 44)  || (profile_idc == 83)  || (profile_idc == 86)  || (profile_idc == 118) ||
        (profile_idc == 128) || (profile_idc == 138) || (profile_idc == 139) || (profile_idc == 134) ||
        (profile_idc == 135))
    {
        sps->chroma_format_idc = (StdVideoH264ChromaFormatIdc)ue();
        if ((sps->chroma_format_idc < 0) || (sps->chroma_format_idc > 3))
        {
            nvParserLog("Invalid chroma_format_idc value in SPS (%d)\n", sps->chroma_format_idc);
            return -1;
        }
        if (sps->chroma_format_idc == 3) {
            sps->flags.separate_colour_plane_flag = u(1);
        }
        sps->bit_depth_luma_minus8 = ue();
        sps->bit_depth_chroma_minus8 = ue();
        sps->flags.qpprime_y_zero_transform_bypass_flag = u(1);
        sps->seqScalingList.scaling_matrix_present_flag = u(1);
        if (sps->seqScalingList.scaling_matrix_present_flag) {
            for (int i = 0; i < 8; i++) {
                int scaling_list_type;
                if (i < 6) {
                    scaling_list_type = scaling_list(sps->seqScalingList.ScalingList4x4[i], 16);
                } else {
                    scaling_list_type = scaling_list(sps->seqScalingList.ScalingList8x8[i - 6], 64);
                }
                sps->seqScalingList.scaling_list_type[i] = (unsigned char)scaling_list_type;
            }
        }
    }
    sps->log2_max_frame_num_minus4 = ue();
    if ((uint32_t)sps->log2_max_frame_num_minus4 > 12) {
        nvParserLog("Invalid log2_max_frame_num_minus4 value in SPS (%d)\n", sps->log2_max_frame_num_minus4);
        return -1;
    }
    sps->pic_order_cnt_type = (StdVideoH264PocType)ue();
    if ((sps->pic_order_cnt_type < 0) || (sps->pic_order_cnt_type > 2)) {
        nvParserLog("Invalid pic_order_cnt_type value in SPS (%d)\n", sps->pic_order_cnt_type);
        return -1;
    }
    if (sps->pic_order_cnt_type == 0) {
        sps->log2_max_pic_order_cnt_lsb_minus4 = ue();
        if ((uint32_t)sps->log2_max_pic_order_cnt_lsb_minus4 > 12) {
            nvParserLog("Invalid log2_max_pic_order_cnt_lsb_minus4 value in SPS (%d)\n", sps->log2_max_pic_order_cnt_lsb_minus4);
            return -1;
        }
    }
    else if (sps->pic_order_cnt_type == 1) {
        sps->flags.delta_pic_order_always_zero_flag = u(1);
        sps->offset_for_non_ref_pic = se();
        sps->offset_for_top_to_bottom_field = se();
        uint32_t num_ref_frames_in_pic_order_cnt_cycle = ue();
        if (num_ref_frames_in_pic_order_cnt_cycle > 255) {
            nvParserLog("Invalid num_ref_frames_in_pic_order_cnt_cycle value in SPS (%d)\n", sps->num_ref_frames_in_pic_order_cnt_cycle);
            return -1;
        }
        sps->num_ref_frames_in_pic_order_cnt_cycle = (uint8_t)num_ref_frames_in_pic_order_cnt_cycle;

        for (int i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++) {
            sps->offset_for_ref_frame[i] = se();
        }
    }
    sps->max_num_ref_frames = ue();
    if (sps->max_num_ref_frames > 16) {
        nvParserLog("SPS: Invalid num_ref_frames value (%d)", sps->max_num_ref_frames);
        sps->max_num_ref_frames = 2;
        return -1;
    }
    sps->flags.gaps_in_frame_num_value_allowed_flag = u(1);
    sps->pic_width_in_mbs_minus1 = ue();
    sps->pic_height_in_map_units_minus1 = ue();
    if ((sps->pic_width_in_mbs_minus1 > 511) || (sps->pic_height_in_map_units_minus1 > 511)) { // enable upto 8192x8192
        nvParserLog("SPS: Unsupported picture size (%d x %d)",
            (sps->pic_width_in_mbs_minus1 + 1) * 16, (sps->pic_height_in_map_units_minus1 + 1) * 16);
        return -1;
    }
    sps->flags.frame_mbs_only_flag = u(1);
    if (!sps->flags.frame_mbs_only_flag) {
        sps->flags.mb_adaptive_frame_field_flag = u(1);
    }
    sps->flags.direct_8x8_inference_flag = u(1);
    sps->flags.frame_cropping_flag = u(1);
    if (sps->flags.frame_cropping_flag) {
        sps->frame_crop_left_offset = ue();
        sps->frame_crop_right_offset = ue();
        sps->frame_crop_top_offset = ue();
        sps->frame_crop_bottom_offset = ue();
    }
    sps->flags.vui_parameters_present_flag = u(1);
    sps->vui.initial_cpb_removal_delay_length = 24;

    if (sps->flags.vui_parameters_present_flag) {
        vui_parameters(&sps->vui);
    }

    int MaxDpbSize = derive_MaxDpbFrames(sps);
    if (MaxDpbSize < sps->max_num_ref_frames) {
        nvParserLog("WARNING: num_ref_frames violates level restrictions (%d/%d)\n", sps->max_num_ref_frames, MaxDpbSize);
        MaxDpbSize = sps->max_num_ref_frames;
    }

    if (sps->vui.max_num_reorder_frames > sps->vui.max_dec_frame_buffering) {
        nvParserLog("WARNING: Invalid max_num_reorder_frames (%d)\n", sps->vui.max_num_reorder_frames);
        sps->vui.max_num_reorder_frames = sps->vui.max_dec_frame_buffering;
    }

    if (sps->vui.max_dec_frame_buffering == 0 &&
        (!(constraint_set_flags & 0x10) || // constraint_set3_flag == 0 or profile_idc is not equal to below values
        !(profile_idc == 44 || profile_idc == 86 || profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 244))) {
        sps->vui.max_dec_frame_buffering = MaxDpbSize;
        if ((sps->pic_order_cnt_type != 2)) {
            sps->vui.max_num_reorder_frames = MaxDpbSize;
        }
    }

    if (m_bUseMVC)
    {
        m_MaxRefFramesPerView = std::max<int32_t>(m_MaxRefFramesPerView,
            std::max<int32_t>(sps->vui.max_dec_frame_buffering, (int)sps->max_num_ref_frames));
    }

    sps->flags.seq_scaling_matrix_present_flag = sps->UpdateStdScalingList(sps, &sps->stdScalingLists);
    if (sps->flags.seq_scaling_matrix_present_flag) {
        sps->pScalingLists = &sps->stdScalingLists;
    } else {
        sps->pScalingLists = NULL;
    }

    if (sps->UpdateStdVui(sps, &sps->stdVui, &sps->stdHrdParameters)) {
        sps->pSequenceParameterSetVui = &sps->stdVui;
    } else {
        sps->pSequenceParameterSetVui = NULL;
    }

    if (spssvc == nullptr)
    {
        if ((spsNalUnitTarget == SPS_NAL_UNIT_TARGET_SPS) && m_outOfBandPictureParameters && m_pClient) {

            sps->SetSequenceCount(m_pParserData->spssClientUpdateCount[sps_id]++);
            VkSharedBaseObj<StdVideoPictureParametersSet> picParamObj(sps);
            bool success = m_pClient->UpdatePictureParameters(picParamObj, sps->client);
            assert(success);
            if (success == false) {
                nvParserErrorLog("s", "\nError Updating the h.264 SPS parameters\n");
            }
        }
        m_spss[sps_id] = sps;
    }

    return sps_id;
}

// SVC extension (Annex G.7.3.2.1.4)
bool VulkanH264Decoder::seq_parameter_set_svc_extension_rbsp()
{

    VkSharedBaseObj<seq_parameter_set_s> spssvc;
    VkResult result = seq_parameter_set_s::Create(0, spssvc);
    assert((result == VK_SUCCESS) && spssvc);
    if (result != VK_SUCCESS) {
        return false;
    }

    int32_t sps_id = seq_parameter_set_rbsp(SPS_NAL_UNIT_TARGET_SPS_SVC, spssvc);
    if (spssvc->profile_idc == 83 || spssvc->profile_idc == 86) // Scalable Baseline or Scalable High
    {
        spssvc->svc.chroma_phase_x_plus1_flag = 1;
        spssvc->svc.chroma_phase_y_plus1 = 1;

        spssvc->svc.inter_layer_deblocking_filter_control_present_flag = u(1);
        spssvc->svc.extended_spatial_scalability_idc = u(2);
        if (spssvc->chroma_format_idc == 1 || spssvc->chroma_format_idc == 2)
            spssvc->svc.chroma_phase_x_plus1_flag = u(1);
        if (spssvc->chroma_format_idc == 1)
            spssvc->svc.chroma_phase_y_plus1 = u(2);

        spssvc->svc.seq_ref_layer_chroma_phase_x_plus1_flag = spssvc->svc.chroma_phase_x_plus1_flag;
        spssvc->svc.seq_ref_layer_chroma_phase_y_plus1      = spssvc->svc.chroma_phase_y_plus1;

        if (spssvc->svc.extended_spatial_scalability_idc == 1)
        {
            if (spssvc->chroma_format_idc > 0)
            {
                spssvc->svc.seq_ref_layer_chroma_phase_x_plus1_flag = u(1);
                spssvc->svc.seq_ref_layer_chroma_phase_y_plus1      = u(2);
            }
            spssvc->svc.seq_scaled_ref_layer_left_offset   = se();
            spssvc->svc.seq_scaled_ref_layer_top_offset    = se();
            spssvc->svc.seq_scaled_ref_layer_right_offset  = se();
            spssvc->svc.seq_scaled_ref_layer_bottom_offset = se();
        }
        spssvc->svc.seq_tcoeff_level_prediction_flag = u(1);
        if (spssvc->svc.seq_tcoeff_level_prediction_flag)
            spssvc->svc.adaptive_tcoeff_level_prediction_flag = u(1);
        spssvc->svc.slice_header_restriction_flag = u(1);
    }

    if (m_outOfBandPictureParameters && m_pClient) {

        assert(sps_id == m_last_sps_id);
        spssvc->SetSequenceCount(m_pParserData->spssvcsClientUpdateCount[m_last_sps_id]++);
        VkSharedBaseObj<StdVideoPictureParametersSet> picParamObj(spssvc);
        bool success = m_pClient->UpdatePictureParameters(picParamObj, spssvc->client);
        assert(success);
        if (success == false) {
            nvParserErrorLog("s", "\nError Updating the h.264 SPS ID %d SVC parameters\n", sps_id);
        }
    }

    m_spssvcs[m_last_sps_id] = spssvc;
    return true;
}

// MVC extension (Annex H.7.4.2.1.4)
bool VulkanH264Decoder::seq_parameter_set_mvc_extension_rbsp(int32_t sps_id)
{
    seq_parameter_set_mvc_extension_s spstmp = seq_parameter_set_mvc_extension_s();

    u(1); // bit_equal_to_one, should always be 1;

    spstmp.num_views_minus1 = ue();
    spstmp.view_id = new int[spstmp.num_views_minus1 + 1];
    for(int i = 0; i <= spstmp.num_views_minus1; i++)
    {
        spstmp.view_id[i] = ue();
    }
    spstmp.num_anchor_refs_l0 = new int[spstmp.num_views_minus1 + 1];
    spstmp.num_anchor_refs_l1 = new int[spstmp.num_views_minus1 + 1];
    spstmp.anchor_ref_l0 = new int *[spstmp.num_views_minus1 + 1];
    spstmp.anchor_ref_l1 = new int *[spstmp.num_views_minus1 + 1];
    for(int i = 1; i <= spstmp.num_views_minus1; i++)
    {
        spstmp.num_anchor_refs_l0[i] = ue();
        spstmp.anchor_ref_l0[i] = new int[spstmp.num_anchor_refs_l0[i]];
        for(int j = 0; j < spstmp.num_anchor_refs_l0[i]; j++)
        {
            spstmp.anchor_ref_l0[i][j] = ue();
        }
        spstmp.num_anchor_refs_l1[i] = ue();
        spstmp.anchor_ref_l1[i] = new int[spstmp.num_anchor_refs_l1[i]];
        for(int j = 0; j < spstmp.num_anchor_refs_l1[i]; j++)
        {
            spstmp.anchor_ref_l1[i][j] = ue();
        }
    }
    spstmp.num_non_anchor_refs_l0 = new int[spstmp.num_views_minus1 + 1];
    spstmp.num_non_anchor_refs_l1 = new int[spstmp.num_views_minus1 + 1];
    spstmp.non_anchor_ref_l0 = new int *[spstmp.num_views_minus1 + 1];
    spstmp.non_anchor_ref_l1 = new int *[spstmp.num_views_minus1 + 1];
    for(int i = 1; i <= spstmp.num_views_minus1; i++)
    {
        spstmp.num_non_anchor_refs_l0[i] = ue();
        spstmp.non_anchor_ref_l0[i] = new int[spstmp.num_non_anchor_refs_l0[i]];
        for(int j = 0; j < spstmp.num_non_anchor_refs_l0[i]; j++)
        {
            spstmp.non_anchor_ref_l0[i][j] = ue();
        }
        spstmp.num_non_anchor_refs_l1[i] = ue();
        spstmp.non_anchor_ref_l1[i] = new int[spstmp.num_non_anchor_refs_l1[i]];
        for(int j = 0; j < spstmp.num_non_anchor_refs_l1[i]; j++)
        {
            spstmp.non_anchor_ref_l1[i][j] = ue();
        }
    }

    spstmp.num_level_values_signalled_minus1 = ue();
    spstmp.level_idc = new int[spstmp.num_level_values_signalled_minus1+1];
    spstmp.num_applicable_ops_minus1 = new int[spstmp.num_level_values_signalled_minus1+1];
    spstmp.applicable_op_temporal_id = new int *[spstmp.num_level_values_signalled_minus1+1];
    spstmp.applicable_op_num_target_views_minus1 = new int *[spstmp.num_level_values_signalled_minus1+1];
    spstmp.applicable_op_target_view_id = new int **[spstmp.num_level_values_signalled_minus1+1];
    spstmp.applicable_op_num_views_minus1 = new int *[spstmp.num_level_values_signalled_minus1+1];

    for(int i = 0; i <= spstmp.num_level_values_signalled_minus1; i++)
    {
        spstmp.level_idc[i] = u(8);
        spstmp.num_applicable_ops_minus1[i] = ue();

        spstmp.applicable_op_temporal_id[i] = new int [spstmp.num_applicable_ops_minus1[i]+1];
        spstmp.applicable_op_num_target_views_minus1[i] = new int [spstmp.num_applicable_ops_minus1[i]+1];
        spstmp.applicable_op_target_view_id[i] = new int *[spstmp.num_applicable_ops_minus1[i]+1];
        spstmp.applicable_op_num_views_minus1[i] = new int [spstmp.num_applicable_ops_minus1[i]+1];

        for(int j = 0; j <= spstmp.num_applicable_ops_minus1[i]; j++)
        {
            spstmp.applicable_op_temporal_id[i][j] = u(3);
            spstmp.applicable_op_num_target_views_minus1[i][j] = ue();
            spstmp.applicable_op_target_view_id[i][j] = new int[spstmp.applicable_op_num_target_views_minus1[i][j]+1];
            for(int k = 0; k <= spstmp.applicable_op_num_target_views_minus1[i][j]; k++)
            {
                spstmp.applicable_op_target_view_id[i][j][k] = ue();
            }
            spstmp.applicable_op_num_views_minus1[i][j] = ue();
        }
    }

    u(1); // mvc_vui_parameters_present_flag, should always be 0;
    u(1); // additional_extension2_flag

    m_pParserData->spsmes[m_last_sps_id].release();
    m_pParserData->spsmes[m_last_sps_id] = spstmp;
    m_spsmes[m_last_sps_id] = &(m_pParserData->spsmes[m_last_sps_id]);

    if (m_outOfBandPictureParameters && m_pClient) {
        assert(sps_id == m_last_sps_id);
        assert(m_spss[sps_id]);

        m_spss[sps_id]->SetSequenceCount(m_pParserData->spsmesClientUpdateCount[sps_id]++);
        VkSharedBaseObj<StdVideoPictureParametersSet> picParamObj(m_spss[sps_id]);
        bool success = m_pClient->UpdatePictureParameters(picParamObj, m_spss[sps_id]->client);
        assert(success);
        if (success == false) {
            nvParserErrorLog("s", "\nError Updating the h.264 SPS MVC parameters\n");
        }
    }
    return true;
}

// NOTE: Does not bypass the start code emulation prevention, which may lead
// to incorrect results for MVC if the nal unit header contains 00.00.03
void VulkanH264Decoder::nal_unit_header_extension()
{
    memset(&m_nhe, 0, sizeof(nalu_header_extension_u));
    m_nhe.svc_extension_flag = u(1);
    if (m_nhe.svc_extension_flag)
    {
        // SVC
        m_nhe.svc.idr_flag = u(1);
        m_nhe.svc.priority_id = u(6);
        m_nhe.svc.no_inter_layer_pred_flag = u(1);
        m_nhe.svc.dependency_id = u(3);
        m_nhe.svc.quality_id = u(4);
        m_nhe.svc.temporal_id = u(3);
        m_nhe.svc.use_ref_base_pic_flag = u(1);
        m_nhe.svc.discardable_flag = u(1);
        m_nhe.svc.output_flag = u(1);
        f(2, 3); // reserved_three_2bits
    } else
    {
        // MVC
        m_nhe.mvc.non_idr_flag = (unsigned char) u(1);
        m_nhe.mvc.priority_id = u(6);
        m_nhe.mvc.view_id = u(10);
        m_nhe.mvc.temporal_id = u(3);
        m_nhe.mvc.anchor_pic_flag = (unsigned char) u(1);
        m_nhe.mvc.inter_view_flag = (unsigned char) u(1);
        f(1, 1); // reserved_one_bit
    }
}

// VUI parameters (Annex E.1)
void VulkanH264Decoder::vui_parameters(vui_parameters_s *vui)
{
    vui->aspect_ratio_info_present_flag = u(1);
    if (vui->aspect_ratio_info_present_flag) { // aspect_ratio_info_present_flag
        vui->aspect_ratio_idc = u(8);
    } else {
        vui->aspect_ratio_idc = 0;
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
        vui->sar_width = u(16);
        vui->sar_height = u(16);
        break;
    default:    // Default to square pixels for everything else
        vui->sar_width = 1;
        vui->sar_height = 1;
        break;
    }
    vui->overscan_info_present_flag = u(1);
    if (vui->overscan_info_present_flag)   // overscan_info_present_flag
    {
        vui->overscan_appropriate_flag = u(1);   // overscan_appropriate_flag
    }
    // Default values
    vui->video_signal_type_present_flag = u(1);
    if (vui->video_signal_type_present_flag)
    {
        vui->video_format = u(3);
        vui->video_full_range_flag = u(1);
        vui->color_description_present_flag = u(1);
        if (vui->color_description_present_flag)   // colour_description_present_flag
        {
            vui->colour_primaries = u(8);
            vui->transfer_characteristics = u(8);
            vui->matrix_coefficients = u(8);
        }
    }

    vui->chroma_loc_info_present_flag = u(1);
    if (vui->chroma_loc_info_present_flag) // chroma_loc_info_present_flag
    {
        ue();   // chroma_sample_loc_type_top_field
        ue();   // chroma_sample_loc_type_bottom_field
    }
    vui->timing_info_present_flag = u(1);
    if (vui->timing_info_present_flag)
    {
        vui->num_units_in_tick = u(32);
        vui->time_scale = u(32);
        vui->fixed_frame_rate_flag = u(1);
    }
    vui->nal_hrd_parameters_present_flag = u(1);
    if (vui->nal_hrd_parameters_present_flag)
        hrd_parameters(vui, &vui->nal_hrd);
    vui->vcl_hrd_parameters_present_flag = u(1);
    if (vui->vcl_hrd_parameters_present_flag)
        hrd_parameters(vui, &vui->vcl_hrd);
    if (vui->nal_hrd_parameters_present_flag || vui->vcl_hrd_parameters_present_flag)
    {
        u(1); // low_delay_hrd_flag;
    }
    vui->pic_struct_present_flag = u(1);
    vui->bitstream_restriction_flag = u(1);
    if (vui->bitstream_restriction_flag) // bitstream_restriction_flag
    {
        u(1);   // motion_vectors_over_pic_boundaries_flag
        ue();   // max_bytes_per_pic_denom
        ue();   // max_bits_per_mb_denom
        ue();   // log2_max_mv_length_horizontal
        ue();   // log2_max_mv_length_vertical
        vui->max_num_reorder_frames = ue();
        vui->max_dec_frame_buffering = ue();
    }
}


// HRD parameters (E.1.2)
void VulkanH264Decoder::hrd_parameters(vui_parameters_s *vui, hrd_parameters_s *hrd)
{
    uint8_t cpb_cnt_minus1 = ue();   // cpb_cnt_minus1
    hrd->bit_rate_scale = u(4) + 6; // bit_rate_scale
    hrd->cpb_size_scale = u(4) + 4; // cpb_size_scale
    hrd->cpb_cnt_minus1 = cpb_cnt_minus1;
    for (int SchedSelIdx = 0; SchedSelIdx <= cpb_cnt_minus1; SchedSelIdx++)
    {
        hrd->bit_rate = (ue() + 1) << hrd->bit_rate_scale;   // bit_rate_value_minus1[SchedSelIdx]
        hrd->cbp_size = (ue() + 1) << hrd->cpb_size_scale;   // cpb_size_value_minus1[SchedSelIdx]
        u(1);   // cbr_flag[SchedSelIdx]
        if (m_nalu.get_offset >= m_nalu.end_offset) { // In case of bitstream error
            break;
        }
    }
    vui->initial_cpb_removal_delay_length = u(5)+1;
    vui->cpb_removal_delay_length_minus1 = u(5);
    vui->dpb_output_delay_length_minus1 = u(5);
    hrd->time_offset_length = u(5);   // time_offset_length
}


int VulkanH264Decoder::scaling_list(unsigned char scalingList[], int sizeOfScalingList)
{
    int lastScale, nextScale, j, delta_scale;
    int scaling_list_type = SCALING_LIST_NOT_PRESENT;
    if (u(1)) // scaling_list_present_flag
    {
        scaling_list_type = SCALING_LIST_PRESENT;
        lastScale = 8;
        nextScale = 8;
        for (j = 0; j < sizeOfScalingList; j++)
        {
            if (nextScale != 0 )
            {       
                delta_scale = se();
                nextScale = (lastScale + delta_scale) & 0xff;
                scaling_list_type = ((j == 0) && (nextScale == 0)) ? SCALING_LIST_USE_DEFAULT : SCALING_LIST_PRESENT;
            }
            scalingList[j] = (unsigned char)((nextScale == 0) ? lastScale : nextScale);
            lastScale = scalingList[j];
        }
    }
    return scaling_list_type;
}


bool VulkanH264Decoder::pic_parameter_set_rbsp()
{
    int pps_id = ue();
    int sps_id = ue();
    if ((pps_id < 0) || (pps_id >= MAX_NUM_PPS) || (sps_id < 0) || (sps_id >= MAX_NUM_SPS))
    {
        nvParserLog("Invalid PPS: pps_id=%d, sps_id=%d\n", pps_id, sps_id);
        return false;
    }
    m_last_sps_id = sps_id;

    VkSharedBaseObj<pic_parameter_set_s> pps;
    VkResult result = pic_parameter_set_s::Create(0, pps);
    assert((result == VK_SUCCESS) && pps);
    if (result != VK_SUCCESS) {
        return false;
    }

    pps->pic_parameter_set_id = (uint8_t)pps_id;
    pps->seq_parameter_set_id = (uint8_t)sps_id;
    pps->flags.entropy_coding_mode_flag = u(1);
    pps->flags.bottom_field_pic_order_in_frame_present_flag = u(1);
    // FMO
    uint8_t num_slice_groups_minus1 = (uint8_t)ue();
    if (pps->num_slice_groups_minus1 > 7)
    {
        nvParserLog("Invalid num_slice_groups_minus1 value in PPS (%d)\n", pps->num_slice_groups_minus1);
        return false;
    }
    pps->num_slice_groups_minus1 = (unsigned char)num_slice_groups_minus1;
    if (num_slice_groups_minus1 > 0)
    {
        if (m_slice_group_map == nullptr) {
            m_slice_group_map = new slice_group_map_s[MAX_NUM_PPS];
            if (m_slice_group_map == nullptr) {
                return false;
            }
            memset(m_slice_group_map, 0x00, sizeof(slice_group_map_s[MAX_NUM_PPS]));
        }

        slice_group_map_s *slcgrp = &m_slice_group_map[pps_id];
        slcgrp->slice_group_map_type = ue();
        if (slcgrp->slice_group_map_type > 6)
        {
            nvParserLog("Invalid slice_group_map_type value in PPS (%d)\n", slcgrp->slice_group_map_type);
            return false;
        }
        if (slcgrp->slice_group_map_type == 0)
        {
            for (uint32_t iGroup = 0; iGroup <= pps->num_slice_groups_minus1; iGroup++) {
                /* slcgrp->run_length_minus1[iGroup] = */ ue();
            }
        }
        else if (slcgrp->slice_group_map_type == 2)
        {
            for (uint32_t iGroup = 0; iGroup < pps->num_slice_groups_minus1; iGroup++)
            {
                /* slcgrp->top_left[iGroup] = */ ue();
                /* slcgrp->bottom_right[iGroup] = */ ue();
            }
        }
        else if ((slcgrp->slice_group_map_type >= 3) && (slcgrp->slice_group_map_type < 6))
        {
            /* slcgrp->slice_group_change_direction_flag = */ u(1);
            slcgrp->slice_group_change_rate_minus1 = ue();
        }
        else if (slcgrp->slice_group_map_type == 6)
        {
            uint32_t pic_size_in_map_units_minus1 = ue();

            uint32_t v = 0;
            for (v = 0; pps->num_slice_groups_minus1 >= (1 << v); v++);

            for (uint32_t i = 0; i <= pic_size_in_map_units_minus1; i++)
            {
                /* slice_group_id = */ u(v);
                // if (!(i & 1))
                //    slcgrp->mb2sliceid[i>>1] = (slice_group_id & 0xf);
                // else
                //    slcgrp->mb2sliceid[i>>1] |= (slice_group_id << 4);
            }
        }
    }
    uint8_t num_ref_idx_l0_active_minus1 = (uint8_t)ue();
    uint8_t num_ref_idx_l1_active_minus1 = (uint8_t)ue();
    if ((num_ref_idx_l0_active_minus1 > 31) || (num_ref_idx_l1_active_minus1 > 31))
    {
        nvParserLog("Invalid num_ref_idx_lX_active_minus1 in PPS (L0=%d,L1=%d)\n",
                num_ref_idx_l0_active_minus1, num_ref_idx_l1_active_minus1);
        return false;
    }
    pps->num_ref_idx_l0_default_active_minus1 = (unsigned char)num_ref_idx_l0_active_minus1;
    pps->num_ref_idx_l1_default_active_minus1 = (unsigned char)num_ref_idx_l1_active_minus1;
    pps->flags.weighted_pred_flag = u(1);
    pps->weighted_bipred_idc = (StdVideoH264WeightedBipredIdc)u(2);
    if (pps->weighted_bipred_idc > 2)
    {
        nvParserLog("Invalid weighted_bipred_idc value in PPS (%d)\n", pps->weighted_bipred_idc);
        return false;
    }
    pps->pic_init_qp_minus26 = (signed char)se();
    pps->pic_init_qs_minus26 = (signed char)se();
    pps->second_chroma_qp_index_offset = pps->chroma_qp_index_offset = (signed char)se();
    pps->flags.deblocking_filter_control_present_flag = u(1);
    pps->flags.constrained_intra_pred_flag = u(1);
    pps->flags.redundant_pic_cnt_present_flag = u(1);
    if ((next_bits(8) & 0x7f) != 0) // if (more_rbsp_data())
    {
        pps->flags.transform_8x8_mode_flag = u(1);
        pps->picScalinList.scaling_matrix_present_flag = u(1);
        if (pps->picScalinList.scaling_matrix_present_flag) {
            for (uint32_t i = 0; i < (uint32_t)(6 + 2 * pps->flags.transform_8x8_mode_flag); i++) {
                int scaling_list_type;
                if (i < 6) {
                    scaling_list_type = scaling_list(pps->picScalinList.ScalingList4x4[i], 16);
                } else {
                    scaling_list_type = scaling_list(pps->picScalinList.ScalingList8x8[i - 6], 64);
                }
                pps->picScalinList.scaling_list_type[i] = (unsigned char)scaling_list_type;
            }
        }
        pps->second_chroma_qp_index_offset = (signed char)se();
    }

    pps->flags.pic_scaling_matrix_present_flag = pps->UpdateStdScalingList(pps, &pps->stdScalingLists);
    if (pps->flags.pic_scaling_matrix_present_flag) {
        pps->pScalingLists = &pps->stdScalingLists;
    } else {
        pps->pScalingLists = NULL;
    }

    if (m_outOfBandPictureParameters && m_pClient) {

        pps->SetSequenceCount(m_pParserData->ppssClientUpdateCount[pps_id]++);
        VkSharedBaseObj<StdVideoPictureParametersSet> picParamObj(pps);
        bool success = m_pClient->UpdatePictureParameters(picParamObj, pps->client);
        assert(success);
        if (success == false) {
            nvParserErrorLog("s", "\nError Updating the h.264 PPS parameters\n");
        }
    }

    m_ppss[pps_id] = pps;
    return true;
}


// Parse the beginning of the slice header
bool VulkanH264Decoder::slice_header(slice_header_s *slh, int nal_ref_idc, int nal_unit_type)
{
    seq_parameter_set_s *sps;
    pic_parameter_set_s *pps;
    int MbaffFrameFlag, PicSizeInMbs;
    int no_inter_layer_pred_flag, quality_id;
    bool base_layer;

    memset(slh, 0, sizeof(slice_header_s));
    if (m_prefix_nalu_valid && (nal_unit_type == NAL_UNIT_CODED_SLICE || nal_unit_type == NAL_UNIT_CODED_SLICE_IDR))
    {
        // Store the prefix_nalu information in the slice header
        slh->nhe = m_bUseMVC ? m_nhe : m_prefix_nal_unit_svc.nalu;
        m_prefix_nalu_valid = false;
    }
    else
    {
        if (m_bUseMVC && !m_prefix_nalu_valid &&
            (nal_unit_type == NAL_UNIT_CODED_SLICE || 
             nal_unit_type == NAL_UNIT_CODED_SLICE_IDR))
        {
            // H.7.4.1.1: Defaults for base-view when no prefix:
            //     view_id = 0, inter_view_flag = 1
            // (Other fields don't matter)
            m_nhe.mvc.view_id = 0;
            m_nhe.mvc.inter_view_flag = 1;
        }
        slh->nhe = m_nhe;
    }
    slh->nal_ref_idc = (unsigned char)nal_ref_idc;
    slh->nal_unit_type = (unsigned char)nal_unit_type;
    
    if (slh->nhe.svc_extension_flag)
    {
        no_inter_layer_pred_flag = slh->nhe.svc.no_inter_layer_pred_flag;
        quality_id = slh->nhe.svc.quality_id;
        base_layer = slh->nal_unit_type == 1 || slh->nal_unit_type == 5;
        if (base_layer)
        {
            slh->store_ref_base_pic_flag = m_prefix_nal_unit_svc.store_ref_base_pic_flag;
            slh->adaptive_ref_base_pic_marking_mode_flag = m_prefix_nal_unit_svc.adaptive_ref_base_pic_marking_mode_flag;
            memcpy(slh->mmbco, m_prefix_nal_unit_svc.mmbco, sizeof(slh->mmbco));
        }
    }
    else
    {
        no_inter_layer_pred_flag = 1;
        quality_id = 0;
        base_layer = true;
    }

    slh->first_mb_in_slice = ue();
    slh->slice_type_raw = ue();
    //if ((uint32_t)slh->slice_type_raw > 9)
    //    return false;
    slh->slice_type = slh->slice_type_raw % 5;
    slh->pic_parameter_set_id = ue();
    if ((slh->pic_parameter_set_id < 0) || (slh->pic_parameter_set_id >= MAX_NUM_PPS)
     || (!m_ppss[slh->pic_parameter_set_id]))
    {
        nvParserLog("Invalid PPS id in slice header (%d)\n", slh->pic_parameter_set_id);
        return false;
    }
    pps = m_ppss[slh->pic_parameter_set_id];
    sps = base_layer ? m_spss[pps->seq_parameter_set_id] : m_spssvcs[pps->seq_parameter_set_id];
    if (!sps)
    {
        nvParserLog("PPS with missing associated SPS!\n");
        return false;
    }

    if ((m_lErrorThreshold < 60) && (slh->slice_type == B) && (sps->profile_idc == 66))
    {
        nvParserLog("B-slices not allowed in baseline profile!\n");
        m_eError = NV_NON_COMPLIANT_STREAM;
        return false;
    }
    m_pps = pps;
    m_sps = sps;
 
    if ((!sps->max_num_ref_frames) && (slh->slice_type != I) && (slh->slice_type != SI))
        return false;
    if (slh->nal_unit_type == 20)
    {
        if (slh->nhe.svc_extension_flag)
        {
            slh->IdrPicFlag = m_nhe.svc.idr_flag;
        } else
        {
            slh->IdrPicFlag = !m_nhe.mvc.non_idr_flag;
            slh->view_id = m_nhe.mvc.view_id;
        }
    } else
    {
        slh->IdrPicFlag = (slh->nal_unit_type == 5);
    }
    if (sps->flags.separate_colour_plane_flag)
    {
        slh->colour_plane_id = u(2);
        if ((slh->colour_plane_id < 0) || (slh->colour_plane_id > 2))
            return false;
    }
    slh->frame_num = u(sps->log2_max_frame_num_minus4 + 4);
    PicSizeInMbs = (sps->pic_width_in_mbs_minus1 + 1) * (sps->pic_height_in_map_units_minus1 + 1);
    if (!sps->flags.frame_mbs_only_flag)
    {
        slh->field_pic_flag = flag();
        if (slh->field_pic_flag)
        {
            slh->bottom_field_flag = flag();
        } else
        {
            PicSizeInMbs <<= 1;
        }
    }
    MbaffFrameFlag = sps->flags.mb_adaptive_frame_field_flag && !slh->field_pic_flag;
    if ((uint32_t)slh->first_mb_in_slice >= (uint32_t)(PicSizeInMbs >> MbaffFrameFlag))
        return false;
    
    if (slh->IdrPicFlag)
        slh->idr_pic_id = ue();
    if (sps->pic_order_cnt_type == 0)
    {
        slh->pic_order_cnt_lsb = u(sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
        if (pps->flags.bottom_field_pic_order_in_frame_present_flag && !slh->field_pic_flag)
            slh->delta_pic_order_cnt_bottom = se();
    }
    if (sps->pic_order_cnt_type == 1 && !sps->flags.delta_pic_order_always_zero_flag)
    {
        slh->delta_pic_order_cnt[0] = se();
        if (pps->flags.bottom_field_pic_order_in_frame_present_flag && !slh->field_pic_flag)
            slh->delta_pic_order_cnt[1] = se();
    }
    if (pps->flags.redundant_pic_cnt_present_flag)
    {
        slh->redundant_pic_cnt = ue();
        if (slh->redundant_pic_cnt != 0) {
            return false;   // ignore redundant slices
        }
    }
    
    if (quality_id == 0)
    {
        if (slh->slice_type == B) {
            slh->direct_spatial_mv_pred_flag = flag();
        }
        if (slh->slice_type == P || slh->slice_type == SP || slh->slice_type == B)
        {
            if (u(1)) // num_ref_idx_active_override_flag
            {
                slh->num_ref_idx_l0_active_minus1 = ue();
                if (slh->slice_type == B)
                    slh->num_ref_idx_l1_active_minus1 = ue();
                if (((uint32_t)slh->num_ref_idx_l0_active_minus1 > 31) || ((uint32_t)slh->num_ref_idx_l1_active_minus1 > 31)) {
                    return false;
                }
            } else
            {
                slh->num_ref_idx_l0_active_minus1 = pps->num_ref_idx_l0_default_active_minus1;
                slh->num_ref_idx_l1_active_minus1 = pps->num_ref_idx_l1_default_active_minus1;
            }
        }
        if (!ref_pic_list_reordering(slh))
        {
            return false;
        }
        if ((pps->flags.weighted_pred_flag && (slh->slice_type == P || slh->slice_type == SP)) || (pps->weighted_bipred_idc == 1 && slh->slice_type == B))
        {
            if (!no_inter_layer_pred_flag)
                slh->base_pred_weight_table_flag = u(1);
            if (no_inter_layer_pred_flag || !slh->base_pred_weight_table_flag)
            {
                if (!pred_weight_table(slh, sps->flags.separate_colour_plane_flag ? 0 : sps->chroma_format_idc)) {
                    return false;
                }
            }
        }
        if (slh->nal_ref_idc != 0)
        {
            dec_ref_pic_marking(slh);
            if (!base_layer && !sps->svc.slice_header_restriction_flag)
            {
                slh->store_ref_base_pic_flag = u(1);
                if ((slh->nhe.svc.use_ref_base_pic_flag || slh->store_ref_base_pic_flag) && !slh->IdrPicFlag)
                    slh->adaptive_ref_pic_marking_mode_flag = (unsigned char)dec_ref_base_pic_marking(slh->mmbco);
            }
        }
    }
    else
    {
        // infer
    }
    if (pps->flags.entropy_coding_mode_flag && slh->slice_type != I && slh->slice_type != SI)
        ue(); // cabac_init_idc
    se(); // slice_qp_delta
    if (slh->slice_type == SP || slh->slice_type == SI)
    {
        if (slh->slice_type == SP)
            u(1); // sp_for_switch_flag
        se(); // slice_qs_delta
    }
    if (pps->flags.deblocking_filter_control_present_flag)
    {
        if(ue() != 1) // disable_deblocking_filter_idc
        {
            se(); // slice_alpha_c0_offset_div2
            se(); // slice_beta_offset_div2                
        }
    }
    if (pps->num_slice_groups_minus1 > 0)
    {
        const slice_group_map_s *slcgrp = (m_slice_group_map == nullptr) ? nullptr : &m_slice_group_map[slh->pic_parameter_set_id];
        if ((slcgrp != nullptr) && (slcgrp->slice_group_map_type >= 3) && (slcgrp->slice_group_map_type <= 5))
        {
            uint32_t a = (sps->pic_width_in_mbs_minus1 + 1) * (sps->pic_height_in_map_units_minus1 + 1); // PicSizeInMapUnits
            int16_t b = slcgrp->slice_group_change_rate_minus1 + 1; // SliceGroupChangeRate
            if ((b <= 0) || ((uint32_t)b > a)) {
                return false;
            }
            int32_t c = (a + b - 1) / b; // Ceil(PicSizeInMapUnits / SliceGroupChangeRate)
            uint32_t v = 0;
            for (v = 0; c >= (1 << v); v++);
            slh->slice_group_change_cycle = u(v);
        }
    }

    // Need further parsing for SVC only
    if (slh->nhe.svc_extension_flag)
    {
        if (!no_inter_layer_pred_flag && quality_id == 0)
        {
            slh->ref_layer_dq_id = ue();
            if (sps->svc.inter_layer_deblocking_filter_control_present_flag)
            {
                slh->disable_inter_layer_deblocking_filter_idc = ue();
                if (slh->disable_inter_layer_deblocking_filter_idc != 1)
                {
                    slh->inter_layer_slice_alpha_c0_offset_div2 = se();
                    slh->inter_layer_slice_beta_offset_div2 = se();
                }
            }
            slh->constrained_intra_resampling_flag = u(1);
            // defaults
            slh->ref_layer_chroma_phase_x_plus1_flag = sps->svc.seq_ref_layer_chroma_phase_x_plus1_flag;
            slh->ref_layer_chroma_phase_y_plus1      = sps->svc.seq_ref_layer_chroma_phase_y_plus1;
            slh->scaled_ref_layer_left_offset        = sps->svc.seq_scaled_ref_layer_left_offset;
            slh->scaled_ref_layer_top_offset         = sps->svc.seq_scaled_ref_layer_top_offset;
            slh->scaled_ref_layer_right_offset       = sps->svc.seq_scaled_ref_layer_right_offset;
            slh->scaled_ref_layer_bottom_offset      = sps->svc.seq_scaled_ref_layer_bottom_offset;
            if (sps->svc.extended_spatial_scalability_idc == 2)
            {
                if (sps->chroma_format_idc > 0) // ChromaArrayType > 0
                {
                    slh->ref_layer_chroma_phase_x_plus1_flag = u(1);
                    slh->ref_layer_chroma_phase_y_plus1      = u(2);
                }
                slh->scaled_ref_layer_left_offset   = se();
                slh->scaled_ref_layer_top_offset    = se();
                slh->scaled_ref_layer_right_offset  = se();
                slh->scaled_ref_layer_bottom_offset = se();
            }
        }        
        if (!no_inter_layer_pred_flag)
        {
            slh->slice_skip_flag = u(1);
            if (slh->slice_skip_flag)
                slh->num_mbs_in_slice_minus1 = ue();
            else
            {
                slh->adaptive_base_mode_flag = u(1);
                if (!slh->adaptive_base_mode_flag)
                    slh->default_base_mode_flag = u(1);
                if (!slh->default_base_mode_flag)
                {
                    slh->adaptive_motion_prediction_flag = u(1);
                    if (!slh->adaptive_motion_prediction_flag)
                        slh->default_motion_prediction_flag = u(1);
                }
                slh->adaptive_residual_prediction_flag = u(1);
                if (!slh->adaptive_residual_prediction_flag)
                    slh->default_residual_prediction_flag = u(1);
            }
            // defaults
            slh->tcoeff_level_prediction_flag = sps->svc.seq_tcoeff_level_prediction_flag;
            if (sps->svc.adaptive_tcoeff_level_prediction_flag == 1)
                slh->tcoeff_level_prediction_flag = u(1);
        }
        m_slh_prev = *slh;
    }

    // update layer info
    if (m_bUseSVC) {
        update_layer_info(m_sps, m_pps, slh);
    }
    return true;
}

void VulkanH264Decoder::update_layer_info(seq_parameter_set_s *sps, pic_parameter_set_s *pps, slice_header_s *slh)
{  
    int dqid = (slh->nhe.svc.dependency_id << 4) + slh->nhe.svc.quality_id;
    if (!m_layer_data[dqid].available) // first slice of layer
    {
        m_layer_data[dqid].available = true;
        m_layer_data[dqid].sps = sps;
        m_layer_data[dqid].pps = pps;
        m_layer_data[dqid].slh = *slh;
        m_layer_data[dqid].MaxRefLayerDQId = -1;
    }

    // keep a slice header with no_inter_layer_pred_flag==0 (if any)
    if (m_layer_data[dqid].MaxRefLayerDQId < 0 && !slh->nhe.svc.no_inter_layer_pred_flag)
    {
        m_layer_data[dqid].slh = *slh;
        m_layer_data[dqid].MaxRefLayerDQId = (slh->nhe.svc.quality_id == 0) ? slh->ref_layer_dq_id : dqid - 1;
    }

    m_layer_data[dqid].slice_count++;
    
    m_slh_prev = *slh;
    m_bLayerFirstSlice = 0;
    return ;
}

// 7.4.3.1
bool VulkanH264Decoder::ref_pic_list_reordering(slice_header_s *slh)
{
    int i;

    if (slh->slice_type != I && slh->slice_type != SI)
    {
        slh->ref_pic_list_reordering_flag_l0 = flag();
        if (slh->ref_pic_list_reordering_flag_l0)
        {
            for (i=0; ; i++)
            {
                unsigned int reordering_of_pic_nums_idc = ue();
                if (reordering_of_pic_nums_idc > 5)
                {
                    return false;
                }
                if (i >= MAX_REFS)
                    break;
                slh->ref_pic_list_reordering_l0[i].reordering_of_pic_nums_idc = reordering_of_pic_nums_idc;
                if (reordering_of_pic_nums_idc == 3)
                    break;
                slh->ref_pic_list_reordering_l0[i].PicNumIdx = ue();
            }
        }
    }
    if (slh->slice_type == B)
    {
        slh->ref_pic_list_reordering_flag_l1 = flag();
        if (slh->ref_pic_list_reordering_flag_l1)
        {
            for (i=0; ; i++)
            {
                unsigned int reordering_of_pic_nums_idc = ue();
                if (reordering_of_pic_nums_idc > 5)
                {
                    return false;
                }
                if (i >= MAX_REFS)
                    break;
                slh->ref_pic_list_reordering_l1[i].reordering_of_pic_nums_idc = reordering_of_pic_nums_idc;
                if (reordering_of_pic_nums_idc == 3)
                    break;
                slh->ref_pic_list_reordering_l1[i].PicNumIdx = ue();
            }
        }
    }
    return true;
}


bool VulkanH264Decoder::pred_weight_table(slice_header_s *slh, int chromaArrayType)
{
    int i, j;

    slh->luma_log2_weight_denom = ue();
    if (chromaArrayType != 0)
        slh->chroma_log2_weight_denom = ue();
    if ((uint32_t)(slh->luma_log2_weight_denom|slh->chroma_log2_weight_denom) > 7)
        return false;
    for (i = 0; i <= slh->num_ref_idx_l0_active_minus1; i++)
    {
        int luma_weight_l0_flag = u(1);
        if (luma_weight_l0_flag)
        {
            int weight = se();
            int offset = se();
            slh->weights_out_of_range += ((weight < -128) || (weight > 127) || (offset < -128) || (offset > 127));
            slh->luma_weight[0][i] = (int16_t)weight;
            slh->luma_offset[0][i] = (int16_t)offset;
        }
        else
        {
            slh->luma_weight[0][i] = (int16_t)(1 << slh->luma_log2_weight_denom);
            slh->luma_offset[0][i] = 0;
        }
        if (chromaArrayType != 0)
        {
            int chroma_weight_l0_flag = u(1);
            if (chroma_weight_l0_flag)
            {
                for (j = 0; j < 2; j++)
                {
                    int weight = se();
                    int offset = se();
                    slh->weights_out_of_range += ((weight < -128) || (weight > 127) || (offset < -128) || (offset > 127));
                    slh->chroma_weight[0][i][j] = (int16_t)weight;
                    slh->chroma_offset[0][i][j] = (int16_t)offset;
                }
            }
            else
            {
                for (j = 0; j < 2; j++)
                {
                    slh->chroma_weight[0][i][j] = (int16_t)(1 << slh->chroma_log2_weight_denom);
                    slh->chroma_offset[0][i][j] = 0;
                }
            }
        }
    }
    if (slh->slice_type == B)
    {
        for (i = 0; i <= slh->num_ref_idx_l1_active_minus1; i++)
        {
            int luma_weight_l1_flag = u(1);
            if (luma_weight_l1_flag)
            {
                int weight = se();
                int offset = se();
                slh->weights_out_of_range += ((weight < -128) || (weight > 127) || (offset < -128) || (offset > 127));
                slh->luma_weight[1][i] = (int16_t)weight;
                slh->luma_offset[1][i] = (int16_t)offset;
            }
            else
            {
                slh->luma_weight[1][i] = (int16_t)(1 << slh->luma_log2_weight_denom);
                slh->luma_offset[1][i] = 0;
            }
            if (chromaArrayType != 0)
            {
                int chroma_weight_l1_flag = u(1);
                if (chroma_weight_l1_flag)
                {
                    for (j = 0; j < 2; j++)
                    {
                        int weight = se();
                        int offset = se();
                        slh->weights_out_of_range += ((weight < -128) || (weight > 127) || (offset < -128) || (offset > 127));
                        slh->chroma_weight[1][i][j] = (int16_t)weight;
                        slh->chroma_offset[1][i][j] = (int16_t)offset;
                    }
                }
                else
                {
                    for (j = 0; j < 2; j++)
                    {
                        slh->chroma_weight[1][i][j] = (int16_t)(1 << slh->chroma_log2_weight_denom);
                        slh->chroma_offset[1][i][j] = 0;
                    }
                }
            }
        }
    }
    return true;
}


void VulkanH264Decoder::dec_ref_pic_marking(slice_header_s *slh)
{
    if (slh->IdrPicFlag)
    {
        slh->no_output_of_prior_pics_flag = flag();
        slh->long_term_reference_flag = flag();
    }
    else
    {
        slh->adaptive_ref_pic_marking_mode_flag = flag();
        if (slh->adaptive_ref_pic_marking_mode_flag)
        {
            for (int i=0; i<MAX_MMCOS; i++)
            {
                slh->mmco[i].memory_management_control_operation = ue();
                if (slh->mmco[i].memory_management_control_operation == 0)
                    break;
                if (slh->mmco[i].memory_management_control_operation == 1 || slh->mmco[i].memory_management_control_operation == 3)
                    slh->mmco[i].difference_of_pic_nums_minus1 = ue();
                if ((slh->mmco[i].memory_management_control_operation == 2) || (slh->mmco[i].memory_management_control_operation == 3)
                 || (slh->mmco[i].memory_management_control_operation == 4) || (slh->mmco[i].memory_management_control_operation == 6))
                    slh->mmco[i].long_term_frame_idx = ue();
                if (slh->mmco[i].memory_management_control_operation == 5)
                    slh->mmco5 = 1;
            }
        }
    }
}



/////////////////////////////////////////////////////////////////////////////////////
//
// DPB management
//

bool VulkanH264Decoder::dpb_sequence_start(slice_header_s *slh)
{
    VkParserSequenceInfo nvsi;
    int PicWidthInMbs, FrameHeightInMbs, MaxDecFrameBuffering;
    
    m_PrevViewId = 0;
    m_PrevRefFrameNum = 0;
    
    m_slh = *slh;
    m_slh_prev = *slh;
    m_sps = m_spss[m_ppss[slh->pic_parameter_set_id]->seq_parameter_set_id];
    m_spsme = m_spsmes[m_ppss[slh->pic_parameter_set_id]->seq_parameter_set_id];
    //m_spssvc = m_spssvcs[m_ppss[slh->pic_parameter_set_id]->seq_parameter_set_id];

    const seq_parameter_set_s* sps = m_sps;

    if (!slh->no_output_of_prior_pics_flag) {
        flush_decoded_picture_buffer();
    }
    PicWidthInMbs    = sps->pic_width_in_mbs_minus1 + 1;
    FrameHeightInMbs = (2 - sps->flags.frame_mbs_only_flag) * (sps->pic_height_in_map_units_minus1 + 1);
    MaxDecFrameBuffering = std::min<int32_t>(std::max<int32_t>(sps->vui.max_dec_frame_buffering, (int)sps->max_num_ref_frames), 16);
    if (m_bUseMVC)
    {
        // Assuming 2 views; m_MaxRefFramesPerView is the max of any view component (H.7.4.2.1.1)
        MaxDecFrameBuffering = std::min<int32_t>(2 * m_MaxRefFramesPerView, 16);
    }

    memset(&nvsi, 0, sizeof(nvsi));
    // m_bUseSVC ? NVCS_H264_SVC : NVCS_H264;
    assert(!m_bUseSVC);
    nvsi.isSVC = m_bUseSVC;
    nvsi.eCodec = VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR;
    nvsi.frameRate = NV_FRAME_RATE_UNKNOWN;
    nvsi.bProgSeq = sps->flags.frame_mbs_only_flag;
    nvsi.nCodedWidth = PicWidthInMbs * 16;
    nvsi.nCodedHeight = FrameHeightInMbs * 16;
    nvsi.nDisplayWidth = nvsi.nCodedWidth;
    nvsi.nDisplayHeight = nvsi.nCodedHeight;
    if (sps->flags.frame_cropping_flag)
    {
        int crop_right = sps->frame_crop_right_offset * 2;
        int crop_bottom = sps->frame_crop_bottom_offset * 2 * (2 - sps->flags.frame_mbs_only_flag);
        if ((crop_right >= 0) && (crop_right < nvsi.nCodedWidth / 2)
         && (crop_bottom >= 0) && (crop_bottom < nvsi.nCodedHeight / 2))
        {
            nvsi.nDisplayWidth -= crop_right;
            nvsi.nDisplayHeight -= crop_bottom;
        }
    }
    nvsi.nChromaFormat = (uint8_t)sps->chroma_format_idc;
    nvsi.uBitDepthLumaMinus8 = (uint8_t)sps->bit_depth_luma_minus8;
    nvsi.uBitDepthChromaMinus8 = (uint8_t)sps->bit_depth_chroma_minus8;
    nvsi.lDARWidth = nvsi.nDisplayWidth;
    nvsi.lDARHeight = nvsi.nDisplayHeight;
    nvsi.lVideoFormat = VideoFormatUnspecified;
    nvsi.lColorPrimaries = ColorPrimariesUnspecified;
    nvsi.lTransferCharacteristics = TransferCharacteristicsUnspecified;
    nvsi.lMatrixCoefficients = MatrixCoefficientsUnspecified;
    nvsi.nMinNumDecodeSurfaces = MaxDecFrameBuffering + 1;
    if (sps->flags.vui_parameters_present_flag)
    {
        if ((sps->vui.sar_width > 0) && (sps->vui.sar_height > 0))
        {
            nvsi.lDARWidth = sps->vui.sar_width * nvsi.nDisplayWidth;
            nvsi.lDARHeight = sps->vui.sar_height * nvsi.nDisplayHeight;
        }
        if (sps->vui.video_signal_type_present_flag)
        {
            nvsi.lVideoFormat = sps->vui.video_format;
            nvsi.uVideoFullRange = (uint8_t)sps->vui.video_full_range_flag;
            if (sps->vui.color_description_present_flag)
            {
                nvsi.lColorPrimaries = sps->vui.colour_primaries;
                nvsi.lTransferCharacteristics = sps->vui.transfer_characteristics;
                nvsi.lMatrixCoefficients = sps->vui.matrix_coefficients;
            }
        }
        if (sps->vui.timing_info_present_flag)
        {
            uint32_t lNum = sps->vui.time_scale;   // lNum/lDenom = field rate in Hz
            uint32_t lDenom = sps->vui.num_units_in_tick;
        
            if ((lDenom > 0) && (lNum > lDenom)) // > 1Hz
            {
                nvsi.frameRate = PackFrameRate((lNum + 1) >> 1, lDenom);
            }
        }
        nvsi.lBitrate = sps->vui.nal_hrd.bit_rate;
    }
    SimplifyAspectRatio(&nvsi.lDARWidth, &nvsi.lDARHeight);

    int32_t MaxDpbSize = derive_MaxDpbFrames(sps);
    if (MaxDpbSize < sps->max_num_ref_frames)
    {
        nvParserLog("WARNING: num_ref_frames violates level restrictions (%d/%d)\n", sps->max_num_ref_frames, MaxDpbSize);
        MaxDpbSize = sps->max_num_ref_frames;
    }
    nvsi.nMinNumDpbSlots = std::min((MaxDpbSize + 1), (MAX_DPB_SIZE + 1)); // one extra slot for the current setup
    nvsi.codecProfile = sps->profile_idc;

    if (!m_bUseSVC)
    {
        if (!init_sequence(&nvsi)) {
            return false;
        }
    }

    // Update MaxDpbSize according to level limits
    if (m_MaxFrameBuffers > 0)
    {
        m_MaxDpbSize = std::min<int32_t>(m_MaxFrameBuffers, MaxDpbSize);
    }
    return true;
}

bool VulkanH264Decoder::is_comp_field_pair(dpb_entry_s *dpb_local, slice_header_s *slh)
{
    // check if this is the second field of a complementary field pair
    //
    // 3.30 complementary non-reference field pair:
    // Two non-reference fields that are in consecutive access units in decoding order as
    // - two coded fields of opposite parity where 
    // - the first field is not already a paired field.
    //
    // 3.31 complementary reference field pair:
    // Two reference fields that are in consecutive access units in decoding order as
    // - two coded fields and
    // - share the same value of the frame_num syntax element, where
    // - the second field in decoding order is not an IDR picture and
    // - does not include a memory_management_control_operation syntax element equal to 5.

    return (dpb_local->state == 1 || dpb_local->state == 2) &&          // contains a single field
            slh->field_pic_flag &&                          // current is a field
           ((dpb_local->state == 1 && slh->bottom_field_flag) ||
            (dpb_local->state == 2 && !slh->bottom_field_flag)) &&// opposite parity
           ((!dpb_local->reference_picture &&                     // first is a non-reference picture
           slh->nal_ref_idc == 0)                           // current is a non-reference picture
           ||
           (dpb_local->reference_picture &&                       // first is reference picture
           slh->nal_ref_idc != 0 &&                         // current is reference picture
           dpb_local->FrameNum == slh->frame_num &&               // same frame_num
           slh->nal_unit_type != 5 &&                       // current is not an IDR picture
           !slh->mmco5));                                   // current picture does not include an MMCO 5

}

bool VulkanH264Decoder::find_comp_field_pair(slice_header_s *slh, int *icur)
{
    int VOIdx = get_view_output_index(slh->view_id);
    VkPicIf *pPicBuf = CurrFrmViewPic[VOIdx];
    int i;

    if (pPicBuf)
    {
        for (i = 0; i < 16; i++)
        {
            if (dpb[i].pPicBuf == pPicBuf &&
                dpb[i].view_id == slh->view_id &&
                is_comp_field_pair(&dpb[i], slh))
            {
                (*icur) = i;
                return true;
            }
        }
    }
    return false;
}

// per picture processing before decoding first slice
void VulkanH264Decoder::dpb_picture_start(pic_parameter_set_s *pps, slice_header_s *slh)
{
    m_slh = *slh;
    m_slh_prev = *slh;
    m_pps = pps;
    m_spsme = m_spsmes[pps->seq_parameter_set_id];

    if (slh->view_id == m_PrevViewId)
        gaps_in_frame_num();

    // select decoded picture buffer
    if (dpb[iCur].view_id == slh->view_id &&        // same view_id as prev pic
        is_comp_field_pair(&dpb[iCur], slh))        // second field of a complementary field pair
    {
        // second field
        cur->complementary_field_pair = true;
    }
    else if (dpb[iCur].view_id != slh->view_id &&
             find_comp_field_pair(slh, &iCur))
    {
        cur = &dpb[iCur];
        cur->complementary_field_pair = true;
        // Reset view indices when we get the base view
        if ((slh->nal_unit_type == 1) || (slh->nal_unit_type == 5))
        {
            for (int i = 0; i <= MAX_DPB_SIZE; i++)
            {
                dpb[i].inter_view_flag = 0; // Reset inter view flags
            }
        }
        cur->inter_view_flag = m_nhe.mvc.inter_view_flag;
    }
    else
    {
        // Reset view indices when we get the base view
        if ((slh->nal_unit_type == 1) || (slh->nal_unit_type == 5))
        {
            memset(CurrFrmViewPic, 0, sizeof(CurrFrmViewPic));
            for (int i = 0; i <= MAX_DPB_SIZE; i++)
            {
                dpb[i].inter_view_flag = 0; // Reset inter view flags
            }
        }
        iCur = MAX_DPB_SIZE;
        // initialize DPB frame buffer
        cur = &dpb[iCur];
        if (cur->state != 0)
        {
            output_picture(iCur, dpb[iCur].state);
        }
        if (cur->pPicBuf)
        {
            cur->pPicBuf->Release();
            cur->pPicBuf = NULL;
        }
        cur->state = 0;
        cur->top_needed_for_output = cur->bottom_needed_for_output = false;
        cur->top_field_marking = cur->bottom_field_marking = MARKING_UNUSED;
        cur->reference_picture = (slh->nal_ref_idc != 0);
        cur->complementary_field_pair = false;
        cur->not_existing = false;
        cur->FrameNum = slh->frame_num;
        // uint32_t width = (m_sps->pic_width_in_mbs_minus1 + 1) * 16;
        // uint32_t frameHeightInMbs = (2 - m_sps->flags.frame_mbs_only_flag) * (m_sps->pic_height_in_map_units_minus1 + 1);
        // uint32_t height = frameHeightInMbs * 16;
        cur->pPicBuf = alloc_picture();
        if (!cur->pPicBuf)
        {
            nvParserLog("%s : Failed to allocate buffer for current picture\n", __FUNCTION__);
        }
        cur->view_id = slh->view_id;
        cur->VOIdx = get_view_output_index(slh->view_id);
        CurrFrmViewPic[cur->VOIdx] = cur->pPicBuf;
        cur->inter_view_flag = m_nhe.mvc.inter_view_flag;
    }

    picture_order_count(m_sps, &m_slh);
    picture_numbers(&m_slh, 1 << (m_sps->log2_max_frame_num_minus4 + 4)); // (7-1)
    
    picture_started = true;

    // WAR for SPS matrix changes at non-idr boundaries (use matrix from most recent SPS)
    if (((uint32_t)pps->seq_parameter_set_id < MAX_NUM_SPS) && (m_spss[pps->seq_parameter_set_id])) {
        // seq_scale = &m_spss[pps->seq_parameter_set_id]->seqScalingList;
    } else {
        // seq_scale = &m_sps->seqScalingList;
    }

    if (pps->num_slice_groups_minus1)
    {
        // slice_group_map_s is not ssupported with this version of the parser
    }
}


// per picture processing after decoding last slice
void VulkanH264Decoder::dpb_picture_end()
{
    slice_header_s * const slh = &m_slh;
    int i;

    m_PrevViewId = slh->view_id;

    if (!picture_started)
        return;
    picture_started = false;

    if (slh->nal_ref_idc != 0) // reference picture
        decoded_reference_picture_marking(slh, m_sps->max_num_ref_frames);

    // C.4.4 Removal of pictures from the DPB before possible insertion of the current picture
    if (slh->nal_unit_type == 5) // IDR picture
    {
        if (slh->no_output_of_prior_pics_flag)
        {
            // note: *_field_marking has already been set to unused in dec ref pic marking
            for (i = 0; i < MAX_DPB_SIZE; i++)
            {
                dpb[i].state = 0; // empty
            }
        }
    }

    // empty frame buffers marked as "not needed for output" and "unused for reference"
    for (i = 0; i < MAX_DPB_SIZE; i++)
    {
        if ((!(dpb[i].state & 1) || (!dpb[i].top_needed_for_output    && dpb[i].top_field_marking    == MARKING_UNUSED))
         && (!(dpb[i].state & 2) || (!dpb[i].bottom_needed_for_output && dpb[i].bottom_field_marking == MARKING_UNUSED)))
        {
            dpb[i].state = 0; // empty
            if (dpb[i].pPicBuf)
            {
                dpb[i].pPicBuf->Release();
                dpb[i].pPicBuf = NULL;
            }
        }
    }

    if ((slh->nal_unit_type == 5 && !slh->no_output_of_prior_pics_flag) || slh->mmco5)
    {
        while (!dpb_empty())
            dpb_bumping();
    }

    // C.4.5
    if ((slh->nal_ref_idc != 0) // reference picture
        || (m_nhe.mvc.inter_view_flag)) // inter-view reference
    {
        // C.4.5.1
        if (cur->state == 0)
        {
            while (dpb_full())
                dpb_bumping();
            // find an empty DPB entry, copy current to it
            for (iCur=0; iCur<MAX_DPB_SIZE; iCur++)
            {
                if (dpb[iCur].state == 0)
                    break;
            }
            if (iCur >= MAX_DPB_SIZE)
            {
                nvParserLog("could not allocate a frame buffer\n");
            }
            if (cur != &dpb[iCur])
            {
                if (dpb[iCur].pPicBuf)
                {
                    dpb[iCur].pPicBuf->Release();
                    dpb[iCur].pPicBuf = NULL;
                }
                dpb[iCur] = *cur;
                if (dpb[iCur].pPicBuf)
                {
                    dpb[iCur].pPicBuf->AddRef();
                }
            }
            cur = &dpb[iCur];
        }
        if (!slh->field_pic_flag || !slh->bottom_field_flag)
        {
            cur->state |= 1;
            cur->top_needed_for_output = true;
        }
        if (!slh->field_pic_flag || slh->bottom_field_flag)
        {
            cur->state |= 2;
            cur->bottom_needed_for_output = true;
        }
    }
    else
    {
        // C.4.5.2
        if (cur->state != 0)
        {
            // second field of a complementary non-reference field pair
            if (iCur >= MAX_DPB_SIZE)
            {
                // output immediately
                output_picture(iCur, 3);
                dpb[iCur].top_needed_for_output = 0;
                dpb[iCur].bottom_needed_for_output = 0;
                cur->state = 0;
                if (dpb[iCur].pPicBuf)
                {
                    dpb[iCur].pPicBuf->Release();
                    dpb[iCur].pPicBuf = NULL;
                }
            }
            else
            {
                cur->state = 3;
                cur->top_needed_for_output = true;
                cur->bottom_needed_for_output = true;
            }
        }
        else
        {
            for (;;)
            {
                if (dpb_full())
                {
                    // does current have the lowest value of PicOrderCnt?
                    for (i=0; i<MAX_DPB_SIZE; i++)
                    {
                        if (((dpb[i].state & 1) &&
                             dpb[i].top_needed_for_output &&
                             dpb[i].TopFieldOrderCnt <= cur->PicOrderCnt) ||
                            ((dpb[i].state & 2) &&
                             dpb[i].bottom_needed_for_output &&
                             dpb[i].BottomFieldOrderCnt <= cur->PicOrderCnt))
                            break;
                    }
                    if (i < MAX_DPB_SIZE)
                    {
                        dpb_bumping();
                    }
                    else
                    {
                        // DPB is full, current has lowest value of PicOrderCnt
                        if (!slh->field_pic_flag)
                        {
                            // frame: output current picture immediately
                            output_picture(iCur, 3);
                            // This frame buffer is now available: free it right away
                            cur->top_needed_for_output = cur->bottom_needed_for_output = false; // redundant
                            cur->top_field_marking = cur->bottom_field_marking = MARKING_UNUSED; // redundant
                            cur->state = 0;
                            if (dpb[iCur].pPicBuf)
                            {
                                dpb[iCur].pPicBuf->Release();
                                dpb[iCur].pPicBuf = NULL;
                            }
                        }
                        else
                        {
                            // field: wait for second field
                            if (!slh->bottom_field_flag)
                            {
                                cur->state |= 1;
                                cur->top_needed_for_output = true;
                            }
                            else
                            {
                                cur->state |= 2;
                                cur->bottom_needed_for_output = true;
                            }
                        }
                        break; // exit while (1)
                    }
                }
                else
                {
                    // find an empty DPB entry, copy current to it
                    for (iCur=0; iCur<MAX_DPB_SIZE; iCur++)
                    {
                        if (dpb[iCur].state == 0)
                            break;
                    }
                    if (iCur >= MAX_DPB_SIZE)
                    {
                        nvParserLog("could not allocate a frame buffer\n");
                    }
                    if (cur != &dpb[iCur])
                    {
                        if (dpb[iCur].pPicBuf)
                        {
                            dpb[iCur].pPicBuf->Release();
                            dpb[iCur].pPicBuf = NULL;
                        }
                        dpb[iCur] = *cur;
                        if (dpb[iCur].pPicBuf)
                        {
                            dpb[iCur].pPicBuf->AddRef();
                        }
                    }
                    cur = &dpb[iCur];
                    // store current picture
                    if (!slh->field_pic_flag || !slh->bottom_field_flag)
                    {
                        cur->state |= 1;
                        cur->top_needed_for_output = true;
                    }
                    if (!slh->field_pic_flag || slh->bottom_field_flag)
                    {
                        cur->state |= 2;
                        cur->bottom_needed_for_output = true;
                    }
                    break; // exit while (1)
                }
            }
        }
    }
    
    // Limit decode->display latency according to max_num_reorder_frames (no optimizations for MVC/SVC to keep things simple)
    if (!m_bUseMVC && !m_bUseSVC && (m_sps->vui.max_num_reorder_frames < MAX_DPB_SIZE))
    {
        // NOTE: Assuming that display_bumping will only output full frames (no optimizations for unpaired fields)
        if (dpb_reordering_delay() > m_sps->vui.max_num_reorder_frames)
        {
            display_bumping();
        }
    }
}


// 8.2.1
void VulkanH264Decoder::picture_order_count(const seq_parameter_set_s *sps, const slice_header_s *slh)
{
    switch(sps->pic_order_cnt_type)
    {
    case STD_VIDEO_H264_POC_TYPE_0:
        picture_order_count_type_0(sps, slh);
        break;
    case STD_VIDEO_H264_POC_TYPE_1:
        picture_order_count_type_1(sps, slh);
        break;
    case STD_VIDEO_H264_POC_TYPE_2:
        picture_order_count_type_2(sps, slh);
        break;
    default:
        assert(!"Unsupported sps->pic_order_cnt_type type");

    }
    // (8-1)
    if (!slh->field_pic_flag || cur->complementary_field_pair)
        cur->PicOrderCnt = Min(cur->TopFieldOrderCnt, cur->BottomFieldOrderCnt);
    else if (!slh->bottom_field_flag)
        cur->PicOrderCnt = cur->TopFieldOrderCnt;
    else
        cur->PicOrderCnt = cur->BottomFieldOrderCnt;
}

// 8.2.1.1
void VulkanH264Decoder::picture_order_count_type_0(const seq_parameter_set_s *sps, const slice_header_s *slh)
{
    if (slh->nal_unit_type == 5) // IDR picture
    {
        prevPicOrderCntMsb = 0;
        prevPicOrderCntLsb = 0;
    }

    int MaxPicOrderCntLsb = 1 << (sps->log2_max_pic_order_cnt_lsb_minus4 + 4); // (7-2)

    // (8-3)
    int PicOrderCntMsb = 0;
    if ((slh->pic_order_cnt_lsb < prevPicOrderCntLsb) && ((prevPicOrderCntLsb - slh->pic_order_cnt_lsb) >= (MaxPicOrderCntLsb / 2)))
        PicOrderCntMsb = prevPicOrderCntMsb + MaxPicOrderCntLsb;
    else if ((slh->pic_order_cnt_lsb > prevPicOrderCntLsb) && ((slh->pic_order_cnt_lsb - prevPicOrderCntLsb) > (MaxPicOrderCntLsb / 2)))
        PicOrderCntMsb = prevPicOrderCntMsb - MaxPicOrderCntLsb;
    else
        PicOrderCntMsb = prevPicOrderCntMsb;

    // (8-4)
    if (!slh->field_pic_flag || !slh->bottom_field_flag)
        cur->TopFieldOrderCnt = PicOrderCntMsb + slh->pic_order_cnt_lsb;
    // (8-5)
    if (!slh->field_pic_flag)
        cur->BottomFieldOrderCnt = cur->TopFieldOrderCnt + slh->delta_pic_order_cnt_bottom;
    else if (slh->bottom_field_flag)
        cur->BottomFieldOrderCnt = PicOrderCntMsb + slh->pic_order_cnt_lsb;

    if (slh->mmco5)
    {
        prevPicOrderCntMsb = 0;
        if (!slh->field_pic_flag)
        {
            // set to TopFieldOrderCount after having been reset by mmco 5
            int tempPicOrderCnt = Min(cur->TopFieldOrderCnt, cur->BottomFieldOrderCnt);
            prevPicOrderCntLsb = cur->TopFieldOrderCnt - tempPicOrderCnt;
        }
        else
        {
            // note that for a top field TopFieldOrderCnt is 0 after mmco 5
            // therefore we don't have to distinguish between top and bottom fields here
            prevPicOrderCntLsb = 0;
        }
    }
    else if (slh->nal_ref_idc != 0) // reference picture
    {
        prevPicOrderCntMsb = PicOrderCntMsb;
        prevPicOrderCntLsb = slh->pic_order_cnt_lsb;
    }
}

// 8.2.1.2
void VulkanH264Decoder::picture_order_count_type_1(const seq_parameter_set_s *sps, const slice_header_s *slh)
{
    int MaxFrameNum, FrameNumOffset, absFrameNum;
    int expectedPicOrderCnt;
    int i;

    MaxFrameNum = 1 << (sps->log2_max_frame_num_minus4 + 4); // (7-1)

    // FrameNumOffset (8-6)
    if (slh->IdrPicFlag) // (slh->nal_unit_type == 5)
        FrameNumOffset = 0;
    else if (prevFrameNum > slh->frame_num)
        FrameNumOffset = prevFrameNumOffset + MaxFrameNum;
    else
        FrameNumOffset = prevFrameNumOffset;

    // absFrameNum (8-7)
    if (sps->num_ref_frames_in_pic_order_cnt_cycle > 0)
        absFrameNum = FrameNumOffset + slh->frame_num;
    else
        absFrameNum = 0;
    if (slh->nal_ref_idc == 0 && absFrameNum > 0)
        absFrameNum = absFrameNum - 1;

    // picOrderCntCycleCnt, frameNumInPicOrderCntCycle (8-8)
    if (absFrameNum > 0)
    {
        int picOrderCntCycleCnt = (absFrameNum - 1) / sps->num_ref_frames_in_pic_order_cnt_cycle;
        int frameNumInPicOrderCntCycle = (absFrameNum - 1) % sps->num_ref_frames_in_pic_order_cnt_cycle;
        // expectedDeltaPerPicOrderCntCycle (8-9)
        int expectedDeltaPerPicOrderCntCycle = 0;
        for (i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle;  i++) {
            expectedDeltaPerPicOrderCntCycle += sps->offset_for_ref_frame[i];
        }
        // expectedPicOrderCnt (8-10)
        expectedPicOrderCnt = picOrderCntCycleCnt * expectedDeltaPerPicOrderCntCycle;
        for (i = 0; i <= frameNumInPicOrderCntCycle; i++) {
            expectedPicOrderCnt = expectedPicOrderCnt + sps->offset_for_ref_frame[i];
        }
    }
    else
    {
        expectedPicOrderCnt = 0;
    }

    if (slh->nal_ref_idc == 0)
        expectedPicOrderCnt += sps->offset_for_non_ref_pic;

    // TopFieldOrderCnt, BottomFieldOrderCnt (8-11)
    if (!slh->field_pic_flag)
    {
        cur->TopFieldOrderCnt = expectedPicOrderCnt + slh->delta_pic_order_cnt[0];
        cur->BottomFieldOrderCnt = cur->TopFieldOrderCnt + sps->offset_for_top_to_bottom_field + slh->delta_pic_order_cnt[1];
    }
    else if (!slh->bottom_field_flag)
        cur->TopFieldOrderCnt = expectedPicOrderCnt + slh->delta_pic_order_cnt[0];
    else
        cur->BottomFieldOrderCnt = expectedPicOrderCnt + sps->offset_for_top_to_bottom_field + slh->delta_pic_order_cnt[0];

    if (slh->mmco5)
    {
        prevFrameNumOffset = 0;
        prevFrameNum = 0;
    }
    else
    {
        prevFrameNumOffset = FrameNumOffset;
        prevFrameNum = slh->frame_num;
    }
}

// 8.2.1.3
void VulkanH264Decoder::picture_order_count_type_2(const seq_parameter_set_s *sps, const slice_header_s *slh)
{
    int MaxFrameNum, FrameNumOffset, tempPicOrderCnt;

    MaxFrameNum = 1 << (sps->log2_max_frame_num_minus4 + 4); // (7-1)

    // FrameNumOffset (8-12)
    if (slh->IdrPicFlag) // (slh->nal_unit_type == 5)
        FrameNumOffset = 0;
    else if (prevFrameNum > slh->frame_num)
        FrameNumOffset = prevFrameNumOffset + MaxFrameNum;
    else
        FrameNumOffset = prevFrameNumOffset;

    // tempPicOrderCnt (8-13)
    if (slh->IdrPicFlag) // (slh->nal_unit_type == 5)
        tempPicOrderCnt = 0;
    else if (slh->nal_ref_idc == 0)
        tempPicOrderCnt = 2 * (FrameNumOffset + slh->frame_num) - 1;
    else
        tempPicOrderCnt = 2 * (FrameNumOffset + slh->frame_num);

    // TopFieldOrderCnt, BottomFieldOrderCnt (8-14)
    if (!slh->field_pic_flag)
    {
        cur->TopFieldOrderCnt = tempPicOrderCnt;
        cur->BottomFieldOrderCnt = tempPicOrderCnt;
    }
    else if (slh->bottom_field_flag)
        cur->BottomFieldOrderCnt = tempPicOrderCnt;
    else
        cur->TopFieldOrderCnt = tempPicOrderCnt;
    
    if (slh->mmco5)
    {
        prevFrameNumOffset = 0;
        prevFrameNum = 0;
    }
    else
    {
        prevFrameNumOffset = FrameNumOffset;
        prevFrameNum = slh->frame_num;
    }
}

// G.8.2.1 SVC decoding process for picture order count
// 8.2.1 Decoding process for picture order count
void VulkanH264Decoder::picture_order_count_SVC(dependency_data_s *dd, dependency_state_s *ds)
{
    switch (dd->sps->pic_order_cnt_type)
    {
    case STD_VIDEO_H264_POC_TYPE_0:
        picture_order_count_type_0_SVC(dd, ds);
        break;
    case STD_VIDEO_H264_POC_TYPE_1:
        picture_order_count_type_1_SVC(dd, ds);
        break;
    case STD_VIDEO_H264_POC_TYPE_2:
        picture_order_count_type_2_SVC(dd, ds);
        break;
    default:
        assert(!"Unsupported sps->pic_order_cnt_type type");
    }
    // (8-1)
    if (!dd->slh.field_pic_flag || ds->dpb_entry[16].complementary_field_pair)
        ds->dpb_entry[16].PicOrderCnt = Min(ds->dpb_entry[16].TopFieldOrderCnt, ds->dpb_entry[16].BottomFieldOrderCnt);
    else if (!dd->slh.bottom_field_flag)
        ds->dpb_entry[16].PicOrderCnt = ds->dpb_entry[16].TopFieldOrderCnt;
    else
        ds->dpb_entry[16].PicOrderCnt = ds->dpb_entry[16].BottomFieldOrderCnt;
}

// 8.2.1.1
void VulkanH264Decoder::picture_order_count_type_0_SVC(dependency_data_s *dd, dependency_state_s *ds)
{
    int MaxPicOrderCntLsb, PicOrderCntMsb, tempPicOrderCnt;

    if (dd->slh.IdrPicFlag) // IDR picture
    {
        ds->prevPicOrderCntMsb = 0;
        ds->prevPicOrderCntLsb = 0;
    }

    MaxPicOrderCntLsb = 1 << (dd->sps->log2_max_pic_order_cnt_lsb_minus4 + 4); // (7-2)

    // (8-3)
    if ((dd->slh.pic_order_cnt_lsb < ds->prevPicOrderCntLsb) && ((ds->prevPicOrderCntLsb - dd->slh.pic_order_cnt_lsb) >= (MaxPicOrderCntLsb / 2)))
        PicOrderCntMsb = ds->prevPicOrderCntMsb + MaxPicOrderCntLsb;
    else if ((dd->slh.pic_order_cnt_lsb > ds->prevPicOrderCntLsb) && ((dd->slh.pic_order_cnt_lsb - ds->prevPicOrderCntLsb) > (MaxPicOrderCntLsb / 2)))
        PicOrderCntMsb = ds->prevPicOrderCntMsb - MaxPicOrderCntLsb;
    else
        PicOrderCntMsb = ds->prevPicOrderCntMsb;

    // (8-4)
    if (!dd->slh.field_pic_flag || !dd->slh.bottom_field_flag)
        ds->dpb_entry[16].TopFieldOrderCnt = PicOrderCntMsb + dd->slh.pic_order_cnt_lsb;
    // (8-5)
    if (!dd->slh.field_pic_flag)
        ds->dpb_entry[16].BottomFieldOrderCnt = ds->dpb_entry[16].TopFieldOrderCnt + dd->slh.delta_pic_order_cnt_bottom;
    else if (dd->slh.bottom_field_flag)
        ds->dpb_entry[16].BottomFieldOrderCnt = PicOrderCntMsb + dd->slh.pic_order_cnt_lsb;

    if (dd->slh.mmco5)
    {
        ds->prevPicOrderCntMsb = 0;
        // set to TopFieldOrderCount after having been reset by mmco 5
        tempPicOrderCnt = Min(ds->dpb_entry[16].TopFieldOrderCnt, ds->dpb_entry[16].BottomFieldOrderCnt);
        ds->prevPicOrderCntLsb = ds->dpb_entry[16].TopFieldOrderCnt - tempPicOrderCnt;
    }
    else if (dd->slh.nal_ref_idc != 0) // reference picture
    {
        ds->prevPicOrderCntMsb = PicOrderCntMsb;
        ds->prevPicOrderCntLsb = dd->slh.pic_order_cnt_lsb;
    }
}

// 8.2.1.2
void VulkanH264Decoder::picture_order_count_type_1_SVC(dependency_data_s *dd, dependency_state_s *ds)
{
    int MaxFrameNum, FrameNumOffset, absFrameNum, picOrderCntCycleCnt = 0;
    int frameNumInPicOrderCntCycle = 0, expectedDeltaPerPicOrderCntCycle = 0, expectedPicOrderCnt = 0;
    int i;

    MaxFrameNum = 1 << (dd->sps->log2_max_frame_num_minus4 + 4); // (7-1)

    // FrameNumOffset (8-6)
    if (dd->slh.IdrPicFlag)
        FrameNumOffset = 0;
    else if (ds->prevFrameNum > dd->slh.frame_num)
        FrameNumOffset = ds->prevFrameNumOffset + MaxFrameNum;
    else
        FrameNumOffset = ds->prevFrameNumOffset;

    // absFrameNum (8-7)
    if (dd->sps->num_ref_frames_in_pic_order_cnt_cycle != 0)
        absFrameNum = FrameNumOffset + dd->slh.frame_num;
    else
        absFrameNum = 0;
    if (dd->slh.nal_ref_idc == 0 && absFrameNum > 0)
        absFrameNum = absFrameNum - 1;

    // picOrderCntCycleCnt, frameNumInPicOrderCntCycle (8-8)
    if (absFrameNum > 0)
    {
        picOrderCntCycleCnt = (absFrameNum - 1) / dd->sps->num_ref_frames_in_pic_order_cnt_cycle;
        frameNumInPicOrderCntCycle = (absFrameNum - 1) % dd->sps->num_ref_frames_in_pic_order_cnt_cycle;
    }

    // expectedDeltaPerPicOrderCntCycle (8-9)
    expectedDeltaPerPicOrderCntCycle = 0;
    for (i = 0; i < dd->sps->num_ref_frames_in_pic_order_cnt_cycle;  i++) {
        expectedDeltaPerPicOrderCntCycle += dd->sps->offset_for_ref_frame[i];
    }
    // expectedPicOrderCnt (8-10)
    if (absFrameNum > 0)
    {
        expectedPicOrderCnt = picOrderCntCycleCnt * expectedDeltaPerPicOrderCntCycle;
        for (i = 0; i <= frameNumInPicOrderCntCycle; i++) {
            expectedPicOrderCnt = expectedPicOrderCnt + dd->sps->offset_for_ref_frame[i];
        }
    }
    else
    {
        expectedPicOrderCnt = 0;
    }

    if (dd->slh.nal_ref_idc == 0)
        expectedPicOrderCnt = expectedPicOrderCnt + dd->sps->offset_for_non_ref_pic;

    // TopFieldOrderCnt, BottomFieldOrderCnt (8-11)
    if (!dd->slh.field_pic_flag)
    {
        ds->dpb_entry[16].TopFieldOrderCnt = expectedPicOrderCnt + dd->slh.delta_pic_order_cnt[0];
        ds->dpb_entry[16].BottomFieldOrderCnt = ds->dpb_entry[16].TopFieldOrderCnt + dd->sps->offset_for_top_to_bottom_field + dd->slh.delta_pic_order_cnt[1];
    }
    else if (!dd->slh.bottom_field_flag)
        ds->dpb_entry[16].TopFieldOrderCnt = expectedPicOrderCnt + dd->slh.delta_pic_order_cnt[0];
    else
        ds->dpb_entry[16].BottomFieldOrderCnt = expectedPicOrderCnt + dd->sps->offset_for_top_to_bottom_field + dd->slh.delta_pic_order_cnt[0];

    if (dd->slh.mmco5)
    {
        ds->prevFrameNumOffset = 0;
        ds->prevFrameNum = 0;
    }
    else
    {
        ds->prevFrameNumOffset = FrameNumOffset;
        ds->prevFrameNum = dd->slh.frame_num;
    }
}

// 8.2.1.3
void VulkanH264Decoder::picture_order_count_type_2_SVC(dependency_data_s *dd, dependency_state_s *ds)
{
    int MaxFrameNum, FrameNumOffset, tempPicOrderCnt;

    MaxFrameNum = 1 << (dd->sps->log2_max_frame_num_minus4 + 4); // (7-1)

    // FrameNumOffset (8-12)
    if (dd->slh.IdrPicFlag)
        FrameNumOffset = 0;
    else if (ds->prevFrameNum > dd->slh.frame_num)
        FrameNumOffset = ds->prevFrameNumOffset + MaxFrameNum;
    else
        FrameNumOffset = ds->prevFrameNumOffset;

    // tempPicOrderCnt (8-13)
    if (dd->slh.IdrPicFlag)
        tempPicOrderCnt = 0;
    else if (dd->slh.nal_ref_idc == 0)
        tempPicOrderCnt = 2 * (FrameNumOffset + dd->slh.frame_num) - 1;
    else
        tempPicOrderCnt = 2 * (FrameNumOffset + dd->slh.frame_num);

    // TopFieldOrderCnt, BottomFieldOrderCnt (8-14)
    if (!dd->slh.field_pic_flag)
    {
        ds->dpb_entry[16].TopFieldOrderCnt = tempPicOrderCnt;
        ds->dpb_entry[16].BottomFieldOrderCnt = tempPicOrderCnt;
    }
    else if (dd->slh.bottom_field_flag)
        ds->dpb_entry[16].BottomFieldOrderCnt = tempPicOrderCnt;
    else
        ds->dpb_entry[16].TopFieldOrderCnt = tempPicOrderCnt;
    
    if (dd->slh.mmco5)
    {
        ds->prevFrameNumOffset = 0;
        ds->prevFrameNum = 0;
    }
    else
    {
        ds->prevFrameNumOffset = FrameNumOffset;
        ds->prevFrameNum = dd->slh.frame_num;
    }
}

// 8.2.4.1  Decoding process for picture numbers
void VulkanH264Decoder::picture_numbers(const slice_header_s *slh, int MaxFrameNum)
{
    int i;

    for (i=0; i<MAX_DPB_SIZE; i++)
    {
        // (8-28)
        if (dpb[i].FrameNum > slh->frame_num)
            dpb[i].FrameNumWrap = dpb[i].FrameNum - MaxFrameNum;
        else
            dpb[i].FrameNumWrap = dpb[i].FrameNum;
        if (!slh->field_pic_flag)
        {
            // frame
            dpb[i].TopPicNum = dpb[i].BottomPicNum = dpb[i].FrameNumWrap; // (8-29)
            dpb[i].TopLongTermPicNum = dpb[i].BottomLongTermPicNum = dpb[i].LongTermFrameIdx; // (8-30)
        }
        else if (!slh->bottom_field_flag)
        {
            // top field
            dpb[i].TopPicNum            = 2 * dpb[i].FrameNumWrap + 1;     // same parity (8-31)
            dpb[i].BottomPicNum         = 2 * dpb[i].FrameNumWrap;         // opposite parity (8-32)
            dpb[i].TopLongTermPicNum    = 2 * dpb[i].LongTermFrameIdx + 1; // same parity (8-33)
            dpb[i].BottomLongTermPicNum = 2 * dpb[i].LongTermFrameIdx;     // opposite parity (8-34)
        }
        else
        {
            // bottom field
            dpb[i].TopPicNum            = 2 * dpb[i].FrameNumWrap;         // opposite parity (8-32)
            dpb[i].BottomPicNum         = 2 * dpb[i].FrameNumWrap + 1;     // same parity (8-31)
            dpb[i].TopLongTermPicNum    = 2 * dpb[i].LongTermFrameIdx;     // opposite parity (8-34)
            dpb[i].BottomLongTermPicNum = 2 * dpb[i].LongTermFrameIdx + 1; // same parity (8-33)
        }
    }
}

// G.8.2.2
void VulkanH264Decoder::picture_numbers_SVC(dependency_data_s *dd, dependency_state_s *ds)
{
    int MaxFrameNum = 1 << (dd->sps->log2_max_frame_num_minus4 + 4);

    for (int k=0; k<MAX_DPB_SVC_SIZE; k++)
    {
        if (ds->dpb_entry[k].ref == MARKING_SHORT)
        {
            if (ds->dpb_entry[k].FrameNum > dd->slh.frame_num)
                ds->dpb_entry[k].FrameNumWrap = ds->dpb_entry[k].FrameNum - MaxFrameNum;
            else
                ds->dpb_entry[k].FrameNumWrap = ds->dpb_entry[k].FrameNum;

            ds->dpb_entry[k].PicNum = ds->dpb_entry[k].FrameNumWrap;
        }
        else if (ds->dpb_entry[k].ref == MARKING_LONG)
        {
            ds->dpb_entry[k].LongTermPicNum = ds->dpb_entry[k].LongTermFrameIdx;
        }
    }
}

// G.8.2.4.1 SVC reference picture marking process for a dependency representation
void VulkanH264Decoder::decoded_reference_picture_marking_SVC(dependency_data_s *dd, dependency_state_s *ds)
{
    if (dd->slh.IdrPicFlag)
    {
        // mark all reference pictures as "unused for reference"
        for (int k = 0; k < MAX_DPB_SIZE; k++)
            ds->dpb_entry[k].ref = MARKING_UNUSED;

        if (!dd->slh.long_term_reference_flag)
        {
            ds->dpb_entry[16].ref = MARKING_SHORT; // short-term
            ds->MaxLongTermFrameIdx = -1;
        }
        else
        {
            ds->dpb_entry[16].ref = MARKING_LONG; // long-term
            ds->dpb_entry[16].LongTermFrameIdx = 0;
            ds->MaxLongTermFrameIdx = 0;
        }
    }
    else // current picture is not an IDR picture
    {
        picture_numbers_SVC(dd, ds);
        if (dd->slh.adaptive_ref_base_pic_marking_mode_flag)
            adaptive_ref_base_pic_marking(dd, ds);
        if (dd->slh.adaptive_ref_pic_marking_mode_flag) {
            adaptive_ref_pic_marking(dd, ds);
        } else {
            sliding_window_ref_pic_marking(dd, ds);
        }
        if (ds->dpb_entry[16].ref != MARKING_LONG)
        {
            ds->dpb_entry[16].ref = MARKING_SHORT;
            if (dd->slh.store_ref_base_pic_flag && !dd->slh.adaptive_ref_base_pic_marking_mode_flag)
            {
                picture_numbers_SVC(dd, ds);
                sliding_window_ref_pic_marking(dd, ds);
            }
        }
    }
}

// G.8.2.4.2 SVC sliding window decoded reference picture marking process
void VulkanH264Decoder::sliding_window_ref_pic_marking(dependency_data_s *dd, dependency_state_s *ds)
{
    int n, kmin = 0, imin = 0, minFrameNumWrap, maxLongTermFrameIdx;

    n = 0;
    minFrameNumWrap = 65536;
    maxLongTermFrameIdx = -1;
    for (int k=0; k< MAX_DPB_SVC_SIZE; k++)
    {
        if (ds->dpb_entry[k].ref != MARKING_UNUSED)
            n++;
        if (ds->dpb_entry[k].ref == MARKING_SHORT)
        {
            if ( (ds->dpb_entry[k].FrameNumWrap <  minFrameNumWrap) ||
                ((ds->dpb_entry[k].FrameNumWrap == minFrameNumWrap) && ds->dpb_entry[k].base))
            {
                kmin = k;
                minFrameNumWrap = ds->dpb_entry[k].FrameNumWrap;
            }
        }
        else if (ds->dpb_entry[k].ref == MARKING_LONG)
        {
            if (ds->dpb_entry[k].LongTermFrameIdx > maxLongTermFrameIdx)
            {
                imin = k;
                maxLongTermFrameIdx = ds->dpb_entry[k].LongTermFrameIdx;
            }
        }
    }

    if (n >= Max(dd->sps->max_num_ref_frames, 1))
    {
        if (minFrameNumWrap != 65536)
        {
            ds->dpb_entry[kmin].ref = MARKING_UNUSED;
        }
        else
        {
            // all ref frames are long-term (not allowed)
            // remove long-term with largest LongTermFrameIdx (arbitrary choice)
            if (maxLongTermFrameIdx != -1) // should always be true
                ds->dpb_entry[imin].ref = MARKING_UNUSED;
        }
    }
}

// G.8.2.4.3 SVC adaptive memory control reference base picture marking process
void VulkanH264Decoder::adaptive_ref_base_pic_marking(dependency_data_s *dd, dependency_state_s *ds)
{
    int CurrPicNum, i, picNumX;

    CurrPicNum = dd->slh.frame_num;

    for (i = 0; i < MAX_MMCOS; i++)
    {
        switch (dd->slh.mmbco[i].memory_management_base_control_operation)
        {
        case 1: // mark a short-term reference base picture as "unused for reference"
            picNumX = CurrPicNum - (dd->slh.mmbco[i].difference_of_base_pic_nums_minus1 + 1);
            for (int k = 0; k < MAX_DPB_SIZE; k++)
            {
                if (ds->dpb_entry[k].base && ds->dpb_entry[k].ref == MARKING_SHORT && ds->dpb_entry[k].PicNum == picNumX)
                    ds->dpb_entry[k].ref = MARKING_UNUSED;
            }
            break;

        case 2: // mark a long-term reference base picture as "unused for reference"
            for (int k = 0; k < MAX_DPB_SIZE; k++)
            {
                if (ds->dpb_entry[k].base && ds->dpb_entry[k].ref == MARKING_LONG && ds->dpb_entry[k].LongTermPicNum == dd->slh.mmbco[i].long_term_base_pic_num)
                    ds->dpb_entry[k].ref = MARKING_UNUSED;
            }
            break;

        default:
            return;
        }
    }
}

// G.8.2.4.4 SVC adaptive memory control decoded reference picture marking process
void VulkanH264Decoder::adaptive_ref_pic_marking(dependency_data_s *dd, dependency_state_s *ds)
{
    int CurrPicNum, i, picNumX;

    CurrPicNum = dd->slh.frame_num;

    for (i = 0; i < MAX_MMCOS; i++)
    {
        switch (dd->slh.mmco[i].memory_management_control_operation)
        {
        case 1: // 8.2.5.4.1 Marking process of a short-term reference picture as "unused for reference"
            picNumX = CurrPicNum - (dd->slh.mmco[i].difference_of_pic_nums_minus1 + 1);
            for (int k = 0; k < MAX_DPB_SIZE; k++)
            {
                if (!ds->dpb_entry[k].base && ds->dpb_entry[k].ref == MARKING_SHORT && ds->dpb_entry[k].PicNum == picNumX)
                    ds->dpb_entry[k].ref = MARKING_UNUSED;
            }
            break;

        case 2: // 8.2.5.4.2 Marking process of a long-term reference picture as "unused for reference"
            for (int k = 0; k < MAX_DPB_SIZE; k++)
            {
                if (!ds->dpb_entry[k].base && ds->dpb_entry[k].ref == MARKING_LONG && ds->dpb_entry[k].LongTermPicNum == dd->slh.mmco[i].long_term_frame_idx)
                    ds->dpb_entry[k].ref = MARKING_UNUSED;
            }
            break;

        case 3: // 8.2.5.4.3 Assignment process of a LongTermFrameIdx to a short-term reference picture
            picNumX = CurrPicNum - (dd->slh.mmco[i].difference_of_pic_nums_minus1 + 1);
            for (int k = 0; k < MAX_DPB_SIZE; k++)
            {
                if (ds->dpb_entry[k].ref == MARKING_LONG && ds->dpb_entry[k].LongTermFrameIdx == dd->slh.mmco[i].long_term_frame_idx)
                    ds->dpb_entry[k].ref = MARKING_UNUSED;
                if (ds->dpb_entry[k].ref == MARKING_SHORT && ds->dpb_entry[k].PicNum == picNumX)
                {
                    ds->dpb_entry[k].ref = MARKING_LONG;
                    ds->dpb_entry[k].LongTermFrameIdx = dd->slh.mmco[i].long_term_frame_idx;
                }
            }
            break;

        case 4: // 8.2.5.4.4 Decoding process for MaxLongTermFrameIdx
            for (int k = 0; k < MAX_DPB_SIZE; k++)
            {
                if (ds->dpb_entry[k].ref == MARKING_LONG && ds->dpb_entry[k].LongTermFrameIdx > dd->slh.mmco[i].long_term_frame_idx-1)
                    ds->dpb_entry[k].ref = MARKING_UNUSED;
            }
            ds->MaxLongTermFrameIdx = dd->slh.mmco[i].long_term_frame_idx - 1;
            break;

        case 5: // 8.2.5.4.5 Marking process of all reference pictures as "unused for reference" and setting MaxLongTermFrameIdx to "no long-term frame indices"
            for (int k = 0; k < MAX_DPB_SIZE; k++)
                ds->dpb_entry[k].ref = MARKING_UNUSED;
            ds->MaxLongTermFrameIdx = -1;
            break;

        case 6: // 8.2.5.4.6 Process for assigning a long-term frame index to the current picture
            for (int k = 0; k < MAX_DPB_SIZE; k++)
            {
                if (ds->dpb_entry[k].ref == MARKING_LONG && ds->dpb_entry[k].LongTermFrameIdx == dd->slh.mmco[i].long_term_frame_idx)
                    ds->dpb_entry[k].ref = MARKING_UNUSED;
            }
            ds->dpb_entry[16].ref = MARKING_LONG;
            ds->dpb_entry[16].LongTermFrameIdx = dd->slh.mmco[i].long_term_frame_idx;
            break;

        default:
            return;
        }
    }
}

// 8.2.5, 8.2.5.1
void VulkanH264Decoder::decoded_reference_picture_marking(slice_header_s *slh, uint32_t num_ref_frames)
{
    if (slh->IdrPicFlag) // IDR picture
    {
        // All reference pictures shall be marked as "unused for reference"
        for (int i = 0; i < MAX_DPB_SIZE; i++)
        {
            if (dpb[i].view_id == slh->view_id)
            {
                dpb[i].top_field_marking = MARKING_UNUSED;
                dpb[i].bottom_field_marking = MARKING_UNUSED;
            }
        }
        if (slh->long_term_reference_flag == 0)
        {
            // the IDR picture shall be marked as "used for short-term reference"
            if (!slh->field_pic_flag || !slh->bottom_field_flag)
                cur->top_field_marking = MARKING_SHORT;
            if (!slh->field_pic_flag || slh->bottom_field_flag)
                cur->bottom_field_marking = MARKING_SHORT;
            // MaxLongTermFrameIdx shall be set equal to "no long-term frame indices".
            MaxLongTermFrameIdx = -1;
        }
        else // (slh->long_term_reference_flag == 1)
        {
            // the IDR picture shall be marked as "used for long-term reference"
            if (!slh->field_pic_flag || !slh->bottom_field_flag)
                cur->top_field_marking = MARKING_LONG;
            if (!slh->field_pic_flag || slh->bottom_field_flag)
                cur->bottom_field_marking = MARKING_LONG;
            // the LongTermFrameIdx for the IDR picture shall be set equal to 0
            cur->LongTermFrameIdx = 0;
            // MaxLongTermFrameIdx shall be set equal to 0.
            MaxLongTermFrameIdx = 0;
        }
    }
    else
    {
        if (slh->adaptive_ref_pic_marking_mode_flag == 0)
            sliding_window_decoded_reference_picture_marking(num_ref_frames);
        else // (slh->adaptive_ref_pic_marking_mode_flag == 1)
            adaptive_memory_control_decoded_reference_picture_marking(slh, num_ref_frames);

        // mark current as short-term if not marked as long-term (8.2.5.1)
        if ((!slh->field_pic_flag || !slh->bottom_field_flag) && cur->top_field_marking == MARKING_UNUSED)
            cur->top_field_marking = MARKING_SHORT;
        if ((!slh->field_pic_flag || slh->bottom_field_flag) && cur->bottom_field_marking == MARKING_UNUSED)
            cur->bottom_field_marking = MARKING_SHORT;
    }
}

// G.8.2.5 SVC decoding process for gaps in frame_num
void VulkanH264Decoder::gaps_in_frame_num_SVC()
{
    // 7.4.3
    if (m_dd->slh.IdrPicFlag)
        m_ds->PrevRefFrameNum = 0;

    int MaxFrameNum = 1 << (m_dd->sps->log2_max_frame_num_minus4 + 4); // (7-9)
    int UnusedShortTermFrameNum = (m_ds->PrevRefFrameNum + 1) % MaxFrameNum; // (7-23)

    if (m_dd->slh.frame_num != m_ds->PrevRefFrameNum && m_dd->slh.frame_num != UnusedShortTermFrameNum)
    {
        dependency_data_s ddsave = *m_dd;
        // inferred values
        // m_dd->slh.IdrPicFlag = 0 (always)
        m_dd->slh.nal_ref_idc = 1;
        m_dd->slh.delta_pic_order_cnt[0] = 0;
        m_dd->slh.delta_pic_order_cnt[1] = 0;
        m_dd->slh.mmco5 = 0;
        while (UnusedShortTermFrameNum != ddsave.slh.frame_num)
        {
            m_dd->slh.frame_num = UnusedShortTermFrameNum;

            // initialize current picture
            m_ds->dpb_entry[16].base     = false;
            m_ds->dpb_entry[16].FrameNum = m_dd->slh.frame_num;
            m_ds->dpb_entry[16].ref      = 0;
            picture_numbers_SVC(m_dd, m_ds);
            sliding_window_ref_pic_marking(m_dd, m_ds);
            m_ds->dpb_entry[16].ref = 1;
            while (dpb_full_SVC(m_dd, m_ds))
                dpb_bumping_SVC(m_ds);
            if (m_dd->sps->pic_order_cnt_type != 0)
                picture_order_count_SVC(m_dd, m_ds);
            for (int k=0; k<16; k++)
            {
                if (!m_ds->dpb_entry[k].ref && !m_ds->dpb_entry[k].output)
                {
                    if (m_ds->dpb_entry[k].pPicBuf)
                    {
                        m_ds->dpb_entry[k].pPicBuf->Release();
                        m_ds->dpb_entry[k].pPicBuf = NULL;
                    }
                    m_ds->dpb_entry[k] = m_ds->dpb_entry[16];
                    m_ds->dpb_entry[k].output = false;
                    m_ds->dpb_entry[k].non_existing = true;
                    if (m_ds->dpb_entry[k].pPicBuf)
                    {
                        m_ds->dpb_entry[k].pPicBuf->AddRef();
                    }
                    break;
                }
            }

            // 7.4.3
            m_ds->PrevRefFrameNum = UnusedShortTermFrameNum;
            UnusedShortTermFrameNum = (UnusedShortTermFrameNum + 1) % MaxFrameNum;
        }
        *m_dd = ddsave;
    }

    // 7.4.3
    if (m_dd->slh.mmco5)
        m_ds->PrevRefFrameNum = 0;
    else if (m_dd->slh.nal_ref_idc != 0)
        m_ds->PrevRefFrameNum = m_dd->slh.frame_num;
}

void VulkanH264Decoder::dpb_bumping_SVC(dependency_state_s *ds)
{
    // find entry with smallest POC
    int kmin = -1;
    int minPOC = 0;
    for (int k=0; k<MAX_DPB_SIZE; k++)
    {
        if (ds->dpb_entry[k].output)
        {
            int poc = Min(ds->dpb_entry[k].TopFieldOrderCnt, ds->dpb_entry[k].BottomFieldOrderCnt);
            if (kmin < 0 || poc < minPOC)
            {
                minPOC = poc;
                kmin = k;
            }
        }
    }
    if (kmin < 0)
        return;
    output_picture_SVC(ds->dpb_entry[kmin].pPicBuf, 3);
    ds->dpb_entry[kmin].output = false;
    // empty frame buffer
    if (ds->dpb_entry[kmin].ref == MARKING_UNUSED)
    {
        if (ds->dpb_entry[kmin].pPicBuf)
        {
            ds->dpb_entry[kmin].pPicBuf->Release();
            ds->dpb_entry[kmin].pPicBuf = NULL;
        }
    }
}

// 8.2.5.2
void VulkanH264Decoder::gaps_in_frame_num()
{
    const seq_parameter_set_s* sps = m_sps;
    int MaxFrameNum, UnusedShortTermFrameNum;
    int i;
    
    MaxFrameNum = 1 << (sps->log2_max_frame_num_minus4 + 4); // (7-1)

    // 7.4.3
    if (m_slh.IdrPicFlag) // IDR picture
        PrevRefFrameNum = 0;

    assert(m_slh.frame_num < MaxFrameNum);
    if (m_slh.frame_num != PrevRefFrameNum)
    {
        slice_header_s slhtmp = m_slh;
        slice_header_s * const slh = &slhtmp;
        // (7-10)
        UnusedShortTermFrameNum = (PrevRefFrameNum + 1) % MaxFrameNum;
        while (UnusedShortTermFrameNum != m_slh.frame_num)
        {
            int bad_edit = false;
        
            slh->frame_num = UnusedShortTermFrameNum;
            slh->field_pic_flag = 0;
            slh->bottom_field_flag = 0;
            slh->nal_ref_idc = 1;
            slh->nal_unit_type = 1;
            slh->IdrPicFlag = 0;
            slh->adaptive_ref_pic_marking_mode_flag = 0;
            slh->delta_pic_order_cnt[0] =
            slh->delta_pic_order_cnt[1] = 0;
            // WAR for bad editing tools truncating frame_num to 8-bit at edit points (VideoRedo)
            for (i = 0; i < MAX_DPB_SIZE; i++)
            {
                if ((dpb[i].state != 0) && ((dpb[i].FrameNum & 0xff) == (slh->frame_num & 0xff))
                 && (dpb[i].view_id == slh->view_id) && (!dpb[i].not_existing))
                {
                    iCur = i;
                    cur = &dpb[iCur];
                    cur->FrameNum = slh->frame_num;
                    picture_numbers(slh, MaxFrameNum);
                    bad_edit = true;
                    break;
                }
            }
            if ((!bad_edit) && ((sps->flags.gaps_in_frame_num_value_allowed_flag) || (sps->max_num_ref_frames > 1)))
            {
                // DPB handling (C.4.2)
                while (dpb_full())
                    dpb_bumping();
                for (iCur = 0; iCur < MAX_DPB_SIZE; iCur++)
                {
                    if (dpb[iCur].state == 0)
                        break;
                }
                if (iCur < MAX_DPB_SIZE)
                {
                    // initialize DPB frame buffer
                    cur = &dpb[iCur];
                    cur->FrameNum = slh->frame_num;
                    cur->complementary_field_pair = false;
                    if (sps->pic_order_cnt_type != 0) {
                        picture_order_count(sps, slh);
                    }
                    picture_numbers(slh, MaxFrameNum);
                    sliding_window_decoded_reference_picture_marking(sps->max_num_ref_frames);

                    cur->top_field_marking = cur->bottom_field_marking = MARKING_SHORT;
                    cur->reference_picture = true;
                    cur->not_existing = true;
                    // C.4.2
                    cur->top_needed_for_output = cur->bottom_needed_for_output = false;
                    cur->state = 3; // frame
                    // no buffer index
                    if (cur->pPicBuf)
                    {
                        cur->pPicBuf->Release();
                        cur->pPicBuf = NULL;
                    }
                    // empty frame buffers marked as "not needed for output" and "unused for reference"
                    for (i=0; i<MAX_DPB_SIZE; i++)
                    {
                        if ((!(dpb[i].state & 1) || (!dpb[i].top_needed_for_output    && dpb[i].top_field_marking    == MARKING_UNUSED)) &&
                            (!(dpb[i].state & 2) || (!dpb[i].bottom_needed_for_output && dpb[i].bottom_field_marking == MARKING_UNUSED)))
                        {
                            dpb[i].state = 0; // empty
                            if (dpb[i].pPicBuf)
                            {
                                dpb[i].pPicBuf->Release();
                                dpb[i].pPicBuf = NULL;
                            }
                        }
                    }
                }
            }
            // 7.4.3
            PrevRefFrameNum = slh->frame_num;
            UnusedShortTermFrameNum = (UnusedShortTermFrameNum + 1) % MaxFrameNum;
        }
    }

    // 7.4.3
    if (m_slh.mmco5)
        PrevRefFrameNum = 0;
    else if ((m_slh.nal_ref_idc != 0) // reference picture
         // WAR for some encoders where frame_num is also incremented for non-reference pictures
          || (m_slh.frame_num == (PrevRefFrameNum+1) % MaxFrameNum))
        PrevRefFrameNum = m_slh.frame_num;
}


// 8.2.5.3
void VulkanH264Decoder::sliding_window_decoded_reference_picture_marking(uint32_t num_ref_frames)
{
    // If the current picture is a coded field that is the second field in decoding order
    // of a complementary reference field pair, and the first field has been marked as
    // "used for short-term reference", the current picture is also marked as
    // "used for short-term reference".
    if (cur->top_field_marking == MARKING_SHORT || cur->bottom_field_marking == MARKING_SHORT)
    {
        cur->top_field_marking = MARKING_SHORT;
        cur->bottom_field_marking = MARKING_SHORT;
    }
    else
    {
        uint32_t numShortTerm = 0;
        uint32_t numLongTerm = 0;
        uint32_t numShortTermExisting = 0;
        for (uint32_t  i = 0; i < MAX_DPB_SIZE; i++)
        {
            if (dpb[i].view_id == m_nhe.mvc.view_id)
            {
                if ((dpb[i].top_field_marking == MARKING_SHORT || dpb[i].bottom_field_marking == MARKING_SHORT)
                    && (dpb[i].FrameNum == cur->FrameNum))
                {
                    // If we hit this case, the stream is non-conforming (7.4.3, frame_num, constraing (7-23)).
                    // However if it really happens, we'll remove the bad dpb entry now, since it's the oldest.
                    // Otherwise normal sorting later by FrameNumWrap will not be able to remove the oldest.
                    nvParserLog("FrameNum %d already exists in DPB!!\n", cur->FrameNum);
                    if (dpb[i].top_field_marking == MARKING_SHORT)
                        dpb[i].top_field_marking = MARKING_UNUSED;
                    if (dpb[i].bottom_field_marking == MARKING_SHORT)
                        dpb[i].bottom_field_marking = MARKING_UNUSED;
                }

                if (dpb[i].top_field_marking == MARKING_SHORT || dpb[i].bottom_field_marking == MARKING_SHORT)
                {
                    numShortTerm++;
                    numShortTermExisting += (dpb[i].not_existing == false);
                }
                if (dpb[i].top_field_marking == MARKING_LONG || dpb[i].bottom_field_marking == MARKING_LONG)
                {
                    numLongTerm++;
                }
            }
        }

        // assert(numShortTerm + numLongTerm <= sps->num_ref_frames);
        if (numShortTerm + numLongTerm >= num_ref_frames)
        {
            int32_t minFrameNumWrap = 65536;
            uint32_t imin = 0;
            for (uint32_t i = 0; i < MAX_DPB_SIZE; i++)
            {
                if (dpb[i].view_id != m_nhe.mvc.view_id)
                    continue;

                if (numShortTerm > 0)
                {
                    if ((dpb[i].top_field_marking == MARKING_SHORT || dpb[i].bottom_field_marking == MARKING_SHORT) &&
                        dpb[i].FrameNumWrap < minFrameNumWrap)
                    {
                        if ((numShortTermExisting > 1) || (numShortTermExisting == numShortTerm) || 
                            (dpb[i].not_existing) || (m_sps->flags.gaps_in_frame_num_value_allowed_flag))
                        {
                            imin = i;
                            minFrameNumWrap = dpb[i].FrameNumWrap;
                        }
                    }
                } else
                {
                    if ((dpb[i].top_field_marking == MARKING_LONG || dpb[i].bottom_field_marking == MARKING_LONG) &&
                        dpb[i].FrameNumWrap < minFrameNumWrap)
                    {
                        imin = i;
                        minFrameNumWrap = dpb[i].FrameNumWrap;
                    }
                }
            }
            dpb[imin].top_field_marking = MARKING_UNUSED;
            dpb[imin].bottom_field_marking = MARKING_UNUSED;
        }
    }
}


// 8.2.5.4
void VulkanH264Decoder::adaptive_memory_control_decoded_reference_picture_marking(slice_header_s *slh, int num_ref_frames)
{
    int CurrPicNum = (!slh->field_pic_flag) ? slh->frame_num : 2 * slh->frame_num + 1;

    for (int k = 0; k < MAX_MMCOS && slh->mmco[k].memory_management_control_operation!=0; k++)
    {
        switch (slh->mmco[k].memory_management_control_operation)
        {
        case 1:
            {
                // 8.2.5.4.1 Marking process of a short-term picture as "unused for reference"
                int picNumX = CurrPicNum - (slh->mmco[k].difference_of_pic_nums_minus1 + 1); // (8-40)
                for (int i = 0; i < MAX_DPB_SIZE; i++)
                {
                    if(dpb[i].view_id == slh->view_id)
                    {
                        if (dpb[i].top_field_marking == MARKING_SHORT && dpb[i].TopPicNum == picNumX)
                            dpb[i].top_field_marking = MARKING_UNUSED;
                        if (dpb[i].bottom_field_marking == MARKING_SHORT && dpb[i].BottomPicNum == picNumX)
                            dpb[i].bottom_field_marking = MARKING_UNUSED;
                    }
                }
            }
            break;
        case 2:
            // 8.2.5.4.2 Marking process of a long-term picture as "unused for reference"
            for (int i = 0; i < MAX_DPB_SIZE; i++)
            {
                if (dpb[i].view_id == slh->view_id)
                {
                    if (dpb[i].top_field_marking == MARKING_LONG && dpb[i].TopLongTermPicNum == slh->mmco[k].long_term_frame_idx)
                        dpb[i].top_field_marking = MARKING_UNUSED;
                    if (dpb[i].bottom_field_marking == MARKING_LONG && dpb[i].BottomLongTermPicNum == slh->mmco[k].long_term_frame_idx)
                        dpb[i].bottom_field_marking = MARKING_UNUSED;
                }
            }
            break;
        case 3:
            {
                // 8.2.5.4.3 Assignment process of a LongTermFrameIdx to a short-term reference picture
                int picNumX = CurrPicNum - (slh->mmco[k].difference_of_pic_nums_minus1 + 1); // (8-40)
                for (int i = 0; i < MAX_DPB_SIZE; i++)
                {
                    if (dpb[i].view_id != slh->view_id)
                        continue;

                    if (dpb[i].top_field_marking == MARKING_LONG && dpb[i].LongTermFrameIdx == slh->mmco[k].long_term_frame_idx &&
                        !(dpb[i].bottom_field_marking == MARKING_SHORT && dpb[i].BottomPicNum == picNumX))
                        dpb[i].top_field_marking = MARKING_UNUSED;
                    if (dpb[i].bottom_field_marking == MARKING_LONG && dpb[i].LongTermFrameIdx == slh->mmco[k].long_term_frame_idx &&
                        !(dpb[i].top_field_marking == MARKING_SHORT && dpb[i].TopPicNum == picNumX))
                        dpb[i].bottom_field_marking = MARKING_UNUSED;
                    if (dpb[i].top_field_marking == MARKING_SHORT && dpb[i].TopPicNum == picNumX)
                    {
                        dpb[i].top_field_marking = MARKING_LONG;
                        dpb[i].LongTermFrameIdx = slh->mmco[k].long_term_frame_idx;
                        // update TopLongTermPicNum, BottomLongTermPicNum for subsequent mmco 2
                        if (!slh->field_pic_flag)
                        {
                            // frame
                            dpb[i].TopLongTermPicNum = dpb[i].BottomLongTermPicNum = dpb[i].LongTermFrameIdx; // (8-30)
                        }
                        else if (!slh->bottom_field_flag)
                        {
                            // top field
                            dpb[i].TopLongTermPicNum    = 2 * dpb[i].LongTermFrameIdx + 1; // same parity (8-33)
                            dpb[i].BottomLongTermPicNum = 2 * dpb[i].LongTermFrameIdx;     // opposite parity (8-34)
                        }
                        else
                        {
                            // bottom field
                            dpb[i].TopLongTermPicNum    = 2 * dpb[i].LongTermFrameIdx;     // opposite parity (8-34)
                            dpb[i].BottomLongTermPicNum = 2 * dpb[i].LongTermFrameIdx + 1; // same parity (8-33)
                        }
                    }
                    if (dpb[i].bottom_field_marking == MARKING_SHORT && dpb[i].BottomPicNum == picNumX)
                    {
                        dpb[i].bottom_field_marking = MARKING_LONG;
                        dpb[i].LongTermFrameIdx = slh->mmco[k].long_term_frame_idx;
                        // update TopLongTermPicNum, BottomLongTermPicNum for subsequent mmco 2
                        if (!slh->field_pic_flag)
                        {
                            // frame
                            dpb[i].TopLongTermPicNum = dpb[i].BottomLongTermPicNum = dpb[i].LongTermFrameIdx; // (8-30)
                        }
                        else if (!slh->bottom_field_flag)
                        {
                            // top field
                            dpb[i].TopLongTermPicNum    = 2 * dpb[i].LongTermFrameIdx + 1; // same parity (8-33)
                            dpb[i].BottomLongTermPicNum = 2 * dpb[i].LongTermFrameIdx;     // opposite parity (8-34)
                        }
                        else
                        {
                            // bottom field
                            dpb[i].TopLongTermPicNum    = 2 * dpb[i].LongTermFrameIdx;     // opposite parity (8-34)
                            dpb[i].BottomLongTermPicNum = 2 * dpb[i].LongTermFrameIdx + 1; // same parity (8-33)
                        }
                    }
                }
            }
            break;
        case 4:
            // 8.2.5.4.4 Decoding process for MaxLongTermFrameIdx
            MaxLongTermFrameIdx = slh->mmco[k].long_term_frame_idx - 1;
            for (int i = 0; i < MAX_DPB_SIZE; i++)
            {
                if (dpb[i].view_id == slh->view_id)
                {
                    if (dpb[i].top_field_marking == MARKING_LONG && dpb[i].LongTermFrameIdx > MaxLongTermFrameIdx)
                        dpb[i].top_field_marking = MARKING_UNUSED;
                    if (dpb[i].bottom_field_marking == MARKING_LONG && dpb[i].LongTermFrameIdx > MaxLongTermFrameIdx)
                        dpb[i].bottom_field_marking = MARKING_UNUSED;
                }
            }
            break;
        case 5:
            // 8.2.5.4.5 Marking process of all reference pictures as "unused for reference" and setting MaxLongTermFrameIdx to "no long-term frame indices"
            for (int i = 0; i < MAX_DPB_SIZE; i++)
            {
                if (dpb[i].view_id == slh->view_id)
                {
                    dpb[i].top_field_marking = MARKING_UNUSED;
                    dpb[i].bottom_field_marking = MARKING_UNUSED;
                }
            }
            MaxLongTermFrameIdx = -1;
            cur->FrameNum = 0; // 7.4.3
            // 8.2.1
            cur->TopFieldOrderCnt -= cur->PicOrderCnt;
            cur->BottomFieldOrderCnt -= cur->PicOrderCnt;
            cur->PicOrderCnt = 0;
            break;
        case 6:
            // 8.2.5.4.6 Process for assigning a long-term frame index to the current picture
            for (int i = 0; i < MAX_DPB_SIZE; i++)
            {
                if (dpb[i].view_id == slh->view_id)
                {
                    if (i != iCur && dpb[i].top_field_marking == MARKING_LONG && dpb[i].LongTermFrameIdx == slh->mmco[k].long_term_frame_idx)
                        dpb[i].top_field_marking = MARKING_UNUSED;
                    if (i != iCur && dpb[i].bottom_field_marking == MARKING_LONG && dpb[i].LongTermFrameIdx == slh->mmco[k].long_term_frame_idx)
                        dpb[i].bottom_field_marking = MARKING_UNUSED;
                }
            }
            if (!slh->field_pic_flag || !slh->bottom_field_flag)
                cur->top_field_marking = MARKING_LONG;
            if (!slh->field_pic_flag || slh->bottom_field_flag)
                cur->bottom_field_marking = MARKING_LONG;
            cur->LongTermFrameIdx = slh->mmco[k].long_term_frame_idx;
            // update TopLongTermPicNum, BottomLongTermPicNum
            // (subsequent mmco 2 is not allowed to reference it, but to avoid accidental matches they have to be updated)
            if (!slh->field_pic_flag)
            {
                // frame
                cur->TopLongTermPicNum = cur->BottomLongTermPicNum = cur->LongTermFrameIdx; // (8-30)
            }
            else if (!slh->bottom_field_flag)
            {
                // top field
                cur->TopLongTermPicNum    = 2 * cur->LongTermFrameIdx + 1; // same parity (8-33)
                cur->BottomLongTermPicNum = 2 * cur->LongTermFrameIdx;     // opposite parity (8-34)
            }
            else
            {
                // bottom field
                cur->TopLongTermPicNum    = 2 * cur->LongTermFrameIdx;     // opposite parity (8-34)
                cur->BottomLongTermPicNum = 2 * cur->LongTermFrameIdx + 1; // same parity (8-33)
            }
            break;
        }
    }
    // Make sure that MMCO doesn't cause a num_ref_frame violation, which can happen with missing references
    // In this case evict the oldest non-existing reference first if any, or the oldest existing reference
    for (;;)
    {
        int numRefs = 0;
        int oldestIndex = -1, oldestIndexNE = -1;
    
        for (int i = 0; i < MAX_DPB_SIZE; i++)
        {
            if (dpb[i].view_id != slh->view_id)
                continue;

            // Evict all short-term non-existing references, as the non-existing references may never be evicted if
            // adaptive_ref_pic_marking=1
            if ((dpb[i].not_existing) && (dpb[i].top_field_marking == MARKING_SHORT) && (dpb[i].bottom_field_marking == MARKING_SHORT) && 
                (i != iCur) && (!m_sps->flags.gaps_in_frame_num_value_allowed_flag))
            {
                dpb[i].top_field_marking = MARKING_UNUSED;
                dpb[i].bottom_field_marking = MARKING_UNUSED;
            }
            else if ((dpb[i].top_field_marking != MARKING_UNUSED || dpb[i].bottom_field_marking != MARKING_UNUSED) && (i != iCur))
            {
                numRefs++;
                if ((oldestIndex < 0) || (dpb[i].FrameNumWrap < dpb[oldestIndex].FrameNumWrap))
                {
                    oldestIndex = i;
                }
                if (dpb[i].not_existing)
                {
                    if ((oldestIndexNE < 0) || (dpb[i].FrameNumWrap < dpb[oldestIndexNE].FrameNumWrap))
                    {
                        oldestIndexNE = i;
                    }
                }
            }
        }
        if ((numRefs >= num_ref_frames) && (oldestIndex >= 0))
        {
            if (oldestIndexNE >= 0)
            {
                dpb[oldestIndexNE].top_field_marking = MARKING_UNUSED;
                dpb[oldestIndexNE].bottom_field_marking = MARKING_UNUSED;
            } else
            {
                dpb[oldestIndex].top_field_marking = MARKING_UNUSED;
                dpb[oldestIndex].bottom_field_marking = MARKING_UNUSED;
            }
        } else
        {
            // We're done
            break;
        }
    }
}


// DPB
int VulkanH264Decoder::dpb_fullness()
{
    int dpb_fullness = 0;
    for (int i = 0; i < MAX_DPB_SIZE; i++)
    {
        dpb_fullness += (dpb[i].state != 0);
    }
    return dpb_fullness;
}


// C.4.5.3
void VulkanH264Decoder::dpb_bumping(int MaxDpbSize)
{
    int i, pocMin, iMin, VOIdxMin;

    // select the frame buffer that contains the picture having the smallest value
    // of PicOrderCnt of all pictures in the DPB marked as "needed for output"
    // when PicOrderCnt is the same (MVC), select the picture with smallest VOIdx
    pocMin = INF_MAX;
    iMin = -1;
    VOIdxMin = -1;

    for (i = 0; i < MaxDpbSize; i++)
    {
        if ((dpb[i].state & 1) && dpb[i].top_needed_for_output && ((dpb[i].TopFieldOrderCnt < pocMin)||
            (dpb[i].TopFieldOrderCnt == pocMin && dpb[i].VOIdx < VOIdxMin) || (iMin<0)))
        {
            pocMin = dpb[i].TopFieldOrderCnt;
            VOIdxMin = dpb[i].VOIdx;
            iMin = i;
        }
        if ((dpb[i].state & 2) && dpb[i].bottom_needed_for_output && ((dpb[i].BottomFieldOrderCnt < pocMin)||
            (dpb[i].BottomFieldOrderCnt == pocMin && dpb[i].VOIdx < VOIdxMin) || (iMin<0)))
        {
            pocMin = dpb[i].BottomFieldOrderCnt;
            VOIdxMin = dpb[i].VOIdx;
            iMin = i;
        }
    }

    if (iMin < 0)
    {
        int fnMin = INF_MAX, jMin = -1;
        for (i = 0; i < MaxDpbSize; i++)
        {
            if ((dpb[i].state & 1) && dpb[i].TopFieldOrderCnt <= pocMin)
            {
                pocMin = dpb[i].TopFieldOrderCnt;
                iMin = i;
            }
            if ((dpb[i].state & 2) && dpb[i].BottomFieldOrderCnt <= pocMin)
            {
                pocMin = dpb[i].BottomFieldOrderCnt;
                iMin = i;
            }
            if ((dpb[i].state) && (dpb[i].not_existing) && (dpb[i].FrameNum <= fnMin))
            {
                fnMin = dpb[i].FrameNum;
                jMin = i;
            }
        }
        if (jMin >= 0)
            iMin = jMin;
        if (iMin >= 0)
        {
            dpb[iMin].state = 0;
            dpb[iMin].top_field_marking = MARKING_UNUSED;
            dpb[iMin].bottom_field_marking = MARKING_UNUSED;
            if (dpb[iMin].pPicBuf)
            {
                dpb[iMin].pPicBuf->Release();
                dpb[iMin].pPicBuf = NULL;
            }
        }
        return;
    }

    if (dpb[iMin].state == 3 &&
        dpb[iMin].top_needed_for_output &&
        dpb[iMin].bottom_needed_for_output)
    {
        // output frame
        output_picture(iMin, 3);
        dpb[iMin].top_needed_for_output = 0;
        dpb[iMin].bottom_needed_for_output = 0;
    }
    else if (dpb[iMin].state == 1)
    {
        // output top field
        output_picture(iMin, 1);
        dpb[iMin].top_needed_for_output = 0;
    }
    else // (dpb[iMin].state == 2)
    {
        // output bottom field
        output_picture(iMin, 2);
        dpb[iMin].bottom_needed_for_output = 0;
    }
    // empty frame buffer
    if ((!(dpb[iMin].state & 1) || (!dpb[iMin].top_needed_for_output && dpb[iMin].top_field_marking == MARKING_UNUSED))
     && (!(dpb[iMin].state & 2) || (!dpb[iMin].bottom_needed_for_output && dpb[iMin].bottom_field_marking == MARKING_UNUSED)))
    {
        dpb[iMin].state = 0;
        if (dpb[iMin].pPicBuf)
        {
            dpb[iMin].pPicBuf->Release();
            dpb[iMin].pPicBuf = NULL;
        }
    }
}


void VulkanH264Decoder::flush_decoded_picture_buffer()
{
    int i;
    
    // mark all reference pictures as "unused for reference"
    for (i=0; i<=MAX_DPB_SIZE; i++)
    {
        dpb[i].top_field_marking = MARKING_UNUSED;
        dpb[i].bottom_field_marking = MARKING_UNUSED;
    }
    // empty frame buffers marked as "not needed for output" and "unused for reference"
    for (i=0; i<=MAX_DPB_SIZE; i++)
    {
        if ((!(dpb[i].state & 1) || (!dpb[i].top_needed_for_output    && dpb[i].top_field_marking    == MARKING_UNUSED)) &&
            (!(dpb[i].state & 2) || (!dpb[i].bottom_needed_for_output && dpb[i].bottom_field_marking == MARKING_UNUSED)))
        {
            dpb[i].state = 0; // empty
            if (dpb[i].pPicBuf)
            {
                dpb[i].pPicBuf->Release();
                dpb[i].pPicBuf = NULL;
            }
        }
    }
    while ((!dpb_empty()) || (dpb[MAX_DPB_SIZE].state & 3))
    {
        dpb_bumping(MAX_DPB_SIZE+1);
    }
}


int VulkanH264Decoder::dpb_reordering_delay()
{
    int reordering_delay = 0;
    for (int i = 0; i < MAX_DPB_SIZE; i++)
    {
        if ((dpb[i].state == 3) && (dpb[i].top_needed_for_output) && (dpb[i].bottom_needed_for_output))
        {
            reordering_delay++;
        }
    }
    return reordering_delay;
}

void VulkanH264Decoder::display_bumping()
{
    int i, pocMin, iMin;

    // select the frame buffer that contains the picture having the smallest value
    // of PicOrderCnt of all pictures in the DPB marked as "needed for output"
    pocMin = INF_MAX;
    iMin = -1;
    for (i=0; i<MAX_DPB_SIZE; i++)
    {
        if ((dpb[i].state & 1) && dpb[i].top_needed_for_output && dpb[i].TopFieldOrderCnt <= pocMin)
        {
            if (pocMin == dpb[i].TopFieldOrderCnt)
                return; // that's weird: duplicate poc -> bail
            pocMin = dpb[i].TopFieldOrderCnt;
            iMin = i;
        }
        if ((dpb[i].state & 2) && dpb[i].bottom_needed_for_output && dpb[i].BottomFieldOrderCnt <= pocMin)
        {
            if (pocMin == dpb[i].BottomFieldOrderCnt && iMin != i)
                return; // that's weird: duplicate poc -> bail
            pocMin = dpb[i].BottomFieldOrderCnt;
            iMin = i;
        }
    }
    // Only output if it's a full frame
    if (iMin >= 0 && dpb[iMin].state == 3)
    {
        output_picture(iMin, 3);
        dpb[iMin].top_needed_for_output = 0;
        dpb[iMin].bottom_needed_for_output = 0;
    }
}

// Compute VOIdx
int VulkanH264Decoder::get_view_output_index(int view_id)
{
    if (((m_sps->profile_idc == 118) || (m_sps->profile_idc == 128)) && (m_spsme))
    {
        for (int VOIdx = 0; VOIdx <= m_spsme->num_views_minus1; VOIdx++)
        {
            if (view_id == m_spsme->view_id[VOIdx])
            {
                return VOIdx;
            }
        }
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// SEI payloads (D.1)
//

void VulkanH264Decoder::sei_payload(int payloadType, int payloadSize)
{
    switch(payloadType)
    {
    case 0: // buffering_period (D.1.1)
        {
            uint32_t sps_id = ue();
            if ((sps_id < MAX_NUM_SPS) && (m_spss[sps_id]))
            {
                const seq_parameter_set_s *sps = m_spss[sps_id];
                if (sps->vui.nal_hrd_parameters_present_flag)
                {
                    for (int SchedSelIdx = 0; SchedSelIdx <= sps->vui.nal_hrd.cpb_cnt_minus1; SchedSelIdx++)
                    {
                        u(sps->vui.initial_cpb_removal_delay_length);   // initial_cpb_removal_delay
                        u(sps->vui.initial_cpb_removal_delay_length);   // initial_cpb_removal_delay_offset
                        if (m_nalu.get_offset >= m_nalu.end_offset)     // bitstream error
                            break;
                    }
                }
                if (sps->vui.vcl_hrd_parameters_present_flag)
                {
                    for (int SchedSelIdx = 0; SchedSelIdx <= sps->vui.nal_hrd.cpb_cnt_minus1; SchedSelIdx++)
                    {
                        u(sps->vui.initial_cpb_removal_delay_length); // initial_cpb_removal_delay
                        u(sps->vui.initial_cpb_removal_delay_length); // initial_cpb_removal_delay_offset
                        if (m_nalu.get_offset >= m_nalu.end_offset)   // bitstream error
                            break;
                    }
                }
                m_last_sps_id = sps_id;
            }
        }
        break;
    case 1: // pic_timing (D.1.2)
        if (m_spss[m_last_sps_id])
        {
            const seq_parameter_set_s *sps = m_spss[m_last_sps_id];
            if (sps->vui.nal_hrd_parameters_present_flag || sps->vui.vcl_hrd_parameters_present_flag) // CpbDpbDelaysPresentFlag
            {
                u(sps->vui.cpb_removal_delay_length_minus1+1);  // cpb_removal_delay
                u(sps->vui.dpb_output_delay_length_minus1+1);   // dpb_output_delay
            }
            if (sps->vui.pic_struct_present_flag)
            {
                m_last_sei_pic_struct = u(4);   // Primarily used to detect 3:2 pulldown
                // ...
            }
        }
        break;
    case 45: // frame_packing_arrangement
        {
            int frame_packing_arrangement_cancel_flag;
            ue();    // frame_packing_arrangement_id
            frame_packing_arrangement_cancel_flag = u(1);
            if (!frame_packing_arrangement_cancel_flag)
            {
                m_fpa.frame_packing_arrangement_type = u(7);
                u(1); //quincunx_sampling_flag
                m_fpa.content_interpretation_flag = u(6);
            }
            else
            {
                m_fpa.frame_packing_arrangement_type = 0;
                m_fpa.content_interpretation_flag = 0;
            }
        }
        break;
    default:
        nvParserVerboseLog("SEI(%d): %d bytes (0x%06X)\n", payloadType, payloadSize, next_bits(24));
        break;
    }
#if 0   // Caller will use payloadSize to skip over the SEI payload
    if (!byte_aligned())
    {
        f(1,1); // bit_equal_to_one
        while (!byte_aligned())
            f(1,0); // bit_equal_to_zero
    }
#endif
}

const char* seq_parameter_set_s::m_refClassId = "h264SpsVideoPictureParametersSet";
const char* pic_parameter_set_s::m_refClassId = "h264PpsVideoPictureParametersSet";
