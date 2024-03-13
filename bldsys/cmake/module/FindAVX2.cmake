# Adapted from https://github.com/jeffdaily/parasail/blob/600fb26151ff19899ee39a214972dcf2b9b11ed7/cmake/FindAVX2.cmake
#[[
    Copyright (c) 2015-2024, Battelle Memorial Institute


1.  Battelle Memorial Institute (hereinafter Battelle) hereby grants
    permission to any person or entity lawfully obtaining a copy of this
    software and associated documentation files (hereinafter “the
    Software”) to redistribute and use the Software in source and binary
    forms, with or without modification.  Such person or entity may use,
    copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and may permit others to do so, subject to
    the following conditions:

    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimers.

    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.

    - Other than as used herein, neither the name Battelle Memorial
      Institute or Battelle may be used in any form whatsoever without
      the express written consent of Battelle.

    - Redistributions of the software in any form, and publications
      based on work performed using the software should include the
      following citation as a reference:

    Daily, Jeff. (2016). Parasail: SIMD C library for global,
    semi-global, and local pairwise sequence alignments. *BMC
    Bioinformatics*, 17(1), 1-11.  doi:10.1186/s12859-016-0930-z

2.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BATTELLE
    OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
    USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
    OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
]]



# .rst:
#  FindAVX2
#  --------
#
#  Finds AVX2 support
#
#  This module can be used to detect AVX2 support in a C compiler.  If
#  the compiler supports AVX2, the flags required to compile with
#  AVX2 support are returned in variables for the different languages.
#  The variables may be empty if the compiler does not need a special
#  flag to support AVX2.
#
#  The following variables are set:
#
#  ::
#
#     AVX2_C_FLAGS - flags to add to the C compiler for AVX2 support
#     AVX2_FOUND - true if AVX2 is detected
# =============================================================================

set(_AVX2_REQUIRED_VARS)
set(CMAKE_REQUIRED_QUIET_SAVE ${CMAKE_REQUIRED_QUIET})
set(CMAKE_REQUIRED_QUIET ${AVX2_FIND_QUIETLY})

# sample AVX2 source code to test
set(AVX2_C_TEST_SOURCE
"
#include <immintrin.h>
void parasail_memset___m256i(__m256i *b, __m256i c, size_t len)
{
    size_t i;
    for (i=0; i<len; ++i) {
        _mm256_store_si256(&b[i], c);
    }
}

int foo() {
    __m256i vOne = _mm256_set1_epi8(1);
    __m256i result =  _mm256_add_epi8(vOne,vOne);
    return _mm_extract_epi16(_mm256_extracti128_si256(result,0),0);
}
int main(void) { return (int)foo(); }
")

# if these are set then do not try to find them again,
# by avoiding any try_compiles for the flags
if((DEFINED AVX2_C_FLAGS) OR (DEFINED HAVE_AVX2))
else()
  if(WIN32)
    # MSVC can compile AVX intrinsics without the arch flag, however it
    # will detect that AVX code is found and "consider using /arch:AVX".
    set(AVX2_C_FLAG_CANDIDATES
      "/arch:AVX"
      "/arch:AVX2")
  else()
    set(AVX2_C_FLAG_CANDIDATES
      #Empty, if compiler automatically accepts AVX2
      " "
      #GNU, Intel
      "-march=core-avx2"
      #clang
      "-mavx2"
    )
  endif()

  include(CheckCSourceCompiles)

  foreach(FLAG IN LISTS AVX2_C_FLAG_CANDIDATES)
    set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
    set(CMAKE_REQUIRED_FLAGS "${FLAG}")
    unset(HAVE_AVX2 CACHE)
    if(NOT CMAKE_REQUIRED_QUIET)
      message(STATUS "Try AVX2 C flag = [${FLAG}]")
    endif()
    check_c_source_compiles("${AVX2_C_TEST_SOURCE}" HAVE_AVX2)
    set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
    if(HAVE_AVX2)
      set(AVX2_C_FLAGS_INTERNAL "${FLAG}")
      break()
    endif()
  endforeach()

  unset(AVX2_C_FLAG_CANDIDATES)

  set(AVX2_C_FLAGS "${AVX2_C_FLAGS_INTERNAL}"
    CACHE STRING "C compiler flags for AVX2 intrinsics")
endif()

list(APPEND _AVX2_REQUIRED_VARS AVX2_C_FLAGS)

set(CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET_SAVE})

if(_AVX2_REQUIRED_VARS)
  include(FindPackageHandleStandardArgs)

  find_package_handle_standard_args(AVX2
                                    REQUIRED_VARS ${_AVX2_REQUIRED_VARS})

  mark_as_advanced(${_AVX2_REQUIRED_VARS})

  unset(_AVX2_REQUIRED_VARS)
else()
  message(SEND_ERROR "FindAVX2 requires C or CXX language to be enabled")
endif()

# begin tests for AVX2 specfic features

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(SAFE_CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(SAFE_CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
endif()

set(AVX2_C_TEST_SOURCE_SET1_EPI64X
"
#include <stdint.h>
#include <immintrin.h>
__m256i foo() {
    __m256i vOne = _mm256_set1_epi64x(1);
    return vOne;
}
int main(void) { foo(); return 0; }
")

if(AVX2_C_FLAGS)
  include(CheckCSourceCompiles)
  set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  set(CMAKE_REQUIRED_FLAGS "${AVX2_C_FLAGS}")
  check_c_source_compiles("${AVX2_C_TEST_SOURCE_SET1_EPI64X}" HAVE_AVX2_MM256_SET1_EPI64X)
  set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
endif()

set(AVX2_C_TEST_SOURCE_SET_EPI64X
"
#include <stdint.h>
#include <immintrin.h>
__m256i foo() {
    __m256i vOne = _mm256_set_epi64x(1,1,1,1);
    return vOne;
}
int main(void) { foo(); return 0; }
")

if(AVX2_C_FLAGS)
  include(CheckCSourceCompiles)
  set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  set(CMAKE_REQUIRED_FLAGS "${AVX2_C_FLAGS}")
  check_c_source_compiles("${AVX2_C_TEST_SOURCE_SET_EPI64X}" HAVE_AVX2_MM256_SET_EPI64X)
  set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
endif()

set(AVX2_C_TEST_SOURCE_INSERT64
"
#include <stdint.h>
#include <immintrin.h>
__m256i foo() {
    __m256i vOne = _mm256_set1_epi8(1);
    return _mm256_insert_epi64(vOne,INT64_MIN,0);
}
int main(void) { foo(); return 0; }
")

if(AVX2_C_FLAGS)
  include(CheckCSourceCompiles)
  set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  set(CMAKE_REQUIRED_FLAGS "${AVX2_C_FLAGS}")
  check_c_source_compiles("${AVX2_C_TEST_SOURCE_INSERT64}" HAVE_AVX2_MM256_INSERT_EPI64)
  set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
endif()

set(AVX2_C_TEST_SOURCE_INSERT32
"
#include <stdint.h>
#include <immintrin.h>
__m256i foo() {
    __m256i vOne = _mm256_set1_epi8(1);
    return _mm256_insert_epi32(vOne,INT32_MIN,0);
}
int main(void) { foo(); return 0; }
")

if(AVX2_C_FLAGS)
  include(CheckCSourceCompiles)
  set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  set(CMAKE_REQUIRED_FLAGS "${AVX2_C_FLAGS}")
  check_c_source_compiles("${AVX2_C_TEST_SOURCE_INSERT32}" HAVE_AVX2_MM256_INSERT_EPI32)
  set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
endif()

set(AVX2_C_TEST_SOURCE_INSERT16
"
#include <stdint.h>
#include <immintrin.h>
__m256i foo() {
    __m256i vOne = _mm256_set1_epi8(1);
    return _mm256_insert_epi16(vOne,INT16_MIN,0);
}
int main(void) { foo(); return 0; }
")

if(AVX2_C_FLAGS)
  include(CheckCSourceCompiles)
  set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  set(CMAKE_REQUIRED_FLAGS "${AVX2_C_FLAGS}")
  check_c_source_compiles("${AVX2_C_TEST_SOURCE_INSERT16}" HAVE_AVX2_MM256_INSERT_EPI16)
  set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
endif()

set(AVX2_C_TEST_SOURCE_INSERT8
"
#include <stdint.h>
#include <immintrin.h>
__m256i foo() {
    __m256i vOne = _mm256_set1_epi8(1);
    return _mm256_insert_epi8(vOne,INT8_MIN,0);
}
int main(void) { foo(); return 0; }
")

if(AVX2_C_FLAGS)
  include(CheckCSourceCompiles)
  set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  set(CMAKE_REQUIRED_FLAGS "${AVX2_C_FLAGS}")
  check_c_source_compiles("${AVX2_C_TEST_SOURCE_INSERT8}" HAVE_AVX2_MM256_INSERT_EPI8)
  set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
endif()


set(AVX2_C_TEST_SOURCE_EXTRACT64
"
#include <stdint.h>
#include <immintrin.h>
int64_t foo() {
    __m256i vOne = _mm256_set1_epi8(1);
    return (int64_t)_mm256_extract_epi64(vOne,0);
}
int main(void) { return (int)foo(); }
")

if(AVX2_C_FLAGS)
  include(CheckCSourceCompiles)
  set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  set(CMAKE_REQUIRED_FLAGS "${AVX2_C_FLAGS}")
  check_c_source_compiles("${AVX2_C_TEST_SOURCE_EXTRACT64}" HAVE_AVX2_MM256_EXTRACT_EPI64)
  set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
endif()

set(AVX2_C_TEST_SOURCE_EXTRACT32
"
#include <stdint.h>
#include <immintrin.h>
int32_t foo() {
    __m256i vOne = _mm256_set1_epi8(1);
    return (int32_t)_mm256_extract_epi32(vOne,0);
}
int main(void) { return (int)foo(); }
")

if(AVX2_C_FLAGS)
  include(CheckCSourceCompiles)
  set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  set(CMAKE_REQUIRED_FLAGS "${AVX2_C_FLAGS}")
  check_c_source_compiles("${AVX2_C_TEST_SOURCE_EXTRACT32}" HAVE_AVX2_MM256_EXTRACT_EPI32)
  set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
endif()

set(AVX2_C_TEST_SOURCE_EXTRACT16
"
#include <stdint.h>
#include <immintrin.h>
int16_t foo() {
    __m256i vOne = _mm256_set1_epi8(1);
    return (int16_t)_mm256_extract_epi16(vOne,0);
}
int main(void) { return (int)foo(); }
")

if(AVX2_C_FLAGS)
  include(CheckCSourceCompiles)
  set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  set(CMAKE_REQUIRED_FLAGS "${AVX2_C_FLAGS}")
  check_c_source_compiles("${AVX2_C_TEST_SOURCE_EXTRACT16}" HAVE_AVX2_MM256_EXTRACT_EPI16)
  set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
endif()

set(AVX2_C_TEST_SOURCE_EXTRACT8
"
#include <stdint.h>
#include <immintrin.h>
int8_t foo() {
    __m256i vOne = _mm256_set1_epi8(1);
    return (int8_t)_mm256_extract_epi8(vOne,0);
}
int main(void) { return (int)foo(); }
")

if(AVX2_C_FLAGS)
  include(CheckCSourceCompiles)
  set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  set(CMAKE_REQUIRED_FLAGS "${AVX2_C_FLAGS}")
  check_c_source_compiles("${AVX2_C_TEST_SOURCE_EXTRACT8}" HAVE_AVX2_MM256_EXTRACT_EPI8)
  set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_C_FLAGS "${SAFE_CMAKE_C_FLAGS}")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_C_FLAGS "${SAFE_CMAKE_C_FLAGS}")
endif()