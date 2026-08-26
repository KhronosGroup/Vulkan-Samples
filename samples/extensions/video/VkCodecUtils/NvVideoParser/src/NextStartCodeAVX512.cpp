#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#include <cpudetect.h>
#include "ByteStreamParser.h"
#include "NvVideoParser/nvVulkanVideoUtils.h"
#include "NvVideoParser/nvVulkanVideoParser.h"

bool VulkanVideoDecoder::ParseByteStreamAVX512(const VkParserBitstreamPacket* pck, size_t *pParsedBytes)
{
    return ParseByteStreamSimd<SIMD_ISA::AVX512>(pck, pParsedBytes);
}

template<>
size_t VulkanVideoDecoder::next_start_code<SIMD_ISA::AVX512>(const uint8_t *pdatain, size_t datasize, bool& found_start_code)
{
    size_t i = 0;
    size_t datasize128 = (datasize >> 7) << 7;
    if (datasize128 > 128)
    {
        const __m512i v1 = _mm512_set1_epi8(1);
        const __m512i v254 = _mm512_set1_epi8(-2);
        __m512i vdata = _mm512_loadu_si512((const void*)pdatain);
        __m512i vBfr = _mm512_set1_epi16(((m_BitBfr << 8) & 0xFF00) | ((m_BitBfr >> 8) & 0xFF));
        __m512i vdata_alignr48b_init = _mm512_alignr_epi32(vdata, vBfr, 12);
        __m512i vdata_prev1 = _mm512_alignr_epi8(vdata, vdata_alignr48b_init, 15);
        __m512i vdata_prev2 = _mm512_alignr_epi8(vdata, vdata_alignr48b_init, 14);
        for ( ; i < datasize128 - 128; i += 128)
        {
            for (int c = 0; c < 128; c += 64) // this might force compiler to unroll the loop so we might have 2 loads in parallel
            {
                // hotspot begin
                __m512i vmask0 = _mm512_ternarylogic_epi64(vdata_prev2, vdata_prev1, vdata, 0x2); // 1 clock ..
                __m512i vmask1 = _mm512_ternarylogic_epi64(vdata_prev2, vdata_prev1, vdata, 0xFE); // in parallel.
                //__m512i vmask0 = _mm512_andnot_si512(_mm512_or_si512(vdata_prev2, vdata_prev1), vdata); // 1 clock .. debug
                //__m512i vmask1 = _mm512_or_si512(_mm512_or_si512(vdata_prev2, vdata_prev1), vdata);; // in parallel. debug
                // const uint64_t resmask = _mm512_cmpeq_epi8_mask(_mm512_or_si512(vmask0, _mm512_andnot_si512(v1, vmask1)), v1); // 4 = 3 + 1 clocks + 1 extra clock after debug
                const uint64_t resmask = _mm512_cmpeq_epi8_mask(_mm512_ternarylogic_epi64(vmask0, v254, vmask1, 0xF8), v1); // 4 = 3 + 0.5 clocks
                // hotspot end
                if (resmask)
                {
                    const int offset = count_trailing_zeros(resmask);
                    found_start_code = true;
                    m_BitBfr =  1;
                    return offset + i + c + 1;
                }
                // hotspot begin
                __m512i vdata_next = _mm512_loadu_si512((const void*)(&pdatain[i + c + 64])); // 7-8 clocks
                __m512i vdata_alignr48b_next = _mm512_alignr_epi32(vdata_next, vdata, 12);
                vdata_prev1 = _mm512_alignr_epi8(vdata_next, vdata_alignr48b_next, 15);
                vdata_prev2 = _mm512_alignr_epi8(vdata_next, vdata_alignr48b_next, 14);
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