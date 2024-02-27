# Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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
#
# SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
# SPDX-License-Identifier: Apache-2.0


get_filename_component(FOLDER_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} PATH)
get_filename_component(CATEGORY_NAME ${PARENT_DIR} NAME)

file(GLOB GLSL_FILES "${CMAKE_SOURCE_DIR}/shaders/${FOLDER_NAME}/*.*")

add_sample(
    ID ${FOLDER_NAME}
    FILES ${GLSL_FILES}
    CATEGORY ${CATEGORY_NAME}
    AUTHOR "Martin-Karl Lefrançois"
    NAME "Ray tracing reflection"
    DESCRIPTION "Example for hardware accelerated ray tracing"
    SHADER_FILES_GLSL
        "ray_tracing_reflection/raygen.rgen"
        "ray_tracing_reflection/miss.rmiss"
        "ray_tracing_reflection/missShadow.rmiss"
        "ray_tracing_reflection/closesthit.rchit")

