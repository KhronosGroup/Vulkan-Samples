# Copyright (c) 2019-2024, Sascha Willems
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
    NAME "Terrain tessellation"
    DESCRIPTION "Using tessellation shaders for dynamic terrain level-of-detail"
    SHADER_FILES_GLSL
        "terrain_tessellation/glsl/terrain.vert"
        "terrain_tessellation/glsl/terrain.frag"
        "terrain_tessellation/glsl/terrain.tesc"
        "terrain_tessellation/glsl/terrain.tese"
        "terrain_tessellation/glsl/skysphere.vert"
        "terrain_tessellation/glsl/skysphere.frag"
    SHADER_FILES_HLSL
        "terrain_tessellation/hlsl/terrain.vert.hlsl"
        "terrain_tessellation/hlsl/terrain.frag.hlsl"
        "terrain_tessellation/hlsl/terrain.tesc.hlsl"
        "terrain_tessellation/hlsl/terrain.tese.hlsl"
        "terrain_tessellation/hlsl/skysphere.vert.hlsl"
        "terrain_tessellation/hlsl/skysphere.frag.hlsl")
