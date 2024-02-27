# Copyright (c) 2019-2023, Sascha Willems
# Copyright (c) 2023, Holochip Corporation
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
        AUTHOR "Holochip Corporation"
        NAME "Memory Budget"
        DESCRIPTION "Modifications were created by Holochip Corporation. Physical device memory usage/budget are displayed using memory budget extension features."
        SHADER_FILES_GLSL
        "instancing/instancing.vert"
        "instancing/instancing.frag"
        "instancing/planet.vert"
        "instancing/planet.frag"
        "instancing/starfield.vert"
        "instancing/starfield.frag")
