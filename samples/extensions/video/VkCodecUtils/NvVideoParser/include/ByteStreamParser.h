#ifndef _VULKANBYTESTREAMPARSER_H_
#define _VULKANBYTESTREAMPARSER_H_

#include <stdarg.h>
#include "vkvideo_parser/VulkanVideoParserIf.h"
#include "VulkanVideoDecoder.h"
#include "NvVideoParser/nvVulkanVideoUtils.h"
#include "NvVideoParser/nvVulkanVideoParser.h"
#include <algorithm>

template<SIMD_ISA T>
bool VulkanVideoDecoder::ParseByteStreamSimd(const VkParserBitstreamPacket* pck, size_t *pParsedBytes)
{
    VkDeviceSize curr_data_size = pck->nDataLength;
    unsigned int framesinpkt = 0;
    const uint8_t *pdatain = (curr_data_size > 0) ? pck->pByteStream : NULL;

    if (!m_bitstreamData) { // make sure we're initialized
        return false;
    }

    m_eError = NV_NO_ERROR; // Reset the flag to catch errors if any in current frame

    m_nCallbackEventCount = 0;
    // Handle discontinuity
    if (pck->bDiscontinuity)
    {
        if (!m_bNoStartCodes)
        {
            if (m_nalu.start_offset == 0)
                m_llNaluStartLocation = m_llParsedBytes - m_nalu.end_offset;

            // Pad the data after the NAL unit with start_code_prefix
            // make the room for 3 bytes
            if (((VkDeviceSize)(m_nalu.end_offset + 3) > m_bitstreamDataLen) &&
                    !resizeBitstreamBuffer(m_nalu.end_offset + 3 - m_bitstreamDataLen)) {
                return false;
            }
            m_bitstreamData.SetSliceStartCodeAtOffset(m_nalu.end_offset);

            // Complete the current NAL unit (if not empty)
            nal_unit();
            // Decode the current picture (NOTE: may be truncated)
            end_of_picture();
            framesinpkt++;

            m_bitstreamDataLen = swapBitstreamBuffer(m_nalu.start_offset, m_nalu.end_offset - m_nalu.start_offset);
        }
        // Reset the PTS queue to prevent timestamps from before the discontinuity to be associated with
        // a frame past the discontinuity
        memset(&m_PTSQueue, 0, sizeof(m_PTSQueue));
        m_bDiscontinuityReported = true;
    }
    // Remember the packet PTS and its location in the byte stream
    if (pck->bPTSValid)
    {
        m_PTSQueue[m_lPTSPos].bPTSValid = true;
        m_PTSQueue[m_lPTSPos].llPTS = pck->llPTS;
        m_PTSQueue[m_lPTSPos].llPTSPos = m_llParsedBytes;
        m_PTSQueue[m_lPTSPos].bDiscontinuity = m_bDiscontinuityReported;
        m_bDiscontinuityReported = false;
        m_lPTSPos = (m_lPTSPos + 1) % MAX_QUEUED_PTS;
    }
    // In case the bitstream is not startcode-based, the input always only contains a single frame
    if (m_bNoStartCodes)
    {
        if (curr_data_size > m_bitstreamDataLen - 4)
        {
            if (!resizeBitstreamBuffer(curr_data_size - (m_bitstreamDataLen - 4)))
                return false;
        }
        if (curr_data_size > 0)
        {
            m_nalu.start_offset = 0;
            m_nalu.end_offset = m_nalu.start_offset + curr_data_size;
            VkSharedBaseObj<VulkanBitstreamBuffer> bitstreamBuffer(m_bitstreamData.GetBitstreamBuffer());
            bitstreamBuffer->CopyDataFromBuffer(pdatain, 0, m_nalu.start_offset, curr_data_size);
            m_llNaluStartLocation = m_llParsedBytes;
            m_llParsedBytes += curr_data_size;
            m_bitstreamData.ResetStreamMarkers();
            init_dbits();
            if (ParseNalUnit() == NALU_SLICE)
            {
                m_llFrameStartLocation = m_llNaluStartLocation;
                m_bitstreamData.AddStreamMarker(0);
                m_nalu.start_offset = m_nalu.end_offset;
                // Decode only one frame if EOP is set and ignore remaining frames in current packet
                if ((!pck->bEOP) || (pck->bEOP && (framesinpkt < 1)))
                {
                    end_of_picture();
                    framesinpkt++;

                    m_bitstreamDataLen = swapBitstreamBuffer(m_nalu.start_offset, m_nalu.end_offset - m_nalu.start_offset);
                }
            }
        }
        m_nalu.start_offset = 0;
        m_nalu.end_offset = 0;
        if (pck->bEOS)
        {
            end_of_stream();
        }
        if (pParsedBytes)
        {
            *pParsedBytes = pck->nDataLength;
        }

        return (m_eError == NV_NO_ERROR ? true : false);
    }
    // Parse start codes
    while (curr_data_size > 0) {

        VkDeviceSize buflen = curr_data_size;

        // If bPartialParsing is set, we return immediately once we decoded or displayed a frame
        if ((pck->bPartialParsing) && (m_nCallbackEventCount != 0))
        {
            break;
        }
        if ((m_nalu.start_offset > 0) && ((m_nalu.end_offset - m_nalu.start_offset) < (int64_t)m_lMinBytesForBoundaryDetection))
        {
            buflen = std::min<VkDeviceSize>(buflen, (m_lMinBytesForBoundaryDetection - (m_nalu.end_offset - m_nalu.start_offset)));
        }
        bool found_start_code = false;
        VkDeviceSize start_offset = next_start_code<T>(pdatain, (size_t)buflen, found_start_code);
        VkDeviceSize data_used = found_start_code ? start_offset : buflen;
        if (data_used > 0)
        {
            if (data_used > (m_bitstreamDataLen - m_nalu.end_offset))
            {
                resizeBitstreamBuffer(data_used - (m_bitstreamDataLen - m_nalu.end_offset));
            }
            VkDeviceSize bytes = std::min<VkDeviceSize>(data_used, m_bitstreamDataLen - m_nalu.end_offset);
            if (bytes > 0) {
                VkSharedBaseObj<VulkanBitstreamBuffer> bitstreamBuffer(m_bitstreamData.GetBitstreamBuffer());
                bitstreamBuffer->CopyDataFromBuffer(pdatain, 0, m_nalu.end_offset, bytes);
            }
            m_nalu.end_offset += bytes;
            m_llParsedBytes += bytes;
            pdatain += data_used;
            curr_data_size -= data_used;
            // Check for picture boundaries before we have the entire NAL data
            if ((m_nalu.start_offset > 0) && (m_nalu.end_offset == (m_nalu.start_offset + (int64_t)m_lMinBytesForBoundaryDetection)))
            {
                init_dbits();
                if (IsPictureBoundary(available_bits() >> 3)) {
                    // Decode only one frame if EOP is set and ignore remaining frames in current packet
                    if ((!pck->bEOP) || (pck->bEOP && (framesinpkt < 1)))
                    {
                        end_of_picture();
                        framesinpkt++;
                    }
                    // This swap will copy to the new buffer most of the time.
                    m_bitstreamDataLen = swapBitstreamBuffer(m_nalu.start_offset, m_nalu.end_offset - m_nalu.start_offset);
                    m_nalu.end_offset -= m_nalu.start_offset;
                    m_nalu.start_offset = 0;
                    m_bitstreamData.ResetStreamMarkers();
                    m_llNaluStartLocation = m_llParsedBytes - m_nalu.end_offset;
                }
            }
        }
        // Did we find a startcode ?
        if (found_start_code)
        {
            if (m_nalu.start_offset == 0) {
                m_llNaluStartLocation = m_llParsedBytes - m_nalu.end_offset;
            }
            // Remove the trailing 00.00.01 from the NAL unit
            m_nalu.end_offset = (m_nalu.end_offset >= 3) ? (m_nalu.end_offset - 3) : 0;
            nal_unit();
            if (m_bDecoderInitFailed)
            {
                return false;
            }
            // Add back the start code prefix for the next NAL unit
            m_bitstreamData.SetSliceStartCodeAtOffset(m_nalu.end_offset);
            m_nalu.end_offset += 3;
        }
    }
    if (pParsedBytes)
    {
        assert(curr_data_size < std::numeric_limits<size_t>::max());
        *pParsedBytes = pck->nDataLength - (size_t)curr_data_size;
    }
    if (pck->bEOP || pck->bEOS)
    {
        if (m_nalu.start_offset == 0)
            m_llNaluStartLocation = m_llParsedBytes - m_nalu.end_offset;
        // Remove the trailing 00.00.01 from the NAL unit
        if (!!m_bitstreamData && (m_nalu.end_offset >= 3) &&
            m_bitstreamData.HasSliceStartCodeAtOffset(m_nalu.end_offset - 3))
        {
            m_nalu.end_offset = m_nalu.end_offset - 3;
        }
        // Complete the current NAL unit (if not empty)
        nal_unit();

        // Pad the data after the NAL unit with start_code_prefix
        if (((VkDeviceSize)(m_nalu.end_offset + 3) > m_bitstreamDataLen) &&
                !resizeBitstreamBuffer(m_nalu.end_offset + 3 - m_bitstreamDataLen)) {
            return false;
        }
        m_bitstreamData.SetSliceStartCodeAtOffset(m_nalu.end_offset);
        m_nalu.end_offset += 3;

        // Decode the current picture
        if ((!pck->bEOP) || (pck->bEOP && framesinpkt < 1))
        {
            end_of_picture();

             m_bitstreamDataLen = swapBitstreamBuffer(0, 0);
        }
        m_nalu.end_offset = 0;
        m_nalu.start_offset = 0;
        m_bitstreamData.ResetStreamMarkers();
        m_llNaluStartLocation = m_llParsedBytes;
        if (pck->bEOS)
        {
            // Flush everything, release all picture buffers
            end_of_stream();
        }
    }

    return (m_eError == NV_NO_ERROR ? true : false);
}

#endif //_VULKANBYTESTREAMPARSER_H_