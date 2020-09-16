# Copyright (c) 2019-2020, Arm Limited and Contributors
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

project(external_project LANGUAGES C CXX)

set(VKB_BUILD_SAMPLES OFF)

# Add vkb as subdirectory
set(VKB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../)
add_subdirectory(${VKB_DIR} ${CMAKE_CURRENT_BINARY_DIR}/vkb)

if(NOT TARGET framework)
	message(FATAL_ERROR "Cannot find target framework")
endif()
