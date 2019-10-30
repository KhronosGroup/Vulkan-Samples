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
    set(GRADLE_FILE "gradle.bat")
else()
    set(GRADLE_FILE "gradle")
endif()

find_program(GRADLE_EXECUTABLE
    NAMES 
        ${GRADLE_FILE}
    PATH_SUFFIXES 
        bin
    PATHS
        $ENV{GRADLE_HOME})

mark_as_advanced(GRADLE_EXECUTABLE)

if(GRADLE_EXECUTABLE)
    execute_process(
        COMMAND ${GRADLE_EXECUTABLE} --version
        OUTPUT_VARIABLE GRADLE_VERSION
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    
	string(REGEX MATCH  "Gradle [0-9]+.[0-9]+" GRADLE_VERSION ${GRADLE_VERSION})
    string(REPLACE "Gradle " "" GRADLE_VERSION ${GRADLE_VERSION})
endif()
        
find_package_handle_standard_args(Gradle 
    VERSION_VAR GRADLE_VERSION
    REQUIRED_VARS GRADLE_EXECUTABLE)
