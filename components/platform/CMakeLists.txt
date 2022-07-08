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
  INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

vkb__register_tests_no_catch2(
  NAME "platform_tests"
  SRC
    tests/platform.test.cpp
  LIBS
    vkb__platform
)