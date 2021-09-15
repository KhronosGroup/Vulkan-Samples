#[[
 Copyright (c) 2019-2021, Arm Limited and Contributors

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

cmake_minimum_required(VERSION 3.10.2)

# Snake case to Pascal case helper

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

# Plugins
set(PLUGIN_NAMES
    batch_mode
    benchmark_mode
    force_close
    fps_logger
    screenshot
    start_sample
    start_test
    stop_after
    window_options
)

string(REGEX REPLACE "([^;]+)" "${CMAKE_CURRENT_SOURCE_DIR}/\\1" PLUGIN_FILES "${PLUGIN_NAMES}")

set(PLUGINS)
foreach(DIR IN LISTS PLUGIN_FILES)
    if (IS_DIRECTORY ${DIR})
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" PLUGIN ${DIR})
        list(APPEND PLUGINS "${PLUGIN}")
    endif()
endforeach()

# filter compiled plugins
if(NOT ${VKB_BUILD_TESTS})
    list(REMOVE_ITEM PLUGINS "start_test")
endif()

# Generate plugins.cpp

set(PLUGIN_INCLUDE_FILES)
set(INIT_PLUGINS)

foreach(EXT_SNAKE IN LISTS PLUGINS)
    message("-- Plugin `${EXT_SNAKE}` - BUILD")
    snake_case_to_pascal_case("${EXT_SNAKE}" EXT_PASCAL)

    list(APPEND PLUGIN_INCLUDE_FILES "#include \"${EXT_SNAKE}/${EXT_SNAKE}.h\"")
    list(APPEND INIT_PLUGINS "\t\tADD_PLUGIN(${EXT_PASCAL})")
endforeach()

string_join(INPUT ${PLUGIN_INCLUDE_FILES} GLUE "\n" OUTPUT PLUGIN_INCLUDE_FILES)
string_join(INPUT ${INIT_PLUGINS} GLUE "\n" OUTPUT INIT_PLUGINS)
set(INIT_PLUGINS "${INIT_PLUGINS}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/plugins.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/plugins.cpp)

# Create plugins library

set(SRC_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/plugins.cpp
    plugins.h
)

foreach(PLUGIN IN LISTS PLUGINS)
    list(APPEND SRC_FILES "${PLUGIN}/${PLUGIN}.h")
    list(APPEND SRC_FILES "${PLUGIN}/${PLUGIN}.cpp")
endforeach()

add_library(plugins STATIC ${SRC_FILES})
target_link_libraries(plugins PRIVATE framework apps)
target_include_directories(plugins PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})