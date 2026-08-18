#include <cpudetect.h>
#include "ByteStreamParser.h"
#include "NvVideoParser/nvVulkanVideoUtils.h"
#include "NvVideoParser/nvVulkanVideoParser.h"

bool VulkanVideoDecoder::ParseByteStreamC(const VkParserBitstreamPacket* pck, size_t *pParsedBytes)
{
    return ParseByteStreamSimd<SIMD_ISA::NOSIMD>(pck, pParsedBytes);
}

template<>
size_t VulkanVideoDecoder::next_start_code<SIMD_ISA::NOSIMD>(const uint8_t *pdatain, size_t datasize, bool& found_start_code)
{
    uint32_t bfr = m_BitBfr;
    size_t i = 0;
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