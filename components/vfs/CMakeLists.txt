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

### components__vfs

set(PROJECT_FILES
    src/root_file_system.cpp
    src/helpers.cpp
)

if(ANDROID)
    set(ANDROID_FILES
        src/android_aasset_manager.cpp
        src/android.cpp
        src/unix.cpp
    )

    list(APPEND PROJECT_FILES ${ANDROID_FILES})
elseif(WIN32)
    set(WINDOWS_FILES
        src/windows.cpp
    )

    list(APPEND PROJECT_FILES ${WINDOWS_FILES})
else()
    set(UNIX_FILES
        src/unix.cpp
    )

    list(APPEND PROJECT_FILES ${UNIX_FILES})
endif()

vkb__register_headers(
    FOLDER
        vfs
    HEADERS
        android.hpp
        filesystem.hpp
        helpers.hpp
        unix.hpp
        windows.hpp
    OUTPUT
      VFS_HEADERS
)

vkb__register_component(
  NAME vfs
  SRC ${PROJECT_FILES}
  HEADERS ${VFS_HEADERS}
  INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
  LINK_LIBS
    re2
    vkb__common
)

if (ANDROID)
    target_link_libraries(vkb__vfs PRIVATE log android native_app_glue)
endif()

if(${VKB_BUNDLE_ASSETS})
    target_compile_definitions(vkb__vfs PUBLIC VKB_BUNDLE_ASSETS)
endif()



### virtual_file_system_tests - should work on desktop platforms as they require no specific configuration from a context

vkb__register_tests(
    NAME "virtual_file_system_tests"
    SRC
        tests/basic.test.cpp
        tests/helpers.test.cpp
    LIBS
        vkb__vfs
)