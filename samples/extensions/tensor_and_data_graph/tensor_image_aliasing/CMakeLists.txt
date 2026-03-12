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
        ID "tensor_image_aliasing"
        CATEGORY "extensions/tensor_and_data_graph"
        AUTHOR "Arm Ltd."
        NAME "Tensor image aliasing"
        DESCRIPTION "Demonstrates aliasing tensors and images to the same underlying memory to achieve zero-copy interop between rendering and data graphs"
        FILES
            "tensor_image_aliasing.h"
            "tensor_image_aliasing.cpp"
        SHADER_FILES_GLSL
            "base.vert"
            "base.frag"
            "tensor_and_data_graph/glsl/blit.frag"
            "tensor_and_data_graph/glsl/fullscreen.vert"
        SHADER_FILES_SPVASM
            "tensor_and_data_graph/tensor_image_aliasing/spirv/conv2d_int8.spvasm"
    )
endif()