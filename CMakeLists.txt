# Copyright (c) 2020-2024, Arm Limited and Contributors
# Copyright (c) 2024, Mobica Limited
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

include(FindVulkan)

# If CMake has the FindVulkan module and it works, use it.
find_package(Vulkan QUIET)

if(DEFINED ENV{VULKAN_SDK})
    message("Found VULKAN_SDK under $ENV{VULKAN_SDK}")
    if(WIN32)
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            find_program(DXC_EXECUTABLE
                        NAMES dxc
                        HINTS
                        "$ENV{VULKAN_SDK}/Bin"
                        )
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
            find_program(DXC_EXECUTABLE
                        NAMES dxc
                        HINTS
                        "$ENV{VULKAN_SDK}/Bin32"
                        )
        endif()
    else()
        find_program(DXC_EXECUTABLE
                    NAMES dxc
                    HINTS
                    "$ENV{VULKAN_SDK}/bin"
                    )
    endif()
    if(DEFINED DXC_EXECUTABLE)
        message("Found DirectX Shader Compiler under ${DXC_EXECUTABLE}")
    elseif()
        message("Couldn't find DirectX Shader Compiler executable, make sure it is present in Vulkan SDK or add it manually via Vulkan_dxc_EXECUTABLE env variable")
    endif()
else()
    message("VULKAN_SDK path not found, HLSL shaders won't be compiled")
endif()

# globally add VKB_DEBUG for the debug build
add_compile_definitions($<$<CONFIG:DEBUG>:VKB_DEBUG>)

project(vulkan_samples)

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
