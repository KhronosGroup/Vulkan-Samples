# Copyright (c) 2019-2020, Bradley Austin Davis
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

# The OpenGL interoperability only works on Linux and Windows, and only if glfw is present 
# (implying NOT DIRECT_TO_DISPLAY)
# In theory it could work on Android as well, but would need a compeltely different set of code to 
# create an offscreen OpenGL context.  Further, only a tiny fraction of devices (none of which I have
# access to) appear to support the required semaphore extension: 
# https://opengles.gpuinfo.org/listreports.php?extension=GL_EXT_semaphore
# 
get_filename_component(FOLDER_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} PATH)
get_filename_component(CATEGORY_NAME ${PARENT_DIR} NAME)

# Needed for the OpenGL interoperability example
set(GLAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad")
set(GLAD_SOURCES
    ${GLAD_DIR}/src/glad.c
    ${GLAD_DIR}/include/glad/glad.h
    ${GLAD_DIR}/include/KHR/khrplatform.h
)
add_library(glad ${GLAD_SOURCES})
target_sources(glad PRIVATE ${GLAD_SOURCES})
target_include_directories(glad PUBLIC ${GLAD_DIR}/include)

# Only Compile on platforms that support glfw
# Corresponds to the logic in framework/CMakeLists.txt near "if(VKB_DIRECT_2_DISPLAY)"
if (NOT (VKB_DIRECT_2_DISPLAY OR ANDROID OR DIRECT_TO_DISPLAY))
    add_sample_with_tags(
        ID ${FOLDER_NAME}
        CATEGORY ${CATEGORY_NAME}
        AUTHOR "Bradley Austin Davis"
        NAME "OpenGL Interoperability"
        DESCRIPTION "Example showing sharing resources between OpenGL and Vulkan"
        TAGS
            "opengl"
        LIBS
            glad)
endif()