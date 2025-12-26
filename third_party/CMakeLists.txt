# Copyright (c) 2019-2025, Arm Limited and Contributors
# Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 the "License";
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.10)

project(third_party LANGUAGES C CXX)

# glm
add_library(glm INTERFACE)
set(GLM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glm)
target_sources(glm INTERFACE ${GLM_DIR}/glm/glm.hpp)
target_include_directories(glm SYSTEM INTERFACE ${GLM_DIR})

target_compile_definitions(glm INTERFACE
    GLM_FORCE_SWIZZLE
    GLM_FORCE_RADIANS
    GLM_FORCE_CTOR_INIT
    GLM_ENABLE_EXPERIMENTAL
)
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_definitions(glm INTERFACE GLM_FORCE_CXX14)
endif()

# tinygltf
add_library(tinygltf INTERFACE)
set(TINYGLTF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tinygltf)
target_sources(tinygltf INTERFACE ${TINYGLTF_DIR}/tiny_gltf.h ${TINYGLTF_DIR}/json.hpp)
target_include_directories(tinygltf SYSTEM INTERFACE ${TINYGLTF_DIR})

# vulkan
add_library(vulkan INTERFACE)
set(VULKAN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/include)
target_sources(vulkan INTERFACE ${VULKAN_INCLUDE_DIR}/vulkan/vulkan.h)
target_include_directories(vulkan SYSTEM INTERFACE ${VULKAN_INCLUDE_DIR})

target_compile_definitions(vulkan INTERFACE VK_NO_PROTOTYPES)

if(VKB_ENABLE_PORTABILITY)
    # When portability is enabled, must enable Vulkan beta extensions for access to VK_KHR_portability_subset
    target_compile_definitions(vulkan INTERFACE VK_ENABLE_BETA_EXTENSIONS)
endif()

if(ANDROID)
    target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_ANDROID_KHR)
elseif(WIN32)
    target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_WIN32_KHR)
elseif(APPLE)
    target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_METAL_EXT)
elseif(UNIX)
    # Choose WSI based on VKB_WSI_SELECTION
    if (VKB_WSI_SELECTION STREQUAL XCB OR VKB_WSI_SELECTION STREQUAL XLIB OR VKB_WSI_SELECTION STREQUAL WAYLAND)
        find_package(PkgConfig REQUIRED)
    endif()
    if (VKB_WSI_SELECTION STREQUAL XCB)
        pkg_check_modules(XCB xcb REQUIRED)
        if (XCB_FOUND)
            target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_XCB_KHR)
        endif()
    elseif (VKB_WSI_SELECTION STREQUAL XLIB)
        pkg_check_modules(X11 x11 REQUIRED)
        if (X11_FOUND)
            target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_XLIB_KHR)
        endif()
    elseif (VKB_WSI_SELECTION STREQUAL WAYLAND)
        pkg_check_modules(WAYLAND wayland-client REQUIRED)
        if (WAYLAND_FOUND)
            target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_WAYLAND_KHR)
        endif()
    elseif (VKB_WSI_SELECTION STREQUAL D2D)
        set(DIRECT_TO_DISPLAY TRUE)
        set(DIRECT_TO_DISPLAY TRUE PARENT_SCOPE)
        target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_DISPLAY_KHR)
    else()
        message(FATAL_ERROR "Unknown WSI")
    endif()
endif()

# vma
add_library(vma INTERFACE)
set(VMA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vma/include)
target_sources(vma INTERFACE ${VMA_DIR}/vk_mem_alloc.h)
target_include_directories(vma SYSTEM INTERFACE ${VMA_DIR})
target_link_libraries(vma INTERFACE vulkan)

# libktx
set(KTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ktx)

set(KTX_SOURCES
    ${KTX_DIR}/lib/checkheader.c
    ${KTX_DIR}/external/dfdutils/createdfd.c
    ${KTX_DIR}/external/dfdutils/colourspaces.c
    ${KTX_DIR}/external/dfdutils/dfd.h
    ${KTX_DIR}/external/dfdutils/dfd2vk.inl
    ${KTX_DIR}/external/dfdutils/interpretdfd.c
    ${KTX_DIR}/external/dfdutils/printdfd.c
    ${KTX_DIR}/external/dfdutils/queries.c
    ${KTX_DIR}/external/dfdutils/vk2dfd.c
    ${KTX_DIR}/external/etcdec/etcdec.cxx
    ${KTX_DIR}/lib/etcunpack.cxx
    ${KTX_DIR}/lib/filestream.c
    ${KTX_DIR}/lib/filestream.h
    ${KTX_DIR}/lib/formatsize.h
    ${KTX_DIR}/lib/hashlist.c
    ${KTX_DIR}/lib/info.c
    ${KTX_DIR}/lib/ktxint.h
    ${KTX_DIR}/lib/memstream.c
    ${KTX_DIR}/lib/memstream.h
    ${KTX_DIR}/lib/strings.c
    ${KTX_DIR}/lib/swap.c
    ${KTX_DIR}/lib/uthash.h
    ${KTX_DIR}/lib/texture.c
    ${KTX_DIR}/lib/texture.h
    ${KTX_DIR}/utils/unused.h
    
    # Basis Universal
    ${KTX_DIR}/lib/basis_sgd.h
    ${KTX_DIR}/lib/basis_transcode.cpp
    ${KTX_DIR}/lib/miniz_wrapper.cpp
    ${KTX_DIR}/external/basisu/transcoder/basisu_containers.h
    ${KTX_DIR}/external/basisu/transcoder/basisu_containers_impl.h
    ${KTX_DIR}/external/basisu/transcoder/basisu_file_headers.h
    ${KTX_DIR}/external/basisu/transcoder/basisu_transcoder_internal.h
    ${KTX_DIR}/external/basisu/transcoder/basisu_transcoder_uastc.h
    ${KTX_DIR}/external/basisu/transcoder/basisu_transcoder.cpp
    ${KTX_DIR}/external/basisu/transcoder/basisu_transcoder.h
    ${KTX_DIR}/external/basisu/transcoder/basisu.h
    ${KTX_DIR}/external/basisu/zstd/zstd.c

    # KT1
    ${KTX_DIR}/lib/texture1.c
    ${KTX_DIR}/lib/texture1.h

    # KTX2
    ${KTX_DIR}/lib/texture2.c
    ${KTX_DIR}/lib/texture2.h

    # Vulkan support
    ${KTX_DIR}/lib/vk_format.h
    ${KTX_DIR}/lib/vkformat_check.c
    ${KTX_DIR}/lib/vkformat_enum.h
    ${KTX_DIR}/lib/vkformat_str.c
    ${KTX_DIR}/lib/vkformat_typesize.c
    ${KTX_DIR}/lib/vkformat_check_variant.c
    ${KTX_DIR}/lib/vk_funcs.c
    ${KTX_DIR}/lib/vk_funcs.h
    ${KTX_DIR}/lib/vkloader.c
)

set(KTX_INCLUDE_DIRS
    ${KTX_DIR}/include
    ${KTX_DIR}/lib
    ${KTX_DIR}/utils
    ${KTX_DIR}/external    
    ${KTX_DIR}/external/basisu/zstd
    ${KTX_DIR}/external/basisu/transcoder
    ${KTX_DIR}/other_include
)

add_library(ktx STATIC ${KTX_SOURCES})

target_compile_definitions(ktx PUBLIC LIBKTX)
if (WIN32)
target_compile_definitions(ktx PUBLIC "KTX_API=__declspec(dllexport)")
endif()
target_compile_definitions(ktx PUBLIC KTX_FEATURE_WRITE=0)
target_compile_definitions(ktx PUBLIC BASISD_SUPPORT_KTX2_ZSTD=0)
target_compile_definitions(ktx PUBLIC BASISU_NO_ITERATOR_DEBUG_LEVEL)

target_include_directories(ktx SYSTEM PUBLIC ${KTX_INCLUDE_DIRS})

target_link_libraries(ktx PUBLIC vulkan)

set_target_properties(ktx PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)

# volk
set(VOLK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/volk")
set(VOLK_FILES
    "${VOLK_DIR}/volk.c"
    "${VOLK_DIR}/volk.h")

add_library(volk STATIC ${VOLK_FILES})
set_target_properties(volk PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(volk PUBLIC vulkan)

target_include_directories(volk SYSTEM PUBLIC ${VOLK_DIR})
if (VKB_WSI_SELECTION STREQUAL XCB)
    target_include_directories(volk SYSTEM PUBLIC ${XCB_INCLUDE_DIRS})
elseif (VKB_WSI_SELECTION STREQUAL XLIB)
    target_include_directories(volk SYSTEM PUBLIC ${X11_INCLUDE_DIRS})
elseif (VKB_WSI_SELECTION STREQUAL WAYLAND)
    target_include_directories(volk SYSTEM PUBLIC ${WAYLAND_INCLUDE_DIRS})
endif()

set_property(TARGET volk PROPERTY FOLDER "ThirdParty")

# imgui
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
set(IMGUI_FILES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/imconfig.h"
    "${IMGUI_DIR}/imgui.h"
    "${IMGUI_DIR}/imgui_internal.h"
    "${IMGUI_DIR}/imstb_rectpack.h"
    "${IMGUI_DIR}/imstb_textedit.h"
    "${IMGUI_DIR}/imstb_truetype.h")

add_library(imgui STATIC ${IMGUI_FILES})

target_include_directories(imgui SYSTEM PUBLIC ${IMGUI_DIR})

set_target_properties(imgui PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)

# spirv-cross
add_subdirectory(spirv-cross)

set_target_properties(spirv-cross PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
set_target_properties(spirv-cross-c PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
set_target_properties(spirv-cross-core PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
set_target_properties(spirv-cross-glsl PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
set_target_properties(spirv-cross-cpp PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
set_target_properties(spirv-cross-hlsl PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
set_target_properties(spirv-cross-msl PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
set_target_properties(spirv-cross-reflect PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
set_target_properties(spirv-cross-util PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)

if(ANDROID)
# hwcpipe
add_subdirectory(hwcpipe)
set_target_properties(hwcpipe PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
endif()

# stb
add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/stb")

# Allow the user to specify the ASTC architecture explicitly if building for a platform
# other than the host architecture (i.e. x86 on an x64 CPU)
if (NOT (DEFINED ASTC_ARCH))
    if (APPLE)
        # Always use NEON when host is arm64 or cross-compiling to iOS physical devices,
        # otherwise look for supported vector instructions based on x86_64 host processor.
        # Don't use AVX2 since it builds for x86_64h which does not match x86_64 project,
        # and lastly fall back to NATIVE if no matches found.
        if(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "arm64" OR ${CMAKE_OSX_SYSROOT} STREQUAL "iphoneos")
            set(ASTC_ARCH NEON)
        else()
            include( FindSSE41 )
            include( FindSSE2 )
            if (${SSE41_FOUND})
                set(ASTC_ARCH SSE41)
            elseif(${SSE2_FOUND})
                set(ASTC_ARCH SSE2)
            else()
                set(ASTC_ARCH NATIVE)
            endif()
        endif()
    else()
        include( FindAVX2 )
        include( FindSSE41 )
        include( FindSSE2 )
        include( FindNEON )
        if (${AVX2_FOUND})
            set(ASTC_ARCH AVX2)
        elseif(${SSE41_FOUND})
            set(ASTC_ARCH SSE41)
        elseif(${SSE2_FOUND})
            set(ASTC_ARCH SSE2)
        elseif(${NEON_FOUND})
            set(ASTC_ARCH NEON)
        else()
            set(ASTC_ARCH NATIVE)
        endif()
    endif()
endif()

string(TOUPPER ${ASTC_ARCH} ASTC_ARCH)
string(TOLOWER ${ASTC_ARCH} ASTC_ARCH_LOWER)
# This is seemingly inconsistent, but the target library for SSE41 has a "." in its name
if(${ASTC_ARCH_LOWER} STREQUAL "sse41")
    set(ASTC_ARCH_LOWER "sse4.1")
endif()

set(ASTCENC_ISA_${ASTC_ARCH} ON)
set(ASTCENC_CLI OFF)
set(ASTCENC_UNITTEST OFF)
set(ASTCENC_DECOMPRESSOR ON)
set(ASTCENC_UNIVERSAL_BUILD OFF)
set(ASTC_RAW_TARGET astcdec-${ASTC_ARCH_LOWER}-static)
set(ASTC_TARGET ${ASTC_RAW_TARGET} PARENT_SCOPE)

# astc
add_subdirectory(astc)

# astc doesn't have separate directories for it's source code and public interface.  Additionally, it includes it's
# own copy of STB. In order to avoid conflicts, we copy the only header we need to the build directory and alter the
# INTERFACE_INCLUDE_DIRECTORIES of the target
if (NOT TARGET ${ASTC_RAW_TARGET})
    message(FATAL_ERROR "Couldn't find expected ASTC library target")
endif()

get_target_property(ASTC_BINARY_DIR ${ASTC_RAW_TARGET} BINARY_DIR)
file(
    COPY astc/Source/astcenc.h
    DESTINATION ${ASTC_BINARY_DIR}/Include)

    set_target_properties(${ASTC_RAW_TARGET} PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ""
    INCLUDE_DIRECTORIES ""
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${ASTC_BINARY_DIR}/Include
)
target_include_directories(${ASTC_RAW_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/astc/Source)
target_include_directories(${ASTC_RAW_TARGET} SYSTEM INTERFACE  ${ASTC_BINARY_DIR}/Include)

if (MSVC)
  target_compile_definitions(${ASTC_RAW_TARGET} PRIVATE _CRT_SECURE_NO_WARNINGS)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(gtest PUBLIC -Wno-deprecated-declarations)
  endif ()
endif ()
set_property(TARGET ${ASTC_RAW_TARGET} PROPERTY FOLDER "ThirdParty")


if(NOT ANDROID AND NOT IOS)
    if (NOT DIRECT_TO_DISPLAY)
        # GLFW
        option(GLFW_BUILD_DOCS OFF)
        option(GLFW_BUILD_TESTS OFF)
        option(GLFW_BUILD_EXAMPLES OFF)
        option(GLFW_BUILD_TESTS OFF)
        option(GLFW_INSTALL OFF)
        option(BUILD_SHARED_LIBS ON)
        if (VKB_WSI_SELECTION STREQUAL WAYLAND)
            set(GLFW_BUILD_WAYLAND ON)
        endif()

        add_subdirectory(glfw)

        if (TARGET update_mappings)
            set_target_properties(update_mappings PROPERTIES FOLDER "ThirdParty")
        endif()
        set_property(TARGET glfw PROPERTY FOLDER "ThirdParty")
    endif()
endif()

# fmt
add_subdirectory(fmt)
set_property(TARGET fmt PROPERTY FOLDER "ThirdParty")

# spdlog
set(SPDLOG_FMT_EXTERNAL ON)
add_subdirectory(spdlog)
set_property(TARGET spdlog PROPERTY FOLDER "ThirdParty")

# OpenCL
add_library(opencl INTERFACE)
set(OPENCL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/opencl)
target_sources(opencl INTERFACE ${OPENCL_INCLUDE_DIR}/CL/opencl.h)
target_include_directories(opencl SYSTEM INTERFACE ${OPENCL_INCLUDE_DIR})

# Tracy
if (VKB_PROFILING)
    set(TRACY_ENABLE ${VKB_PROFILING})
    set(TRACY_TIMER_FALLBACK ON)
    add_subdirectory(tracy)
    add_compile_definitions(TRACY_NO_INVARIANT_CHECK=1)
    set_property(TARGET TracyClient PROPERTY FOLDER "ThirdParty")
endif()
