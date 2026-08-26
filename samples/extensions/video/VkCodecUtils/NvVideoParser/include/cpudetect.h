#ifndef CPUDETECT_H
#define CPUDETECT_H

enum SIMD_ISA
{
    NOSIMD = 0,
    SSSE3,
    AVX2,
    AVX512,
    NEON,
    SVE
};

static int inline count_trailing_zeros(unsigned long long resmask)
{
#ifndef _WIN32
    int offset = __builtin_ctzll(resmask);
#else
    unsigned long offset = 0;
    const unsigned char dummyIsNonZero =_BitScanForward64(&offset, resmask); // resmask can't be 0 in this if
#endif
    return offset;
}

SIMD_ISA check_simd_support();

#endif