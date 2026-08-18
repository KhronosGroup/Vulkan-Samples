# Copyright (c) 2023, Holochip Inc.
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

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_LIST_DIR}/CMake")

find_package(FFmpeg COMPONENTS avcodec avformat)

# FFMPEG is required for THIS sample, but isn't needed for any other.
# FFMPEG can be a fairly large commitment to install on developer's machines, so test for it and only enable this
# if FFMPEG is setup.
# NB: Vulkan Video isn't supported on any mobile device, so no care is given to figure out if we're building on a mobile device and are linking against the host FFMPEG.
if(${FFMPEG_avdevice_FOUND})
    get_filename_component(FOLDER_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
    get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} PATH)
    get_filename_component(CATEGORY_NAME ${PARENT_DIR} NAME)

    add_subdirectory(VkCodecUtils)

    add_sample(
        ID ${FOLDER_NAME}
        CATEGORY ${CATEGORY_NAME}
        AUTHOR "Holochip"
        NAME "video"
        DESCRIPTION "Sample description"
            LIBS VkCodecUtils
    )
endif()