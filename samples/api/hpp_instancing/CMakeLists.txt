# Copyright (c) 2022-2024, NVIDIA CORPORATION. All rights reserved.
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

get_filename_component(FOLDER_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} PATH)
get_filename_component(CATEGORY_NAME ${PARENT_DIR} NAME)

add_sample_with_tags(
    ID ${FOLDER_NAME}
    CATEGORY ${CATEGORY_NAME}
    AUTHOR "Sascha Willems"
    NAME "HPP Instancing"
    DESCRIPTION "Instanced mesh rendering, uses a separate vertex buffer for instanced data, using vulkan.hpp"
    SHADER_FILES_GLSL
        "instancing/glsl/instancing.vert"
        "instancing/glsl/instancing.frag"
        "instancing/glsl/planet.vert"
        "instancing/glsl/planet.frag"
        "instancing/glsl/starfield.vert"
        "instancing/glsl/starfield.frag"
    SHADER_FILES_HLSL
        "instancing/hlsl/instancing.vert.hlsl"
        "instancing/hlsl/instancing.frag.hlsl"
        "instancing/hlsl/planet.vert.hlsl"
        "instancing/hlsl/planet.frag.hlsl"
        "instancing/hlsl/starfield.vert.hlsl"
        "instancing/hlsl/starfield.frag.hlsl")
