#[[

 Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 Copyright (c) 2026, Stephen Saunders

 SPDX-License-Identifier: BSD-3-Clause-Clear

 Licensed under the Apache License, Version 2.0 the "License";
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 ------------------------------------------------------------------------
 
 Co-Authored-By: (Qualcomm) Giancane, Francesco <fgiancan@qti.qualcomm.com>
 Co-Authored-By: Stephen Saunders

 ]]

# As per upstream CMake recommendation in https://gitlab.kitware.com/cmake/cmake/-/issues/27661
# for iOS we can use a toolchain file to augment search paths and simplify command line
#
# iOS/setup-env.sh will first take care of defining the VULKAN_SDK environment variable
# Toolchain file shall define:
# - Target system name to iOS
# - Minimum deployment target to iOS 16.3
# - Build only the active arch for devices and simulator
# - Export the $ENV{VULKAN_SDK} variable to `CMAKE_FIND_ROOT_PATH`
#
# Note: We augment CMAKE_FIND_ROOT_PATH to find the iOS Vulkan frameworks from the SDK.
# We don't define CMAKE_OSX_SYSROOT here since we need to build for both physical devices
# (iphoneos) and simulator (iphonesimulator) - do that instead on the cmake command line.

set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_OSX_DEPLOYMENT_TARGET 16.3)
set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH YES)
list(APPEND CMAKE_FIND_ROOT_PATH "$ENV{VULKAN_SDK}")
