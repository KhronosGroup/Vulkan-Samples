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
    NAME "Ray tracing position fetch"
    DESCRIPTION "Uses the VK_KHR_ray_tracing_position_fetch to fetch vertex positions from an acceleration structure"
    SHADER_FILES_GLSL
        "ray_tracing_position_fetch/glsl/closesthit.rchit"
        "ray_tracing_position_fetch/glsl/miss.rmiss"
        "ray_tracing_position_fetch/glsl/raygen.rgen"
    GLSLC_ADDITIONAL_ARGUMENTS
        "--target-spv=spv1.4"
    SHADER_FILES_HLSL
        "ray_tracing_position_fetch/hlsl/closesthit.rchit.hlsl"
        "ray_tracing_position_fetch/hlsl/miss.rmiss.hlsl"
        "ray_tracing_position_fetch/hlsl/raygen.rgen.hlsl"
    DXC_ADDITIONAL_ARGUMENTS
        "-fspv-extension=SPV_KHR_ray_query -fspv-extension=SPV_KHR_ray_tracing"
    )
