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

set(PROJECT_FILES
    src/root_file_system.cpp
    src/helpers.cpp
)

set(ANDROID_FILES
    src/android_aasset_manager.cpp
    src/android.cpp
)

set(UNIX_FILES
    src/unix.cpp
)

set(WINDOWS_FILES
    src/windows.cpp
)

if(ANDROID)
    list(APPEND PROJECT_FILES ${ANDROID_FILES})
    list(APPEND PROJECT_FILES ${UNIX_FILES})
elseif(WIN32)
    list(APPEND PROJECT_FILES ${WINDOWS_FILES})
else()
    list(APPEND PROJECT_FILES ${UNIX_FILES})
endif()

add_library(components_vfs STATIC ${PROJECT_FILES})
target_include_directories(components_vfs PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (ANDROID)
    target_link_libraries(components_vfs PRIVATE log android native_app_glue)
endif()

if(${VKB_BUNDLE_ASSETS})
    target_compile_definitions(components_vfs PUBLIC VKB_BUNDLE_ASSETS)
endif()

add_library(Components::VFS ALIAS components_vfs)