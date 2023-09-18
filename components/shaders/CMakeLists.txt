
# Copyright (c) 2023, Thomas Atkinson
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

vkb__register_component(
    NAME shaders
    HEADERS
        # Compilers
        include/shaders/compilers/glsl_compiler.hpp

        # Reflectors
        include/shaders/reflectors/spirv_reflector.hpp

        # Strategies
        include/shaders/strategies/offline_strategy.hpp
        include/shaders/strategies/online_strategy.hpp

        include/shaders/shader_cache.hpp
        include/shaders/shader_handle.hpp
        include/shaders/shader_resources.hpp
    SRC
        src/shader_resources.cpp
        src/reflectors/spirv_reflector.cpp
    LINK_LIBS
        vkb__core
        spirv-reflect-static
        nlohmann_json::nlohmann_json
)