# SPDX-FileCopyrightText: Copyright (c) 2021 - 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 2.8.12)
project(nvidia-vkvideo-parser, LANGUAGES CXX)

add_library(nvidia-vkvideo-parser SHARED
        src/VulkanH264Parser.cpp
  src/VulkanH265Parser.cpp
  src/VulkanAV1Decoder.cpp
  src/VulkanVP9Decoder.cpp
  src/VulkanAV1GlobalMotionDec.cpp
  src/VulkanVideoDecoder.cpp
  src/nvVulkanh264ScalingList.cpp
  src/cpudetect.cpp
)

target_include_directories(nvidia-vkvideo-parser PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/../..
        ${CMAKE_CURRENT_LIST_DIR}/..
        ${CMAKE_CURRENT_LIST_DIR}/../include
        ${CMAKE_CURRENT_LIST_DIR}/include/NvVideoParser
        ${CMAKE_CURRENT_LIST_DIR}/include/vkvideo_parser
)

if ((CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64") OR (CMAKE_SYSTEM_PROCESSOR MATCHES "^arm64") OR (CMAKE_SYSTEM_PROCESSOR MATCHES "^ARM64"))
  if(WIN32)
    set(NEON_CPU_FEATURE "/arch:armv8.0")
    # set(SVE_CPU_FEATURE "/arch:armv8.0+sve") # clang-cl limitation (SVE intrinsics are not supported by MSVC at the moment)
  elseif(UNIX)
    set(NEON_CPU_FEATURE "-march=armv8-a")
    set(SVE_CPU_FEATURE "-march=armv8-a+sve")
  endif()
    add_library(next_start_code_c OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/NextStartCodeC.cpp include)
    target_include_directories(next_start_code_c PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
    target_include_directories(next_start_code_c PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../.. ${CMAKE_CURRENT_LIST_DIR}/../include)
    add_library(next_start_code_neon OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/NextStartCodeNEON.cpp include)
    set_target_properties(next_start_code_neon PROPERTIES COMPILE_FLAGS ${NEON_CPU_FEATURE} )
    target_include_directories(next_start_code_neon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  if(WIN32) # clang-cl limitation (SVE intrinsics are not supported by MSVC at the moment)
    target_link_libraries(nvidia-vkvideo-parser next_start_code_c next_start_code_neon)
  elseif(UNIX)
    add_library(next_start_code_sve OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/NextStartCodeSVE.cpp include)
    set_target_properties(next_start_code_sve PROPERTIES COMPILE_FLAGS ${SVE_CPU_FEATURE} )
    target_include_directories(next_start_code_sve PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(nvidia-vkvideo-parser next_start_code_c next_start_code_neon next_start_code_sve)
  endif()
elseif ((CMAKE_SYSTEM_PROCESSOR MATCHES "^arm") OR (CMAKE_SYSTEM_PROCESSOR MATCHES "^ARM"))
  if(WIN32)
    set(NEON_CPU_FEATURE "/arch:VFPv4")
  elseif(UNIX)
    set(NEON_CPU_FEATURE "-march=armv7-a+simd")
  endif()
    add_library(next_start_code_c OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/NextStartCodeC.cpp include)
    target_include_directories(next_start_code_c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_include_directories(next_start_code_c PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../.. ${CMAKE_CURRENT_LIST_DIR}/../include)
    add_library(next_start_code_neon OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/NextStartCodeNEON.cpp include)
    set_target_properties(next_start_code_neon PROPERTIES COMPILE_FLAGS ${NEON_CPU_FEATURE} )
    target_include_directories(next_start_code_neon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(nvidia-vkvideo-parser next_start_code_c next_start_code_neon)
else()
  if(WIN32)
    set(SSSE3_CPU_FEATURE "/arch:SSE2")
    set(AVX2_CPU_FEATURE "/arch:AVX2")
    # set(AVX512_CPU_FEATURE "/arch:AVX512")
  elseif(UNIX)
    set(SSSE3_CPU_FEATURE "-mssse3")
    set(AVX2_CPU_FEATURE "-mavx2")
    set(AVX512_CPU_FEATURE "-mavx512f -mavx512bw")
  endif()
  add_library(next_start_code_c OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/NextStartCodeC.cpp include)
  target_include_directories(next_start_code_c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_include_directories(next_start_code_c PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../.. ${CMAKE_CURRENT_LIST_DIR}/../include)
  add_library(next_start_code_ssse3 OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/NextStartCodeSSSE3.cpp include)
  set_target_properties(next_start_code_ssse3 PROPERTIES COMPILE_FLAGS ${SSSE3_CPU_FEATURE} )
  target_include_directories(next_start_code_ssse3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_include_directories(next_start_code_ssse3 PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../.. ${CMAKE_CURRENT_LIST_DIR}/../include)
  add_library(next_start_code_avx2 OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/NextStartCodeAVX2.cpp include)
  set_target_properties(next_start_code_avx2 PROPERTIES COMPILE_FLAGS ${AVX2_CPU_FEATURE} )
  target_include_directories(next_start_code_avx2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_include_directories(next_start_code_avx2 PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../.. ${CMAKE_CURRENT_LIST_DIR}/../include)
  add_library(next_start_code_avx512 OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/NextStartCodeAVX512.cpp include)
  target_include_directories(next_start_code_avx512 PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../.. ${CMAKE_CURRENT_LIST_DIR}/../include)
  if(NOT WIN32)
    set_target_properties(next_start_code_avx512 PROPERTIES COMPILE_FLAGS ${AVX512_CPU_FEATURE} )
  endif()
  target_include_directories(next_start_code_avx512 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(nvidia-vkvideo-parser next_start_code_c next_start_code_ssse3 next_start_code_avx2 next_start_code_avx512)
endif()

target_compile_definitions(nvidia-vkvideo-parser
    PRIVATE NVPARSER_IMPLEMENTATION
    PUBLIC NVPARSER_SHAREDLIB
)
