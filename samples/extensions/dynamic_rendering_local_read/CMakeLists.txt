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

get_filename_component(FOLDER_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} PATH)
get_filename_component(CATEGORY_NAME ${PARENT_DIR} NAME)

add_sample(
    ID ${FOLDER_NAME}
    CATEGORY ${CATEGORY_NAME}
    AUTHOR "Sascha Willems"
    NAME "Dynamic Rendering local reads"
    DESCRIPTION "Demonstrates the dynamic rendering local read extension to use input attachments with dynamic rendering"
    SHADER_FILES_GLSL
        "dynamic_rendering_local_read/glsl/composition.frag"
        "dynamic_rendering_local_read/glsl/composition.vert"
        "dynamic_rendering_local_read/glsl/scene_opaque.vert"
        "dynamic_rendering_local_read/glsl/scene_opaque.frag"
        "dynamic_rendering_local_read/glsl/scene_transparent.vert"
        "dynamic_rendering_local_read/glsl/scene_transparent.frag"
    SHADER_FILES_HLSL
        "dynamic_rendering_local_read/hlsl/composition.frag.hlsl"
        "dynamic_rendering_local_read/hlsl/composition.vert.hlsl"
        "dynamic_rendering_local_read/hlsl/scene_opaque.vert.hlsl"
        "dynamic_rendering_local_read/hlsl/scene_opaque.frag.hlsl"
        "dynamic_rendering_local_read/hlsl/scene_transparent.vert.hlsl"
        "dynamic_rendering_local_read/hlsl/scene_transparent.frag.hlsl"
    SHADER_FILES_SLANG
        "dynamic_rendering_local_read/slang/composition.frag.slang"
        "dynamic_rendering_local_read/slang/composition.vert.slang"
        "dynamic_rendering_local_read/slang/scene_opaque.vert.slang"
        "dynamic_rendering_local_read/slang/scene_opaque.frag.slang"
        "dynamic_rendering_local_read/slang/scene_transparent.vert.slang"
        "dynamic_rendering_local_read/slang/scene_transparent.frag.slang")