# Copyright (c) 2024-2025, Arm Limited and Contributors
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

add_sample_with_tags(
        ID "hello_graph_pipeline"
        CATEGORY "extensions/graph_pipelines"
        AUTHOR "Arm Ltd."
        NAME "Hello graph pipeline"
        DESCRIPTION "Demonstrates use of graph pipelines for machine learning inference"
        TAGS
            "arm"
        FILES
            "../graph_pipelines_common.h"
            "../graph_pipelines_common.cpp"
            "hello_graph_pipeline.h"
            "hello_graph_pipeline.cpp"
        SHADER_FILES_GLSL
            "graph_pipelines/glsl/blit.frag"
            "graph_pipelines/glsl/fullscreen.vert"
            "graph_pipelines/glsl/visualization.comp"
        SHADER_FILES_SPVASM
            "graph_pipelines/spirv/pooling.spvasm"
)
