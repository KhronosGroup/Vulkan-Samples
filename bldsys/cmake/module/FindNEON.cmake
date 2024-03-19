# Adapted from https://github.com/jeffdaily/parasail/blob/600fb26151ff19899ee39a214972dcf2b9b11ed7/cmake/FindNEON.cmake
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
# FindNEON
# --------
#
# Finds NEON support
#
# This module can be used to detect NEON support in a C compiler.  If
# the compiler supports NEON, the flags required to compile with
# NEON support are returned in variables for the different languages.
# The variables may be empty if the compiler does not need a special
# flag to support NEON.
#
# The following variables are set:
#
# ::
#
#    NEON_C_FLAGS - flags to add to the C compiler for NEON support
#    NEON_FOUND - true if NEON is detected
#
#=============================================================================

set(_NEON_REQUIRED_VARS)
set(CMAKE_REQUIRED_QUIET_SAVE ${CMAKE_REQUIRED_QUIET})
set(CMAKE_REQUIRED_QUIET ${NEON_FIND_QUIETLY})

# sample NEON source code to test
set(NEON_C_TEST_SOURCE
"
#include <arm_neon.h>
uint32x4_t double_elements(uint32x4_t input)
{
    return(vaddq_u32(input, input));
}
int main(void)
{
    uint32x4_t one;
    uint32x4_t two = double_elements(one);
    return 0;
}
")

# if these are set then do not try to find them again,
# by avoiding any try_compiles for the flags
if((DEFINED NEON_C_FLAGS) OR (DEFINED HAVE_NEON))
else()
  if(WIN32)
    set(NEON_C_FLAG_CANDIDATES
      #Empty, if compiler automatically accepts NEON
      " ")
  else()
    set(NEON_C_FLAG_CANDIDATES
      #Empty, if compiler automatically accepts NEON
      " "
      "-mfpu=neon"
      "-mfpu=neon -mfloat-abi=softfp"
    )
  endif()

  include(CheckCSourceCompiles)

  foreach(FLAG IN LISTS NEON_C_FLAG_CANDIDATES)
    set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
    set(CMAKE_REQUIRED_FLAGS "${FLAG}")
    unset(HAVE_NEON CACHE)
    if(NOT CMAKE_REQUIRED_QUIET)
      message(STATUS "Try NEON C flag = [${FLAG}]")
    endif()
    check_c_source_compiles("${NEON_C_TEST_SOURCE}" HAVE_NEON)
    set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
    if(HAVE_NEON)
      set(NEON_C_FLAGS_INTERNAL "${FLAG}")
      break()
    endif()
  endforeach()

  unset(NEON_C_FLAG_CANDIDATES)

  set(NEON_C_FLAGS "${NEON_C_FLAGS_INTERNAL}"
    CACHE STRING "C compiler flags for NEON intrinsics")
endif()

list(APPEND _NEON_REQUIRED_VARS NEON_C_FLAGS)

set(CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET_SAVE})

if(_NEON_REQUIRED_VARS)
  include(FindPackageHandleStandardArgs)

  find_package_handle_standard_args(NEON
                                    REQUIRED_VARS ${_NEON_REQUIRED_VARS})

  mark_as_advanced(${_NEON_REQUIRED_VARS})

  unset(_NEON_REQUIRED_VARS)
else()
  message(SEND_ERROR "FindNEON requires C or CXX language to be enabled")
endif()
