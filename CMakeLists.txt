# Copyright (c) 2020-2024, Arm Limited and Contributors
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
if(NOT DEFINED CMAKE_C_COMPILER_LAUNCHER AND NOT DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
    find_program(CCACHE_FOUND ccache)
    find_program(SCCACHE_FOUND sccache)
    if (SCCACHE_FOUND)
        message("setting SCCACHE to ${SCCACHE_FOUND}")
        set(CMAKE_C_COMPILER_LAUNCHER ${SCCACHE_FOUND})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE_FOUND})
    elseif(CCACHE_FOUND)
        message("setting CCACHE to ${CCACHE_FOUND}")
        set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_FOUND})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_FOUND})
    endif()
endif()

cmake_minimum_required(VERSION 3.16)

set(VCPKG_PACKAGES catch2 cli11 fmt glm glslang imgui spdlog spirv-cross stb tinygltf vulkan-memory-allocator)
include(${CMAKE_CURRENT_SOURCE_DIR}/bldsys/cmake/ezvcpkg.cmake)
set(VCPKG_LIBRARY_LINKAGE static)
if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(VCPKG_TARGET_TRIPLET "arm64-android")
elseif(NOT (VKB_WSI_SELECTION STREQUAL D2D))
    list(APPEND VCPKG_PACKAGES glfw3)
endif()
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

ezvcpkg_fetch(
    COMMIT 2024.01.12
    PACKAGES ${VCPKG_PACKAGES}
    CLEAN_BUILDTREES
)
list(APPEND CMAKE_PREFIX_PATH ${EZVCPKG_DIR} ${EZVCPKG_DIR}/share)

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    # Android's build system freaks out if the toolchain is updated (even though CMake runs successfully,
    # the wrapping build system can't seem to parse the output), but also can't find the bulk of these
    # packages without special help otherwise, so we're giving it a hand here.
    set(fmt_DIR ${EZVCPKG_DIR}/share/fmt)
    set(spdlog_DIR ${EZVCPKG_DIR}/share/spdlog)
    set(imgui_DIR ${EZVCPKG_DIR}/share/imgui)
    set(CLI11_DIR ${EZVCPKG_DIR}/share/cli11)
    set(spirv_cross_core_DIR ${EZVCPKG_DIR}/share/spirv_cross_core)
    set(spirv_cross_glsl_DIR ${EZVCPKG_DIR}/share/spirv_cross_glsl)
else()
    find_package(Catch2 CONFIG REQUIRED)
    # These packages aren't stricktly required, because they're header only libraries and we already get the
    # main vcpkg header path from all the other packages.  However, they lack actual CMake config files
    # and so we can't find them in the Android setup
    find_package(VulkanMemoryAllocator CONFIG REQUIRED)
    find_package(glm CONFIG REQUIRED)
    if (NOT (VKB_WSI_SELECTION STREQUAL D2D))
        find_package(glfw3 CONFIG)
    endif()
endif()

project(vulkan_samples)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)

# globally add VKB_DEBUG for the debug build
add_compile_definitions($<$<CONFIG:DEBUG>:VKB_DEBUG>)

if(MSVC AND (DEFINED CMAKE_C_COMPILER_LAUNCHER))
    message(DEBUG "Setting MSVC flags to /Z7 for ccache compatibility.  Current flags: ${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    message(DEBUG "New flags: ${CMAKE_CXX_FLAGS_DEBUG}")
endif()

# create output folder
file(MAKE_DIRECTORY output)

# Add path for local cmake scripts
list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/bldsys/cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/bldsys/cmake/module)

include(utils)
include(global_options)
include(sample_helper)
include(check_atomic)
include(component_helper)

# Add third party libraries
add_subdirectory(third_party)

vkb__enable_testing()

# Framework v2.0 components directory
add_subdirectory(components)

# Add vulkan framework
add_subdirectory(framework)

if(VKB_BUILD_TESTS)
    # Add vulkan tests
    add_subdirectory(tests)
endif()

if(VKB_BUILD_SAMPLES)
    # Add vulkan samples
    add_subdirectory(samples)
endif()

# Add vulkan app (runs all samples)
add_subdirectory(app)

add_subdirectory(antora)
