# Copyright (c) 2019-2021, Arm Limited and Contributors
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
target_include_directories(glm INTERFACE ${GLM_DIR})

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
target_include_directories(tinygltf INTERFACE ${TINYGLTF_DIR})

# vulkan
add_library(vulkan INTERFACE)
set(VULKAN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/include)
target_sources(vulkan INTERFACE ${VULKAN_INCLUDE_DIR}/vulkan/vulkan.h)
target_include_directories(vulkan INTERFACE ${VULKAN_INCLUDE_DIR})

target_compile_definitions(vulkan INTERFACE VK_NO_PROTOTYPES)


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
set(VMA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vma/src)
target_sources(vma INTERFACE ${VMA_DIR}/vk_mem_alloc.h)
target_include_directories(vma INTERFACE ${VMA_DIR})
target_link_libraries(vma INTERFACE vulkan)

# libktx
set(KTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ktx)

set(KTX_SOURCES  
    ${KTX_DIR}/lib/checkheader.c
    ${KTX_DIR}/lib/dfdutils/createdfd.c
    ${KTX_DIR}/lib/dfdutils/colourspaces.c
    ${KTX_DIR}/lib/dfdutils/dfd.h
    ${KTX_DIR}/lib/dfdutils/dfd2vk.inl
    ${KTX_DIR}/lib/dfdutils/interpretdfd.c
    ${KTX_DIR}/lib/dfdutils/printdfd.c
    ${KTX_DIR}/lib/dfdutils/queries.c
    ${KTX_DIR}/lib/dfdutils/vk2dfd.c
    ${KTX_DIR}/lib/etcdec.cxx
    ${KTX_DIR}/lib/etcunpack.cxx
    ${KTX_DIR}/lib/filestream.c
    ${KTX_DIR}/lib/filestream.h
    ${KTX_DIR}/lib/formatsize.h
    ${KTX_DIR}/lib/hashlist.c
    ${KTX_DIR}/lib/info.c
    ${KTX_DIR}/lib/ktxint.h
    ${KTX_DIR}/lib/memstream.c
    ${KTX_DIR}/lib/memstream.h
    ${KTX_DIR}/lib/stream.h
    ${KTX_DIR}/lib/strings.c
    ${KTX_DIR}/lib/swap.c
    ${KTX_DIR}/lib/uthash.h
    ${KTX_DIR}/lib/texture.c
    ${KTX_DIR}/lib/texture.h

    # Basis Universal
    ${KTX_DIR}/lib/basis_sgd.h
    ${KTX_DIR}/lib/basis_transcode.cpp
    ${KTX_DIR}/lib/basisu/transcoder/basisu_containers.h
    ${KTX_DIR}/lib/basisu/transcoder/basisu_containers_impl.h
    ${KTX_DIR}/lib/basisu/transcoder/basisu_file_headers.h
    ${KTX_DIR}/lib/basisu/transcoder/basisu_global_selector_cb.h
    ${KTX_DIR}/lib/basisu/transcoder/basisu_global_selector_palette.h
    ${KTX_DIR}/lib/basisu/transcoder/basisu_transcoder_internal.h
    ${KTX_DIR}/lib/basisu/transcoder/basisu_transcoder_uastc.h
    ${KTX_DIR}/lib/basisu/transcoder/basisu_transcoder.cpp
    ${KTX_DIR}/lib/basisu/transcoder/basisu_transcoder.h
    ${KTX_DIR}/lib/basisu/transcoder/basisu.h
    ${KTX_DIR}/lib/basisu/zstd/zstd.c
   
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
    ${KTX_DIR}/lib/vk_funcs.c
    ${KTX_DIR}/lib/vk_funcs.h
    ${KTX_DIR}/lib/vkloader.c    
)

set(KTX_INCLUDE_DIRS
    ${KTX_DIR}/include
    ${KTX_DIR}/lib
    ${KTX_DIR}/lib/basisu/zstd
    ${KTX_DIR}/lib/basisu/transcoder
    ${KTX_DIR}/other_include
)

add_library(ktx ${KTX_SOURCES})

target_compile_definitions(ktx PUBLIC LIBKTX)
if (WIN32)
target_compile_definitions(ktx PUBLIC "KTX_API=__declspec(dllexport)")
endif()
target_compile_definitions(ktx PUBLIC KTX_FEATURE_WRITE=0)
target_compile_definitions(ktx PUBLIC BASISD_SUPPORT_KTX2_ZSTD=0)
target_compile_definitions(ktx PUBLIC BASISU_NO_ITERATOR_DEBUG_LEVEL)

target_include_directories(ktx PUBLIC ${KTX_INCLUDE_DIRS})

target_link_libraries(ktx PUBLIC vulkan)

set_property(TARGET ktx PROPERTY FOLDER "third_party")

# volk
set(VOLK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/volk")
set(VOLK_FILES
    "${VOLK_DIR}/volk.c"
    "${VOLK_DIR}/volk.h")

add_library(volk STATIC ${VOLK_FILES})

target_link_libraries(volk PUBLIC vulkan)

target_include_directories(volk PUBLIC ${VOLK_DIR})
if (VKB_WSI_SELECTION STREQUAL XCB)
    target_include_directories(volk PUBLIC ${XCB_INCLUDE_DIRS})
elseif (VKB_WSI_SELECTION STREQUAL XLIB)
    target_include_directories(volk PUBLIC ${X11_INCLUDE_DIRS})
elseif (VKB_WSI_SELECTION STREQUAL WAYLAND)
    target_include_directories(volk PUBLIC ${WAYLAND_INCLUDE_DIRS})
endif()

set_property(TARGET volk PROPERTY FOLDER "third_party")

# imgui
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
set(IMGUI_FILES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/imconfig.h"
    "${IMGUI_DIR}/imgui.h"
    "${IMGUI_DIR}/imgui_internal.h"
    "${IMGUI_DIR}/imstb_rectpack.h"
    "${IMGUI_DIR}/imstb_textedit.h"
    "${IMGUI_DIR}/imstb_truetype.h")

add_library(imgui STATIC ${IMGUI_FILES})

target_include_directories(imgui PUBLIC ${IMGUI_DIR})

set_property(TARGET imgui PROPERTY FOLDER "third_party")

# glslang
option(ENABLE_SPVREMAPPER OFF)
option(ENABLE_GLSLANG_BINARIES OFF)
set(ENABLE_HLSL ON)
option(ENABLE_OPT OFF)
option(BUILD_TESTING OFF)
option(BUILD_EXTERNAL OFF)

add_subdirectory(glslang)

if (NOT TARGET glslang-default-resource-limits)
    add_library(glslang-default-resource-limits
                glslang/StandAlone/ResourceLimits.cpp)

    set_property(TARGET glslang-default-resource-limits PROPERTY FOLDER "third_party")
    
    target_include_directories(glslang-default-resource-limits
                                PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glslang/StandAlone)
endif()

if(NOT MSVC)
    target_compile_options(glslang PRIVATE
        "-Wno-logical-op-parentheses"
        "-Wno-unused-parameter")

    target_compile_options(SPIRV PRIVATE
        "-Wno-logical-op-parentheses"
        "-Wno-unused-parameter")
endif()

set_property(TARGET glslang PROPERTY FOLDER "third_party")
set_property(TARGET OGLCompiler PROPERTY FOLDER "third_party")
set_property(TARGET OSDependent PROPERTY FOLDER "third_party")
set_property(TARGET SPIRV PROPERTY FOLDER "third_party")
set_property(TARGET HLSL PROPERTY FOLDER "third_party")
set_property(TARGET GenericCodeGen PROPERTY FOLDER "third_party")
set_property(TARGET MachineIndependent PROPERTY FOLDER "third_party")

# spirv-cross
# add_subdirectory(spirv-cross)

# set_property(TARGET spirv-cross PROPERTY FOLDER "third_party")
# set_property(TARGET spirv-cross-core PROPERTY FOLDER "third_party")
# set_property(TARGET spirv-cross-glsl PROPERTY FOLDER "third_party")
# set_property(TARGET spirv-cross-cpp PROPERTY FOLDER "third_party")
# set_property(TARGET spirv-cross-hlsl PROPERTY FOLDER "third_party")
# set_property(TARGET spirv-cross-msl PROPERTY FOLDER "third_party")
# set_property(TARGET spirv-cross-reflect PROPERTY FOLDER "third_party")
# set_property(TARGET spirv-cross-util PROPERTY FOLDER "third_party")

# hwcpipe
add_subdirectory(hwcpipe)

set_property(TARGET hwcpipe PROPERTY FOLDER "third_party")

# stb
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/stb")

# astc
set(ASTC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/astc)
set(ASTC_INCLUDE_DIR ${ASTC_DIR}/Source)
set(ASTC_SOURCE_DIR ${ASTC_DIR}/Source)

set(ASTC_SOURCES
    ${ASTC_SOURCE_DIR}/astc_image_load_store.cpp
    ${ASTC_SOURCE_DIR}/astc_pick_best_endpoint_format.cpp
    ${ASTC_SOURCE_DIR}/astc_color_quantize.cpp
    ${ASTC_SOURCE_DIR}/astc_weight_align.cpp
    ${ASTC_SOURCE_DIR}/astc_integer_sequence.cpp
    ${ASTC_SOURCE_DIR}/astc_ideal_endpoints_and_weights.cpp
    ${ASTC_SOURCE_DIR}/astc_find_best_partitioning.cpp
    ${ASTC_SOURCE_DIR}/astc_compress_symbolic.cpp
    ${ASTC_SOURCE_DIR}/astc_decompress_symbolic.cpp
    ${ASTC_SOURCE_DIR}/astc_symbolic_physical.cpp
    ${ASTC_SOURCE_DIR}/astc_toplevel.cpp
    ${ASTC_SOURCE_DIR}/astc_stb_tga.cpp
    ${ASTC_SOURCE_DIR}/softfloat.cpp
    ${ASTC_SOURCE_DIR}/mathlib.cpp
    ${ASTC_SOURCE_DIR}/astc_quantization.cpp
    ${ASTC_SOURCE_DIR}/astc_block_sizes2.cpp
    ${ASTC_SOURCE_DIR}/astc_percentile_tables.cpp
    ${ASTC_SOURCE_DIR}/astc_averages_and_directions.cpp
    ${ASTC_SOURCE_DIR}/astc_partition_tables.cpp
    ${ASTC_SOURCE_DIR}/astc_color_unquantize.cpp
    ${ASTC_SOURCE_DIR}/astc_encoding_choice_error.cpp
    ${ASTC_SOURCE_DIR}/astc_kmeans_partitioning.cpp
    ${ASTC_SOURCE_DIR}/astc_weight_quant_xfer_tables.cpp
    ${ASTC_SOURCE_DIR}/astc_compute_variance.cpp
    ${ASTC_SOURCE_DIR}/astc_ktx_dds.cpp
)

add_library(astc ${ASTC_SOURCES})
target_include_directories(astc PUBLIC ${ASTC_INCLUDE_DIR})
target_compile_definitions(astc PRIVATE -DNO_STB_IMAGE_IMPLEMENTATION)
set_property(TARGET astc PROPERTY FOLDER "third_party")

if(ANDROID)
    # native_app_glue
    set(NATIVE_APP_GLUE_DIR "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")

    file(GLOB NATIVE_APP_GLUE_FILES "${NATIVE_APP_GLUE_DIR}/*.c" "${NATIVE_APP_GLUE_DIR}/*.h")
    
    add_library(native_app_glue STATIC ${NATIVE_APP_GLUE_FILES})
        
    target_include_directories(native_app_glue PUBLIC ${NATIVE_APP_GLUE_DIR})
    
    set_property(TARGET native_app_glue PROPERTY FOLDER "third_party")
else()
    if (NOT DIRECT_TO_DISPLAY)
        # GLFW
        option(GLFW_BUILD_DOCS OFF)
        option(GLFW_BUILD_TESTS OFF)
        option(GLFW_BUILD_EXAMPLES OFF)
        option(GLFW_BUILD_TESTS OFF)
        option(GLFW_INSTALL OFF)
        if (VKB_WSI_SELECTION STREQUAL WAYLAND)
            set(GLFW_USE_WAYLAND ON)
        endif()

        add_subdirectory(glfw)

        set_property(TARGET glfw PROPERTY FOLDER "third_party")
    endif()
endif()

# spdlog
add_subdirectory(spdlog)

set_property(TARGET spdlog PROPERTY FOLDER "third_party")

# ctpl
add_library(ctpl INTERFACE)
set(CTPL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/CTPL)
target_sources(ctpl INTERFACE ${CTPL_DIR}/ctpl_stl.h)
target_include_directories(ctpl INTERFACE ${CTPL_DIR})

# cli11
set(CLI11_SANITIZERS OFF)
set(CLI11_BUILD_DOCS OFF)
set(CLI11_BUILD_TESTS OFF)
set(CLI11_BUILD_EXAMPLES OFF)
set(CLI11_BUILD_EXAMPLES_JSON OFF)
set(CLI11_SINGLE_FILE_TESTS OFF)
set(CLI11_INSTALL OFF)
set(CLI11_FORCE_LIBCXX OFF)
set(CLI11_CUDA_TESTS OFF)
set(CLI11_CLANG_TIDY OFF)
add_subdirectory(cli11)

# OpenCL
add_library(opencl INTERFACE)
set(OPENCL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/opencl)
target_sources(opencl INTERFACE ${OPENCL_INCLUDE_DIR}/CL/opencl.h)
target_include_directories(opencl INTERFACE ${OPENCL_INCLUDE_DIR})

# Catch2
add_subdirectory(catch2)

set_property(TARGET Catch2 PROPERTY FOLDER "third_party")
set_property(TARGET Catch2WithMain PROPERTY FOLDER "third_party")

# RE2
set(RE2_BUILD_TESTING OFF)
add_subdirectory(re)

set_property(TARGET re2 PROPERTY FOLDER "third_party")
