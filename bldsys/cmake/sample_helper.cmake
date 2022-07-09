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

 set_property(GLOBAL PROPERTY SAMPLE_DESCRIPTORS "")

function(vkb__register_sample_descriptor)
    set(options)  
    set(oneValueArgs NAME DESCRIPTION LIB)
    set(multiValueArgs)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})    

    get_property(DESCRIPTORS GLOBAL PROPERTY SAMPLE_DESCRIPTORS)
    list(APPEND DESCRIPTORS "{\"name\":\"${TARGET_NAME}\", \"description\": \"${TARGET_DESCRIPTION}\", \"library_name\": \"${TARGET_LIB}\"}")
    set_property(GLOBAL PROPERTY SAMPLE_DESCRIPTORS ${DESCRIPTORS})
endfunction()

add_custom_target(vkb__samples)

function(vkb__register_sample)
    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs SRC LINK_LIBS INCLUDE_DIRS)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(TARGET_NAME STREQUAL "")
        message(FATAL_ERROR "NAME must be defined in vkb__register_tests")
    endif()

    if(TARGET_SRC) ## Create static library
        message("ADDING SAMPLE: sample__${TARGET_NAME}")

        add_library("sample__${TARGET_NAME}" SHARED ${TARGET_SRC})

        target_link_libraries("sample__${TARGET_NAME}" PUBLIC vkb__platform_headers) # sample_main

        if(TARGET_LINK_LIBS)
            target_link_libraries("sample__${TARGET_NAME}" PUBLIC ${TARGET_LINK_LIBS})
        endif()

        if(TARGET_INCLUDE_DIRS)
            target_include_directories("sample__${TARGET_NAME}" PUBLIC ${TARGET_INCLUDE_DIRS})
        endif()
        
        if(MSVC)
            target_compile_options("sample__${TARGET_NAME}" PRIVATE /W4 /WX)
        else()
            target_compile_options("sample__${TARGET_NAME}" PRIVATE -Wall -Wextra -Wpedantic -Werror)
        endif()
    else()
        message(FATAL_ERROR "a sample must contain one or more source files")
    endif()

    # copy lib to samples_launcher executable
    add_custom_command(
        TARGET "sample__${TARGET_NAME}"
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:sample__${TARGET_NAME}>"
            "$<TARGET_FILE_DIR:samples_launcher>/$<TARGET_FILE_NAME:sample__${TARGET_NAME}>"
    )

    add_dependencies("sample__${TARGET_NAME}" samples_launcher) # automatically build the samples_launcher if a sample is set to compile
    add_dependencies(vkb__samples "sample__${TARGET_NAME}")
endfunction()