# Copyright (c) 2025, Arm Limited and Contributors
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
                AUTHOR "Arm"
                NAME "Fragment Density Map"
                DESCRIPTION "Demonstration of how to use VK_EXT_fragment_density_map"
                SHADER_FILES_GLSL
                    "fragment_density_map/forward.frag"
                    "fragment_density_map/forward.vert"
                    "fragment_density_map/forward_debug.frag"
                    "fragment_density_map/generate_density_map.comp"
                    "fragment_density_map/generate_density_map.frag"
                    "fragment_density_map/quad_uv.vert"
                    "fragment_density_map/quad_uvw.vert"
                    "fragment_density_map/sky.frag"
                    "fragment_density_map/sky_debug.frag"
                    "fragment_density_map/texture.frag")
