#[[
 Copyright (c) 2019-2022, Arm Limited and Contributors

 SPDX-License-Identifier: Apache-2.0

 Licensed under the Apache License, Version 2.0 the "License";
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 ]]

if(NOT CMAKE_ANDROID_NDK)
    set(CMAKE_ANDROID_NDK ${ANDROID_NDK})
endif()

# Enable group projects in folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

if(ANDROID)
    set(TARGET_ARCH ${CMAKE_ANDROID_ARCH_ABI})
else()
    set(TARGET_ARCH ${CMAKE_SYSTEM_PROCESSOR})
endif()

if(APPLE)
    # attempt to use Find_Vulkan at least of version 1.3 or greater.  If this is found, then enable the required portability extension so instance.cpp can get created correctly.
    # prior to 1.3 Find_Vulkan should not return a Vulkan_FOUND variable, so should not set an extension that doesn't exist in legacy Vulkan.
    # Note that this is only required for moltenVK implementations.  NB: this does create a warning in CMake that the range isn't supported in FindVulkan.
    cmake_minimum_required(VERSION 3.19)
    find_package(Vulkan 1.3...<2.0)
    if(Vulkan_FOUND)
        set(VKB_ENABLE_PORTABILITY ON CACHE BOOL "Enable portability extension enumeration in the framework.  This is required to be set if running MoltenVK and Vulkan 1.3+" FORCE)
    else()
        set(VKB_ENABLE_PORTABILITY OFF CACHE BOOL "Enable portability extension enumeration in the framework.  This is required to be off if running Vulkan less than 1.3" FORCE)
    endif()
endif()

set(VKB_WARNINGS_AS_ERRORS ON CACHE BOOL "Enable Warnings as Errors")
set(VKB_VALIDATION_LAYERS OFF CACHE BOOL "Enable validation layers for every application.")
set(VKB_VALIDATION_LAYERS_GPU_ASSISTED OFF CACHE BOOL "Enable GPU assisted validation layers for every application.")
set(VKB_VULKAN_DEBUG ON CACHE BOOL "Enable VK_EXT_debug_utils or VK_EXT_debug_marker if supported.")
set(VKB_BUILD_SAMPLES ON CACHE BOOL "Enable generation and building of Vulkan best practice samples.")
set(VKB_BUILD_TESTS OFF CACHE BOOL "Enable generation and building of Vulkan best practice tests.")
set(VKB_WSI_SELECTION "XCB" CACHE STRING "Select WSI target (XCB, XLIB, WAYLAND, D2D)")
set(VKB_CLANG_TIDY OFF CACHE STRING "Use CMake Clang Tidy integration")
set(VKB_CLANG_TIDY_EXTRAS "-header-filter=framework,samples,vulkan_samples;-checks=-*,google-*,-google-runtime-references;-fix" CACHE STRING "Clang Tidy Parameters")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/${CMAKE_BUILD_TYPE}/${TARGET_ARCH}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "lib/${CMAKE_BUILD_TYPE}/${TARGET_ARCH}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "lib/${CMAKE_BUILD_TYPE}/${TARGET_ARCH}")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

string(LENGTH "${CMAKE_SOURCE_DIR}/" ROOT_PATH_SIZE)
add_definitions(-DROOT_PATH_SIZE=${ROOT_PATH_SIZE})

set(CMAKE_C_FLAGS_DEBUG   "-DDEBUG=0 ${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG=0 ${CMAKE_CXX_FLAGS_DEBUG}")

if (VKB_CLANG_TIDY)
    find_program(CLANG_TIDY "clang-tidy", "clang-tidy-15" REQUIRED)
    set(VKB_DO_CLANG_TIDY ${CLANG_TIDY} ${VKB_CLANG_TIDY_EXTRAS})
endif()