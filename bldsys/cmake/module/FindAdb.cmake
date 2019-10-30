#[[
 Copyright (c) 2019, Arm Limited and Contributors

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

include(FindPackageHandleStandardArgs)

if(CMAKE_HOST_WIN32)
    set(ADB_FILE "adb.exe")
else()
    set(ADB_FILE "adb")
endif()

find_program(ADB_EXECUTABLE
    NAMES
        ${ADB_FILE}
    PATH_SUFFIXES
        bin
    PATHS
        $ENV{ANDROID_HOME})

mark_as_advanced(ADB_EXECUTABLE)

if(ADB_EXECUTABLE)
    execute_process(
        COMMAND ${ADB_EXECUTABLE} --version
        OUTPUT_VARIABLE ADB_VERSION
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    set(ADB_VERSION_PREFIX "Android Debug Bridge version ")
	string(REGEX MATCH  "${ADB_VERSION_PREFIX}[0-9]+.[0-9]+.[0-9]+" ADB_VERSION ${ADB_VERSION})
    string(REPLACE ${ADB_VERSION_PREFIX} "" ADB_VERSION ${ADB_VERSION})
endif()

find_package_handle_standard_args(Adb
    VERSION_VAR ADB_VERSION
    REQUIRED_VARS ADB_EXECUTABLE)
