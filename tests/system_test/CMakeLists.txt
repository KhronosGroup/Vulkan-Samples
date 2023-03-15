# Copyright (c) 2019-2023, Arm Limited and Contributors
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
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif(CCACHE_FOUND)


cmake_minimum_required(VERSION 3.16)

scan_dirs(
    DIR ${CMAKE_CURRENT_SOURCE_DIR}/sub_tests/
    LIST SUB_TESTS)

# Create test framework
add_subdirectory(test_framework)

# Add each test from the sub test directory
foreach(TEST ${SUB_TESTS})
    add_subdirectory(sub_tests/${TEST})
endforeach()

# Make test list visible parent scope (required by vulkan_samples project)
set(TOTAL_TEST_ID_LIST ${SUB_TESTS} PARENT_SCOPE)
