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

#ifndef _VULKANVIDEODECODER_H_
#define _VULKANVIDEODECODER_H_

#include <atomic>
#include <limits>

#include <cpudetect.h>
#include "VkCodecUtils/VulkanBitstreamBuffer.h"

#define UNUSED_LOCAL_VAR(expr) do { (void)(expr); } while (0)

typedef struct NvVkNalUnit
{
    int64_t start_offset;     // Start offset in byte stream buffer
    int64_t end_offset;       // End offset in byte
    int64_t get_offset;       // Current read ptr in this NALU
    int32_t get_zerocnt;     // Zero byte count
    uint32_t get_bfr;        // Bit buffer for reading
    uint32_t get_bfroffs;    // Offset in bit buffer
    uint32_t get_emulcnt;    // Emulation prevention byte count
} NvVkNalUnit;

// Presentation information stored with every decoded frame
typedef struct NvVkPresentationInfo
{
    VkPicIf *pPicBuf;   // NOTE: May not be valid -> only used to uniquely identify a frame
    int32_t lNumFields;     // Number of displayed fields (to compute frame duration)
    int32_t bSkipped;       // true if frame was not decoded (skip display)
    int32_t bPTSValid;      // Frame has an associated PTS
    int32_t lPOC;           // Picture Order Count (for initial PTS interpolation)
    int64_t llPTS;          // Frame presentation time
    int32_t bDiscontinuity; // Discontinuity before this PTS, do not check for out of order
} NvVkPresentationInfo;


//
// VulkanVideoDecoder is the base class for all decoders
//
class VulkanVideoDecoder:  public VulkanVideoDecodeParser
{
public:
    enum { MAX_SLICES = 8192 };             // Up to 8K slices per picture
    enum { MAX_DELAY = 32 };                // Maximum frame delay between decode & display
    enum { MAX_QUEUED_PTS = 16};            // Size of PTS queue
    enum {
        NALU_DISCARD=0, // Discard this nal unit
        NALU_SLICE,     // This NALU contains picture data (keep)
        NALU_UNKNOWN,   // This NALU type is not supported (callback client)
    };
    typedef enum {
        NV_NO_ERROR = 0,         // No error detected
        NV_NON_COMPLIANT_STREAM  // Stream is not compliant with codec standards
    } NVCodecErrors;

protected:
    std::atomic<int32_t>             m_refCount;
    VkVideoCodecOperationFlagBitsKHR m_standard;        // Encoding standard
    uint32_t                         m_264SvcEnabled:1;    // enabled NVCS_H264_SVC
    uint32_t                         m_outOfBandPictureParameters:1; // Enable out of band parameters cb
    uint32_t                         m_initSequenceIsCalled:1;
    VkParserVideoDecodeClient *m_pClient;  // Interface to decoder client
    uint32_t m_defaultMinBufferSize;       // Minimum default buffer size that the parser is going to allocate
    uint32_t m_bufferOffsetAlignment;      // Minimum buffer offset alignment of the bitstream data for each frame
    uint32_t m_bufferSizeAlignment;        // Minimum buffer size alignment of the bitstream data for each frame
    VulkanBitstreamBufferStream m_bitstreamData;// bitstream for the current picture
    VkDeviceSize                m_bitstreamDataLen; // bitstream buffer size
    uint32_t m_BitBfr;                          // Bit Buffer for start code parsing
    int32_t m_bEmulBytesPresent;                // Startcode emulation prevention bytes are present in the byte stream
    int32_t m_bNoStartCodes;                    // No startcode parsing (only rely on the presence of PTS to detect frame boundaries)
    int32_t m_bFilterTimestamps;                // Filter input timestamps in case the decoder is sending the DTS instead of the PTS
    int32_t m_MaxFrameBuffers;                  // Max frame buffers to keep as reference
    NvVkNalUnit m_nalu;                         // Current NAL unit being filled
    size_t m_lMinBytesForBoundaryDetection;     // Min number of bytes needed to detect picture boundaries
    int64_t m_lClockRate;                       // System Reference Clock Rate
    int64_t m_lFrameDuration;                   // Approximate frame duration in units of (1/m_lClockRate) seconds
    int64_t m_llExpectedPTS;                    // Expected PTS of the next frame to be displayed
    int64_t m_llParsedBytes;                    // Total bytes parsed by the parser
    int64_t m_llNaluStartLocation;              // Byte count at the first byte of the current nal unit
    int64_t m_llFrameStartLocation;             // Byte count at the first byte of the picture bitstream data buffer
    int32_t m_lErrorThreshold;                  // Error threshold (0=strict, 100=ignore errors)
    int32_t m_bFirstPTS;                        // Number of frames displayed
    int32_t m_lPTSPos;                          // Current write position in PTS queue
    uint32_t m_nCallbackEventCount;              // Decode/Display callback count in current packet
    VkParserSequenceInfo m_PrevSeqInfo;          // Current sequence info
    VkParserSequenceInfo m_ExtSeqInfo;           // External sequence info from system layer
    NvVkPresentationInfo m_DispInfo[MAX_DELAY]; // Keeps track of display attributes
    struct {
        int32_t bPTSValid;                      // Entry is valid
        int64_t llPTS;                          // PTS value
        int64_t llPTSPos;                       // PTS position in byte stream
        int32_t bDiscontinuity;                 // Discontinuity before this PTS, do not check for out of order
    } m_PTSQueue[MAX_QUEUED_PTS];
    int32_t m_bDiscontinuityReported;           // Dicontinuity reported
    VkParserPictureData *m_pVkPictureData;
    int32_t m_iTargetLayer;                     // Specific to SVC only
    int32_t m_bDecoderInitFailed;               // Set when m_pClient->BeginSequence fails to create the decoder
    int32_t m_lCheckPTS;                        // Run the m_bFilterTimestamps for the first few framew to look for out of order PTS
    NVCodecErrors m_eError;
    SIMD_ISA m_NextStartCode;
public:
    VulkanVideoDecoder(VkVideoCodecOperationFlagBitsKHR std);
    virtual ~VulkanVideoDecoder();

public:
    // INvRefCount
    int32_t AddRef()
    {
        return ++m_refCount;
    }

    int32_t Release()
    {
        uint32_t ret;
        ret = --m_refCount;
        // Destroy the device if refcount reaches zero
        if (ret == 0) {
            Deinitialize();
            delete this;
        }
        return ret;
    }

    // VulkanVideoDecodeParser
    virtual VkResult Initialize(const VkParserInitDecodeParameters *pNvVkp);
    virtual bool Deinitialize();
    virtual bool ParseByteStream(const VkParserBitstreamPacket *pck, size_t *pParsedBytes);
    template <SIMD_ISA T>
    bool ParseByteStreamSimd(const VkParserBitstreamPacket *pck, size_t *pParsedBytes);
    bool ParseByteStreamC(const VkParserBitstreamPacket *pck, size_t *pParsedBytes);
#if defined(__x86_64__) || defined (_M_X64)
    bool ParseByteStreamAVX2(const VkParserBitstreamPacket* pck, size_t *pParsedBytes);
    bool ParseByteStreamAVX512(const VkParserBitstreamPacket* pck, size_t *pParsedBytes);
    bool ParseByteStreamSSSE3(const VkParserBitstreamPacket* pck, size_t *pParsedBytes);
#elif defined(__aarch64__)
    bool ParseByteStreamSVE(const VkParserBitstreamPacket* pck, size_t *pParsedBytes);
    bool ParseByteStreamNEON(const VkParserBitstreamPacket* pck, size_t *pParsedBytes);
#elif defined(__ARM_ARCH_7A__) || defined(_M_ARM64)
    bool ParseByteStreamNEON(const VkParserBitstreamPacket* pck, size_t *pParsedBytes);
#endif
    virtual bool GetDisplayMasteringInfo(VkParserDisplayMasteringInfo *) { return false; }

protected:
    virtual void CreatePrivateContext() = 0;                   // Implemented by derived classes
    virtual void InitParser() = 0;                             // Initialize codec-specific parser state
    virtual bool IsPictureBoundary(int32_t rbsp_size) = 0;     // Returns true if the current NAL unit belongs to a new picture
    virtual int32_t  ParseNalUnit() = 0;                       // Must return NALU_DISCARD or NALU_SLICE
    virtual bool BeginPicture(VkParserPictureData *pnvpd) = 0; // Fills in picture data. Return true if picture should be sent to client
    virtual void EndPicture() {}                               // Called after a picture has been decoded
    virtual void EndOfStream() {}                              // Called to reset parser
    virtual void FreeContext() = 0;

protected:
    // Byte stream parsing
    template<SIMD_ISA T>
    size_t next_start_code(const uint8_t *pdatain, size_t datasize, bool& found_start_code);
    void nal_unit();
    void init_dbits();
    int32_t available_bits() {
		// end_offset=566 - get_offset=568 for example are values seen in the debugger, causing an unsigned integer overflow below since get_btroffs is a uint32 while end_offset and get_offset are int64 explicitly casted to int32, which then gets implicitly casted to a unint32. This code could use a review from the original authors about the intentions here. The early return is to avoid the sanitizer violation and appears semantically correct.
		if ((m_nalu.end_offset - m_nalu.get_offset) < 0)
			return 0;
		assert((m_nalu.end_offset - m_nalu.get_offset) < std::numeric_limits<int32_t>::max());
                               return (int32_t)(m_nalu.end_offset - m_nalu.get_offset) * 8 + (32 - m_nalu.get_bfroffs); }
    int32_t consumed_bits() { assert((m_nalu.get_offset - m_nalu.start_offset - m_nalu.get_emulcnt) < std::numeric_limits<int32_t>::max());
                          return (int32_t)(m_nalu.get_offset - m_nalu.start_offset - m_nalu.get_emulcnt) * 8 - (32 - m_nalu.get_bfroffs); }
    uint32_t next_bits(uint32_t n) { return (m_nalu.get_bfr << m_nalu.get_bfroffs) >> (32 - n); } // NOTE: n must be in the [1..25] range
    void skip_bits(uint32_t n);  // advance bitstream position
    uint32_t u(uint32_t n);   // return next n bits, advance bitstream position
    bool flag()          { return (0 != u(1)); }     // returns flag value
    uint32_t u16_le()    { uint32_t tmp = u(8); tmp |= u(8) << 8; return tmp; }
    uint32_t u24_le()    { uint32_t tmp = u16_le(); tmp |= u(8) << 16; return tmp; }
    uint32_t u32_le()    { uint32_t tmp = u16_le(); tmp |= u16_le() << 16; return tmp; }
    uint32_t ue();
    int32_t se();
    uint32_t f(uint32_t n, uint32_t) { return u(n); }
    bool byte_aligned() const { return ((m_nalu.get_bfroffs & 7) == 0); }
    void byte_alignment() { while (!byte_aligned()) u(1); }
    void end_of_picture();
    void end_of_stream();
    bool IsSequenceChange(VkParserSequenceInfo *pnvsi);
    int32_t init_sequence(VkParserSequenceInfo *pnvsi);  // Must be called by derived classes to initialize the sequence
    void display_picture(VkPicIf *pPicBuf, bool bEvict = true);
    void rbsp_trailing_bits();
    bool end() { return m_nalu.get_offset >= m_nalu.end_offset; }
    bool more_rbsp_data();
    bool resizeBitstreamBuffer(VkDeviceSize nExtrabytes);
    VkDeviceSize swapBitstreamBuffer(VkDeviceSize copyCurrBuffOffset, VkDeviceSize copyCurrBuffSize);
};

void nvParserLog(const char* format, ...);
void nvParserVerboseLog(const char* format, ...);
void nvParserErrorLog(const char* format, ...);

#endif // _VULKANVIDEODECODER_H_
