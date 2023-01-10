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

cmake_minimum_required(VERSION 3.16)

project(vulkan_samples)

# create output folder
file(MAKE_DIRECTORY output)

# Add path for local cmake scripts
list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/bldsys/cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/bldsys/cmake/module)

include(utils)
include(global_options)
include(sample_helper)
include(check_atomic)

# Add third party libraries
add_subdirectory(third_party)

# Add vulkan framework
add_subdirectory(framework)

if(VKB_BUILD_TESTS)
    # Add vulkan tests
    add_subdirectory(tests)
endif()

if(VKB_BUILD_SAMPLES)
    # Add vulkan samples
    add_subdirectory(samples)
endif()

# Add vulkan app (runs all samples)
add_subdirectory(app)
