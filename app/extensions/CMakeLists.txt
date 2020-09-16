#[[
 Copyright (c) 2019-2020, Arm Limited and Contributors

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

cmake_minimum_required(VERSION 3.12)
project(extensions)

# Helper

function(snake_case_to_pascal_case SNAKE PASCAL)
    set(SNAKE_CASE ${SNAKE})
    string(LENGTH "${SNAKE_CASE}" LEN)
    string(REGEX MATCH "(^.)" FIRST_LETTER "${SNAKE_CASE}")
    string(TOUPPER "${FIRST_LETTER}" FIRST_LETTER)
    string(SUBSTRING "${SNAKE_CASE}" 1 ${LEN} REST)
    set(SNAKE_CASE "${FIRST_LETTER}${REST}")

    string(REGEX MATCH "_([a-zA-Z])[^_]+" HAS_UNDER_SCORES "${SNAKE_CASE}")
    if(HAS_UNDER_SCORES)
        while(true)
            string(REGEX MATCH "_([a-zA-Z])" NEXT "${SNAKE_CASE}")

            if(NEXT)
                string(SUBSTRING "${NEXT}" 1 1 FIRST_LETTER)
                string(TOUPPER "${FIRST_LETTER}" FIRST_LETTER)
                string(REGEX REPLACE "${NEXT}" "${FIRST_LETTER}" SNAKE_CASE "${SNAKE_CASE}")
            else()
                break()
            endif()

        endwhile()
    endif()

    set(${PASCAL} ${SNAKE_CASE} PARENT_SCOPE)
endfunction()

# Extensions

set(EXTENSIONS
    headless
    start_app
    start_test
)

if(NOT ${VKB_BUILD_TESTS})
    list(REMOVE_ITEM EXTENSIONS "start_test")
endif()

# Generate extensions.cpp

set(EXTENSION_INCLUDE_FILES)
set(INIT_EXTENSIONS)

foreach(EXT_SNAKE IN LISTS EXTENSIONS)
    snake_case_to_pascal_case("${EXT_SNAKE}" EXT_PASCAL)

    list(APPEND EXTENSION_INCLUDE_FILES "#include \"${EXT_SNAKE}/${EXT_SNAKE}.h\"")
    list(APPEND INIT_EXTENSIONS "\t\tADD_EXTENSION(${EXT_PASCAL})")
endforeach()

list(JOIN EXTENSION_INCLUDE_FILES "\n" EXTENSION_INCLUDE_FILES)
list(JOIN INIT_EXTENSIONS ";\n" INIT_EXTENSIONS)
set(INIT_EXTENSIONS "${INIT_EXTENSIONS};")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/extensions.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/extensions.cpp)

# Create extensions library

add_library(extension-headers INTERFACE)
target_include_directories(extension-headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

set(SRC_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/extensions.cpp
    extensions.h
)

foreach(EXTENSION IN LISTS EXTENSIONS)
    list(APPEND SRC_FILES "${EXTENSION}/${EXTENSION}.h")
    list(APPEND SRC_FILES "${EXTENSION}/${EXTENSION}.cpp")
endforeach()

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE framework apps extension-headers)
target_link_libraries(${PROJECT_NAME} PUBLIC extension-headers)