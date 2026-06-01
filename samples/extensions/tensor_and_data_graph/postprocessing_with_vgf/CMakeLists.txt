# Copyright (c) 2025-2026, Arm Limited and Contributors
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

# Define path to the third-party ai-ml-sdk-vgf-library
set(VGF_LIB_ROOT "${CMAKE_SOURCE_DIR}/third_party/ai-ml-sdk-vgf-library")

# Include directories
set(VGF_LIB_INCLUDE_DIR "${VGF_LIB_ROOT}/include-c")
include_directories(${VGF_LIB_INCLUDE_DIR})

if(WIN32 OR ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_sample_with_tags(
        ID "postprocessing_with_vgf"
        CATEGORY "extensions/tensor_and_data_graph"
        AUTHOR "Arm Ltd."
        NAME "Postprocessing with VGF"
        DESCRIPTION "Demonstrates creating and running a data graph pipeline from a VGF file"
        LIBS
            vgf
        FILES
            ${VGF_LIB_INCLUDE_DIR}/vgf/decoder.h
            "postprocessing_with_vgf.h"
            "postprocessing_with_vgf.cpp"
        SHADER_FILES_GLSL
            "base.vert"
            "base.frag"
            "tensor_and_data_graph/glsl/blit.frag"
            "tensor_and_data_graph/glsl/fullscreen.vert"
    )
endif()