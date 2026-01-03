# Copyright (c) 2025-2026, Holochip Inc.
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
    # Note: We do not compile GLSL here because glslc may lack GL_EXT_shader_invocation_reorder.
    # load_shader() will look under shaders/<sample>/glsl, so we copy Slang SPV there (see custom target below).
    # Keep Slang shaders available for alternative toolchains
    SHADER_FILES_SLANG
        "ray_tracing_invocation_reorder/slang/raygen.rgen.slang"
        "ray_tracing_invocation_reorder/slang/miss.rmiss.slang"
    )

# Copy Slang-compiled SPIR-V to the GLSL directory so load_shader() can find them
# This mirrors the pathing used by ray_tracing_extended (GLSL folder) while keeping Slang as the compiler.
add_custom_target(${FOLDER_NAME}-copy-slang-to-glsl ALL
    DEPENDS ${FOLDER_NAME}-SLANG
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/shaders/${FOLDER_NAME}/glsl
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${PROJECT_SOURCE_DIR}/shaders/${FOLDER_NAME}/slang/raygen.rgen.spv
            ${PROJECT_SOURCE_DIR}/shaders/${FOLDER_NAME}/glsl/raygen.rgen.spv
)

# Also copy the GLSL SPIR-V from ray_tracing_extended to ensure shader parity
# Do not reuse `ray_tracing_extended` SPIR-V. Keep local shader copies for this sample only.
