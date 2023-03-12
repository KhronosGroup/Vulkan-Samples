# Copyright (c) 2022, Arm Limited and Contributors
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 the "License";
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

vkb__register_component(
  NAME window_headers
  LINK_LIBS
    vkb__events
    volk
  INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# headless window
vkb__register_component(
  NAME headless_window
  SRC
  src/headless.cpp
  LINK_LIBS
  vkb__window_headers
  vkb__common
)

vkb__register_tests(
  NAME headless_window_tests
  SRC
  tests/headless.test.cpp
  LIBS
  vkb__headless_window
  vkb__common
)

if(DIRECT_TO_DISPLAY)
  # Direct to display can only use specific windows. windows past this point are not compatible
  return()
endif()

# glfw window
vkb__register_component(
  NAME glfw_window
  SRC
  src/glfw.cpp
  LINK_LIBS
    vkb__window_headers
    vkb__common
    glfw
)

option(VKB_TEST_GLFW "Enable GLFW Tests" OFF)

if(VKB_TEST_GLFW)
  vkb__register_tests(
    NAME glfw_window_test
    SRC
    tests/glfw.test.cpp
    LIBS
    vkb__glfw_window
  )
endif()
