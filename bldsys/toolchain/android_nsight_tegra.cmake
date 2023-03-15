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

# Android Nsight Tegra version requirement
set(REQUIRED_NSIGHT_TEGRA_VERSION "3.4")

# Get Android Nsight Tegra environment
set(NSIGHT_TEGRA_VERSION ${CMAKE_VS_NsightTegra_VERSION})
if( "${NSIGHT_TEGRA_VERSION}" STREQUAL "" )
	get_filename_component(NSIGHT_TEGRA_VERSION "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\NVIDIA Corporation\\Nsight Tegra;Version]" NAME)
endif()

# Report and check version if it exist
if(NOT "${NSIGHT_TEGRA_VERSION}" STREQUAL "")
	message(STATUS "Android Nsight Tegra version found: ${NSIGHT_TEGRA_VERSION}")
	if(NOT "${NSIGHT_TEGRA_VERSION}" VERSION_GREATER_EQUAL "${REQUIRED_NSIGHT_TEGRA_VERSION}+")
		message(FATAL_ERROR "Expected Android Nsight Tegra version: ${REQUIRED_NSIGHT_TEGRA_VERSION}")
	endif()
endif()

# We are building Android platform, fail if Android Nsight Tegra not found
if( NOT NSIGHT_TEGRA_VERSION )
	message(FATAL_ERROR "Engine requires Android Nsight Tegra to be installed in order to build Android platform.")
endif()

file(TO_CMAKE_PATH $ENV{ANDROID_NDK_HOME} CMAKE_ANDROID_NDK)
if(NOT CMAKE_ANDROID_NDK)
	message(FATAL_ERROR "Engine requires CMAKE_ANDROID_NDK environment variable to point to your Android NDK.")
endif()

# Tell CMake we are cross-compiling to Android
set(CMAKE_SYSTEM_NAME Android)

# Tell CMake which version of the Android API we are targeting
set(CMAKE_ANDROID_API_MIN 24 CACHE STRING "")
set(CMAKE_ANDROID_API 24 CACHE STRING "")

set(CMAKE_ANDROID_ARCH "arm64-v8a" CACHE STRING "")

# Tell CMake we don't want to skip Ant step
set(CMAKE_ANDROID_SKIP_ANT_STEP 0)

# Use Clang as the C/C++ compiler
set(CMAKE_GENERATOR_TOOLSET DefaultClang)

# Tell CMake we have our java source in the 'java' directory
set(CMAKE_ANDROID_JAVA_SOURCE_DIR ${CMAKE_SOURCE_DIR}/bldsys/android/vulkan_samples/src/main/java)

# Tell CMake we have use Gradle as our default build system
set(CMAKE_ANDROID_BUILD_SYSTEM GradleBuild)
