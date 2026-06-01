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
        ID "graph_constants"
        CATEGORY "extensions/tensor_and_data_graph"
        AUTHOR "Arm Ltd."
        NAME "Graph constants"
        DESCRIPTION "Demonstrates use of data graphs with constant data for machine learning inference"
        TAGS
            "arm"
        FILES
            "graph_constants.h"
            "graph_constants.cpp"
        SHADER_FILES_GLSL
            "tensor_and_data_graph/glsl/blit.frag"
            "tensor_and_data_graph/glsl/fullscreen.vert"
            "tensor_and_data_graph/glsl/visualization_two_tensors.comp"
        SHADER_FILES_SPVASM
            "tensor_and_data_graph/spirv/conv2d.spvasm"
    )
endif()