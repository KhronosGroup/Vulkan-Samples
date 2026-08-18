// Source example is based on: https://learn.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=msvc-170

// InstructionSet.cpp
// Compile by using: cl /EHsc /W4 InstructionSet.cpp
// processor: x86, x64
// Uses the __cpuid intrinsic to get information about
// CPU extended instruction set support.

#include <cpudetect.h>

#if defined(__aarch64__)

#include <asm/hwcap.h>
#include <sys/auxv.h>

#elif defined(_M_X64)

#include <iostream>
#include <vector>
#include <bitset>
#include <array>
#include <string>
#include <intrin.h>

class InstructionSet
{
    // forward declarations
    class InstructionSet_Internal;

public:
    // getters
    static bool SSSE3(void) { return CPU_Rep.f_1_ECX_[9]; }
    static bool AVX(void) { return CPU_Rep.f_1_ECX_[28]; }
    static bool AVX2(void) { return CPU_Rep.f_7_EBX_[5]; }
    static bool AVX512F(void) { return CPU_Rep.f_7_EBX_[16]; }
    static bool AVX512DQ(void) { return CPU_Rep.f_7_EBX_[17]; }
    static bool AVX512CD(void) { return CPU_Rep.f_7_EBX_[28]; }
    static bool AVX512BW(void) { return CPU_Rep.f_7_EBX_[30]; }
    static bool AVX512VL(void) { return CPU_Rep.f_7_EBX_[31]; } // all the 5 bits should be checked if "/arch:AVX512" option is passed

private:
    static const InstructionSet_Internal CPU_Rep;

    class InstructionSet_Internal
    {
    public:
        InstructionSet_Internal()
            : nIds_{ 0 },
            f_1_ECX_{ 0 },
            f_1_EDX_{ 0 },
            f_7_EBX_{ 0 },
            f_7_ECX_{ 0 }
        {
            //int cpuInfo[4] = {-1};
            std::array<int, 4> cpui;

            // Calling __cpuid with 0x0 as the function_id argument
            // gets the number of the highest valid function ID.
            __cpuid(cpui.data(), 0);
            nIds_ = cpui[0];

            // load bitset with flags for function 0x00000001
            if (nIds_ >= 1)
            {
                __cpuidex(cpui.data(), 1, 0);
                f_1_ECX_ = cpui[2];
                f_1_EDX_ = cpui[3];
            }

            // load bitset with flags for function 0x00000007
            if (nIds_ >= 7)
            {
                __cpuidex(cpui.data(), 7, 0);
                f_7_EBX_ = cpui[1];
                f_7_ECX_ = cpui[2];
            }
        };

        int nIds_;
        std::bitset<32> f_1_ECX_;
        std::bitset<32> f_1_EDX_;
        std::bitset<32> f_7_EBX_;
        std::bitset<32> f_7_ECX_;
    };
};

// Initialize static member data
const InstructionSet::InstructionSet_Internal InstructionSet::CPU_Rep;

#endif

// Print out supported instruction set extensions
SIMD_ISA check_simd_support()
{
#if defined(_M_X64)
    if (InstructionSet::AVX512F() && InstructionSet::AVX512BW()) { return SIMD_ISA::AVX512; }
    else if (InstructionSet::AVX2()) { return SIMD_ISA::AVX2; }
    else if (InstructionSet::SSSE3()) { return SIMD_ISA::SSSE3; };
#elif defined (__x86_64__)
    if (__builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512bw")) { return SIMD_ISA::AVX512; }
    else if (__builtin_cpu_supports("avx2")) { return SIMD_ISA::AVX2; }
    else if (__builtin_cpu_supports("ssse3")) { return SIMD_ISA::SSSE3; };
#elif defined(__aarch64__)
    long hwcaps = getauxval(AT_HWCAP);
    if(hwcaps & HWCAP_SVE) { return SIMD_ISA::SVE; }
    else if(hwcaps & HWCAP_ASIMD) { return SIMD_ISA::NEON; }
#elif defined(__ARM_ARCH_7A__) || defined(_M_ARM64)
    return SIMD_ISA::NEON;
#endif
    return SIMD_ISA::NOSIMD;
};