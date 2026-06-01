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

if(WIN32 OR ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_sample_with_tags(
        ID "compute_shaders_with_tensors"
        CATEGORY "extensions/tensor_and_data_graph"
        AUTHOR "Arm Ltd."
        NAME "Compute shaders with tensors"
        DESCRIPTION "Demonstrates use of compute shaders using tensors as input and output"
        TAGS
            "arm"
        FILES
            "compute_shaders_with_tensors.h"
            "compute_shaders_with_tensors.cpp"
        SHADER_FILES_GLSL
            "tensor_and_data_graph/glsl/blit.frag"
            "tensor_and_data_graph/glsl/fullscreen.vert"
            "tensor_and_data_graph/compute_shaders_with_tensors/glsl/visualization_three_tensors.comp"
            "tensor_and_data_graph/compute_shaders_with_tensors/glsl/preprocessing.comp"
            "tensor_and_data_graph/compute_shaders_with_tensors/glsl/postprocessing.comp"
        SHADER_FILES_SPVASM
            "tensor_and_data_graph/spirv/conv2d.spvasm"
    )
endif()