# Adapted from https://github.com/jeffdaily/parasail/blob/600fb26151ff19899ee39a214972dcf2b9b11ed7/cmake/FindSSE2.cmake
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
# FindSSE2
# --------
#
# Finds SSE2 support
#
# This module can be used to detect SSE2 support in a C compiler.  If
# the compiler supports SSE2, the flags required to compile with
# SSE2 support are returned in variables for the different languages.
# The variables may be empty if the compiler does not need a special
# flag to support SSE2.
#
# The following variables are set:
#
# ::
#
#    SSE2_C_FLAGS - flags to add to the C compiler for SSE2 support
#    SSE2_FOUND - true if SSE2 is detected
#
#=============================================================================

set(_SSE2_REQUIRED_VARS)
set(CMAKE_REQUIRED_QUIET_SAVE ${CMAKE_REQUIRED_QUIET})
set(CMAKE_REQUIRED_QUIET ${SSE2_FIND_QUIETLY})

# sample SSE2 source code to test
set(SSE2_C_TEST_SOURCE
"
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <emmintrin.h>
#endif
int foo() {
    __m128i vOne = _mm_set1_epi16(1);
    __m128i result = _mm_add_epi16(vOne,vOne);
    return _mm_extract_epi16(result, 0);
}
int main(void) { return foo(); }
")

# if these are set then do not try to find them again,
# by avoiding any try_compiles for the flags
if((DEFINED SSE2_C_FLAGS) OR (DEFINED HAVE_SSE2))
else()
  if(WIN32)
    set(SSE2_C_FLAG_CANDIDATES
      #Empty, if compiler automatically accepts SSE2
      " "
      "/arch:SSE2")
  else()
    set(SSE2_C_FLAG_CANDIDATES
      #Empty, if compiler automatically accepts SSE2
      " "
      #GNU, Intel
      "-march=core2"
      #clang
      "-msse2"
    )
  endif()

  include(CheckCSourceCompiles)

  foreach(FLAG IN LISTS SSE2_C_FLAG_CANDIDATES)
    set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
    set(CMAKE_REQUIRED_FLAGS "${FLAG}")
    unset(HAVE_SSE2 CACHE)
    if(NOT CMAKE_REQUIRED_QUIET)
      message(STATUS "Try SSE2 C flag = [${FLAG}]")
    endif()
    check_c_source_compiles("${SSE2_C_TEST_SOURCE}" HAVE_SSE2)
    set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
    if(HAVE_SSE2)
      set(SSE2_C_FLAGS_INTERNAL "${FLAG}")
      break()
    endif()
  endforeach()

  unset(SSE2_C_FLAG_CANDIDATES)

  set(SSE2_C_FLAGS "${SSE2_C_FLAGS_INTERNAL}"
    CACHE STRING "C compiler flags for SSE2 intrinsics")
endif()

list(APPEND _SSE2_REQUIRED_VARS SSE2_C_FLAGS)

set(CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET_SAVE})

if(_SSE2_REQUIRED_VARS)
  include(FindPackageHandleStandardArgs)

  find_package_handle_standard_args(SSE2
                                    REQUIRED_VARS ${_SSE2_REQUIRED_VARS})

  mark_as_advanced(${_SSE2_REQUIRED_VARS})

  unset(_SSE2_REQUIRED_VARS)
else()
  message(SEND_ERROR "FindSSE2 requires C or CXX language to be enabled")
endif()

set(SSE2_C_TEST_SOURCE_SET1_EPI64X
"
#include <stdint.h>
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <emmintrin.h>
#endif
__m128i foo() {
    __m128i vOne = _mm_set1_epi64x(1);
    return vOne;
}
int main(void) { foo(); return 0; }
")

if(SSE2_C_FLAGS)
  include(CheckCSourceCompiles)
  set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  set(CMAKE_REQUIRED_FLAGS "${SSE2_C_FLAGS}")
  check_c_source_compiles("${SSE2_C_TEST_SOURCE_SET1_EPI64X}" HAVE_SSE2_MM_SET1_EPI64X)
  set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
endif()

set(SSE2_C_TEST_SOURCE_SET_EPI64X
"
#include <stdint.h>
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <emmintrin.h>
#endif
__m128i foo() {
    __m128i vOne = _mm_set_epi64x(1,1);
    return vOne;
}
int main(void) { foo(); return 0; }
")

if(SSE2_C_FLAGS)
  include(CheckCSourceCompiles)
  set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  set(CMAKE_REQUIRED_FLAGS "${SSE2_C_FLAGS}")
  check_c_source_compiles("${SSE2_C_TEST_SOURCE_SET_EPI64X}" HAVE_SSE2_MM_SET_EPI64X)
  set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
endif()
