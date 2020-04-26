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

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_MODULE_PATH}/module)
set(FOLDER_DIR ${FOLDER_DIR})
set(DEVICE_DIR ${DEVICE_DIR})

find_package(Adb 1.0.39 REQUIRED)

# Ensure that directory exists in the target

set(ADB_COMMAND ${ADB_EXECUTABLE} shell mkdir -p ${DEVICE_DIR})

execute_process(
     COMMAND
     ${ADB_COMMAND})

# Sync files

set(ADB_COMMAND ${ADB_EXECUTABLE} push --sync ${FOLDER_DIR} ${DEVICE_DIR})

execute_process(
     COMMAND
     ${ADB_COMMAND}
     RESULT_VARIABLE
     ret_var
     OUTPUT_VARIABLE
     ret_msg
     OUTPUT_STRIP_TRAILING_WHITESPACE)

if(NOT "${ret_var}" STREQUAL "0")
    message(WARNING "Could not sync ${FOLDER_DIR} to device:\n${ret_msg}")
else()
    message(STATUS "Updating ${FOLDER_DIR}:\n${ret_msg}")
endif()
