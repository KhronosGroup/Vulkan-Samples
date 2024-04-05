# Copyright (c) 2021-2024 Holochip Corporation
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
    AUTHOR "Holochip Corporation"
    NAME "Ray tracing extended"
    DESCRIPTION "Extended example of Ray Tracing highlighting the Bottom and Top Level Acceleration Structure rebuild and AO."
    SHADER_FILES_GLSL
        "khr_ray_tracing_extended/glsl/raygen.rgen"
        "khr_ray_tracing_extended/glsl/miss.rmiss"
        "khr_ray_tracing_extended/glsl/closesthit.rchit"
    SHADER_FILES_HLSL
        "khr_ray_tracing_extended/hlsl/raygen.rgen.hlsl"
        "khr_ray_tracing_extended/hlsl/miss.rmiss.hlsl"
        "khr_ray_tracing_extended/hlsl/closesthit.rchit.hlsl"
    DXC_ADDITIONAL_ARGUMENTS "-fspv-extension=SPV_EXT_descriptor_indexing")

