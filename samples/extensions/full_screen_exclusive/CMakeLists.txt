# Copyright (c) 2023, Holochip Corporation
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

get_filename_component(FOLDER_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} PATH)
get_filename_component(CATEGORY_NAME ${PARENT_DIR} NAME)

#Full Screen exclusive is a windows only supported extension.  We'll disable finding the sample on other platforms.
if (WIN32)
    # Windows 11 fixed the reason for using full screen exclusive.
    # We'll test for that here and not list the sample unless it's a candidate version of windows.
    if (CMAKE_SYSTEM_VERSION)
        set(ver ${CMAKE_SYSTEM_VERSION})
        string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
        if ("${verMajor}" LESS_EQUAL "10")
            add_sample(
                    ID ${FOLDER_NAME}
                    CATEGORY ${CATEGORY_NAME}
                    AUTHOR "Holochip Corporation"
                    NAME "Full Screen Exclusive"
                    DESCRIPTION "Demonstrates and showcases full_screen_exclusive related functionalities."
                    SHADER_FILES_GLSL
                    "hello_triangle/triangle.vert"
                    "hello_triangle/triangle.frag")
        endif ()
    endif ()
endif ()