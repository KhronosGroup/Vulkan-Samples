# Copyright (c) 2021-2026, Arm Limited and Contributors
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
    AUTHOR "Hans-Kristian Arntzen"
    NAME "Async compute"
    DESCRIPTION "Using multiple queues to achieve more parallelism on the GPU."
    SHADER_FILES_GLSL
        "async_compute/glsl/blur_down.comp"
        "async_compute/glsl/blur_up.comp"
        "async_compute/glsl/forward.vert"
        "async_compute/glsl/forward.frag"
        "async_compute/glsl/shadow.vert"
        "async_compute/glsl/shadow.frag"
        "async_compute/glsl/composite.vert"
        "async_compute/glsl/composite.frag"
        "async_compute/glsl/threshold.comp"
    SHADER_FILES_SLANG
        "async_compute/slang/blur_down.comp.slang"
        "async_compute/slang/blur_up.comp.slang"
        "async_compute/slang/forward.vert.slang"
        "async_compute/slang/forward.frag.slang"
        "async_compute/slang/shadow.vert.slang"
        "async_compute/slang/shadow.frag.slang"
        "async_compute/slang/composite.vert.slang"
        "async_compute/slang/composite.frag.slang"
        "async_compute/slang/threshold.comp.slang")
