# Copyright (c) 2020-2022, Arm Limited and Contributors
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

cmake_minimum_required(VERSION 3.12)

project(vulkan_samples)

# Add path for local cmake scripts
list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/bldsys/cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/bldsys/cmake/module)

include(utils)
include(global_options)
include(sample_helper)
include(check_atomic)
include(test_helpers)

# Add third party libraries
add_subdirectory(third_party)

vkb__enable_testing()

add_subdirectory(components)