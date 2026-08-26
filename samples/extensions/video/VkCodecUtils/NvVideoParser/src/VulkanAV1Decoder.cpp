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

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// AV1 elementary stream parser (picture & sequence layer)
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cstdint>
#include <climits>
#include "VulkanVideoParserIf.h"

#include "VulkanAV1Decoder.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

// constructor
VulkanAV1Decoder::VulkanAV1Decoder(VkVideoCodecOperationFlagBitsKHR std, bool annexB)
    : VulkanVideoDecoder(std)
    , m_sps()
    , m_PicData()
    , temporal_id()
    , spatial_id()
    , m_bSPSReceived()
    , m_bSPSChanged()
    , m_obuAnnexB(annexB)
    , timing_info_present()
    , timing_info()
    , buffer_model()
    , op_params{}
    , op_frame_timing{}
    , delta_frame_id_length()
    , frame_id_length()
    , last_frame_type()
    , last_intra_only()
    , coded_lossless()
    , all_lossless(0)
    , upscaled_width()
    , frame_width()
    , frame_height()
    , render_width()
    , render_height()
    , intra_only()
    , showable_frame()
    , last_show_frame()
    , show_existing_frame()
    , tu_presentation_delay()
    , lossless{}
    , tile_size_bytes_minus_1(3)
    , log2_tile_cols()
    , log2_tile_rows()
    , global_motions{}
    , ref_frame_id{}
    , pic_idx{}
    , RefValid{}
    , ref_frame_idx{}
    , RefOrderHint{}
    , m_pBuffers{}
    , m_pCurrPic(nullptr)
    , m_bOutputAllLayers()
    , m_OperatingPointIDCActive()
    , m_numOutFrames()
    , m_pOutFrame{}
    , m_showableFrame{}

{
    for (int i = 0; i < STD_VIDEO_AV1_NUM_REF_FRAMES; i++) {
        ref_frame_id[i] = -1;
		pic_idx[i] = -1;
    }

	m_PicData.std_info.primary_ref_frame = STD_VIDEO_AV1_PRIMARY_REF_NONE;
    m_PicData.std_info.refresh_frame_flags = (1 << STD_VIDEO_AV1_NUM_REF_FRAMES) - 1;

    for (int i = 0; i < GM_GLOBAL_MODELS_PER_FRAME; ++i)
    {
        global_motions[i] = default_warp_params;
    }
}

// destructor
VulkanAV1Decoder::~VulkanAV1Decoder()
{

}

// initialization
void VulkanAV1Decoder::InitParser()
{
    m_bNoStartCodes = true;
    m_bEmulBytesPresent = false;
    m_bSPSReceived = false;
    EndOfStream();
}

// EOS
void VulkanAV1Decoder::EndOfStream()
{
    if (m_pCurrPic) {
        m_pCurrPic->Release();
        m_pCurrPic = nullptr;
    }

    for (int i = 0; i < 8; i++) {
        if (m_pBuffers[i].buffer) {
            m_pBuffers[i].buffer->Release();
            m_pBuffers[i].buffer = nullptr;
        }
    }
    for (int i = 0; i < MAX_NUM_SPATIAL_LAYERS; i++) {
        if (m_pOutFrame[i]) {
            m_pOutFrame[i]->Release();
            m_pOutFrame[i] = nullptr;
        }
    }
}

bool VulkanAV1Decoder::AddBuffertoOutputQueue(VkPicIf* pDispPic, bool bShowableFrame)
{
    if (m_bOutputAllLayers) {
/*
        if (m_numOutFrames >= MAX_NUM_SPATIAL_LAYERS)
        {
            // We can't store the new frame anywhere, so drop it and return an error
            return false;
        }
        else {
            m_pOutFrame[m_numOutFrames] = pDispPic;
            m_showableFrame[m_numOutFrames] = bShowableFrame;
            m_numOutFrames++;
        }
*/
        // adding buffer to output queue will cause display latency so display immediately to avoid latency
        AddBuffertoDispQueue(pDispPic);
        lEndPicture(pDispPic, !bShowableFrame);
        if (pDispPic) {
            pDispPic->Release();
        }
    } else {
        assert(m_numOutFrames == 0 || m_numOutFrames == 1);

        if (m_numOutFrames > 0) {
            m_pOutFrame[0]->Release();
        }

        m_pOutFrame[0] = pDispPic;
        m_showableFrame[0] = bShowableFrame;
        m_numOutFrames++;
    }
    return true;
}

void VulkanAV1Decoder::AddBuffertoDispQueue(VkPicIf* pDispPic)
{
    int32_t lDisp = 0;

    // Find an entry in m_DispInfo
    for (int32_t i = 0; i < MAX_DELAY; i++) {
        if (m_DispInfo[i].pPicBuf == pDispPic) {
            lDisp = i;
            break;
        }
        if ((m_DispInfo[i].pPicBuf == nullptr)
            || ((m_DispInfo[lDisp].pPicBuf != nullptr) && (m_DispInfo[i].llPTS - m_DispInfo[lDisp].llPTS < 0))) {
            lDisp = i;
        }
    }
    m_DispInfo[lDisp].pPicBuf = pDispPic;
    m_DispInfo[lDisp].bSkipped = false;
    m_DispInfo[lDisp].bDiscontinuity = false;
    //m_DispInfo[lDisp].lPOC = m_pNVDPictureData->picture_order_count;
    m_DispInfo[lDisp].lNumFields = 2;

    // Find a PTS in the list
    unsigned int ndx = m_lPTSPos;
    m_DispInfo[lDisp].bPTSValid = false;
    m_DispInfo[lDisp].llPTS = m_llExpectedPTS; // Will be updated later on
    for (int k = 0; k < MAX_QUEUED_PTS; k++) {
        if ((m_PTSQueue[ndx].bPTSValid) && (m_PTSQueue[ndx].llPTSPos - m_llFrameStartLocation <= (m_bNoStartCodes ? 0 : 3))) {
            m_DispInfo[lDisp].bPTSValid = true;
            m_DispInfo[lDisp].llPTS = m_PTSQueue[ndx].llPTS;
            m_DispInfo[lDisp].bDiscontinuity = m_PTSQueue[ndx].bDiscontinuity;
            m_PTSQueue[ndx].bPTSValid = false;
        }
        ndx = (ndx + 1) % MAX_QUEUED_PTS;
    }
}

// kick-off decoding
bool VulkanAV1Decoder::end_of_picture(uint32_t frameSize)
{
	StdVideoDecodeAV1PictureInfo const* pStd = &m_PicData.std_info;

    *m_pVkPictureData = VkParserPictureData();
    m_pVkPictureData->numSlices = m_PicData.tileInfo.TileCols * m_PicData.tileInfo.TileRows;  // set number of tiles as AV1 doesn't have slice concept

    m_pVkPictureData->bitstreamDataLen = frameSize;
    m_pVkPictureData->bitstreamData = m_bitstreamData.GetBitstreamBuffer();
    m_pVkPictureData->bitstreamDataOffset = 0; // TODO: The extra storage in this library and necessarily the app is silly.

    m_PicData.needsSessionReset = m_bSPSChanged;
    m_bSPSChanged = false;

    m_pVkPictureData->firstSliceIndex = 0;
    memcpy(&m_pVkPictureData->CodecSpecific.av1, &m_PicData, sizeof(m_PicData));
    m_pVkPictureData->intra_pic_flag = (pStd->frame_type == STD_VIDEO_AV1_FRAME_TYPE_KEY);

    if (!BeginPicture(m_pVkPictureData)) {
        // Error: BeginPicture failed
        return false;
    }

    bool bSkipped = false;
    if (m_pClient != nullptr) {
        // Notify client
        if (!m_pClient->DecodePicture(m_pVkPictureData)) {
            bSkipped = true;
            // WARNING: skipped decoding current picture;
        } else {
            m_nCallbackEventCount++;
        }
    } else {
        // "WARNING: no valid render target for current picture
    }

    // decode_frame_wrapup
    UpdateFramePointers(m_pCurrPic);
    if (m_PicData.showFrame && !bSkipped) {
        AddBuffertoOutputQueue(m_pCurrPic, !!showable_frame);
        m_pCurrPic = nullptr;
    } else {
        if (m_pCurrPic) {
            m_pCurrPic->Release();
            m_pCurrPic = nullptr;
        }
    }

    return true;
}

// BeginPicture
bool VulkanAV1Decoder::BeginPicture(VkParserPictureData* pnvpd)
{
    VkParserAv1PictureData* const av1 = &pnvpd->CodecSpecific.av1;
    av1_seq_param_s *const sps = m_sps.Get();
    assert(sps != nullptr);

    av1->upscaled_width = upscaled_width;
    av1->frame_width = frame_width;
    av1->frame_height = frame_height;

    VkParserSequenceInfo nvsi = m_ExtSeqInfo;
    nvsi.eCodec         = (VkVideoCodecOperationFlagBitsKHR)VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR;
    nvsi.nChromaFormat  = sps->color_config.flags.mono_chrome ? 0 : (sps->color_config.subsampling_x && sps->color_config.subsampling_y) ? 1 : (!sps->color_config.subsampling_x && !sps->color_config.subsampling_y) ? 3 : 2;
    nvsi.nMaxWidth      = (sps->max_frame_width_minus_1 + 2) & ~1;
    nvsi.nMaxHeight     = (sps->max_frame_height_minus_1 + 2) & ~1;
    nvsi.nCodedWidth    = upscaled_width;
    nvsi.nCodedHeight   = frame_height;
    nvsi.nDisplayWidth  = av1->upscaled_width; // (nvsi.nCodedWidth + 1) & (~1);
    nvsi.nDisplayHeight = nvsi.nCodedHeight; //(nvsi.nCodedHeight + 1) & (~1);
    nvsi.bProgSeq = true; // AV1 doesnt have explicit interlaced coding.

    nvsi.uBitDepthLumaMinus8 = sps->color_config.BitDepth - 8;
    nvsi.uBitDepthChromaMinus8 = nvsi.uBitDepthLumaMinus8;

    nvsi.lDARWidth = nvsi.nDisplayWidth;
    nvsi.lDARHeight = nvsi.nDisplayHeight;
    // nMinNumDecodeSurfaces = dpbsize (8 for av1)  + 1
    // double the decode RT count to account film grained output if film grain present
    nvsi.nMinNumDecodeSurfaces = 9;

    nvsi.lVideoFormat = VideoFormatUnspecified;
    nvsi.lColorPrimaries = sps->color_config.color_primaries;
    nvsi.lTransferCharacteristics = sps->color_config.transfer_characteristics;
    nvsi.lMatrixCoefficients = sps->color_config.matrix_coefficients;

    nvsi.hasFilmGrain = sps->flags.film_grain_params_present;

    if (av1->needsSessionReset && !init_sequence(&nvsi))
        return false;

    // Allocate a buffer for the current picture
    if (m_pCurrPic == nullptr) {
        m_pClient->AllocPictureBuffer(&m_pCurrPic);
    }

    pnvpd->PicWidthInMbs    = nvsi.nCodedWidth >> 4;
    pnvpd->FrameHeightInMbs = nvsi.nCodedHeight >> 4;
    pnvpd->pCurrPic         = m_pCurrPic;
    pnvpd->progressive_frame = 1;
    pnvpd->ref_pic_flag     = true;
    pnvpd->chroma_format    = nvsi.nChromaFormat; // 1 : 420

    // Setup slot information
    av1->setupSlot.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_DPB_SLOT_INFO_KHR;
    av1->setupSlotInfo.OrderHint = m_PicData.std_info.OrderHint;
    memcpy(&av1->setupSlotInfo.SavedOrderHints, m_PicData.std_info.OrderHints, STD_VIDEO_AV1_NUM_REF_FRAMES);
    for (size_t av1name = 0; av1name < STD_VIDEO_AV1_NUM_REF_FRAMES; av1name += 1) {
        av1->setupSlotInfo.RefFrameSignBias |= (m_pBuffers[0].RefFrameSignBias[av1name] <= 0) << av1name;
    }
    av1->setupSlotInfo.flags.disable_frame_end_update_cdf = m_PicData.std_info.flags.disable_frame_end_update_cdf;
    av1->setupSlotInfo.flags.segmentation_enabled = m_PicData.std_info.flags.segmentation_enabled;

    // Referenced frame information
    for (int i = 0; i < STD_VIDEO_AV1_NUM_REF_FRAMES; i++) {
        vkPicBuffBase *pb = reinterpret_cast<vkPicBuffBase *>(m_pBuffers[i].buffer);
        av1->pic_idx[i] = pb ? pb->m_picIdx : -1;
        av1->dpbSlotInfos[i].flags.disable_frame_end_update_cdf = m_pBuffers[i].disable_frame_end_update_cdf;
        av1->dpbSlotInfos[i].flags.segmentation_enabled = m_pBuffers[i].segmentation_enabled;
        av1->dpbSlotInfos[i].frame_type = m_pBuffers[i].frame_type;
        av1->dpbSlotInfos[i].OrderHint = m_pBuffers[i].order_hint;
        for (size_t av1name = STD_VIDEO_AV1_REFERENCE_NAME_LAST_FRAME; av1name < STD_VIDEO_AV1_NUM_REF_FRAMES; av1name += 1) {
            av1->dpbSlotInfos[i].RefFrameSignBias |= (m_pBuffers[i].RefFrameSignBias[av1name] <= 0) << av1name;
            av1->dpbSlotInfos[i].SavedOrderHints[av1name] = m_pBuffers[i].SavedOrderHints[av1name];
        }
    }

    // TODO: It's weird that the intra frame motion isn't tracked by the parser.
    // Need an affine translation test case to properly check this.
    for (int i = 1; i < STD_VIDEO_AV1_NUM_REF_FRAMES; i++) {
        av1->globalMotion.GmType[i] = global_motions[i-1].wmtype;
        for (int j = 0; j <= 5; j++) {
            av1->globalMotion.gm_params[i][j] = global_motions[i-1].wmmat[j];
        }
    }

    for (int i = 0; i < STD_VIDEO_AV1_REFS_PER_FRAME; i++) {
        av1->ref_frame_idx[i] = ref_frame_idx[i];
    }

    return true;
}

int VulkanAV1Decoder::GetRelativeDist(int a, int b)
{
    auto* sps = m_sps.Get();
    if (sps->flags.enable_order_hint == false) {
        return 0;
    }

    const int bits = sps->order_hint_bits_minus_1 + 1;

    assert(bits >= 1);
    assert(a >= 0 && a < (1 << bits));
    assert(b >= 0 && b < (1 << bits));

    int diff = a - b;
    const int m = 1 << (bits - 1);
    diff = (diff & (m - 1)) - (diff & m);
    return diff;
}

void VulkanAV1Decoder::UpdateFramePointers(VkPicIf* currentPicture)
{
    VkParserAv1PictureData *const pic_data = &m_PicData;
	StdVideoDecodeAV1PictureInfo *const pStd = &m_PicData.std_info;

    // uint32_t i;
    uint32_t mask, ref_index = 0;

    for (mask = pStd->refresh_frame_flags; mask; mask >>= 1) {
        if (mask & 1) {
            if (m_pBuffers[ref_index].buffer) {
                m_pBuffers[ref_index].buffer->Release();
            }

            m_pBuffers[ref_index].buffer = currentPicture;
            m_pBuffers[ref_index].showable_frame = showable_frame;

            m_pBuffers[ref_index].frame_type = pStd->frame_type;
            m_pBuffers[ref_index].order_hint = pStd->OrderHint;
            for (uint8_t refName = STD_VIDEO_AV1_REFERENCE_NAME_LAST_FRAME; refName < STD_VIDEO_AV1_NUM_REF_FRAMES; refName ++) {
                uint8_t ref_order_hint = pStd->OrderHints[refName];
                m_pBuffers[ref_index].SavedOrderHints[refName] = ref_order_hint;
                m_pBuffers[ref_index].RefFrameSignBias[refName] = GetRelativeDist(pStd->OrderHint, ref_order_hint);
            }

            // film grain
            memcpy(&m_pBuffers[ref_index].film_grain_params, &m_PicData.filmGrain, sizeof(StdVideoAV1FilmGrain));
            // global motion
            memcpy(&m_pBuffers[ref_index].global_models, &global_motions, sizeof(AV1WarpedMotionParams) * GM_GLOBAL_MODELS_PER_FRAME);
            // loop filter
            memcpy(&m_pBuffers[ref_index].lf_ref_delta, m_PicData.loopFilter.loop_filter_ref_deltas, ARRAY_SIZE(m_PicData.loopFilter.loop_filter_ref_deltas));
            memcpy(&m_pBuffers[ref_index].lf_mode_delta, m_PicData.loopFilter.loop_filter_mode_deltas, ARRAY_SIZE(m_PicData.loopFilter.loop_filter_mode_deltas));
            // segmentation
            memcpy(&m_pBuffers[ref_index].seg.FeatureEnabled, pic_data->segmentation.FeatureEnabled, ARRAY_SIZE(pic_data->segmentation.FeatureEnabled));
            memcpy(&m_pBuffers[ref_index].seg.FeatureData, pic_data->segmentation.FeatureData, ARRAY_SIZE(pic_data->segmentation.FeatureData));
            m_pBuffers[ref_index].primary_ref_frame = m_PicData.std_info.primary_ref_frame;
            m_pBuffers[ref_index].base_q_index = pic_data->quantization.base_q_idx;
            m_pBuffers[ref_index].disable_frame_end_update_cdf = pStd->flags.disable_frame_end_update_cdf;

            RefOrderHint[ref_index] = pStd->OrderHint;

            if (m_pBuffers[ref_index].buffer) {
                m_pBuffers[ref_index].buffer->AddRef();
            }
        }
        ++ref_index;
    }

    // Invalidate these references until the next frame starts.
    // for (i = 0; i < ALLOWED_REFS_PER_FRAME; i++)
    //     pic_info->activeRefIdx[i] = 0xffff;
}

// EndPicture
void VulkanAV1Decoder::lEndPicture(VkPicIf* pDispPic, bool bEvict)
{
    if (pDispPic) {
        display_picture(pDispPic, bEvict);
    }
}

//
uint32_t VulkanAV1Decoder::ReadUvlc()
{
    int lz = 0;
    while (!u(1)) lz++;

    if (lz >= 32) {
        return BIT32_MAX;
    }
    uint32_t v = u(lz);
    v += (1 << lz) - 1;
    return v;
}

// Read OBU size (size does not include obu_header or the obu_size syntax element
bool VulkanAV1Decoder::ReadObuSize(const uint8_t* data, uint32_t datasize, uint32_t* obu_size, uint32_t* length_feild_size)
{
    for (uint32_t i = 0; i < 8 && (i < datasize); ++i) {
        const uint8_t decoded_byte = data[i] & 0x7f;
        *obu_size |= ((uint64_t)decoded_byte) << (i * 7);
        if ((data[i] >> 7) == 0) {
            *length_feild_size = i + 1;
            if (*obu_size > BIT32_MAX) {
                return false;
            } else {
                return true;
            }
        }
    }

    return false;
}

// Parses OBU header
bool VulkanAV1Decoder::ReadObuHeader(const uint8_t* pData, uint32_t datasize, AV1ObuHeader* hdr)
{
    const uint8_t* local = pData;
    hdr->header_size = 1;

    if (((local[0] >> 7) & 1) != 0) {
        // Forbidden bit
        // Corrupt frame
        return false;
    }

    hdr->type = (AV1_OBU_TYPE)((local[0] >> 3) & 0xf);

    if (!((hdr->type >= AV1_OBU_SEQUENCE_HEADER && hdr->type <= AV1_OBU_PADDING))) {
        // Invalid OBU type
        return false;
    }

    hdr->has_extension = (local[0] >> 2) & 1;
    hdr->has_size_field = (local[0] >> 1) & 1;

    if (!hdr->has_size_field && !m_obuAnnexB) {
        // obu streams must have obu_size field set.
        // Unsupported bitstream
        return false;
    }

    if (((local[0] >> 0) & 1) != 0) {
        // must be set to 0.
        // Corrupt frame
        return false;
    }

    if (hdr->has_extension) {
        if (datasize < 2)
            return false;
        hdr->header_size += 1;
        hdr->temporal_id = (local[1] >> 5) & 0x7;
        hdr->spatial_id = (local[1] >> 3) & 0x3;
        if (((local[1] >> 0) & 0x7) != 0) {
            // must be set to 0.
            // Corrupt frame
            return false;
        }
    }

    return true;
}

//
bool VulkanAV1Decoder::ParseOBUHeaderAndSize(const uint8_t* data, uint32_t datasize, AV1ObuHeader* hdr)
{
    uint32_t annexb_obu_length = 0, annexb_uleb_length = 0;

    if (datasize == 0) {
        return false;
    }

    if (m_obuAnnexB) {
        if (!ReadObuSize(data, datasize, &annexb_obu_length, &annexb_uleb_length)) {
            return false;
        }
    }

    if (!ReadObuHeader(data + annexb_uleb_length, datasize - annexb_uleb_length, hdr)) {
        // read_obu_header() failed
        return false;;
    }

    if (m_obuAnnexB) {
        // Derive the payload size from the data we've already read
        if (annexb_obu_length < hdr->header_size) return false;

        // The Annex B OBU length includes the OBU header.
        hdr->payload_size = annexb_obu_length - hdr->header_size;
        hdr->header_size += annexb_uleb_length;
        uint32_t obu_size = 0;
        uint32_t size_field_uleb_length = 0;
        if (hdr->has_size_field) {
            if (!ReadObuSize(data + hdr->header_size, datasize - hdr->header_size, &obu_size, &size_field_uleb_length)) {
                return false;
            }
            hdr->header_size += size_field_uleb_length;
            hdr->payload_size = obu_size;
        }
    } else {
        assert(hdr->has_size_field);
        // Size field comes after the OBU header, and is just the payload size
        uint32_t obu_size = 0;
        uint32_t size_field_uleb_length = 0;

        if (!ReadObuSize(data + hdr->header_size, datasize - hdr->header_size, &obu_size, &size_field_uleb_length)) {
            return false;
        }
        hdr->payload_size = obu_size;
        hdr->header_size += size_field_uleb_length;
    }

    return true;
}

bool VulkanAV1Decoder::ParseObuTemporalDelimiter()
{
    return true;
}

void VulkanAV1Decoder::ReadTimingInfoHeader()
{
    timing_info.num_units_in_display_tick = u(32);  // Number of units in a display tick
    timing_info.time_scale = u(32);  // Time scale
    if (timing_info.num_units_in_display_tick == 0 || timing_info.time_scale == 0){
        // num_units_in_display_tick and time_scale must be greater than 0.
    }
    timing_info.equal_picture_interval = u(1);  // Equal picture interval bit
    if (timing_info.equal_picture_interval) {
        timing_info.num_ticks_per_picture = ReadUvlc() + 1;  // ticks per picture
        if (timing_info.num_ticks_per_picture == 0)
        {
            // num_ticks_per_picture_minus_1 cannot be (1 << 32) ? 1.
        }
    }
}

void VulkanAV1Decoder::ReadDecoderModelInfo()
{
    buffer_model.encoder_decoder_buffer_delay_length = u(5) + 1;
    buffer_model.num_units_in_decoding_tick = u(32);  // Number of units in a decoding tick
    buffer_model.buffer_removal_time_length = u(5) + 1;
    buffer_model.frame_presentation_time_length = u(5) + 1;
}

int VulkanAV1Decoder::ChooseOperatingPoint()
{
    int operating_point = 0;
    if (m_pClient != nullptr) {
        VkParserOperatingPointInfo OPInfo;
        memset(&OPInfo, 0, sizeof(OPInfo));

        OPInfo.eCodec = (VkVideoCodecOperationFlagBitsKHR)VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR;
        OPInfo.av1.operating_points_cnt = m_sps->operating_points_cnt_minus_1 + 1;
        for (int i = 0; i < OPInfo.av1.operating_points_cnt; i++) {
            OPInfo.av1.operating_points_idc[i] = m_sps->operating_point_idc[i];
        }

        operating_point = 0; // GetOperatingPoint was deprecated because it always returned 0 - m_pClient->GetOperatingPoint(&OPInfo);

        if (operating_point < 0) {
            assert(!"GetOperatingPoint callback failed");
            // ignoring error and continue with operating point 0
            operating_point = 0;
        }
        m_bOutputAllLayers = !!(operating_point & 0x400);
        operating_point = operating_point & ~0x400;
        if (operating_point < 0 || operating_point > m_sps->operating_points_cnt_minus_1) {
            operating_point = 0;
        }
    }
    return operating_point;
}

static int spsSequenceCounter = 0;

bool VulkanAV1Decoder::ParseObuSequenceHeader()
{
    auto prevSps = m_sps;
    VkResult result = av1_seq_param_s::Create(spsSequenceCounter++, m_sps);

    assert((result == VK_SUCCESS) && m_sps);
    if (result != VK_SUCCESS)
        return false;

    auto* sps = m_sps.Get();

	memset(&sps->color_config, 0, sizeof(sps->color_config));
	memset(&sps->timing_info, 0, sizeof(sps->timing_info));
    sps->pColorConfig = &sps->color_config;
    sps->pTimingInfo = &sps->timing_info;
    sps->seq_profile = (StdVideoAV1Profile)u(3);
    if (sps->seq_profile > STD_VIDEO_AV1_PROFILE_PROFESSIONAL) {
        // Unsupported profile
        return false;
    }

    sps->flags.still_picture = u(1);
    sps->flags.reduced_still_picture_header = u(1);

    if (!sps->flags.still_picture && sps->flags.reduced_still_picture_header) {
        // Error: Video must have reduced_still_picture_hdr == 0
        return false;
    }

    if (sps->flags.reduced_still_picture_header) {
        timing_info_present = 0;
        sps->decoder_model_info_present = 0;
        sps->display_model_info_present = 0;
        sps->operating_points_cnt_minus_1 = 0;
        sps->operating_point_idc[0] = 0;
        sps->level[0] = (StdVideoAV1Level)u(5);
        if (sps->level[0] > STD_VIDEO_AV1_LEVEL_3_3) {
            return false;
        }

        sps->tier[0] = 0;
        op_params[0].decoder_model_param_present = 0;
        op_params[0].display_model_param_present = 0;
    } else {
        timing_info_present = u(1);
        if (timing_info_present) {
            ReadTimingInfoHeader();

            sps->decoder_model_info_present = u(1);
            if (sps->decoder_model_info_present) {
                ReadDecoderModelInfo();
            }
        } else {
            sps->decoder_model_info_present = 0;
        }
        sps->display_model_info_present = u(1);
        sps->operating_points_cnt_minus_1 = u(5);
        for (int i = 0; i <= sps->operating_points_cnt_minus_1; i++) {
            sps->operating_point_idc[i] = u(12);
            sps->level[i] = (StdVideoAV1Level)u(5);
            if (!(sps->level[i] <= STD_VIDEO_AV1_LEVEL_7_3 || sps->level[i] == 31 /* LEVEL_MAX */)) {
                return false;
            }
            if (sps->level[i] > STD_VIDEO_AV1_LEVEL_3_3) {
                sps->tier[i] = u(1);
            } else {
                sps->tier[i] = 0;
            }
            if (sps->decoder_model_info_present) {
                op_params[i].decoder_model_param_present = u(1);
                if (op_params[i].decoder_model_param_present) {
                    int n = buffer_model.encoder_decoder_buffer_delay_length;
                    op_params[i].decoder_buffer_delay = u(n);
                    op_params[i].encoder_buffer_delay = u(n);
                    op_params[i].low_delay_mode_flag = u(1);
                }
            } else {
                op_params[i].decoder_model_param_present = 0;
            }
            if (sps->display_model_info_present) {
                op_params[i].display_model_param_present = u(1);
                if (op_params[i].display_model_param_present) {
                    op_params[i].initial_display_delay = u(4) + 1;
#if 0
                    if (op_params[i].initial_display_delay > 10)
                    {
                        // doesn't support delay of 10 decode frames
                        return false;
                    }
#endif
                } else {
                    op_params[i].initial_display_delay = 10;
                }
            } else {
                op_params[i].display_model_param_present = 0;
                op_params[i].initial_display_delay = 10;
            }
        }
    }

    sps->frame_width_bits_minus_1 = u(4);
    sps->frame_height_bits_minus_1 = u(4);
    sps->max_frame_width_minus_1 = u(sps->frame_width_bits_minus_1 + 1);
    sps->max_frame_height_minus_1 = u(sps->frame_height_bits_minus_1 + 1);

    if (sps->flags.reduced_still_picture_header) {
        sps->flags.frame_id_numbers_present_flag = 0;
    } else {
        sps->flags.frame_id_numbers_present_flag = u(1);
    }

    if (sps->flags.frame_id_numbers_present_flag) {
        delta_frame_id_length = u(4) + 2;
        frame_id_length = u(3) + delta_frame_id_length + 1;
        if (frame_id_length > 16) {
            // Invalid frame_id_length
            return false;
        }
    }

    sps->flags.use_128x128_superblock = u(1);
    sps->flags.enable_filter_intra = u(1);
    sps->flags.enable_intra_edge_filter = u(1);

    if (sps->flags.reduced_still_picture_header) {
        sps->flags.enable_interintra_compound = 0;
        sps->flags.enable_masked_compound = 0;
        sps->flags.enable_warped_motion = 0;
        sps->flags.enable_dual_filter = 0;
        sps->flags.enable_order_hint = 0;
        sps->flags.enable_jnt_comp = 0;
        sps->flags.enable_ref_frame_mvs = 0;
        sps->seq_force_screen_content_tools = STD_VIDEO_AV1_SELECT_SCREEN_CONTENT_TOOLS;
        sps->seq_force_integer_mv = STD_VIDEO_AV1_SELECT_INTEGER_MV;
        sps->order_hint_bits_minus_1 = 0;
    } else {
        sps->flags.enable_interintra_compound = u(1);
        sps->flags.enable_masked_compound = u(1);
        sps->flags.enable_warped_motion = u(1);
        sps->flags.enable_dual_filter = u(1);
        sps->flags.enable_order_hint = u(1);
        if (sps->flags.enable_order_hint) {
            sps->flags.enable_jnt_comp = u(1);
            sps->flags.enable_ref_frame_mvs = u(1);
        } else {
            sps->flags.enable_jnt_comp = 0;
            sps->flags.enable_ref_frame_mvs = 0;
        }

        if (u(1)) {
            sps->seq_force_screen_content_tools = STD_VIDEO_AV1_SELECT_SCREEN_CONTENT_TOOLS;
        }
        else
            sps->seq_force_screen_content_tools = u(1);

        if (sps->seq_force_screen_content_tools > 0) {
            if (u(1)) {
                sps->seq_force_integer_mv = STD_VIDEO_AV1_SELECT_INTEGER_MV;
            } else {
                sps->seq_force_integer_mv = u(1);
            }
        } else {
            sps->seq_force_integer_mv = STD_VIDEO_AV1_SELECT_INTEGER_MV;
        }
        sps->order_hint_bits_minus_1 = sps->flags.enable_order_hint ? u(3) : 0;
    }

    sps->flags.enable_superres = u(1);
    sps->flags.enable_cdef = u(1);
    sps->flags.enable_restoration = u(1);
    // color config
    bool high_bitdepth = u(1);
    if (sps->seq_profile == STD_VIDEO_AV1_PROFILE_PROFESSIONAL && high_bitdepth) {
        const bool twelve_bit = u(1);
        sps->color_config.BitDepth = twelve_bit ? 12 : 10;
    } else if (sps->seq_profile <= STD_VIDEO_AV1_PROFILE_PROFESSIONAL) {
        sps->color_config.BitDepth = high_bitdepth ? 10 : 8;
    } else {
        // Unsupported profile/bit-depth combination
		return false;
    }
    sps->color_config.BitDepth = sps->color_config.BitDepth;
    sps->color_config.flags.color_range = sps->color_config.flags.color_range;
    sps->color_config.flags.separate_uv_delta_q = sps->color_config.flags.separate_uv_delta_q;
    sps->color_config.subsampling_x = sps->color_config.subsampling_x;
    sps->color_config.subsampling_y = sps->color_config.subsampling_y;

    sps->color_config.flags.mono_chrome = sps->seq_profile != STD_VIDEO_AV1_PROFILE_HIGH ? u(1) : 0;
    sps->color_config.flags.color_description_present_flag = u(1);
    if (sps->color_config.flags.color_description_present_flag) {
        sps->color_config.color_primaries = (StdVideoAV1ColorPrimaries)u(8);
        sps->color_config.transfer_characteristics = (StdVideoAV1TransferCharacteristics)u(8);
        sps->color_config.matrix_coefficients = (StdVideoAV1MatrixCoefficients)u(8);
    } else {
        sps->color_config.color_primaries = STD_VIDEO_AV1_COLOR_PRIMARIES_BT_UNSPECIFIED;
        sps->color_config.transfer_characteristics = STD_VIDEO_AV1_TRANSFER_CHARACTERISTICS_UNSPECIFIED;
        sps->color_config.matrix_coefficients = STD_VIDEO_AV1_MATRIX_COEFFICIENTS_UNSPECIFIED;
    }

    if (sps->color_config.flags.mono_chrome) {
        sps->color_config.flags.color_range = u(1);
        sps->color_config.subsampling_x = sps->color_config.subsampling_y = 1;
        sps->color_config.flags.separate_uv_delta_q = 0;
    } else {
        if (sps->color_config.color_primaries == STD_VIDEO_AV1_COLOR_PRIMARIES_BT_709 &&
            sps->color_config.transfer_characteristics == STD_VIDEO_AV1_TRANSFER_CHARACTERISTICS_SRGB &&
            sps->color_config.matrix_coefficients == STD_VIDEO_AV1_MATRIX_COEFFICIENTS_IDENTITY) {
            sps->color_config.subsampling_y = sps->color_config.subsampling_x = 0;
            sps->color_config.flags.color_range = 1;  // assume full color-range
        } else {
            sps->color_config.flags.color_range = u(1);
            if (sps->seq_profile == STD_VIDEO_AV1_PROFILE_MAIN) {
                sps->color_config.subsampling_x = sps->color_config.subsampling_y = 1;// 420
            } else if (sps->seq_profile == STD_VIDEO_AV1_PROFILE_HIGH) {
                sps->color_config.subsampling_x = sps->color_config.subsampling_y = 0;// 444
            } else {
                if (sps->color_config.BitDepth == 12) {
                    sps->color_config.subsampling_x = u(1);
                    if (sps->color_config.subsampling_x) {
                        sps->color_config.subsampling_y = u(1);
                    } else {
                        sps->color_config.subsampling_y = 0;
                    }
                } else {
                    sps->color_config.subsampling_x = 1; //422
                    sps->color_config.subsampling_y = 0;
                }
            }
            if (sps->color_config.subsampling_x&&sps->color_config.subsampling_y) { // subsampling equals 1 1
	        sps->color_config.chroma_sample_position = (StdVideoAV1ChromaSamplePosition)u(2);
            }
        }
        sps->color_config.flags.separate_uv_delta_q = u(1);
    }
    sps->flags.film_grain_params_present = u(1);

    // check_trailing_bits()
    int bits_before_byte_alignment = 8 - (m_nalu.get_bfroffs % 8);
    int trailing = u(bits_before_byte_alignment);
    if (trailing != (1 << (bits_before_byte_alignment - 1))) {
        // trailing bits of SPS corrupted
        return false;
    }

    if (m_bSPSReceived) {
        // @review: this is not correct
	if (m_sps->isDifferentFrom(prevSps.Get()))
            m_bSPSChanged = true;
    } else {
        m_bSPSChanged = true;
    }

    m_bSPSReceived = true;

    VkSharedBaseObj<StdVideoPictureParametersSet> picParamObj(m_sps);
    m_PicData.pStdSps = picParamObj.Get();
    if (m_pClient) { // @review need to make sure this has really changed!
        bool success = m_pClient->UpdatePictureParameters(picParamObj, m_sps->client);
        assert(success);
        if (!success) {
            nvParserErrorLog("s", "\nError updating the AV1 sequence parameters\n");
        }
    }

    int operating_point = 0;
    if (m_sps->operating_points_cnt_minus_1 > 0) {
        operating_point = ChooseOperatingPoint();
    }

    m_OperatingPointIDCActive = sps->operating_point_idc[operating_point];

    return true;
}


void VulkanAV1Decoder::SetupFrameSize(int frame_size_override_flag)
{
    av1_seq_param_s *const sps = m_sps.Get();
	StdVideoDecodeAV1PictureInfo *const pStd = &m_PicData.std_info;

    if (frame_size_override_flag) {
        frame_width = u(sps->frame_width_bits_minus_1 + 1) + 1;
        frame_height = u(sps->frame_height_bits_minus_1 + 1) + 1;
        if (frame_width > (sps->max_frame_width_minus_1 + 1) || frame_height > (sps->max_frame_height_minus_1 + 1)) {
            assert(false);
        }
    } else {
        frame_width = sps->max_frame_width_minus_1 + 1;
        frame_height = sps->max_frame_height_minus_1 + 1;
    }

    //superres_params
    upscaled_width = frame_width;
    pStd->coded_denom = 0;
    uint8_t superres_scale_denominator = 8;
    pStd->flags.use_superres = 0;
    if (sps->flags.enable_superres){
        if (u(1)) {
            pStd->flags.use_superres = 1;
            superres_scale_denominator = u(3);
            pStd->coded_denom = superres_scale_denominator;
            superres_scale_denominator += SUPERRES_DENOM_MIN;
            frame_width = (upscaled_width*SUPERRES_NUM + superres_scale_denominator / 2) / superres_scale_denominator;
        }
    }

    //render size
    pStd->flags.render_and_frame_size_different = u(1);
    if (pStd->flags.render_and_frame_size_different) {
        render_width = u(16) + 1;
        render_height = u(16) + 1;

    } else {
        render_width = upscaled_width;
        render_height = frame_height;
    }
}

int VulkanAV1Decoder::SetupFrameSizeWithRefs()
{
    av1_seq_param_s *const sps = m_sps.Get();
	StdVideoDecodeAV1PictureInfo *const pStd = &m_PicData.std_info;

    uint32_t tmp;
    int32_t found, i;

    found = 0;

    for (i = 0; i < STD_VIDEO_AV1_REFS_PER_FRAME; ++i) {
        tmp = u(1);
        if (tmp) {
            found = 1;
            VkPicIf *m_pPic = m_pBuffers[ref_frame_idx[i]].buffer;
            if (m_pPic) {
                upscaled_width = m_pPic->decodeSuperResWidth;
                frame_width = m_pPic->decodeWidth;
                frame_height = m_pPic->decodeHeight;
                render_width = m_pPic->decodeWidth;
                render_height = m_pPic->decodeHeight;
            }
            break;
        }
    }

    if (!found) {
        SetupFrameSize(1);
    } else {
        //superres_params
        uint8_t superres_scale_denominator = SUPERRES_NUM;
        pStd->coded_denom = 0;
        pStd->flags.use_superres = 0;
        if (sps->flags.enable_superres) {
            if (u(1)) {
                pStd->flags.use_superres = 1;
                superres_scale_denominator = u(SUPERRES_DENOM_BITS);
                pStd->coded_denom = superres_scale_denominator;
                superres_scale_denominator += SUPERRES_DENOM_MIN;
            }
        }

        frame_width = (upscaled_width*SUPERRES_NUM + superres_scale_denominator / 2) / superres_scale_denominator;
    }

    return 1;
}

bool VulkanAV1Decoder::ReadFilmGrainParams()
{
    av1_seq_param_s *const sps = m_sps.Get();
    VkParserAv1PictureData *const pic_data = &m_PicData;
	StdVideoDecodeAV1PictureInfo *const pStd = &m_PicData.std_info;
	StdVideoAV1FilmGrain *const pFilmGrain = &m_PicData.filmGrain;

    if (sps->flags.film_grain_params_present && (pic_data->showFrame || showable_frame)) {
		pStd->flags.apply_grain = u(1);
        if (!pStd->flags.apply_grain) {
			memset(pFilmGrain, 0, sizeof(StdVideoAV1FilmGrain));
            return 1;
        }

		pFilmGrain->grain_seed = u(16);
        pFilmGrain->flags.update_grain = pStd->frame_type == STD_VIDEO_AV1_FRAME_TYPE_INTER ? u(1) : 1;

        if (!pFilmGrain->flags.update_grain) {
            // Use previous reference frame film grain params
            int buf_idx = u(3);
            uint16_t random_seed = pFilmGrain->grain_seed;
            if (m_pBuffers[buf_idx].buffer) {
                memcpy(pFilmGrain, &(m_pBuffers[buf_idx].film_grain_params), sizeof(StdVideoAV1FilmGrain));
            }
            pFilmGrain->grain_seed = random_seed;
            pFilmGrain->film_grain_params_ref_idx = buf_idx;
            return 1;
        }

        // Scaling functions parameters
        pFilmGrain->num_y_points = u(4);
        if (pFilmGrain->num_y_points > STD_VIDEO_AV1_MAX_NUM_Y_POINTS) {
            assert("num_y_points exceeds the maximum value\n");
        }

        for (uint32_t i = 0; i < pFilmGrain->num_y_points; i++) {
            pFilmGrain->point_y_value[i] = u(8);
            if (i && pFilmGrain->point_y_value[i-1] >= pFilmGrain->point_y_value[i]) {
                assert(!"Y cordinates should be increasing\n");
            }
            pFilmGrain->point_y_scaling[i] = u(8);
        }

        pFilmGrain->flags.chroma_scaling_from_luma = !sps->color_config.flags.mono_chrome ? u(1) : 0;

        if (sps->color_config.flags.mono_chrome || pFilmGrain->flags.chroma_scaling_from_luma ||
            ((sps->color_config.subsampling_x == 1) && (sps->color_config.subsampling_y == 1) && (pFilmGrain->num_y_points == 0))) {
            pFilmGrain->num_cb_points = 0;
            pFilmGrain->num_cr_points = 0;
        } else {
            pFilmGrain->num_cb_points = u(4);
            if (pFilmGrain->num_cb_points > STD_VIDEO_AV1_MAX_NUM_CR_POINTS) {
                assert(!"num_cb_points exceeds the maximum value\n");
            }

            for (uint32_t i = 0; i < pFilmGrain->num_cb_points; i++) {
                pFilmGrain->point_cb_value[i] = u(8);
                if (i && pFilmGrain->point_cb_value[i-1] >= pFilmGrain->point_cb_value[i]) {
                    assert(!"cb cordinates should be increasing\n");
                }
                pFilmGrain->point_cb_scaling[i] = u(8);
            }

            pFilmGrain->num_cr_points = u(4);
            if (pFilmGrain->num_cr_points > STD_VIDEO_AV1_MAX_NUM_CR_POINTS) {
                assert(!"num_cr_points exceeds the maximum value\n");
            }

            for (uint32_t i = 0; i < pFilmGrain->num_cr_points; i++) {
                pFilmGrain->point_cr_value[i] = u(8);
                if (i && pFilmGrain->point_cr_value[i-1] >= pFilmGrain->point_cr_value[i]) {
                    assert(!"cr cordinates should be increasing\n");
                }
                pFilmGrain->point_cr_scaling[i] = u(8);
            }
        }

        pFilmGrain->grain_scaling_minus_8 = u(2);
        pFilmGrain->ar_coeff_lag = u(2);

        int numPosLuma = 2 * pFilmGrain->ar_coeff_lag * (pFilmGrain->ar_coeff_lag + 1);
		assert(numPosLuma <= STD_VIDEO_AV1_MAX_NUM_POS_LUMA);
        int numPosChroma = numPosLuma;
        if (pFilmGrain->num_y_points > 0) {
            ++numPosChroma;
        }
		assert(numPosChroma <= STD_VIDEO_AV1_MAX_NUM_POS_CHROMA);

        if (pFilmGrain->num_y_points) {
            for (int i = 0; i < numPosLuma; i++) {
                pFilmGrain->ar_coeffs_y_plus_128[i] = u(8);
            }
        }

        if (pFilmGrain->num_cb_points || pFilmGrain->flags.chroma_scaling_from_luma) {
            for (int i = 0; i < numPosChroma; i++) {
                pFilmGrain->ar_coeffs_cb_plus_128[i] = u(8);
            }
        }

        if (pFilmGrain->num_cr_points || pFilmGrain->flags.chroma_scaling_from_luma) {
            for (int i = 0; i < numPosChroma; i++) {
                pFilmGrain->ar_coeffs_cr_plus_128[i] = u(8);
            }
        }

        pFilmGrain->ar_coeff_shift_minus_6 = u(2);
        pFilmGrain->grain_scale_shift = u(2);

        if (pFilmGrain->num_cb_points) {
            pFilmGrain->cb_mult = u(8);
            pFilmGrain->cb_luma_mult = u(8);
            pFilmGrain->cb_offset = u(9);
        }

        if (pFilmGrain->num_cr_points) {
            pFilmGrain->cr_mult = u(8);
            pFilmGrain->cr_luma_mult = u(8);
            pFilmGrain->cr_offset = u(9);
        }

        pFilmGrain->flags.overlap_flag = u(1);
        pFilmGrain->flags.clip_to_restricted_range = u(1);
    } else {
        memset(pFilmGrain, 0, sizeof(StdVideoAV1FilmGrain));
    }

    return true;
}

static uint32_t tile_log2(int blk_size, int target)
{
    uint32_t k;

    for (k = 0; (blk_size << k) < target; k++) {
    }

    return k;
}

uint32_t FloorLog2(uint32_t x)
{
    int s = 0;

    while (x != 0) {
        x = x >> 1;
        s++;
    }

    return s - 1;
}
uint32_t VulkanAV1Decoder::SwGetUniform(uint32_t max_value)
{
    uint32_t w = FloorLog2(max_value) + 1;
    uint32_t m = (1 << w) - max_value;
    uint32_t v = u(w - 1);
    if (v < m) {
        return v;
    }
    uint32_t extral_bit = u(1);
    return (v << 1) - m + extral_bit;
}

bool VulkanAV1Decoder::DecodeTileInfo()
{
    av1_seq_param_s *const sps = m_sps.Get();
    const auto& seq_hdr_flags = sps->flags;
    VkParserAv1PictureData *const pic_data = &m_PicData;
	StdVideoAV1TileInfo *const pTileInfo = &m_PicData.tileInfo;

    int mi_cols = 2 * ((frame_width + 7) >> 3);
    int mi_rows = 2 * ((frame_height + 7) >> 3);

    int min_log2_tile_rows;
    uint32_t max_tile_height_sb;

    // Macroblock dimensions to superblock dimenseions
    uint32_t sb_cols = seq_hdr_flags.use_128x128_superblock ? ((mi_cols + 31) >> 5) : ((mi_cols + 15) >> 4);
    uint32_t sb_rows = seq_hdr_flags.use_128x128_superblock ? ((mi_rows + 31) >> 5) : ((mi_rows + 15) >> 4);
    int numSuperblocks = sb_cols * sb_rows;
    int sb_shift = seq_hdr_flags.use_128x128_superblock ? 5 : 4;
    int sb_size = sb_shift + 2;

    uint32_t max_tile_width_sb = MAX_TILE_WIDTH >> sb_size;
    uint32_t max_tile_area_sb = MAX_TILE_AREA >> (2 * sb_size);
    uint32_t min_log2_tile_cols = tile_log2(max_tile_width_sb, sb_cols);
    uint32_t max_log2_tile_cols = tile_log2(1, std::min(sb_cols, (uint32_t)STD_VIDEO_AV1_MAX_TILE_COLS));
    uint32_t max_log2_tile_rows = tile_log2(1, std::min(sb_rows, (uint32_t)STD_VIDEO_AV1_MAX_TILE_ROWS));
    uint32_t min_log2_tiles = std::max(min_log2_tile_cols, tile_log2(max_tile_area_sb, sb_rows * sb_cols));

	pTileInfo->flags.uniform_tile_spacing_flag = u(1);
    memset(&pic_data->MiColStarts[0], 0, sizeof(pic_data->MiColStarts));
    memset(&pic_data->MiRowStarts[0], 0, sizeof(pic_data->MiRowStarts));
    memset(&pic_data->width_in_sbs_minus_1[0], 0, sizeof(pic_data->width_in_sbs_minus_1));
    memset(&pic_data->height_in_sbs_minus_1[0], 0, sizeof(pic_data->height_in_sbs_minus_1));

    if (pTileInfo->flags.uniform_tile_spacing_flag) {
        int tile_width_sb, tile_height_sb;
        log2_tile_cols = min_log2_tile_cols;
        while (log2_tile_cols < max_log2_tile_cols) {
            if (!u(1))
                break;
            log2_tile_cols++;
        }

        tile_width_sb = (sb_cols + (1 << log2_tile_cols) - 1) >> log2_tile_cols;
        for (uint32_t off = 0, i = 0; off < sb_cols; off += tile_width_sb)
            pic_data->MiColStarts[i++] = off;

        pic_data->tileInfo.TileCols = (sb_cols + tile_width_sb - 1) / tile_width_sb;

        min_log2_tile_rows = std::max(int(min_log2_tiles - log2_tile_cols), 0);
        log2_tile_rows = min_log2_tile_rows;
        while (log2_tile_rows < max_log2_tile_rows) {
            if (!u(1))
                break;
            log2_tile_rows++;
        }

        tile_height_sb = (sb_rows + (1 << log2_tile_rows) - 1) >> log2_tile_rows;
        for (uint32_t off = 0, i = 0; off < sb_rows; off += tile_height_sb)
            pic_data->MiRowStarts[i++] = off;

        pic_data->tileInfo.TileRows = (sb_rows + tile_height_sb - 1) / tile_height_sb;

        // Derive tile_width_in_sbs_minus_1 and tile_height_in_sbs_minus_1
        uint32_t tile_col = 0;
         for ( ; tile_col < pic_data->tileInfo.TileCols - 1u; tile_col++)
            pic_data->width_in_sbs_minus_1[tile_col] = tile_width_sb - 1;
        pic_data->width_in_sbs_minus_1[tile_col] = sb_cols - (pic_data->tileInfo.TileCols - 1) * tile_width_sb - 1;

        uint32_t tile_row = 0;
         for ( ; tile_row < pic_data->tileInfo.TileRows - 1u; tile_row++)
            pic_data->height_in_sbs_minus_1[tile_row] = tile_height_sb - 1;
        pic_data->height_in_sbs_minus_1[tile_row] = sb_rows - (pic_data->tileInfo.TileRows - 1) * tile_height_sb - 1;

        // Derivce superblock column / row start positions
        uint32_t i, start_sb;
        for (i = 0, start_sb = 0; start_sb < sb_cols; i++) {
            pic_data->MiColStarts[i] = start_sb;
            start_sb += tile_width_sb;
        }
        pic_data->MiColStarts[i] = sb_cols;

        for (i = 0, start_sb = 0; start_sb < sb_rows; i++) {
            pic_data->MiRowStarts[i] = start_sb;
            start_sb += tile_height_sb;
        }
        pic_data->MiRowStarts[i] = sb_rows;
    } else {
        uint32_t i, widest_tile_sb, start_sb, size_sb, max_width, max_height;
        widest_tile_sb = 0;
        start_sb = 0;

        start_sb = 0;
        for (i = 0; start_sb < sb_cols && i < STD_VIDEO_AV1_MAX_TILE_COLS; i++) {
            pic_data->MiColStarts[i] = start_sb;
            max_width = std::min(sb_cols - start_sb, max_tile_width_sb);
            pic_data->width_in_sbs_minus_1[i] = (max_width > 1) ? SwGetUniform(max_width) : 0;
            size_sb = pic_data->width_in_sbs_minus_1[i] + 1;
            widest_tile_sb = std::max(size_sb, widest_tile_sb);
            start_sb += size_sb;
        }
        log2_tile_cols = tile_log2(1, i);
        pic_data->tileInfo.TileCols = i;

        if (min_log2_tiles > 0)
            max_tile_area_sb = (numSuperblocks) >> (min_log2_tiles + 1);
        else
            max_tile_area_sb = numSuperblocks;
        max_tile_height_sb = std::max(max_tile_area_sb / widest_tile_sb, 1u);

        start_sb = 0;
        for (i = 0; start_sb < sb_rows && i < STD_VIDEO_AV1_MAX_TILE_ROWS; i++) {
            pic_data->MiRowStarts[i] = start_sb;
            max_height = std::min(sb_rows - start_sb, max_tile_height_sb);
            pic_data->height_in_sbs_minus_1[i] = (max_height > 1) ? SwGetUniform(max_height) : 0;
            size_sb = pic_data->height_in_sbs_minus_1[i] + 1;
            start_sb += size_sb;
        }
        log2_tile_rows = tile_log2(1, i);
        pic_data->tileInfo.TileRows = i;
    }

    pic_data->tileInfo.context_update_tile_id = 0;
    tile_size_bytes_minus_1 = 3;
    if (pic_data->tileInfo.TileRows * pic_data->tileInfo.TileCols > 1) {
        // tile to use for cdf update
        pic_data->tileInfo.context_update_tile_id = u(log2_tile_rows + log2_tile_cols);
        // tile size magnitude
        tile_size_bytes_minus_1 = u(2);
        pic_data->tileInfo.tile_size_bytes_minus_1 = tile_size_bytes_minus_1;
    }

    return true;
}

inline int VulkanAV1Decoder::ReadSignedBits(uint32_t bits)
{
    const int nbits = sizeof(uint32_t) * 8 - bits - 1;
    uint32_t v = (uint32_t)u(bits + 1) << nbits;
    return ((int)v) >> nbits;
}

inline int VulkanAV1Decoder::ReadDeltaQ(uint32_t bits)
{
    return u(1) ? ReadSignedBits(bits) : 0;
}

void VulkanAV1Decoder::DecodeQuantizationData()
{
    av1_seq_param_s *const sps = m_sps.Get();
    VkParserAv1PictureData *const pic_data = &m_PicData;

    pic_data->quantization.base_q_idx = u(8);
    pic_data->quantization.DeltaQYDc = ReadDeltaQ(6);
    if (!sps->color_config.flags.mono_chrome) {
        int diff_uv_delta = 0;
        if (sps->color_config.flags.separate_uv_delta_q)
            diff_uv_delta = u(1);
        pic_data->quantization.DeltaQUDc = ReadDeltaQ(6);
        pic_data->quantization.DeltaQUAc = ReadDeltaQ(6);
        if (diff_uv_delta) {
            pic_data->quantization.DeltaQVDc = ReadDeltaQ(6);
            pic_data->quantization.DeltaQVAc = ReadDeltaQ(6);
        } else {
            pic_data->quantization.DeltaQVDc = pic_data->quantization.DeltaQUDc;
            pic_data->quantization.DeltaQVAc = pic_data->quantization.DeltaQUAc;
        }
    } else {
        pic_data->quantization.DeltaQUDc = 0;
        pic_data->quantization.DeltaQUAc = 0;
        pic_data->quantization.DeltaQVDc = 0;
        pic_data->quantization.DeltaQVAc = 0;
    }

    pic_data->quantization.flags.using_qmatrix = u(1);
    if (pic_data->quantization.flags.using_qmatrix) {
        pic_data->quantization.qm_y = u(4);
        pic_data->quantization.qm_u = u(4);
        if (!sps->color_config.flags.separate_uv_delta_q) {
            pic_data->quantization.qm_v = pic_data->quantization.qm_u;
        } else {
            pic_data->quantization.qm_v = u(4);
        }
    } else {
        pic_data->quantization.qm_y = 0;
        pic_data->quantization.qm_u = 0;
        pic_data->quantization.qm_v = 0;
    }
}

static const int av1_seg_feature_data_signed[STD_VIDEO_AV1_SEG_LVL_MAX] = { 1, 1, 1, 1, 1, 0, 0, 0 };
static const int av1_seg_feature_Bits[STD_VIDEO_AV1_SEG_LVL_MAX] = { 8, 6, 6, 6, 6, 3, 0, 0 };
static const int av1_seg_feature_data_max[STD_VIDEO_AV1_MAX_SEGMENTS] = { 255, 63, 63, 63, 63, 7, 0, 0 };

void VulkanAV1Decoder::DecodeSegmentationData()
{
	StdVideoDecodeAV1PictureInfo *const pStd = &m_PicData.std_info;
	StdVideoAV1Segmentation* pSegmentation = &m_PicData.segmentation;
    StdVideoDecodeAV1PictureInfoFlags *const flags = &pStd->flags;

    flags->segmentation_enabled = u(1);

    if (!flags->segmentation_enabled) {
        memset(pSegmentation, 0, sizeof(*pSegmentation));
        return;
    }

    if (pStd->primary_ref_frame == STD_VIDEO_AV1_PRIMARY_REF_NONE) {
        flags->segmentation_update_map = 1;
        flags->segmentation_update_data = 1;
        flags->segmentation_temporal_update = 0;
    } else {
        flags->segmentation_update_map = u(1);

        if (flags->segmentation_update_map) {
            flags->segmentation_temporal_update = u(1);
        } else {
            flags->segmentation_temporal_update = 0;
        }

        flags->segmentation_update_data = u(1);
    }

    if (flags->segmentation_update_data) {
        for (int i = 0; i < STD_VIDEO_AV1_MAX_SEGMENTS; i++) {
		    pSegmentation->FeatureEnabled[i] = 0;
            for (int j = 0; j < STD_VIDEO_AV1_SEG_LVL_MAX; j++) {
                int feature_value = 0;
				int enabled = u(1);
				pSegmentation->FeatureEnabled[i] |= enabled << j;
                if (enabled) {
                    const int data_max = av1_seg_feature_data_max[j];
                    if (av1_seg_feature_data_signed[j]) {
                        feature_value = ReadSignedBits(av1_seg_feature_Bits[j]);
                        feature_value = CLAMP(feature_value, -data_max, data_max);
                    } else {
                        feature_value = u(av1_seg_feature_Bits[j]);
                        feature_value = CLAMP(feature_value, 0, data_max);
                    }
                }
                pSegmentation->FeatureData[i][j] = feature_value;
            }
        }
    } else {
        if (pStd->primary_ref_frame != STD_VIDEO_AV1_PRIMARY_REF_NONE) {
            // overwrite default values with prev frame data
            int prim_buf_idx = ref_frame_idx[pStd->primary_ref_frame];
            if (m_pBuffers[prim_buf_idx].buffer) {
                memcpy(pSegmentation->FeatureEnabled, m_pBuffers[prim_buf_idx].seg.FeatureEnabled, STD_VIDEO_AV1_MAX_SEGMENTS * sizeof(pSegmentation->FeatureEnabled[0]));
                memcpy(pSegmentation->FeatureData, m_pBuffers[prim_buf_idx].seg.FeatureData, STD_VIDEO_AV1_MAX_SEGMENTS * STD_VIDEO_AV1_SEG_LVL_MAX * sizeof(pSegmentation->FeatureData[0][0]));
            }
        }
    }
}

static const char lf_ref_delta_default[] = { 1, 0, 0, 0, (char)-1, 0, (char)-1, (char)-1 };

void VulkanAV1Decoder::DecodeLoopFilterdata()
{
    av1_seq_param_s *const sps = m_sps.Get();
	StdVideoDecodeAV1PictureInfo* pStd = &m_PicData.std_info;
	StdVideoAV1LoopFilter* pLoopFilter = &m_PicData.loopFilter;

	pLoopFilter->loop_filter_level[2] = pLoopFilter->loop_filter_level[3] = 0; // level_u = level_v = 0
    memcpy(pLoopFilter->loop_filter_ref_deltas, lf_ref_delta_default, STD_VIDEO_AV1_TOTAL_REFS_PER_FRAME);
    memset(pLoopFilter->loop_filter_mode_deltas, 0, STD_VIDEO_AV1_LOOP_FILTER_ADJUSTMENTS);

    if (pStd->flags.allow_intrabc || coded_lossless) {
        pLoopFilter->loop_filter_level[0] = pLoopFilter->loop_filter_level[1] = 0;
        return;
    }

    if (pStd->primary_ref_frame != STD_VIDEO_AV1_PRIMARY_REF_NONE) {
        // overwrite default values with prev frame data
        int prim_buf_idx = ref_frame_idx[pStd->primary_ref_frame];
        if (m_pBuffers[prim_buf_idx].buffer) {
            memcpy(pLoopFilter->loop_filter_ref_deltas, m_pBuffers[prim_buf_idx].lf_ref_delta, sizeof(lf_ref_delta_default));
            memcpy(pLoopFilter->loop_filter_mode_deltas, m_pBuffers[prim_buf_idx].lf_mode_delta, sizeof(pLoopFilter->loop_filter_mode_deltas));
        }
    }

    pLoopFilter->loop_filter_level[0] = u(6);
    pLoopFilter->loop_filter_level[1] = u(6);
    if (!sps->color_config.flags.mono_chrome && (pLoopFilter->loop_filter_level[0] || pLoopFilter->loop_filter_level[1])) {
        pLoopFilter->loop_filter_level[2] = u(6); // loop_filter_level_u
        pLoopFilter->loop_filter_level[3] = u(6); // loop_filter_level_v
    }
    pLoopFilter->loop_filter_sharpness = u(3);

    uint8_t lf_mode_ref_delta_update = 0;
    pLoopFilter->flags.loop_filter_delta_enabled = u(1);
    if (pLoopFilter->flags.loop_filter_delta_enabled) {
        lf_mode_ref_delta_update = u(1);
        pLoopFilter->flags.loop_filter_delta_update = lf_mode_ref_delta_update;
        if (lf_mode_ref_delta_update) {
            for (int i = 0; i < STD_VIDEO_AV1_TOTAL_REFS_PER_FRAME; i++) {
                if (u(1)) {
                    pLoopFilter->loop_filter_ref_deltas[i] = ReadSignedBits(6);
                }
            }

            for (int i = 0; i < STD_VIDEO_AV1_LOOP_FILTER_ADJUSTMENTS; i++) {
                if (u(1)) {
                    pLoopFilter->loop_filter_mode_deltas[i] = ReadSignedBits(6);
                }
            }
        }
    }
}

void VulkanAV1Decoder::DecodeCDEFdata()
{
    av1_seq_param_s *const sps = m_sps.Get();
	StdVideoDecodeAV1PictureInfo *const pStd = &m_PicData.std_info;
	StdVideoAV1CDEF* pCDEF = &m_PicData.CDEF;

    if (pStd->flags.allow_intrabc)
        return;

    pCDEF->cdef_damping_minus_3 = u(2);
    pCDEF->cdef_bits = u(2);

    for (int i = 0; i < 8; i++) {
        if (i == (1 << pCDEF->cdef_bits)) {
            break;
        }
        pCDEF->cdef_y_pri_strength[i] = u(4);
        pCDEF->cdef_y_sec_strength[i] = u(2);
        if (!sps->color_config.flags.mono_chrome) {
            pCDEF->cdef_uv_pri_strength[i] = u(4);
            pCDEF->cdef_uv_sec_strength[i] = u(2);
        }
    }
}

void VulkanAV1Decoder::DecodeLoopRestorationData()
{
    av1_seq_param_s *const sps = m_sps.Get();
	StdVideoDecodeAV1PictureInfo* pStd = &m_PicData.std_info;
	StdVideoAV1LoopRestoration* pLoopRestoration = &m_PicData.loopRestoration;

    if (pStd->flags.allow_intrabc) {
        return;
    }

    int n_planes = sps->color_config.flags.mono_chrome ? 1 : 3;
    bool use_lr = false, use_chroma_lr = false;

    StdVideoAV1FrameRestorationType remap_lr_type[4] = { STD_VIDEO_AV1_FRAME_RESTORATION_TYPE_NONE, STD_VIDEO_AV1_FRAME_RESTORATION_TYPE_SWITCHABLE, STD_VIDEO_AV1_FRAME_RESTORATION_TYPE_WIENER, STD_VIDEO_AV1_FRAME_RESTORATION_TYPE_SGRPROJ };
    for (int pl = 0; pl < n_planes; pl++) {
        uint8_t lr_type = u(2);
        pLoopRestoration->FrameRestorationType[pl] = remap_lr_type[lr_type];

        if (pLoopRestoration->FrameRestorationType[pl] != STD_VIDEO_AV1_FRAME_RESTORATION_TYPE_NONE) {
            use_lr = true;
            if (pl > 0) {
                use_chroma_lr = true;
            }
        }
    }
    pStd->flags.UsesLr = use_lr;
    if (use_lr)  {
        int lr_unit_shift = 0;
        int sb_size = sps->flags.use_128x128_superblock == 1 /*BLOCK_128X128*/ ? 2 : 1; //128 : 64;

        for (int pl = 0; pl < n_planes; pl++) {
            pLoopRestoration->LoopRestorationSize[pl] = sb_size;  // 64 or 128
        }
        if (sps->flags.use_128x128_superblock == 1) {
            lr_unit_shift = 1 + u(1);
        } else {
            lr_unit_shift = u(1);
            if (lr_unit_shift) {
                lr_unit_shift += u(1);
            }
        }
        pLoopRestoration->LoopRestorationSize[0] = 1 + lr_unit_shift;
    } else {
        for (int pl = 0; pl < n_planes; pl++)
            pLoopRestoration->LoopRestorationSize[pl] = 3;
    }
    uint8_t lr_uv_shift = 0;

    if (!sps->color_config.flags.mono_chrome) {
        if (use_chroma_lr && (sps->color_config.subsampling_x && sps->color_config.subsampling_y)) {
            lr_uv_shift = u(1);
            pLoopRestoration->LoopRestorationSize[1] = pLoopRestoration->LoopRestorationSize[0] - lr_uv_shift;
            pLoopRestoration->LoopRestorationSize[2] = pLoopRestoration->LoopRestorationSize[1];
        } else {
            pLoopRestoration->LoopRestorationSize[1] = pLoopRestoration->LoopRestorationSize[0];
            pLoopRestoration->LoopRestorationSize[2] = pLoopRestoration->LoopRestorationSize[0];
        }
    }
    pLoopRestoration->LoopRestorationSize[1] = pLoopRestoration->LoopRestorationSize[0] >> lr_uv_shift;
    pLoopRestoration->LoopRestorationSize[1] = pLoopRestoration->LoopRestorationSize[1] >> lr_uv_shift;
}

int VulkanAV1Decoder::GetRelativeDist1(int a, int b)
{
    av1_seq_param_s *const sps = m_sps.Get();
    if (!sps->flags.enable_order_hint) {
        return 0;
    }

    const int bits = sps->order_hint_bits_minus_1 + 1;

    assert(bits >= 1);
    assert(a >= 0 && a < (1 << bits));
    assert(b >= 0 && b < (1 << bits));

    int diff = a - b;
    const int m = 1 << (bits - 1);
    diff = (diff & (m - 1)) - (diff & m);
    return diff;
}

//follow spec 7.8
void VulkanAV1Decoder::SetFrameRefs(int last_frame_idx, int gold_frame_idx)
{
    av1_seq_param_s *const sps = m_sps.Get();
	StdVideoDecodeAV1PictureInfo* pStd = &m_PicData.std_info;


    assert(sps->flags.enable_order_hint);
    assert(sps->order_hint_bits_minus_1 >= 0);

    const int curFrameHint = 1 << sps->order_hint_bits_minus_1;

    int shiftedOrderHints[STD_VIDEO_AV1_NUM_REF_FRAMES];
    int Ref_OrderHint;
    int usedFrame[STD_VIDEO_AV1_NUM_REF_FRAMES];
    int hint;
    for (int i = 0; i < STD_VIDEO_AV1_REFS_PER_FRAME; i++) {
        ref_frame_idx[i] = -1;
    }

    for (int i = 0; i < STD_VIDEO_AV1_NUM_REF_FRAMES; i++) {
        usedFrame[i] = 0;
    }

    ref_frame_idx[STD_VIDEO_AV1_REFERENCE_NAME_LAST_FRAME - STD_VIDEO_AV1_REFERENCE_NAME_LAST_FRAME] = last_frame_idx;
    ref_frame_idx[STD_VIDEO_AV1_REFERENCE_NAME_GOLDEN_FRAME - STD_VIDEO_AV1_REFERENCE_NAME_LAST_FRAME] = gold_frame_idx;
    usedFrame[last_frame_idx] = 1;
    usedFrame[gold_frame_idx] = 1;

    for (int i = 0; i < STD_VIDEO_AV1_NUM_REF_FRAMES; i++) {
        Ref_OrderHint = RefOrderHint[i];
        shiftedOrderHints[i] = curFrameHint + GetRelativeDist1(Ref_OrderHint, pStd->OrderHint);
    }

    {//ALTREF_FRAME
        int ref = -1;
        int latestOrderHint = -1;
        for (int i = 0; i < STD_VIDEO_AV1_NUM_REF_FRAMES; i++) {
            hint = shiftedOrderHints[i];
            if (!usedFrame[i] &&
                hint >= curFrameHint &&
                (ref < 0 || hint >= latestOrderHint)) {
                ref = i;
                latestOrderHint = hint;
            }
        }
        if (ref >= 0) {
            ref_frame_idx[STD_VIDEO_AV1_REFERENCE_NAME_ALTREF_FRAME - STD_VIDEO_AV1_REFERENCE_NAME_LAST_FRAME] = ref;
            usedFrame[ref] = 1;
        }
    }

    {//BWDREF_FRAME
        int ref = -1;
        int earliestOrderHint = -1;
        for (int i = 0; i < STD_VIDEO_AV1_NUM_REF_FRAMES; i++) {
            hint = shiftedOrderHints[i];
            if (!usedFrame[i] &&
                hint >= curFrameHint &&
                (ref < 0 || hint < earliestOrderHint)) {
                ref = i;
                earliestOrderHint = hint;
            }
        }
        if (ref >= 0) {
            ref_frame_idx[STD_VIDEO_AV1_REFERENCE_NAME_BWDREF_FRAME - STD_VIDEO_AV1_REFERENCE_NAME_LAST_FRAME] = ref;
            usedFrame[ref] = 1;
        }
    }

    {//ALTREF2_FRAME
        int ref = -1;
        int earliestOrderHint = -1;
        for (int i = 0; i < STD_VIDEO_AV1_NUM_REF_FRAMES; i++) {
            hint = shiftedOrderHints[i];
            if (!usedFrame[i] &&
                hint >= curFrameHint &&
                (ref < 0 || hint < earliestOrderHint)) {
                ref = i;
                earliestOrderHint = hint;
            }
        }
        if (ref >= 0) {
            ref_frame_idx[STD_VIDEO_AV1_REFERENCE_NAME_ALTREF2_FRAME - STD_VIDEO_AV1_REFERENCE_NAME_LAST_FRAME] = ref;
            usedFrame[ref] = 1;
        }
    }

    uint32_t Ref_Frame_List[STD_VIDEO_AV1_REFS_PER_FRAME - 2] = {
		STD_VIDEO_AV1_REFERENCE_NAME_LAST2_FRAME,
		STD_VIDEO_AV1_REFERENCE_NAME_LAST3_FRAME,
		STD_VIDEO_AV1_REFERENCE_NAME_BWDREF_FRAME,
		STD_VIDEO_AV1_REFERENCE_NAME_ALTREF2_FRAME,
		STD_VIDEO_AV1_REFERENCE_NAME_ALTREF_FRAME
	};

    for (int j = 0; j < STD_VIDEO_AV1_REFS_PER_FRAME - 2; j++) {
        int refFrame = Ref_Frame_List[j];
        if (ref_frame_idx[refFrame - STD_VIDEO_AV1_REFERENCE_NAME_LAST_FRAME] < 0) {
            int ref = -1;
            int latestOrderHint = -1;
            for (int i = 0; i < STD_VIDEO_AV1_NUM_REF_FRAMES; i++) {
                hint = shiftedOrderHints[i];
                if (!usedFrame[i] &&
                    hint < curFrameHint &&
                    (ref < 0 || hint >= latestOrderHint)) {
                    ref = i;
                    latestOrderHint = hint;
                }
            }
            if (ref >= 0) {
                ref_frame_idx[refFrame - STD_VIDEO_AV1_REFERENCE_NAME_LAST_FRAME] = ref;
                usedFrame[ref] = 1;
            }
        }
    }

    {
        int ref = -1;
        int earliestOrderHint = -1;
        for (int i = 0; i < STD_VIDEO_AV1_NUM_REF_FRAMES; i++) {
            hint = shiftedOrderHints[i];
            if (ref < 0 || hint < earliestOrderHint) {
                ref = i;
                earliestOrderHint = hint;
            }
        }
        for (int i = 0; i < STD_VIDEO_AV1_REFS_PER_FRAME; i++) {
            if (ref_frame_idx[i] < 0) {
                ref_frame_idx[i] = ref;
            }
        }
    }

}


int VulkanAV1Decoder::IsSkipModeAllowed()
{
    av1_seq_param_s *const sps = m_sps.Get();
    StdVideoDecodeAV1PictureInfoFlags* pic_flags = &m_PicData.std_info.flags;
	StdVideoDecodeAV1PictureInfo* pStd = &m_PicData.std_info;

    if (!sps->flags.enable_order_hint || IsFrameIntra() ||
        pic_flags->reference_select == 0) {
        return 0;
    }

    // Identify the nearest forward and backward references.
    int ref0 = -1, ref1 = -1;
    int ref0_off = -1, ref1_off = -1;
    for (int i = 0; i < STD_VIDEO_AV1_REFS_PER_FRAME; i++) {
        int frame_idx = ref_frame_idx[i];
        if (frame_idx != -1) {
            int ref_frame_offset = RefOrderHint[frame_idx];

            int rel_off = GetRelativeDist1(ref_frame_offset, pStd->OrderHint);
            // Forward reference
            if (rel_off < 0
                && (ref0_off == -1 || GetRelativeDist1(ref_frame_offset, ref0_off) > 0)) {
                ref0 = i + STD_VIDEO_AV1_REFERENCE_NAME_LAST_FRAME;
                ref0_off = ref_frame_offset;
            }
            // Backward reference
            if (rel_off > 0
                && (ref1_off == -1 || GetRelativeDist1(ref_frame_offset, ref1_off) < 0)) {
                ref1 = i + STD_VIDEO_AV1_REFERENCE_NAME_LAST_FRAME;
                ref1_off = ref_frame_offset;
            }
        }
    }

    if (ref0 != -1 && ref1 != -1) {
        // == Bi-directional prediction ==
        pStd->SkipModeFrame[0] = std::min(ref0, ref1);
        pStd->SkipModeFrame[1] = std::max(ref0, ref1);
        return 1;
    } else if (ref0 != -1) {
        // == Forward prediction only ==
        // Identify the second nearest forward reference.
        for (int i = 0; i < STD_VIDEO_AV1_REFS_PER_FRAME; i++) {
            int frame_idx = ref_frame_idx[i];
            if (frame_idx != -1) {
                int ref_frame_offset = RefOrderHint[frame_idx];
                // Forward reference
                if (GetRelativeDist1(ref_frame_offset, ref0_off) < 0
                    && (ref1_off == -1 || GetRelativeDist1(ref_frame_offset, ref1_off) > 0)) {
                    ref1 = i + STD_VIDEO_AV1_REFERENCE_NAME_LAST_FRAME;
                    ref1_off = ref_frame_offset;
                }
            }
        }
        if (ref1 != -1) {
            pStd->SkipModeFrame[0] = std::min(ref0, ref1);
            pStd->SkipModeFrame[1] = std::max(ref0, ref1);
            return 1;
        }
    }

    return 0;
}

bool VulkanAV1Decoder::ParseObuFrameHeader()
{
    av1_seq_param_s *const sps = m_sps.Get();
    VkParserAv1PictureData *const pic_info = &m_PicData;
	StdVideoDecodeAV1PictureInfo *const pStd = &m_PicData.std_info;

    StdVideoDecodeAV1PictureInfoFlags* pic_flags = &pStd->flags;

    pic_flags->frame_size_override_flag = 0;

    last_frame_type = pStd->frame_type;
    last_intra_only = intra_only;

    if (sps->flags.reduced_still_picture_header) {
        show_existing_frame = 0;
        showable_frame = 0;
        pic_info->showFrame = 1;
        pStd->frame_type = STD_VIDEO_AV1_FRAME_TYPE_KEY;
        pic_flags->error_resilient_mode = 1;
    } else {
        uint8_t reset_decoder_state = 0;
        show_existing_frame = u(1);

        if (show_existing_frame) {
            int32_t frame_to_show_map_idx = u(3);
            int32_t show_existing_frame_index = frame_to_show_map_idx;

            if (sps->decoder_model_info_present && timing_info.equal_picture_interval == 0) {
                tu_presentation_delay = u(buffer_model.frame_presentation_time_length);
            }
            if (sps->flags.frame_id_numbers_present_flag) {
                int display_frame_id = u(frame_id_length);

                if (display_frame_id != ref_frame_id[frame_to_show_map_idx] ||
                    RefValid[frame_to_show_map_idx] == 0) {
                    assert(!"ref frame ID mismatch");
                }
            }
            if (!m_pBuffers[show_existing_frame_index].buffer) {
                assert("Error: Frame not decoded yet\n");
                return false;
            }

            reset_decoder_state = m_pBuffers[show_existing_frame_index].frame_type == STD_VIDEO_AV1_FRAME_TYPE_KEY;
            m_PicData.loopFilter.loop_filter_level[0] = m_PicData.loopFilter.loop_filter_level[1] = 0;
            pic_info->showFrame = 1;
            showable_frame = m_pBuffers[show_existing_frame_index].showable_frame;

            if (sps->flags.film_grain_params_present) {
                memcpy(&m_PicData.filmGrain, &m_pBuffers[show_existing_frame_index].film_grain_params, sizeof(StdVideoAV1FilmGrain));
            }

            if (reset_decoder_state) {
                showable_frame = 0;
                pStd->frame_type = STD_VIDEO_AV1_FRAME_TYPE_KEY;
                pStd->refresh_frame_flags = (1 << STD_VIDEO_AV1_NUM_REF_FRAMES) - 1;
                // load loop filter params
                memcpy(m_PicData.loopFilter.loop_filter_ref_deltas, m_pBuffers[show_existing_frame_index].lf_ref_delta, sizeof(lf_ref_delta_default));
                memcpy(m_PicData.loopFilter.loop_filter_mode_deltas, m_pBuffers[show_existing_frame_index].lf_mode_delta, sizeof(pStd->pLoopFilter->loop_filter_mode_deltas));
                // load global motions
                memcpy(&global_motions, &m_pBuffers[show_existing_frame_index].global_models, sizeof(AV1WarpedMotionParams) * GM_GLOBAL_MODELS_PER_FRAME);
                // load segmentation
                memcpy(pic_info->segmentation.FeatureEnabled, m_pBuffers[show_existing_frame_index].seg.FeatureEnabled, STD_VIDEO_AV1_MAX_SEGMENTS * sizeof(pic_info->segmentation.FeatureEnabled[0]));
                memcpy(pic_info->segmentation.FeatureData, m_pBuffers[show_existing_frame_index].seg.FeatureData, STD_VIDEO_AV1_MAX_SEGMENTS * STD_VIDEO_AV1_SEG_LVL_MAX * sizeof(pic_info->segmentation.FeatureData[0][0]));
                pStd->OrderHint = RefOrderHint[show_existing_frame_index];
                UpdateFramePointers(m_pBuffers[show_existing_frame_index].buffer);
            } else {
                pStd->refresh_frame_flags = 0;
            }

            VkPicIf* pDispPic = m_pBuffers[show_existing_frame_index].buffer;
            if (pDispPic)
                pDispPic->AddRef();
            AddBuffertoOutputQueue(pDispPic, !!showable_frame);

            return true;
        }

        pStd->frame_type = (StdVideoAV1FrameType)u(2);
        intra_only = pStd->frame_type == STD_VIDEO_AV1_FRAME_TYPE_INTRA_ONLY;

        pic_info->showFrame = u(1);
        if (pic_info->showFrame) {
            if (sps->decoder_model_info_present && timing_info.equal_picture_interval == 0) {
                tu_presentation_delay = u(buffer_model.frame_presentation_time_length);
            }
            showable_frame = pStd->frame_type != STD_VIDEO_AV1_FRAME_TYPE_KEY;
        } else {
            showable_frame = u(1);
        }

        pic_flags->error_resilient_mode = (pStd->frame_type == STD_VIDEO_AV1_FRAME_TYPE_SWITCH || (pStd->frame_type == STD_VIDEO_AV1_FRAME_TYPE_KEY && pic_info->showFrame)) ? 1 : u(1);
    }


    if (pStd->frame_type == STD_VIDEO_AV1_FRAME_TYPE_KEY && pic_info->showFrame) {
        for (int i = 0; i < STD_VIDEO_AV1_NUM_REF_FRAMES; i++) {
            RefValid[i] = 0;
            RefOrderHint[i] = 0;
        }
    }

    pic_flags->disable_cdf_update = u(1);
    if (sps->seq_force_screen_content_tools == STD_VIDEO_AV1_SELECT_SCREEN_CONTENT_TOOLS) {
        pic_flags->allow_screen_content_tools = u(1);
    } else {
        pic_flags->allow_screen_content_tools = sps->seq_force_screen_content_tools;
    }

    if (pic_flags->allow_screen_content_tools) {
        if (sps->seq_force_integer_mv == STD_VIDEO_AV1_SELECT_INTEGER_MV) {
            pic_flags->force_integer_mv = u(1);
        }
        else {
            pic_flags->force_integer_mv = sps->seq_force_integer_mv;
        }
    } else {
        pic_flags->force_integer_mv = 0;
    }

    if (IsFrameIntra()) {
        pic_flags->force_integer_mv = 1;
    }

    pic_flags->frame_refs_short_signaling = 0;
    pic_flags->allow_intrabc = 0;
    pStd->primary_ref_frame = STD_VIDEO_AV1_PRIMARY_REF_NONE;
    pic_flags->frame_size_override_flag = 0;

    if (!sps->flags.reduced_still_picture_header) {
        if (sps->flags.frame_id_numbers_present_flag) {
            int diff_len = delta_frame_id_length;
            int prev_frame_id = 0;
            if (!(pStd->frame_type == STD_VIDEO_AV1_FRAME_TYPE_KEY && pic_info->showFrame)) {
                prev_frame_id = pStd->current_frame_id;
            }
            pStd->current_frame_id = u(frame_id_length);

            if (!(pStd->frame_type == STD_VIDEO_AV1_FRAME_TYPE_KEY && pic_info->showFrame)) {
                int diff_frame_id = 0;
                assert(prev_frame_id >= 0);
                if (pStd->current_frame_id > (uint32_t)prev_frame_id) {
                    diff_frame_id = pStd->current_frame_id - (uint32_t)prev_frame_id;
                } else {
                    diff_frame_id = (1 << frame_id_length) + pStd->current_frame_id - (uint32_t)prev_frame_id;
                }
                // check for conformance
                if (((uint32_t)prev_frame_id == pStd->current_frame_id) || (diff_frame_id >= (1 << (frame_id_length - 1)))) {
                    // Invalid pStd->current_frame_id
                    // return 0;
                }
            }
            // Mark ref frames not valid for referencing
            assert(diff_len >= 0);
            for (int i = 0; i < STD_VIDEO_AV1_NUM_REF_FRAMES; i++) {
                if (pStd->frame_type == STD_VIDEO_AV1_FRAME_TYPE_KEY && pic_info->showFrame) {
                    RefValid[i] = 0;
                } else if (ref_frame_id[i] < 0) {
                    RefValid[i] = 0;
                } else if (pStd->current_frame_id > (uint32_t)(1 << diff_len)) {
                    assert(ref_frame_id[i] >= 0);
                    if (((uint32_t)ref_frame_id[i] > pStd->current_frame_id) ||
                        ((uint32_t)ref_frame_id[i] < pStd->current_frame_id - (uint32_t)(1 << diff_len))) {

                        RefValid[i] = 0;
                    }
                } else {
                    assert(ref_frame_id[i] >= 0);
                    if (((uint32_t)ref_frame_id[i] > pStd->current_frame_id) &&
                        ((uint32_t)ref_frame_id[i] < (1 << frame_id_length) + pStd->current_frame_id - (1 << diff_len))) {

                        RefValid[i] = 0;
                    }
                }
            }
        } else {
            pStd->current_frame_id = 0;
        }

        pic_flags->frame_size_override_flag = pStd->frame_type == STD_VIDEO_AV1_FRAME_TYPE_SWITCH ? 1 : u(1);
        //order_hint
        pStd->OrderHint = sps->flags.enable_order_hint ? u(sps->order_hint_bits_minus_1 + 1) : 0;

        if (!pic_flags->error_resilient_mode && !(IsFrameIntra())) {
            pStd->primary_ref_frame = u(3);
        }
    }

    if (sps->decoder_model_info_present) {
        pic_flags->buffer_removal_time_present_flag = u(1);
        if (pic_flags->buffer_removal_time_present_flag) {
            for (int opNum = 0; opNum <= sps->operating_points_cnt_minus_1; opNum++) {
                if (op_params[opNum].decoder_model_param_present) {
                    int opPtIdc = sps->operating_point_idc[opNum];
                    int inTemporalLayer = (opPtIdc >> temporal_id) & 1;
                    int inSpatialLyaer = (opPtIdc >> (spatial_id + 8)) & 1;
                    if (opPtIdc == 0 || (inTemporalLayer&&inSpatialLyaer)) {
                        op_frame_timing[opNum] = u(buffer_model.buffer_removal_time_length);
                    } else {
                        op_frame_timing[opNum] = 0;
                    }
                } else {
                    op_frame_timing[opNum] = 0;
                }
            }
        }
    }
    if (pStd->frame_type == STD_VIDEO_AV1_FRAME_TYPE_KEY) {
        if (!pic_info->showFrame) {
            pStd->refresh_frame_flags = u(8);
        } else {
            pStd->refresh_frame_flags = (1 << STD_VIDEO_AV1_NUM_REF_FRAMES) - 1;
        }

        for (int i = 0; i < STD_VIDEO_AV1_REFS_PER_FRAME; i++) {
            ref_frame_idx[i] = 0;
        }

        // memset(&ref_frame_names, -1, sizeof(ref_frame_names));
    } else {
        if (intra_only || pStd->frame_type != 3) {
            pStd->refresh_frame_flags = u(STD_VIDEO_AV1_NUM_REF_FRAMES);
            if (pStd->refresh_frame_flags == 0xFF && intra_only) {
                assert(!"Intra_only frames cannot have refresh flags 0xFF");
            }

            //  memset(&ref_frame_names, -1, sizeof(ref_frame_names));
        } else {
            pStd->refresh_frame_flags = (1 << STD_VIDEO_AV1_NUM_REF_FRAMES) - 1;
        }
    }

    if (((!IsFrameIntra()) || pStd->refresh_frame_flags != 0xFF) &&
        pic_flags->error_resilient_mode && sps->flags.enable_order_hint) {
        for (int buf_idx = 0; buf_idx < STD_VIDEO_AV1_NUM_REF_FRAMES; buf_idx++) {
            // ref_order_hint[i]
            int offset = u(sps->order_hint_bits_minus_1 + 1);
            // assert(buf_idx < FRAME_BUFFERS);
            if (buf_idx == -1 || offset != RefOrderHint[buf_idx]) {
                //RefValid[buf_idx] = 0;
                //RefOrderHint[buf_idx] = frame_offset;
                assert(0);
            }
        }
    }

    if (IsFrameIntra()) {
        SetupFrameSize(pic_flags->frame_size_override_flag);

        if (pic_flags->allow_screen_content_tools && frame_width == upscaled_width) {
            pic_flags->allow_intrabc = u(1);
        }
        pic_flags->use_ref_frame_mvs = 0;
    } else {
        pic_flags->use_ref_frame_mvs = 0;
        // if (pbi->need_resync != 1)
        {
            if (sps->flags.enable_order_hint) {
                pic_flags->frame_refs_short_signaling = u(1);
            } else {
                pic_flags->frame_refs_short_signaling = 0;
            }

            if (pic_flags->frame_refs_short_signaling) {
                const int lst_ref = u(REF_FRAMES_BITS);
                const int lst_idx = lst_ref;

                const int gld_ref = u(REF_FRAMES_BITS);
                const int gld_idx = gld_ref;

                if (lst_idx == -1 || gld_idx == -1) {
                    assert(!"invalid reference");
                }

                SetFrameRefs(lst_ref, gld_ref);
            }

            for (int i = 0; i < STD_VIDEO_AV1_REFS_PER_FRAME; i++) {
                if (!pic_flags->frame_refs_short_signaling) {
                    int ref_frame_index = u(REF_FRAMES_BITS);
                    ref_frame_idx[i] = ref_frame_index;

                    if (ref_frame_index == -1) {
                        assert(!"invalid reference");
                    }
                    ref_frame_idx[i] = ref_frame_index;
                }

                if (sps->flags.frame_id_numbers_present_flag) {
                    int diff_len = delta_frame_id_length;
                    int delta_frame_id_minus_1 = u(diff_len);
                    int ref_id = ((pStd->current_frame_id - (delta_frame_id_minus_1 + 1) +
                        (1 << frame_id_length)) % (1 << frame_id_length));

                    if (ref_id != ref_frame_id[ref_frame_idx[i]] || RefValid[ref_frame_idx[i]] == 0) {
                        //assert(!"Ref frame ID mismatch");
                    }
                }
            }

            if (!pic_flags->error_resilient_mode && pic_flags->frame_size_override_flag) {
                SetupFrameSizeWithRefs();
            } else {
                SetupFrameSize(pic_flags->frame_size_override_flag);
            }

            if (pic_flags->force_integer_mv) {
                pic_flags->allow_high_precision_mv = 0;
            } else {
                pic_flags->allow_high_precision_mv = u(1);
            }

            //read_interpolation_filter
            int tmp = u(1);
            pic_flags->is_filter_switchable = tmp;
            if (tmp) {
                pStd->interpolation_filter = STD_VIDEO_AV1_INTERPOLATION_FILTER_SWITCHABLE;
            } else {
                pStd->interpolation_filter = (StdVideoAV1InterpolationFilter)u(2);
            }
            pic_flags->is_motion_mode_switchable = u(1);
        }

        if (!pic_flags->error_resilient_mode && sps->flags.enable_ref_frame_mvs &&
            sps->flags.enable_order_hint && !IsFrameIntra()) {
            pic_flags->use_ref_frame_mvs = u(1);
        } else {
            pic_flags->use_ref_frame_mvs = 0;
        }

        // According to AV1 specification: "5.9.2. Uncompressed header syntax"
        for (int i = 0; i < STD_VIDEO_AV1_REFS_PER_FRAME; i++) {
            // Range check ref_frame_idx, RefOrderHint[] needs to be of size: BUFFER_POOL_MAX_SIZE.
            if ((ref_frame_idx[i] >= BUFFER_POOL_MAX_SIZE) && (ref_frame_idx[i] < 0)) {
                assert(false);
            }

            pStd->OrderHints[i + STD_VIDEO_AV1_REFERENCE_NAME_LAST_FRAME] = RefOrderHint[ref_frame_idx[i]];
         }

        /*      for (int i = 0; i < REFS_PER_FRAME; ++i)
                {
                    RefBuffer *const ref_buf = &cm->frame_refs[i];
                    av1_setup_scale_factors_for_frame(
                        &ref_buf->sf, ref_buf->buf->y_crop_width,
                        ref_buf->buf->y_crop_height, cm->width, cm->height);
                    if ((!av1_is_valid_scale(&ref_buf->sf)))
                        aom_internal_error(&cm->error, AOM_CODEC_UNSUP_BITSTREAM,
                            "Reference frame has invalid dimensions");
                }
        */
    }

    if (sps->flags.frame_id_numbers_present_flag) {
        // Update reference frame id's
        int tmp_flags = pStd->refresh_frame_flags;
        for (int i = 0; i < STD_VIDEO_AV1_NUM_REF_FRAMES; i++) {
            if ((tmp_flags >> i) & 1) {
                ref_frame_id[i] = pStd->current_frame_id;
                RefValid[i] = 1;
            }
        }
    }

    if (!(sps->flags.reduced_still_picture_header) && !(pic_flags->disable_cdf_update)) {
        pic_flags->disable_frame_end_update_cdf = u(1);
    } else {
        pic_flags->disable_frame_end_update_cdf = 1;
    }

    // tile_info
    DecodeTileInfo();
    DecodeQuantizationData();
    DecodeSegmentationData();

    pStd->delta_q_res = 0;
    pStd->delta_lf_res = 0;
    pic_flags->delta_lf_present = 0;
    pic_flags->delta_lf_multi = 0;
    pic_flags->delta_q_present = pic_info->quantization.base_q_idx > 0 ? u(1) : 0;
    if (pic_flags->delta_q_present) {
        pStd->delta_q_res = u(2); // 1 << u(2); use log2(). Shift is done at HW
        if (!pic_flags->allow_intrabc) {
            pic_flags->delta_lf_present = u(1);
        }
        if (pic_flags->delta_lf_present) {
            pStd->delta_lf_res = u(2); //1 << u(2);
            pic_flags->delta_lf_multi = u(1);
            // av1_reset_loop_filter_delta(xd, av1_num_planes(cm)); // FIXME
        }
    }

    for (int i = 0; i < STD_VIDEO_AV1_MAX_SEGMENTS; ++i) {
        int qindex = pic_flags->segmentation_enabled && (pic_info->segmentation.FeatureEnabled[i] & 0)
            ? pic_info->segmentation.FeatureData[i][0] + pic_info->quantization.base_q_idx : pic_info->quantization.base_q_idx;
        qindex = CLAMP(qindex, 0, 255);
        lossless[i] = qindex == 0 && pic_info->quantization.DeltaQYDc == 0 &&
            pic_info->quantization.DeltaQUDc == 0 && pic_info->quantization.DeltaQUAc == 0 &&
            pic_info->quantization.DeltaQVDc == 0 && pic_info->quantization.DeltaQVAc == 0;
    }

    coded_lossless = lossless[0];
    if (pic_flags->segmentation_enabled) {
        for (int i = 1; i < STD_VIDEO_AV1_MAX_SEGMENTS; i++) {
            coded_lossless &= lossless[i];
        }
    }

    all_lossless = coded_lossless && (frame_width == upscaled_width);
    // setup_segmentation_dequant();  //FIXME
    if (coded_lossless) {
        m_PicData.loopFilter.loop_filter_level[0] = 0;
        m_PicData.loopFilter.loop_filter_level[1] = 0;
    }
    if (coded_lossless || !sps->flags.enable_cdef) {
        pic_info->CDEF.cdef_bits = 0;
        //cm->cdef_strengths[0] = 0;
        //cm->cdef_uv_strengths[0] = 0;
    }
    if (all_lossless || !sps->flags.enable_restoration) {
        m_PicData.loopRestoration.FrameRestorationType[0] = STD_VIDEO_AV1_FRAME_RESTORATION_TYPE_NONE;
        m_PicData.loopRestoration.FrameRestorationType[1] = STD_VIDEO_AV1_FRAME_RESTORATION_TYPE_NONE;
        m_PicData.loopRestoration.FrameRestorationType[2] = STD_VIDEO_AV1_FRAME_RESTORATION_TYPE_NONE;
    }
    DecodeLoopFilterdata();

    if (!coded_lossless && sps->flags.enable_cdef && !pic_flags->allow_intrabc) {
        DecodeCDEFdata();
    }
    if (!all_lossless && sps->flags.enable_restoration && !pic_flags->allow_intrabc) {
        DecodeLoopRestorationData();
    }

    pStd->TxMode = coded_lossless ? STD_VIDEO_AV1_TX_MODE_ONLY_4X4 : (u(1) ? STD_VIDEO_AV1_TX_MODE_SELECT : STD_VIDEO_AV1_TX_MODE_LARGEST);
    if (!IsFrameIntra()) {
        pic_flags->reference_select = u(1);
    } else {
        pic_flags->reference_select = 0;
    }

    pic_flags->skip_mode_present = IsSkipModeAllowed() ? u(1) : 0;

    if (!IsFrameIntra() && !pic_flags->error_resilient_mode && sps->flags.enable_warped_motion) {
        pic_flags->allow_warped_motion = u(1);
    } else {
        pic_flags->allow_warped_motion = 0;
    }

    pic_flags->reduced_tx_set = u(1);

    // reset global motions
    for (int i = 0; i < GM_GLOBAL_MODELS_PER_FRAME; ++i) {
        global_motions[i] = default_warp_params;
    }

    if (!IsFrameIntra()) {
        DecodeGlobalMotionParams();
    }

    ReadFilmGrainParams();

    return true;
}


bool VulkanAV1Decoder::ParseObuTileGroup(const AV1ObuHeader& hdr)
{
	int num_tiles = m_PicData.tileInfo.TileCols * m_PicData.tileInfo.TileRows;

	// Tile group header
    int log2_num_tiles = log2_tile_cols + log2_tile_rows;
    bool tile_start_and_end_present_flag = 0;
    if (num_tiles > 1) {
        tile_start_and_end_present_flag = !!(u(1));
    }
    // "For OBU_FRAME type obu tile_start_and_end_present_flag must be 0"
    if (hdr.type == AV1_OBU_FRAME && tile_start_and_end_present_flag) {
		return false;
	}

	int tg_start = 0;
	int tg_end = 0;
    if (num_tiles == 1 || !tile_start_and_end_present_flag) {
        tg_start = 0;
        tg_end = num_tiles - 1;
    } else {
        tg_start = u(log2_num_tiles);
        tg_end = u(log2_num_tiles);
    }

	byte_alignment();
	// Tile payload
    int consumedBytes = (consumed_bits() + 7) / 8;
    assert(consumedBytes > 0);
    assert((m_nalu.start_offset <= UINT32_MAX) && (m_nalu.start_offset >= 0));
	//                                                    offset of obu         number of bytes read getting the tile data

	// Compute the tile group size
    for (int TileNum = tg_start; TileNum <= tg_end; TileNum++)
    {
        int lastTile = TileNum == tg_end;
        size_t tileSize = 0;
        if (lastTile)
        {
            tileSize = hdr.payload_size - consumedBytes;
            m_PicData.tileOffsets[m_PicData.khr_info.tileCount] = (uint32_t)m_nalu.start_offset + (uint32_t)consumedBytes;
        }
        else
        {
            uint64_t tile_size_minus_1 = le(tile_size_bytes_minus_1 + 1);
            consumedBytes += tile_size_bytes_minus_1 + 1;
            m_PicData.tileOffsets[m_PicData.khr_info.tileCount] = (uint32_t)m_nalu.start_offset + (uint32_t)consumedBytes;

            tileSize = tile_size_minus_1 + 1;
            consumedBytes += (uint32_t)tileSize;

            skip_bits((uint32_t)(tileSize * 8));
        }

        m_PicData.tileSizes[m_PicData.khr_info.tileCount] = (uint32_t)tileSize;
        m_PicData.khr_info.tileCount++;
    }

    return (tg_end == num_tiles - 1);
}

bool IsObuInCurrentOperatingPoint(int  current_operating_point, AV1ObuHeader *hdr) {
    if (current_operating_point == 0) return true;
    if (((current_operating_point >> hdr->temporal_id) & 0x1) &&
        ((current_operating_point >> (hdr->spatial_id + 8)) & 0x1)) {
        return true;
    }

    return false;
}

bool VulkanAV1Decoder::ParseOneFrame(const uint8_t*const pFrameStart, const int32_t frameSizeBytes, const VkParserBitstreamPacket* pck, int* pParsedBytes)
{
    m_bSPSChanged = false;
    AV1ObuHeader hdr;

	const uint8_t* pCurrOBU = pFrameStart;
	int32_t remainingFrameBytes = frameSizeBytes;

    while (remainingFrameBytes > 0) {
        memset(&hdr, 0, sizeof(hdr));
		// NOTE: This does not modify any bitstream reader stare.
        if (!ParseOBUHeaderAndSize(pCurrOBU, remainingFrameBytes, &hdr)) {
            // OBU header parsing failed
            return false;
        }

        if (remainingFrameBytes < int(hdr.payload_size + hdr.header_size)) {
            // Error: Truncated frame data
            return false;
        }

        m_nalu.start_offset += hdr.header_size;

        temporal_id = hdr.temporal_id;
        spatial_id = hdr.spatial_id;
        if (hdr.type != AV1_OBU_TEMPORAL_DELIMITER && hdr.type != AV1_OBU_SEQUENCE_HEADER && hdr.type != AV1_OBU_PADDING) {
            if (!IsObuInCurrentOperatingPoint(m_OperatingPointIDCActive, &hdr)) { // TODO: || !DecodeAllLayers
                m_nalu.start_offset += hdr.payload_size;
                pCurrOBU  += (hdr.payload_size + hdr.header_size);
                remainingFrameBytes -= (hdr.payload_size + hdr.header_size);
                continue;
            }
        }

		// Prime the bit buffer with the 4 bytes
        init_dbits();
        switch (hdr.type) {
        case AV1_OBU_TEMPORAL_DELIMITER:
            ParseObuTemporalDelimiter();
			memset(m_PicData.tileOffsets, 0, sizeof(m_PicData.tileOffsets));
			memset(m_PicData.tileSizes, 0, sizeof(m_PicData.tileSizes));
			m_PicData.khr_info.tileCount = 0;
            break;

        case AV1_OBU_SEQUENCE_HEADER:
            ParseObuSequenceHeader();
            break;

        case AV1_OBU_FRAME_HEADER:
        case AV1_OBU_FRAME:
        {
			memset(m_PicData.tileOffsets, 0, sizeof(m_PicData.tileOffsets));
			m_PicData.khr_info.tileCount = 0;
			memset(m_PicData.tileSizes, 0, sizeof(m_PicData.tileSizes));

            ParseObuFrameHeader();

            if (show_existing_frame) break;
            if (hdr.type != AV1_OBU_FRAME) {
                rbsp_trailing_bits();
            }

            if (hdr.type != AV1_OBU_FRAME) break;

			byte_alignment();
        }   // fall through

        case AV1_OBU_TILE_GROUP:
        {
            if (ParseObuTileGroup(hdr)) {
				// Last tile group for this frame
                if (!end_of_picture(frameSizeBytes))
                    return false;
            }

            break;
        }
        case AV1_OBU_REDUNDANT_FRAME_HEADER:
        case AV1_OBU_PADDING:
        case AV1_OBU_METADATA:
        default:
            break;
        }

		// The header was skipped over to parse the payload.
        m_nalu.start_offset += hdr.payload_size;

        pCurrOBU += (hdr.payload_size + hdr.header_size);
        remainingFrameBytes -= (hdr.payload_size + hdr.header_size);

        assert(remainingFrameBytes >= 0);
    }

    if (pParsedBytes) { // TODO: How is this useful with a boolean return value?
        *pParsedBytes += (int)pck->nDataLength;
    }

    return true;
}

bool VulkanAV1Decoder::ParseByteStream(const VkParserBitstreamPacket* pck, size_t* pParsedBytes)
{
    const uint8_t* pdataStart = (pck->nDataLength > 0) ? pck->pByteStream : nullptr;
    const uint8_t* pdataEnd = (pck->nDataLength > 0) ? pck->pByteStream + pck->nDataLength : nullptr;
    int datasize = (int)pck->nDataLength;

    if (pParsedBytes) {
        *pParsedBytes = 0;
    }

    if (m_bitstreamData.GetBitstreamPtr() == nullptr) {
        // make sure we're initialized
        return false;
    }

    m_nCallbackEventCount = 0;

    // Handle discontinuity
    if (pck->bDiscontinuity) {
        memset(&m_nalu, 0, sizeof(m_nalu));
        memset(&m_PTSQueue, 0, sizeof(m_PTSQueue));
        m_bDiscontinuityReported = true;
    }

    if (pck->bPTSValid) {
        m_PTSQueue[m_lPTSPos].bPTSValid = true;
        m_PTSQueue[m_lPTSPos].llPTS = pck->llPTS;
        m_PTSQueue[m_lPTSPos].llPTSPos = m_llParsedBytes;
        m_PTSQueue[m_lPTSPos].bDiscontinuity = m_bDiscontinuityReported;
        m_bDiscontinuityReported = false;
        m_lPTSPos = (m_lPTSPos + 1) % MAX_QUEUED_PTS;
    }

    // Decode in serial mode.
    while (pdataStart < pdataEnd) {
        uint32_t frame_size = 0;
        frame_size = datasize;

        if (frame_size > (uint32_t)m_bitstreamDataLen) {
            if (!resizeBitstreamBuffer(frame_size - (m_bitstreamDataLen))) {
                // Error: Failed to resize bitstream buffer
                return false;
            }
        }

        if (datasize > 0) {
            m_nalu.start_offset = 0;
            m_nalu.end_offset = frame_size;
            memcpy(m_bitstreamData.GetBitstreamPtr(), pdataStart, frame_size);
            m_llNaluStartLocation = m_llFrameStartLocation = m_llParsedBytes; // TODO: NaluStart and FrameStart are always the same here
            m_llParsedBytes += frame_size;

        }
        int parsedBytes = 0;
        if (!ParseOneFrame(pdataStart, frame_size, pck, &parsedBytes)) {
            return false;
        }

		if (pParsedBytes)
			*pParsedBytes = parsedBytes;

        pdataStart += frame_size;
        // Allow extra zero bytes after the frame end
        while (pdataStart < pdataEnd) {
            const uint8_t marker = pdataStart[0];
            if (marker) break;
            ++pdataStart;
        }
    }

    // display frames from output queue
    int index = 0;
    while (index < m_numOutFrames) {
        AddBuffertoDispQueue(m_pOutFrame[index]);
        lEndPicture(m_pOutFrame[index], !m_showableFrame[index]);
        if (m_pOutFrame[index]) {
            m_pOutFrame[index]->Release();
            m_pOutFrame[index] = nullptr;
        }
        index++;
    }
    m_numOutFrames = 0;

    // flush if EOS set
    if (pck->bEOS) {
        end_of_stream();
    }

    return true;
}

const char* av1_seq_param_s::m_refClassId = "av1SpsVideoPictureParametersSet";
