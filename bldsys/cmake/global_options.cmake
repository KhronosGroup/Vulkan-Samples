#[[
 Copyright (c) 2019-2024, Arm Limited and Contributors

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
    cmake_minimum_required(VERSION 3.24)
if(IOS)
    set(CMAKE_XCODE_GENERATE_SCHEME TRUE)
    list(APPEND CMAKE_MODULE_PATH "$ENV{VULKAN_SDK}/iOS/cmake")
endif ()
    find_package(Vulkan)
    if(Vulkan_VERSION GREATER_EQUAL 1.3)
        set(VKB_ENABLE_PORTABILITY ON CACHE BOOL "Enable portability extension enumeration in the framework.  This is required to be set if running MoltenVK and Vulkan 1.3+" FORCE)
    else()
        set(VKB_ENABLE_PORTABILITY OFF CACHE BOOL "Enable portability extension enumeration in the framework.  This is required to be off if running Vulkan less than 1.3" FORCE)
    endif()
endif()

set(VKB_WARNINGS_AS_ERRORS ON CACHE BOOL "Enable Warnings as Errors")
set(VKB_VALIDATION_LAYERS OFF CACHE BOOL "Enable validation layers for every application.")
set(VKB_VALIDATION_LAYERS_GPU_ASSISTED OFF CACHE BOOL "Enable GPU assisted validation layers for every application (implicitly enables VKB_VALIDATION_LAYERS).")
set(VKB_VALIDATION_LAYERS_BEST_PRACTICES OFF CACHE BOOL "Enable best practices validation layers for every application (implicitly enables VKB_VALIDATION_LAYERS).")
set(VKB_VALIDATION_LAYERS_SYNCHRONIZATION OFF CACHE BOOL "Enable synchronization validation layers for every application (implicitly enables VKB_VALIDATION_LAYERS).")
set(VKB_VULKAN_DEBUG ON CACHE BOOL "Enable VK_EXT_debug_utils or VK_EXT_debug_marker if supported.")
set(VKB_BUILD_SAMPLES ON CACHE BOOL "Enable generation and building of Vulkan best practice samples.")
set(VKB_BUILD_TESTS OFF CACHE BOOL "Enable generation and building of Vulkan best practice tests.")
set(VKB_WSI_SELECTION "XCB" CACHE STRING "Select WSI target (XCB, XLIB, WAYLAND, D2D)")
set(VKB_CLANG_TIDY OFF CACHE STRING "Use CMake Clang Tidy integration")
set(VKB_CLANG_TIDY_EXTRAS "-header-filter=framework,samples,app;-checks=-*,google-*,-google-runtime-references;--fix;--fix-errors" CACHE STRING "Clang Tidy Parameters")

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
    find_program(CLANG_TIDY "clang-tidy" "clang-tidy-15" REQUIRED)
    set(VKB_DO_CLANG_TIDY ${CLANG_TIDY} ${VKB_CLANG_TIDY_EXTRAS})
endif()