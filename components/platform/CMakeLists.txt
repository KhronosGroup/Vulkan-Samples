# Copyright (c) 2022, Arm Limited and Contributors
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

vkb__register_component(
  NAME platform
  SRC
    src/dl.cpp
    src/sample.cpp
  LINK_LIBS
    ${CMAKE_DL_LIBS}
  INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

vkb__register_tests_no_catch2(
  NAME "platform_test"
  SRC
    tests/platform.test.cpp
  LIBS
    vkb__platform
    vkb__platform__dummy_sample
)


# Dummy Sample

add_library(vkb__platform__dummy_sample SHARED tests/dummy_sample.cpp)
target_link_libraries(vkb__platform__dummy_sample PUBLIC vkb__platform)
add_dependencies(vkb_tests vkb__platform__dummy_sample)

vkb__register_tests_no_catch2(
  NAME "sample_test"
  SRC
    tests/sample.test.cpp
  LIBS
    vkb__platform
    vkb__platform__dummy_sample
    vkb__vfs
)

