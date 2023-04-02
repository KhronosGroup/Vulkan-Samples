# Copyright (c) 2022, Arm Limited and Contributors
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 the "License";
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

add_custom_target(vkb__components)

set_property(TARGET vkb__components PROPERTY FOLDER "components")

function(vkb__register_headers)
    set(options)
    set(oneValueArgs FOLDER OUTPUT)
    set(multiValueArgs HEADERS)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(TEMP ${TARGET_HEADERS})
    list(TRANSFORM TEMP PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/include/components/${TARGET_FOLDER}/")
    set(${TARGET_OUTPUT} ${TEMP} PARENT_SCOPE)
endfunction()

function(vkb__register_component)
    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs SRC HEADERS LINK_LIBS INCLUDE_DIRS)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(TARGET_NAME STREQUAL "")
        message(FATAL_ERROR "NAME must be defined in vkb__register_tests")
    endif()


    # Create interface header only library
    # This does not garantee that all headers will compile without the source target
    message("ADDING HEADERS: vkb__${TARGET_NAME}__headers")

    set(TARGET_HEADER_ONLY "vkb__${TARGET_NAME}__headers")

    add_library(${TARGET_HEADER_ONLY} INTERFACE ${TARGET_HEADERS})

    if(TARGET_LINK_LIBS)
        target_link_libraries(${TARGET_HEADER_ONLY} INTERFACE ${TARGET_LINK_LIBS})
    endif()

    if(TARGET_INCLUDE_DIRS)
        target_include_directories(${TARGET_HEADER_ONLY} INTERFACE ${TARGET_INCLUDE_DIRS})
    endif()

    target_compile_features(${TARGET_HEADER_ONLY} INTERFACE cxx_std_17)



    if(TARGET_SRC) # Create static library
        message("ADDING STATIC: vkb__${TARGET_NAME}")

        add_library("vkb__${TARGET_NAME}" STATIC ${TARGET_SRC})
        target_link_libraries("vkb__${TARGET_NAME}" PUBLIC ${TARGET_HEADER_ONLY})

        if(${VKB_WARNINGS_AS_ERRORS})
            if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
                target_compile_options("vkb__${TARGET_NAME}" PRIVATE -Werror)

            # target_compile_options("vkb__${TARGET_NAME}" PRIVATE -Wall -Wextra -Wpedantic -Werror)
            elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
                target_compile_options("vkb__${TARGET_NAME}" PRIVATE /W3 /WX)

                # target_compile_options("vkb__${TARGET_NAME}" PRIVATE /W4 /WX)
            endif()
        endif()

        set_target_properties("vkb__${TARGET_NAME}"
            PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )

    else() # Create interface library
        message("ADDING INTERFACE: vkb__${TARGET_NAME}")

        add_library("vkb__${TARGET_NAME}" INTERFACE ${TARGET_HEADERS})
        target_link_libraries("vkb__${TARGET_NAME}" INTERFACE ${TARGET_HEADER_ONLY})
    endif()

    set_property(TARGET "vkb__${TARGET_NAME}" PROPERTY FOLDER "components/${TARGET_NAME}")

    add_dependencies(vkb__components "vkb__${TARGET_NAME}")
endfunction()