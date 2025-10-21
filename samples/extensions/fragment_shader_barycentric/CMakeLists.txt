# Copyright (c) 2023-2025, Mobica Limited
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
    AUTHOR "Mobica"
    NAME "Fragment shader barycentric"
    DESCRIPTION "Demonstrate how to use fragment shader barycentric feature, which allows accessing barycentric coordinates for each processed fragment."
    SHADER_FILES_GLSL
        "fragment_shader_barycentric/glsl/object.vert"
        "fragment_shader_barycentric/glsl/object.frag"
        "fragment_shader_barycentric/glsl/skybox.vert"
        "fragment_shader_barycentric/glsl/skybox.frag"
    SHADER_FILES_SLANG
        "fragment_shader_barycentric/slang/object.vert.slang"
        "fragment_shader_barycentric/slang/object.frag.slang"
        "fragment_shader_barycentric/slang/skybox.vert.slang"
        "fragment_shader_barycentric/slang/skybox.frag.slang")
