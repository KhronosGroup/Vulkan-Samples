# Copyright (c) 2019-2022, Arm Limited and Contributors
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

project(test_framework LANGUAGES C CXX)

set(FRAMEWORK_FILES 
    # Header files
    gltf_loader_test.h
    vulkan_test.h 
    # Source Files
    gltf_loader_test.cpp
    vulkan_test.cpp)

source_group("\\" FILES ${FRAMEWORK_FILES})

add_library(${PROJECT_NAME} SHARED ${FRAMEWORK_FILES})

add_library(test_framework_headers INTERFACE)
target_include_directories(test_framework_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# inherit compile definitions from framework target
target_compile_definitions(${PROJECT_NAME} PRIVATE $<TARGET_PROPERTY:framework,COMPILE_DEFINITIONS>)
target_link_libraries(${PROJECT_NAME} PRIVATE framework) 
target_link_libraries(${PROJECT_NAME} PUBLIC test_framework_headers) 
