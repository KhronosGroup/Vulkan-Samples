# Copyright (c) 2020-2025, Arm Limited and Contributors
# Copyright (c) 2024-2025, Mobica Limited
# Copyright (c) 2024-2025, Sascha Willems
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

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/bldsys/cmake/module")

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

project(vulkan_samples)

if(VKB_GENERATE_ANTORA_SITE)
    add_subdirectory(antora)
else ()
# search for Vulkan SDK
find_package(Vulkan)

if(Vulkan_FOUND)
    if(NOT Vulkan_dxc_exe_FOUND)
        find_program(Vulkan_dxc_EXECUTABLE
                    NAMES dxc
                    HINTS
                    "$ENV{VULKAN_SDK}/Bin"
                    "$ENV{VULKAN_SDK}/bin"
                    )
    endif()
    if(Vulkan_dxc_EXECUTABLE)
        message(STATUS "Found DirectX Shader Compiler under ${Vulkan_dxc_EXECUTABLE}")
    else()
        message(STATUS "Couldn't find DirectX Shader Compiler executable, make sure it is present in Vulkan SDK or add it manually via Vulkan_dxc_EXECUTABLE cmake variable. HLSL shaders won't be compiled.")
    endif()
    # slang compiler
    if(NOT Vulkan_slang_exe_FOUND)
        find_program(Vulkan_slang_EXECUTABLE
                    NAMES slangc
                    HINTS
                    "$ENV{VULKAN_SDK}/Bin"
                    "$ENV{VULKAN_SDK}/bin"
                    )
    endif()
    if(Vulkan_slang_EXECUTABLE)
        message(STATUS "Found slang Shader Compiler under ${Vulkan_slang_EXECUTABLE}")
    else()
        message(STATUS "Couldn't find slang Shader Compiler executable, make sure it is present in Vulkan SDK or add it manually via Vulkan_slang_EXECUTABLE cmake variable. Slang shaders won't be compiled.")
    endif()
    # glsl compiler
    if(NOT Vulkan_glslc_exe_FOUND)
        find_program(Vulkan_glslc_EXECUTABLE
                    NAMES glslc
                    HINTS
                    "$ENV{VULKAN_SDK}/Bin"
                    "$ENV{VULKAN_SDK}/bin"
                    )
    endif()
    if(Vulkan_glslc_EXECUTABLE)
        message(STATUS "Found glslc Shader Compiler under ${Vulkan_glslc_EXECUTABLE}")
    else()
        message(STATUS "Couldn't find glslc Shader Compiler executable, make sure it is present in Vulkan SDK or add it manually via Vulkan_glslc_EXECUTABLE cmake variable. GLSL shaders won't be compiled.")
    endif()
    # spvasm compiler
    if(NOT Vulkan_spirvas_exe_FOUND)
        find_program(Vulkan_spirvas_EXECUTABLE
                    NAMES spirv-as
                    HINTS
                    "$ENV{VULKAN_SDK}/Bin"
                    "$ENV{VULKAN_SDK}/bin"
                    )
    endif()
    if(Vulkan_spirvas_EXECUTABLE)
        message(STATUS "Found spirv-as Shader Compiler under ${Vulkan_spirvas_EXECUTABLE}")
    else()
        message(STATUS "Couldn't find spirv-as Shader Compiler executable, make sure it is present in Vulkan SDK or add it manually via Vulkan_spirvas_EXECUTABLE cmake variable. SPIR-V assembly shaders won't be compiled.")
    endif()

endif()

# globally add VKB_DEBUG for the debug build
add_compile_definitions($<$<CONFIG:DEBUG>:VKB_DEBUG>)

#globally define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS to have designated initializers for the vk-structs
add_compile_definitions(VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)

# globally set -fno-strict-aliasing, needed due to using reinterpret_cast
if (NOT MSVC)
  add_compile_options(-fno-strict-aliasing)
endif()

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

# Framework v2.0 components directory
add_subdirectory(components)

# Add vulkan framework
add_subdirectory(framework)

if(VKB_BUILD_SAMPLES)
    # Add vulkan samples
    add_subdirectory(samples)
endif()

# Add vulkan app (runs all samples)
add_subdirectory(app)
endif ()