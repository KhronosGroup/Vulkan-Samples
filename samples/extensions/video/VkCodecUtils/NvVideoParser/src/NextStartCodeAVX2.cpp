#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#include <cpudetect.h>
#include "ByteStreamParser.h"
#include "NvVideoParser/nvVulkanVideoUtils.h"
#include "NvVideoParser/nvVulkanVideoParser.h"

bool VulkanVideoDecoder::ParseByteStreamAVX2(const VkParserBitstreamPacket* pck, size_t *pParsedBytes)
{
    return ParseByteStreamSimd<SIMD_ISA::AVX2>(pck, pParsedBytes);
}

template<>
size_t VulkanVideoDecoder::next_start_code<SIMD_ISA::AVX2>(const uint8_t *pdatain, size_t datasize, bool& found_start_code)
{
    size_t i = 0;
    size_t datasize64 = (datasize >> 6) << 6;
    if (datasize64 > 64)
    {
        const __m256i v1 = _mm256_set1_epi8(1);
        __m256i vdata = _mm256_loadu_si256((const __m256i*)pdatain);
        __m256i vBfr = _mm256_set1_epi16(((m_BitBfr << 8) & 0xFF00) | ((m_BitBfr >> 8) & 0xFF));
        __m256i vdata_alignr16b_init = _mm256_permute2f128_si256(vBfr, vdata, 1 | (2<<4));
        __m256i vdata_prev1 = _mm256_alignr_epi8(vdata, vdata_alignr16b_init, 15);
        __m256i vdata_prev2 = _mm256_alignr_epi8(vdata, vdata_alignr16b_init, 14);
        for ( ; i < datasize64 - 64; i += 64)
        {
            for (int c = 0; c < 64; c += 32) // this might force compiler to unroll the loop so we might have 2 loads in parallel
            {
                // hotspot begin
                __m256i vdata_prev1or2 = _mm256_or_si256(vdata_prev2, vdata_prev1);
                __m256i vmask = _mm256_cmpeq_epi8(_mm256_and_si256(vdata, _mm256_cmpeq_epi8(vdata_prev1or2, _mm256_setzero_si256())), v1);
                const int resmask = _mm256_movemask_epi8(vmask);
                // hotspot end
                if (resmask)
                {
                    const int offset = count_trailing_zeros((uint64_t) (resmask & 0xFFFFFFFF));
                    found_start_code = true;
                    m_BitBfr =  1;
                    return offset + i + c + 1;
                }
                // hotspot begin
                __m256i vdata_next = _mm256_loadu_si256((const __m256i*)&pdatain[i + c + 32]); // 7-8 clocks
                __m256i vdata_alignr16b_next = _mm256_permute2f128_si256(vdata, vdata_next, 1 | (2<<4));
                vdata_prev1 = _mm256_alignr_epi8(vdata_next, vdata_alignr16b_next, 15);
                vdata_prev2 = _mm256_alignr_epi8(vdata_next, vdata_alignr16b_next, 14);
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