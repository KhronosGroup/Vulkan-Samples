# Copyright (c) 2025, Holochip Inc.
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

get_filename_component(FOLDER_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} PATH)
get_filename_component(CATEGORY_NAME ${PARENT_DIR} NAME)

add_sample_with_tags(
    ID ${FOLDER_NAME}
    CATEGORY ${CATEGORY_NAME}
    AUTHOR "Holochip Inc."
    NAME "Ray tracing invocation reorder"
    DESCRIPTION "Demonstrates Shader Execution Reordering (SER) using VK_EXT_ray_tracing_invocation_reorder to reduce divergence"
#    SHADER_FILES_GLSL #@TODO enable when extension is found in glslc
#        "ray_tracing_invocation_reorder/glsl/raygen.rgen"
#        "ray_tracing_invocation_reorder/glsl/miss.rmiss"
#        "ray_tracing_invocation_reorder/glsl/closesthit_metal.rchit"
#        "ray_tracing_invocation_reorder/glsl/closesthit_diffuse.rchit"
#        "ray_tracing_invocation_reorder/glsl/closesthit_glass.rchit"
#    GLSLC_ADDITIONAL_ARGUMENTS
#        "--target-spv=spv1.4"
    SHADER_FILES_SLANG
        "ray_tracing_invocation_reorder/slang/raygen.rgen.slang"
        "ray_tracing_invocation_reorder/slang/miss.rmiss.slang"
        "ray_tracing_invocation_reorder/slang/closesthit_metal.rchit.slang"
        "ray_tracing_invocation_reorder/slang/closesthit_diffuse.rchit.slang"
        "ray_tracing_invocation_reorder/slang/closesthit_glass.rchit.slang"
    )
