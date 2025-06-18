#if defined(__aarch64__) || defined(__ARM_ARCH_7A__) || defined(_M_ARM64)
#include "arm_neon.h"
#include <cpudetect.h>
#include "ByteStreamParser.h"
#include "NvVideoParser/nvVulkanVideoUtils.h"
#include "NvVideoParser/nvVulkanVideoParser.h"

bool VulkanVideoDecoder::ParseByteStreamNEON(const VkParserBitstreamPacket* pck, size_t *pParsedBytes)
{
    return ParseByteStreamSimd<SIMD_ISA::NEON>(pck, pParsedBytes);
}

template<>
size_t VulkanVideoDecoder::next_start_code<SIMD_ISA::NEON>(const uint8_t *pdatain, size_t datasize, bool& found_start_code)
{
    size_t i = 0;
    size_t datasize32 = (datasize >> 5) << 5;
    if (datasize32 > 32)
    {
        const uint8x16_t v0 = vdupq_n_u8(0);
        const uint8x16_t v1 = vdupq_n_u8(1);
        uint8x16_t vdata = vld1q_u8(pdatain);
        uint8x16_t vBfr = vreinterpretq_u8_u16(vdupq_n_u16(((m_BitBfr << 8) & 0xFF00) | ((m_BitBfr >> 8) & 0xFF)));
        uint8x16_t vdata_prev1 = vextq_u8(vBfr, vdata, 15);
        uint8x16_t vdata_prev2 = vextq_u8(vBfr, vdata, 14);
        uint8_t idx0n[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        uint8x16_t v015 = vld1q_u8(idx0n);
        for ( ; i < datasize32 - 32; i += 32)
        {
            for (int c = 0; c < 32; c += 16)
            {
                // hotspot begin
                uint8x16_t vdata_prev1or2 = vorrq_u8(vdata_prev2, vdata_prev1);
                uint8x16_t vmask = vceqq_u8(vandq_u8(vceqq_u8(vdata_prev1or2, v0), vdata), v1);
                // hotspot end
#if defined (__aarch64__) || defined(_M_ARM64)
                uint64_t resmask = vmaxvq_u8(vmask);
#else
                uint64_t resmask = vget_lane_u64(vmax_u8(vget_low_u8(vmask), vget_high_u8(vmask)), 0);
#endif
                if (resmask)
                {
                    uint8x16_t v015mask = vbslq_u8(vmask, v015, vdupq_n_u8(UINT8_MAX));
#if defined (__aarch64__) || defined(_M_ARM64)
                    const uint8_t offset = vminvq_u8(v015mask);
#else
                    uint8x8_t minval = vmin_u8(vget_low_u8(v015mask), vget_high_u8(v015mask));
                    minval = vpmin_u8(minval, minval);
                    minval = vpmin_u8(minval, minval);
                    const uint8_t offset = vget_lane_u8(vpmin_u8(minval, minval), 0);
#endif
                    found_start_code = true;
                    m_BitBfr =  1;
                    return (size_t)offset + i + c + 1;
                }
                // hotspot begin
                uint8x16_t vdata_next = vld1q_u8(&pdatain[i + c + 16]);
                vdata_prev1 = vextq_u8(vdata, vdata_next, 15);
                vdata_prev2 = vextq_u8(vdata, vdata_next, 14);
                vdata = vdata_next;
                // hotspot end
            }
        } // main processing loop end
        m_BitBfr = (pdatain[i-2] << 8) | pdatain[i-1];
    }
    // process a tail (rest):
    uint32_t bfr = m_BitBfr;
    do
    {
        bfr = (bfr << 8) | pdatain[i++];
        if ((bfr & 0x00ffffff) == 1) {
            break;
        }
    } while (i < datasize);
    m_BitBfr = bfr;
    found_start_code = ((bfr & 0x00ffffff) == 1);
    return i;
}
#endif