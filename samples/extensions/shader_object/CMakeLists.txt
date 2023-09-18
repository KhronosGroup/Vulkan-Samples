# Copyright 2023 Nintendo
# 
# Licensed under the Apache License, Version 2.0 (the "License");
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

add_sample(
    ID ${FOLDER_NAME}
    CATEGORY ${CATEGORY_NAME}
    AUTHOR "Nintendo"
    NAME "Shader Object"
    DESCRIPTION "Demonstrates how to use VK_EXT_shader_object."
    SHADER_FILES_GLSL
        "shader_object/basic_normals.frag.glsl"
        "shader_object/basic_normals.vert.glsl"
        "shader_object/basic_n_dot_l.frag.glsl"
        "shader_object/basic_n_dot_l.vert.glsl"
        "shader_object/basic_pos.frag.glsl"
        "shader_object/basic_pos.vert.glsl"
        "shader_object/basic_uv.frag.glsl"
        "shader_object/basic_uv.vert.glsl"
        "shader_object/skybox.frag.glsl"
        "shader_object/skybox.vert.glsl"
        "shader_object/terrain.frag.glsl"
        "shader_object/terrain.vert.glsl"
        "shader_object/material_gen_normals.geom.glsl"
        "shader_object/material_normals.frag.glsl"
        "shader_object/material_n_dot_l.frag.glsl"
        "shader_object/material_pass_sin_offset.geom.glsl"
        "shader_object/material_pass_through.geom.glsl"
        "shader_object/material_reflective.frag.glsl"
        "shader_object/material_rotates.vert.glsl"
        "shader_object/material_scene.frag.glsl"
        "shader_object/material_scene.vert.glsl"
        "shader_object/material_texture.frag.glsl"
        "shader_object/material_wave_x.vert.glsl"
        "shader_object/material_wave_y.vert.glsl"
        "shader_object/material_wave_z.vert.glsl"
        "shader_object/post_process_brighten.frag.glsl"
        "shader_object/post_process_color_cycle.frag.glsl"
        "shader_object/post_process_edge_detection.frag.glsl"
        "shader_object/post_process_FSQ.vert.glsl"
        "shader_object/post_process_grayscale.frag.glsl"
        "shader_object/post_process_invert.frag.glsl"
        "shader_object/post_process_quantize.frag.glsl")
