#[[
 Copyright (c) 2022, Arm Limited and Contributors

 SPDX-License-Identifier: Apache-2.0

 Licensed under the Apache License, Version 2.0 the "License";
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 ]]

set(CMAKE_SYSTEM_NAME Android)

set(ANDROID_NDK_HOME CACHE STRING "")

if(DEFINED ENV{ANDROID_NDK_HOME} AND NOT ANDROID_NDK_HOME)
    file(TO_CMAKE_PATH $ENV{ANDROID_NDK_HOME} ANDROID_NDK_HOME)
    set(ANDROID_NDK_HOME ${ANDROID_NDK_HOME} CACHE STRING "" FORCE)
else()
    set(ANDROID_STANDALONE_TOOLCHAIN CACHE STRING "")

    if(DEFINED ENV{ANDROID_STANDALONE_TOOLCHAIN})
        file(TO_CMAKE_PATH $ENV{ANDROID_STANDALONE_TOOLCHAIN} ANDROID_STANDALONE_TOOLCHAIN)
        set(ANDROID_STANDALONE_TOOLCHAIN ${ANDROID_STANDALONE_TOOLCHAIN} CACHE STRING "" FORCE)
    endif()
endif()

if(NOT ANDROID_NDK_HOME AND NOT ANDROID_STANDALONE_TOOLCHAIN)
	message(FATAL_ERROR "Required ANDROID_NDK_HOME or ANDROID_STANDALONE_TOOLCHAIN environment variable to point to your Android NDK Root/Android Standalone Toolchain.")
endif()

set(CMAKE_ANDROID_NDK ${ANDROID_NDK_HOME})
set(CMAKE_ANDROID_STANDALONE_TOOLCHAIN ${ANDROID_STANDALONE_TOOLCHAIN})

set(CMAKE_ANDROID_API 24 CACHE STRING "")

set(CMAKE_ANDROID_ARCH_ABI "arm64-v8a" CACHE STRING "")

set(CMAKE_ANDROID_STL_TYPE "c++_static" CACHE STRING "")

set(CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION "clang" CACHE STRING "")
