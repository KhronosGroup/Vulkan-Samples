# Adapted from https://github.com/jeffdaily/parasail/blob/600fb26151ff19899ee39a214972dcf2b9b11ed7/cmake/FindSSE41.cmake
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

#.rst:
# FindSSE41
# ---------
#
# Finds SSE41 support
#
# This module can be used to detect SSE41 support in a C compiler.  If
# the compiler supports SSE41, the flags required to compile with
# SSE41 support are returned in variables for the different languages.
# The variables may be empty if the compiler does not need a special
# flag to support SSE41.
#
# The following variables are set:
#
# ::
#
#    SSE41_C_FLAGS - flags to add to the C compiler for SSE41 support
#    SSE41_FOUND - true if SSE41 is detected
#
#=============================================================================

set(_SSE41_REQUIRED_VARS)
set(CMAKE_REQUIRED_QUIET_SAVE ${CMAKE_REQUIRED_QUIET})
set(CMAKE_REQUIRED_QUIET ${SSE41_FIND_QUIETLY})

# sample SSE41 source code to test
set(SSE41_C_TEST_SOURCE
"
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <smmintrin.h>
#endif
int foo() {
    __m128i vOne = _mm_set1_epi8(1);
    __m128i result =  _mm_max_epi8(vOne,vOne);
    return _mm_extract_epi8(result, 0);
}
int main(void) { return foo(); }
")

# if these are set then do not try to find them again,
# by avoiding any try_compiles for the flags
if((DEFINED SSE41_C_FLAGS) OR (DEFINED HAVE_SSE41))
else()
  if(WIN32)
    set(SSE41_C_FLAG_CANDIDATES
      #Empty, if compiler automatically accepts SSE41
      " "
      "/arch:SSE2")
  else()
    set(SSE41_C_FLAG_CANDIDATES
      #Empty, if compiler automatically accepts SSE41
      " "
      #GNU, Intel
      "-march=corei7"
      #clang
      "-msse4"
      #GNU 4.4.7 ?
      "-msse4.1"
    )
  endif()

  include(CheckCSourceCompiles)

  foreach(FLAG IN LISTS SSE41_C_FLAG_CANDIDATES)
    set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
    set(CMAKE_REQUIRED_FLAGS "${FLAG}")
    unset(HAVE_SSE41 CACHE)
    if(NOT CMAKE_REQUIRED_QUIET)
      message(STATUS "Try SSE41 C flag = [${FLAG}]")
    endif()
    check_c_source_compiles("${SSE41_C_TEST_SOURCE}" HAVE_SSE41)
    set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
    if(HAVE_SSE41)
      set(SSE41_C_FLAGS_INTERNAL "${FLAG}")
      break()
    endif()
  endforeach()

  unset(SSE41_C_FLAG_CANDIDATES)

  set(SSE41_C_FLAGS "${SSE41_C_FLAGS_INTERNAL}"
    CACHE STRING "C compiler flags for SSE41 intrinsics")
endif()

list(APPEND _SSE41_REQUIRED_VARS SSE41_C_FLAGS)

set(CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET_SAVE})

if(_SSE41_REQUIRED_VARS)
  include(FindPackageHandleStandardArgs)

  find_package_handle_standard_args(SSE41
                                    REQUIRED_VARS ${_SSE41_REQUIRED_VARS})

  mark_as_advanced(${_SSE41_REQUIRED_VARS})

  unset(_SSE41_REQUIRED_VARS)
else()
  message(SEND_ERROR "FindSSE41 requires C or CXX language to be enabled")
endif()

# begin tests for SSE4.1 specfic features

set(SSE41_C_TEST_SOURCE_INSERT64
"
#include <stdint.h>
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <smmintrin.h>
#endif
__m128i foo() {
    __m128i vOne = _mm_set1_epi8(1);
    return _mm_insert_epi64(vOne,INT64_MIN,0);
}
int main(void) { foo(); return 0; }
")

if(SSE41_C_FLAGS)
  include(CheckCSourceCompiles)
  set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  set(CMAKE_REQUIRED_FLAGS "${SSE41_C_FLAGS}")
  check_c_source_compiles("${SSE41_C_TEST_SOURCE_INSERT64}" HAVE_SSE41_MM_INSERT_EPI64)
  set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
endif()

set(SSE41_C_TEST_SOURCE_EXTRACT64
"
#include <stdint.h>
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <smmintrin.h>
#endif
int64_t foo() {
    __m128i vOne = _mm_set1_epi8(1);
    return (int64_t)_mm_extract_epi64(vOne,0);
}
int main(void) { return (int)foo(); }
")

if(SSE41_C_FLAGS)
  include(CheckCSourceCompiles)
  set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  set(CMAKE_REQUIRED_FLAGS "${SSE41_C_FLAGS}")
  check_c_source_compiles("${SSE41_C_TEST_SOURCE_EXTRACT64}" HAVE_SSE41_MM_EXTRACT_EPI64)
  set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
endif()
