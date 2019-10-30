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

function(scan_dirs)
    set(options)
    set(oneValueArgs LIST DIR)
    set(multiValueArgs)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT EXISTS ${TARGET_DIR})
        message(FATAL_ERROR "Directory not found `${TARGET_DIR}`")
    endif()

    file(GLOB DIR_FILES RELATIVE ${TARGET_DIR} ${TARGET_DIR}/*)

    set(DIR_LIST)

    foreach(FILE_NAME ${DIR_FILES})
        if(IS_DIRECTORY ${TARGET_DIR}/${FILE_NAME})
            list(APPEND DIR_LIST ${FILE_NAME})
        endif()
    endforeach()

    set(${TARGET_LIST} ${DIR_LIST} PARENT_SCOPE)
endfunction()

function(create_symlink)
    set(options)
    set(oneValueArgs NAME DIR LINK WORK_DIR)
    set(multiValueArgs)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(WIN32)
        file(TO_NATIVE_PATH ${TARGET_LINK} DIR_LINK)
        file(TO_NATIVE_PATH ${TARGET_DIR} DIR_SRC)

        add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD
            COMMENT "Create symlink for ${TARGET_DIR}"
            COMMAND if exist ${DIR_LINK} rmdir /q ${DIR_LINK}
            COMMAND mklink /D ${DIR_LINK} ${DIR_SRC}
            VERBATIM)
    else()
        add_custom_command(
            TARGET ${TARGET_NAME} PRE_BUILD
            COMMENT "Create symlink for ${TARGET_DIR}"
            COMMAND ${CMAKE_COMMAND} -E create_symlink ${TARGET_DIR} ${TARGET_LINK}
            VERBATIM)
    endif()
endfunction()

function(string_join)
    set(options)
    set(oneValueArgs GLUE)
    set(multiValueArgs INPUT OUTPUT)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    string(REPLACE ";" "${TARGET_GLUE}" RESULT_STR "${TARGET_INPUT}")

    set(${TARGET_OUTPUT} ${RESULT_STR} PARENT_SCOPE)
endfunction()
