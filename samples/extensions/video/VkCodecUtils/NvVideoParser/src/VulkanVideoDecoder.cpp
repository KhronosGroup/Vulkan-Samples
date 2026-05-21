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

#include <stdarg.h>
#include "vkvideo_parser/VulkanVideoParserIf.h"
#include "VulkanVideoDecoder.h"
#include "nvVulkanVideoUtils.h"
#include "nvVulkanVideoParser.h"
#include <algorithm>
#ifdef ENABLE_VP9_DECODER
#include <VulkanVP9Decoder.h>
#endif

VulkanVideoDecoder::VulkanVideoDecoder(VkVideoCodecOperationFlagBitsKHR std)
    : m_refCount(0)
    , m_standard(std)
    , m_264SvcEnabled(false)
    , m_outOfBandPictureParameters(false)
    , m_initSequenceIsCalled(false)
    , m_pClient()
    , m_defaultMinBufferSize(2 * 1024 * 1024)
    , m_bufferOffsetAlignment(256)
    , m_bufferSizeAlignment(256)
    , m_bitstreamData()
    , m_bitstreamDataLen()
    , m_BitBfr()
    , m_bEmulBytesPresent()
    , m_bNoStartCodes(false)
    , m_bFilterTimestamps(false)
    , m_MaxFrameBuffers()
    , m_nalu()
    , m_lMinBytesForBoundaryDetection(256)
    , m_lClockRate()
    , m_lFrameDuration()
    , m_llExpectedPTS()
    , m_llParsedBytes()
    , m_llNaluStartLocation()
    , m_llFrameStartLocation()
    , m_lErrorThreshold()
    , m_bFirstPTS()
    , m_lPTSPos()
    , m_nCallbackEventCount()
    , m_PrevSeqInfo()
    , m_ExtSeqInfo()
    , m_DispInfo{}
    , m_PTSQueue{}
    , m_bDiscontinuityReported()
    , m_pVkPictureData()
    , m_iTargetLayer(0)
    , m_bDecoderInitFailed()
    , m_lCheckPTS()
    , m_eError(NV_NO_ERROR)
{
    if (m_264SvcEnabled) {
        m_pVkPictureData = new VkParserPictureData[128];
    } else {
        m_pVkPictureData = new VkParserPictureData;
    }
}


VulkanVideoDecoder::~VulkanVideoDecoder()
{
    if (m_264SvcEnabled)
    {
        delete [] m_pVkPictureData;
        m_pVkPictureData = NULL;
    }
    else
    {
        delete m_pVkPictureData;
        m_pVkPictureData = NULL;
    }
}


VkResult VulkanVideoDecoder::Initialize(const VkParserInitDecodeParameters *pParserPictureData)
{
    if (pParserPictureData->interfaceVersion != NV_VULKAN_VIDEO_PARSER_API_VERSION) {
        return VK_ERROR_INCOMPATIBLE_DRIVER;
    }

    Deinitialize();
    m_pClient = pParserPictureData->pClient;
    m_defaultMinBufferSize  = pParserPictureData->defaultMinBufferSize;
    m_bufferOffsetAlignment = pParserPictureData->bufferOffsetAlignment;
    m_bufferSizeAlignment   = pParserPictureData->bufferSizeAlignment;
    m_outOfBandPictureParameters = pParserPictureData->outOfBandPictureParameters;
    m_lClockRate = (pParserPictureData->referenceClockRate > 0) ? pParserPictureData->referenceClockRate : 10000000; // Use 10Mhz as default clock
    m_lErrorThreshold = pParserPictureData->errorThreshold;
    m_bDiscontinuityReported = false;
    m_lFrameDuration = 0;
    m_llExpectedPTS = 0;
    m_bNoStartCodes = false;
    m_bFilterTimestamps = false;
    m_lCheckPTS = 16;
    m_bEmulBytesPresent = false;
    m_bFirstPTS = true;
    if (pParserPictureData->pExternalSeqInfo) {
        m_ExtSeqInfo = *pParserPictureData->pExternalSeqInfo;
    } else {
        memset(&m_ExtSeqInfo, 0, sizeof(m_ExtSeqInfo));
    }

    m_bitstreamDataLen = m_defaultMinBufferSize; // dynamically increase size if it's not enough
    VkSharedBaseObj<VulkanBitstreamBuffer> bitstreamBuffer;
    m_pClient->GetBitstreamBuffer(m_bitstreamDataLen,
                                  m_bufferOffsetAlignment, m_bufferSizeAlignment,
                                  nullptr, 0, bitstreamBuffer);
    assert(bitstreamBuffer);
    if (!bitstreamBuffer) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    m_bitstreamDataLen = m_bitstreamData.SetBitstreamBuffer(bitstreamBuffer);
    CreatePrivateContext();
    memset(&m_nalu, 0, sizeof(m_nalu));
    memset(&m_PrevSeqInfo, 0, sizeof(m_PrevSeqInfo));
    memset(&m_DispInfo, 0, sizeof(m_DispInfo));
    memset(&m_PTSQueue, 0, sizeof(m_PTSQueue));
    m_bitstreamData.ResetStreamMarkers();
    m_BitBfr = (uint32_t)~0;
    m_MaxFrameBuffers = 0;
    m_bDecoderInitFailed = false;
    m_llParsedBytes = 0;
    m_llNaluStartLocation = 0;
    m_llFrameStartLocation = 0;
    m_lPTSPos = 0;
    InitParser();
    memset(&m_nalu, 0, sizeof(m_nalu)); // reset nalu again (in case parser used init_dbits during initialization)
    m_NextStartCode = check_simd_support();

    return VK_SUCCESS;
}


bool VulkanVideoDecoder::Deinitialize()
{
    FreeContext();
    m_bitstreamData.ResetBitstreamBuffer();
    return true;
}


void VulkanVideoDecoder::init_dbits()
{
    m_nalu.get_offset = m_nalu.start_offset + ((m_bNoStartCodes) ? 0 : 3);  // Skip over start_code_prefix
    m_nalu.get_zerocnt = 0;
    m_nalu.get_emulcnt = 0;
    m_nalu.get_bfr = 0;
    // prime bit buffer
    m_nalu.get_bfroffs = 32;
    skip_bits(0);
}


void VulkanVideoDecoder::skip_bits(uint32_t n)
{
    m_nalu.get_bfroffs += n;
    while (m_nalu.get_bfroffs >= 8)
    {
        m_nalu.get_bfr <<= 8;
        if (m_nalu.get_offset < m_nalu.end_offset)
        {
            VkDeviceSize c = m_bitstreamData[m_nalu.get_offset++];
            if (m_bEmulBytesPresent)
            {
                // detect / discard emulation_prevention_three_byte
                if (m_nalu.get_zerocnt == 2)
                {
                    if (c == 3)
                    {
                        m_nalu.get_zerocnt = 0;
                        c = (m_nalu.get_offset < m_nalu.end_offset) ? m_bitstreamData[m_nalu.get_offset] : 0;
                        m_nalu.get_offset++;
                        m_nalu.get_emulcnt++;
                    }
                }
                if (c != 0)
                    m_nalu.get_zerocnt = 0;
                else
                    m_nalu.get_zerocnt += (m_nalu.get_zerocnt < 2);
            }
            m_nalu.get_bfr |= c;
        } else
        {
            m_nalu.get_offset++;
        }
        m_nalu.get_bfroffs -= 8;
    }
}

void VulkanVideoDecoder::rbsp_trailing_bits()
{
    f(1, 1); // rbsp_stop_one_bit
    while (!byte_aligned())
        f(1, 0); // rbsp_alignment_zero_bit
}

bool VulkanVideoDecoder::more_rbsp_data()
{
    // If the NAL unit contains any non-zero bits past the next bit we have more RBSP data.
    // These non-zero bits may either already be in bfr (first check)
    // or may not have been read yet (second check).
    // Note that the assumption that end() == false implies that there are more unread
    // non-zero bits is invalid for CABAC slices (because of cabac_zero_word). This is not
    // a problem because more_rbsp_data is not used in CABAC slices. 
    return (m_nalu.get_bfr << (m_nalu.get_bfroffs+1)) != 0 || !end();
}

uint32_t VulkanVideoDecoder::u(uint32_t n)
{
    uint32_t bits = 0;
    
    if (n > 0)
    {
        if (n + m_nalu.get_bfroffs <= 32)
        {
            bits = next_bits(n);
            skip_bits(n);
        } else
        {
            // n == 26..32
            bits = next_bits(n-25) << 25;
            skip_bits(n-25);
            bits |= next_bits(25);
            skip_bits(25);
        }
    }
    return bits;
}


// 9.1
uint32_t VulkanVideoDecoder::ue()
{
    int leadingZeroBits, b, codeNum;

    leadingZeroBits = -1;
    for (b = 0; (!b) && (leadingZeroBits<32); leadingZeroBits++)
        b = u(1);

    codeNum = 0;
    if (leadingZeroBits < 32)
    {
        codeNum = (1 << leadingZeroBits) - 1 + u(leadingZeroBits);
    } else
    {
        codeNum = 0xffffffff + u(leadingZeroBits);
    }
    return codeNum;
}


// 9.1.1
int32_t VulkanVideoDecoder::se()
{
    uint32_t eg = ue();  // Table 9-3
    int32_t codeNum;
    
    if (eg & 1)
        codeNum = (int32_t)((eg>>1)+1);
    else
        codeNum = -(int32_t)(eg>>1);
    return codeNum;
}

bool VulkanVideoDecoder::resizeBitstreamBuffer(VkDeviceSize extraBytes)
{
    // increasing min 2MB size per resizeBitstreamBuffer()
    VkDeviceSize newBitstreamDataLen = m_bitstreamDataLen + std::max<VkDeviceSize>(extraBytes, (2 * 1024 * 1024));

    VkDeviceSize retSize = m_bitstreamData.ResizeBitstreamBuffer(newBitstreamDataLen, m_bitstreamDataLen, 0);
    if (retSize < newBitstreamDataLen)
    {
        assert(!"bitstream buffer resize failed");
        nvParserLog("ERROR: bitstream buffer resize failed\n");
        return false;
    }

    m_bitstreamDataLen = (VkDeviceSize)retSize;
    return true;
}

VkDeviceSize VulkanVideoDecoder::swapBitstreamBuffer(VkDeviceSize copyCurrBuffOffset, VkDeviceSize copyCurrBuffSize)
{
    VkSharedBaseObj<VulkanBitstreamBuffer> currentBitstreamBuffer(m_bitstreamData.GetBitstreamBuffer());
    VkSharedBaseObj<VulkanBitstreamBuffer> newBitstreamBuffer;
    VkDeviceSize newBufferSize = currentBitstreamBuffer->GetMaxSize();
    const uint8_t* pCopyData = nullptr;
    if (copyCurrBuffSize) {
        VkDeviceSize maxSize = 0;
        pCopyData = currentBitstreamBuffer->GetReadOnlyDataPtr(copyCurrBuffOffset, maxSize);
    }
    m_pClient->GetBitstreamBuffer(newBufferSize,
                                  m_bufferOffsetAlignment, m_bufferSizeAlignment,
                                  pCopyData, copyCurrBuffSize, newBitstreamBuffer);
    assert(newBitstreamBuffer);
    if (!newBitstreamBuffer) {
        assert(!"Cound't GetBitstreamBuffer()!");
        return false;
    }
    // m_bitstreamDataLen = newBufferSize;
    return m_bitstreamData.SetBitstreamBuffer(newBitstreamBuffer);
}

bool VulkanVideoDecoder::ParseByteStream(const VkParserBitstreamPacket* pck, size_t *pParsedBytes)
{
#if defined(__x86_64__) || defined (_M_X64)
    if (m_NextStartCode == SIMD_ISA::AVX512)
    {
        return ParseByteStreamAVX512(pck, pParsedBytes);
    }
    else if (m_NextStartCode == SIMD_ISA::AVX2)
    {
        return ParseByteStreamAVX2(pck, pParsedBytes);
    }
    else if (m_NextStartCode == SIMD_ISA::SSSE3)
    {
        return ParseByteStreamSSSE3(pck, pParsedBytes);
    } else
#elif defined(__aarch64__) || defined(__ARM_ARCH_7A__) || defined(_M_ARM64)
#if defined(__aarch64__)
    if (m_NextStartCode == SIMD_ISA::SVE)
    {
        return ParseByteStreamSVE(pck, pParsedBytes);
    } else
#endif //__aarch64__
    if (m_NextStartCode == SIMD_ISA::NEON)
    {
        return ParseByteStreamNEON(pck, pParsedBytes);
    } else
#endif
    {
        return ParseByteStreamC(pck, pParsedBytes);
    }
}

void VulkanVideoDecoder::nal_unit()
{
    if (((m_nalu.end_offset - m_nalu.start_offset) > 3) &&
         m_bitstreamData.HasSliceStartCodeAtOffset(m_nalu.start_offset))
    {
        int nal_type;
        init_dbits();
        if (IsPictureBoundary(available_bits() >> 3))
        {
            if (m_nalu.start_offset > 0)
            {
                end_of_picture();

                // This swap will copy to the new buffer most of the time.
                m_bitstreamDataLen = swapBitstreamBuffer(m_nalu.start_offset, m_nalu.end_offset - m_nalu.start_offset);
                m_nalu.end_offset -= m_nalu.start_offset;
                m_nalu.start_offset = 0;
                m_bitstreamData.ResetStreamMarkers();
                m_llNaluStartLocation = m_llParsedBytes - m_nalu.end_offset;
            }
        }
        init_dbits();
        nal_type = ParseNalUnit();
        switch(nal_type)
        {
        case NALU_SLICE:
            if (m_bitstreamData.GetStreamMarkersCount() < MAX_SLICES)
            {
                if (m_bitstreamData.GetStreamMarkersCount() == 0) {
                    m_llFrameStartLocation = m_llNaluStartLocation;
                }
                assert(m_nalu.start_offset < std::numeric_limits<int32_t>::max());
                m_bitstreamData.AddStreamMarker((uint32_t)m_nalu.start_offset);
            }
            break;
        //case NALU_DISCARD:
        default:
            if ((nal_type == NALU_UNKNOWN) && (m_pClient))
            {
                // Called client for handling unsupported NALUs (or user data)
                const uint8_t* bitstreamDataPtr = m_bitstreamData.GetBitstreamPtr();
                int64_t cbData = (m_nalu.end_offset - m_nalu.start_offset - 3);
                assert((uint64_t)cbData < (uint64_t)std::numeric_limits<size_t>::max());
                m_pClient->UnhandledNALU(bitstreamDataPtr + m_nalu.start_offset + 3, (size_t)cbData);
            }
            m_nalu.end_offset = m_nalu.start_offset;
        }
    } else
    {
        // Discard invalid NALU
        m_nalu.end_offset = m_nalu.start_offset;
    }
    m_nalu.start_offset = m_nalu.end_offset;
}


bool VulkanVideoDecoder::IsSequenceChange(VkParserSequenceInfo *pnvsi)
{
    if (m_pClient)
    {
        if (memcmp(pnvsi, &m_PrevSeqInfo, sizeof(VkParserSequenceInfo)))
            return true;
    }
    return false;
}


int VulkanVideoDecoder::init_sequence(VkParserSequenceInfo *pnvsi)
{
    if (m_pClient != NULL)
    {
        // Detect sequence info changes
        if (memcmp(pnvsi, &m_PrevSeqInfo, sizeof(VkParserSequenceInfo)))
        {
            uint32_t lNumerator, lDenominator;
            memcpy(&m_PrevSeqInfo, pnvsi, sizeof(VkParserSequenceInfo));
            m_MaxFrameBuffers = m_pClient->BeginSequence(&m_PrevSeqInfo);
            if (!m_MaxFrameBuffers)
            {
                m_bDecoderInitFailed = true;
                return 0;
            }
            lNumerator = NV_FRAME_RATE_NUM(pnvsi->frameRate);
            lDenominator = NV_FRAME_RATE_DEN(pnvsi->frameRate);
            // Determine frame duration
            if ((m_lClockRate > 0) && (lNumerator > 0) && (lDenominator > 0))
            {
                m_lFrameDuration = (int32_t)((uint64_t)lDenominator * m_lClockRate / (uint32_t)lNumerator);
            }
            else if (m_lFrameDuration <= 0)
            {
                nvParserLog("WARNING: Unknown frame rate\n");
                // Default to 30Hz for timestamp interpolation
                m_lFrameDuration  = m_lClockRate / 30;
            }
        }
    }
    return m_MaxFrameBuffers;
}


void VulkanVideoDecoder::end_of_picture()
{
    if ((m_nalu.end_offset > 3) && (m_bitstreamData.GetStreamMarkersCount() > 0))
    {
        assert(!m_264SvcEnabled);
        // memset(m_pVkPictureData, 0, (m_264SvcEnabled ? 128 : 1) * sizeof(VkParserPictureData));
        m_pVkPictureData[0] = VkParserPictureData();
        m_pVkPictureData->bitstreamDataOffset = 0;
        m_pVkPictureData->firstSliceIndex = 0;
        m_pVkPictureData->bitstreamData = m_bitstreamData.GetBitstreamBuffer();
        assert((uint64_t)m_nalu.start_offset < (uint64_t)std::numeric_limits<size_t>::max());
        m_pVkPictureData->bitstreamDataLen = (size_t)m_nalu.start_offset;
        m_pVkPictureData->numSlices = m_bitstreamData.GetStreamMarkersCount();
        if(BeginPicture(m_pVkPictureData))
        {
            if ((m_pVkPictureData + m_iTargetLayer)->pCurrPic)
            {
                int32_t lDisp = 0;

                // Find an entry in m_DispInfo
                for (int32_t i = 0; i < MAX_DELAY; i++)
                {
                    if (m_DispInfo[i].pPicBuf == (m_pVkPictureData + m_iTargetLayer)->pCurrPic)
                    {
                        lDisp = i;
                        break;
                    }
                    if ((m_DispInfo[i].pPicBuf == NULL)
                     || ((m_DispInfo[lDisp].pPicBuf != NULL) && (m_DispInfo[i].llPTS - m_DispInfo[lDisp].llPTS < 0)))
                        lDisp = i;
                }
                m_DispInfo[lDisp].pPicBuf = (m_pVkPictureData + m_iTargetLayer)->pCurrPic;
                m_DispInfo[lDisp].bSkipped = false;
                m_DispInfo[lDisp].bDiscontinuity = false;
                m_DispInfo[lDisp].lPOC = (m_pVkPictureData + m_iTargetLayer)->picture_order_count;
                if (((m_pVkPictureData + m_iTargetLayer)->field_pic_flag) && (!(m_pVkPictureData + m_iTargetLayer)->second_field))
                    m_DispInfo[lDisp].lNumFields = 1;
                else
                    m_DispInfo[lDisp].lNumFields = 2 + (m_pVkPictureData + m_iTargetLayer)->repeat_first_field;
                if ((!(m_pVkPictureData + m_iTargetLayer)->second_field) // Ignore PTS of second field if we already got a PTS for the 1st field
                 || (!m_DispInfo[lDisp].bPTSValid))
                {
                    // Find a PTS in the list
                    unsigned int ndx = m_lPTSPos;
                    m_DispInfo[lDisp].bPTSValid = false;
                    m_DispInfo[lDisp].llPTS = m_llExpectedPTS; // Will be updated later on
                    for (int k = 0; k < MAX_QUEUED_PTS; k++)
                    {
                        if ((m_PTSQueue[ndx].bPTSValid) && (m_PTSQueue[ndx].llPTSPos - m_llFrameStartLocation <= (m_bNoStartCodes ? 0 : 3)))
                        {
                            m_DispInfo[lDisp].bPTSValid = true;
                            m_DispInfo[lDisp].llPTS = m_PTSQueue[ndx].llPTS;
                            m_DispInfo[lDisp].bDiscontinuity = m_PTSQueue[ndx].bDiscontinuity;
                            m_PTSQueue[ndx].bPTSValid = false;
                        }
                        ndx = (ndx+1) % MAX_QUEUED_PTS;
                    }
                }
                // Client callback
                if (m_pClient != NULL)
                {
                    // Notify client
                    if (!m_pClient->DecodePicture(m_pVkPictureData))
                    {
                        m_DispInfo[lDisp].bSkipped = true;
                        nvParserLog("WARNING: skipped decoding current picture\n");
                    }
                    else
                    {
                        m_nCallbackEventCount++;
                    }
                }
            }
            else
            {
                nvParserLog("WARNING: no valid render target for current picture\n");
            }
            // Call back codec for post-decode event (display the decoded frame)
            EndPicture();
        }
    }
}


void VulkanVideoDecoder::display_picture(VkPicIf *pPicBuf, bool bEvict)
{
    int32_t i, lDisp = -1;

    for (i = 0; i < MAX_DELAY; i++) {
        if (m_DispInfo[i].pPicBuf == pPicBuf)
        {
            lDisp = i;
            break;
        }
    }

    if (lDisp >= 0) {

        int64_t llPTS;
        if (m_DispInfo[lDisp].bPTSValid) {

            llPTS = m_DispInfo[lDisp].llPTS;
            // If filtering timestamps, look for the earliest PTS and swap with the current one so that the output
            // timestamps are always increasing (in case we're incorrectly getting the DTS instead of the PTS)
            if (m_bFilterTimestamps || (m_lCheckPTS && !m_DispInfo[lDisp].bDiscontinuity)) {

                int32_t lEarliest = lDisp;
                for (i = 0; i < MAX_DELAY; i++) {

                    if ((m_DispInfo[i].bPTSValid) && (m_DispInfo[i].pPicBuf) && (m_DispInfo[i].llPTS - m_DispInfo[lEarliest].llPTS < 0)) {
                        lEarliest = i;
                    }
                }
                if (lEarliest != lDisp) {
                    if (m_lCheckPTS) {
                        m_bFilterTimestamps = true;
                    }
                    nvParserLog("WARNING: Input timestamps do not match display order\n");
                    llPTS = m_DispInfo[lEarliest].llPTS;
                    m_DispInfo[lEarliest].llPTS = m_DispInfo[lDisp].llPTS;
                    m_DispInfo[lDisp].llPTS = llPTS;
                }
                if (m_lCheckPTS) {
                    m_lCheckPTS--;
                }
            }

        } else {

            llPTS = m_llExpectedPTS;
            if (m_bFirstPTS) {
                // The frame with the first timestamp has not been displayed yet: try to guess
                // using the difference in POC value if available
                for (i = 0; i < MAX_DELAY; i++) {
                    if ((m_DispInfo[i].pPicBuf) && (m_DispInfo[i].bPTSValid)) {
                        int lPOCDiff = m_DispInfo[i].lPOC - m_DispInfo[lDisp].lPOC;
                        if (lPOCDiff < m_DispInfo[lDisp].lNumFields)
                            lPOCDiff = m_DispInfo[lDisp].lNumFields;
                        llPTS = m_DispInfo[i].llPTS - ((lPOCDiff*m_lFrameDuration) >> 1);
                        break;
                    }
                }
            }
        }

        if (llPTS - m_llExpectedPTS < -(m_lFrameDuration >> 2)) {

#if 0
            nvParserLog("WARNING: timestamps going backwards (new=%d, prev=%d, diff=%d, frame_duration=%d)!\n",
                (int)llPTS, (int)m_llExpectedPTS, (int)(llPTS - m_llExpectedPTS), (int)m_lFrameDuration);
#endif
        }
        if ((m_pClient != NULL) && (!m_DispInfo[lDisp].bSkipped)) {

            m_pClient->DisplayPicture(pPicBuf, llPTS);
            m_nCallbackEventCount++;
        }

        if (bEvict) {
            m_DispInfo[lDisp].pPicBuf = NULL;
        }
        m_llExpectedPTS = llPTS + (((uint32_t)m_lFrameDuration * (uint32_t)m_DispInfo[lDisp].lNumFields) >> 1);
        m_bFirstPTS = false;

    } else {
        nvParserLog("WARNING: Attempting to display a picture that was not decoded (%p)\n", pPicBuf);
    }
}


void VulkanVideoDecoder::end_of_stream()
{
    EndOfStream();
    // Reset common parser state
    memset(&m_nalu, 0, sizeof(m_nalu));
    memset(&m_PrevSeqInfo, 0, sizeof(m_PrevSeqInfo));
    memset(&m_PTSQueue, 0, sizeof(m_PTSQueue));
    m_bitstreamData.ResetStreamMarkers();
    m_BitBfr = (uint32_t)~0;
    m_llParsedBytes = 0;
    m_llNaluStartLocation = 0;
    m_llFrameStartLocation = 0;
    m_lFrameDuration = 0;
    m_llExpectedPTS = 0;
    m_bFirstPTS = true;
    m_lPTSPos = 0;
    for (int i = 0; i < MAX_DELAY; i++)
    {
        m_DispInfo[i].pPicBuf = NULL;
        m_DispInfo[i].bPTSValid = false;
    }
}

#include "nvVulkanh265ScalingList.h"
#include "VulkanH264Decoder.h"
#include "VulkanH265Decoder.h"
#include "VulkanAV1Decoder.h"

static nvParserLogFuncType gParserLogFunc = nullptr;
static int gLogLevel = 0;

void nvParserErrorLog(const char* format, ...)
{
    if (!gParserLogFunc) {
        return;
    }
    va_list argptr;
    va_start(argptr, format);
    gParserLogFunc(format, argptr);
    va_end(argptr);
}

void nvParserLog(const char* format, ...)
{
    if ((gParserLogFunc == nullptr) || (gLogLevel == 0)) {
        return;
    }
    va_list argptr;
    va_start(argptr, format);
    gParserLogFunc(format, argptr);
    va_end(argptr);
}

void nvParserVerboseLog(const char* format, ...)
{
    if (!gParserLogFunc || (gLogLevel < 50)) {
        return;
    }
    va_list argptr;
    va_start(argptr, format);
    gParserLogFunc(format, argptr);
    va_end(argptr);
}

NVPARSER_EXPORT
VkResult CreateVulkanVideoDecodeParser(VkVideoCodecOperationFlagBitsKHR videoCodecOperation,
                                       const VkExtensionProperties* pStdExtensionVersion,
                                       nvParserLogFuncType pParserLogFunc, int logLevel,
                                       const VkParserInitDecodeParameters* pParserPictureData,
                                       VkSharedBaseObj<VulkanVideoDecodeParser>& nvVideoDecodeParser)
{
    gParserLogFunc = pParserLogFunc;
    gLogLevel = logLevel;
    switch((uint32_t)videoCodecOperation)
    {
    case VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR:
    {
        if ((pStdExtensionVersion == nullptr) ||
                (0 != strcmp(pStdExtensionVersion->extensionName, VK_STD_VULKAN_VIDEO_CODEC_H264_DECODE_EXTENSION_NAME)) ||
                (pStdExtensionVersion->specVersion != VK_STD_VULKAN_VIDEO_CODEC_H264_DECODE_SPEC_VERSION)) {
            nvParserErrorLog("The requested decoder h.264 Codec STD version is NOT supported\n");
            nvParserErrorLog("The supported decoder h.264 Codec STD version is version %d of %s\n",
                    VK_STD_VULKAN_VIDEO_CODEC_H264_DECODE_SPEC_VERSION, VK_STD_VULKAN_VIDEO_CODEC_H264_DECODE_EXTENSION_NAME);
            return VK_ERROR_INCOMPATIBLE_DRIVER;
        }
        VkSharedBaseObj<VulkanH264Decoder> nvVideoH264DecodeParser( new VulkanH264Decoder(videoCodecOperation));
        if (!nvVideoH264DecodeParser) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        nvVideoDecodeParser = nvVideoH264DecodeParser;
    }
        break;
    case VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR:
    {
        if ((pStdExtensionVersion == nullptr) ||
                (0 != strcmp(pStdExtensionVersion->extensionName, VK_STD_VULKAN_VIDEO_CODEC_H265_DECODE_EXTENSION_NAME)) ||
                (pStdExtensionVersion->specVersion != VK_STD_VULKAN_VIDEO_CODEC_H265_DECODE_SPEC_VERSION)) {
            nvParserErrorLog("The requested decoder h.265 Codec STD version is NOT supported\n");
             nvParserErrorLog("The supported decoder h.265 Codec STD version is version %d of %s\n",
                     VK_STD_VULKAN_VIDEO_CODEC_H265_DECODE_SPEC_VERSION, VK_STD_VULKAN_VIDEO_CODEC_H265_DECODE_EXTENSION_NAME);
             return VK_ERROR_INCOMPATIBLE_DRIVER;
        }
        VkSharedBaseObj<VulkanH265Decoder> nvVideoH265DecodeParser(new VulkanH265Decoder(videoCodecOperation));
        if (!nvVideoH265DecodeParser) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        nvVideoDecodeParser = nvVideoH265DecodeParser;
    }
        break;
    case VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR:
        if ((pStdExtensionVersion == nullptr) ||
                (0 != strcmp(pStdExtensionVersion->extensionName, VK_STD_VULKAN_VIDEO_CODEC_AV1_DECODE_EXTENSION_NAME)) ||
                (pStdExtensionVersion->specVersion != VK_STD_VULKAN_VIDEO_CODEC_AV1_DECODE_SPEC_VERSION)) {
             nvParserErrorLog("The requested decoder av1 Codec STD version is NOT supported\n");
             nvParserErrorLog("The supported decoder av1 Codec STD version is verion %d of %s\n",
                    VK_STD_VULKAN_VIDEO_CODEC_AV1_DECODE_SPEC_VERSION, VK_STD_VULKAN_VIDEO_CODEC_AV1_DECODE_EXTENSION_NAME);
             return VK_ERROR_INCOMPATIBLE_DRIVER;
        }
        nvVideoDecodeParser =  VkSharedBaseObj<VulkanAV1Decoder>(new VulkanAV1Decoder(videoCodecOperation));
        break;
#ifdef ENABLE_VP9_DECODER
    case VK_VIDEO_CODEC_OPERATION_DECODE_VP9_BIT_KHR:
        // TODO: This will not work and is only here as a placeholder to get the compiler to include and link the class.
        nvVideoDecodeParser =  VkSharedBaseObj<VulkanVP9Decoder>(new VulkanVP9Decoder(videoCodecOperation));
        break;
#endif
    default:
        nvParserErrorLog("Unsupported codec type!!!\n");
    }
    VkResult result = nvVideoDecodeParser->Initialize(pParserPictureData);
    if (result != VK_SUCCESS) {
        nvVideoDecodeParser = nullptr;
    }
    return result;
}
