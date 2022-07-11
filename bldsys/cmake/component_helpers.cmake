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

add_custom_target(vkb_components)

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

    if(TARGET_SRC) # Create static library
        message("ADDING STATIC: vkb__${TARGET_NAME}")

        add_library("vkb__${TARGET_NAME}" STATIC ${TARGET_SRC} ${TARGET_HEADERS})

        if(TARGET_LINK_LIBS)
            target_link_libraries("vkb__${TARGET_NAME}" PUBLIC ${TARGET_LINK_LIBS})
        endif()

        if(TARGET_INCLUDE_DIRS)
            target_include_directories("vkb__${TARGET_NAME}" PUBLIC ${TARGET_INCLUDE_DIRS})
        endif()

        if(MSVC)
            target_compile_options("vkb__${TARGET_NAME}" PRIVATE /W4 /WX)
        else()
            target_compile_options("vkb__${TARGET_NAME}" PRIVATE -Wall -Wextra -Wpedantic -Werror)
        endif()

        set_property(TARGET vkb__${TARGET_NAME} PROPERTY FOLDER "components/${TARGET_NAME}")
    else() # Create interface library
        message("ADDING INTERFACE: vkb__${TARGET_NAME}")

        add_library("vkb__${TARGET_NAME}" INTERFACE)

        if(TARGET_LINK_LIBS)
            target_link_libraries("vkb__${TARGET_NAME}" INTERFACE ${TARGET_LINK_LIBS})
        endif()

        if(TARGET_INCLUDE_DIRS)
            target_include_directories("vkb__${TARGET_NAME}" INTERFACE ${TARGET_INCLUDE_DIRS})
        endif()
    endif()

    add_dependencies(vkb_components "vkb__${TARGET_NAME}")
endfunction()