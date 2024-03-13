# Copyright (c) 2023-2024, Thomas Atkinson
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

## Bucket target for all components
add_custom_target(vkb__components)
set_target_properties(vkb__components PROPERTIES FOLDER "CMake/CustomTargets")

# Create a new component
# Adds the component to the vkb__components target
# Automatically decides whether to create a static or interface library
# If NO_DEFAULT_INCLUDES is set, then the component must provide its own include directories and set its own source groups
function(vkb__register_component)
    set(options NO_DEFAULT_INCLUDES)
    set(oneValueArgs NAME)
    set(multiValueArgs SRC HEADERS LINK_LIBS INCLUDE_DIRS)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(TARGET_NAME STREQUAL "")
        message(FATAL_ERROR "NAME must be defined in vkb__register_tests")
    endif()

    set(TARGET "vkb__${TARGET_NAME}")
    set(TARGET_FOLDER "components")

    set(LINKAGE "UNKNOWN") # Used to determine whether to create a static or interface library

    if(TARGET_SRC)
        message(STATUS "STATIC: ${TARGET}")
        set(LINKAGE "PUBLIC")
        add_library(${TARGET} STATIC ${TARGET_SRC} ${TARGET_HEADERS})

        if(${VKB_WARNINGS_AS_ERRORS})
            if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
                target_compile_options(${TARGET} PRIVATE -Werror)
                # target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)
            elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
                target_compile_options(${TARGET} PRIVATE /W3 /WX)
                # target_compile_options(${TARGET} PRIVATE /W4 /WX)
            endif()
        endif()
    else()
        message(STATUS "INTERFACE: ${TARGET}")
        set(LINKAGE "INTERFACE")
        add_library(${TARGET} INTERFACE)
        target_sources(${TARGET} INTERFACE ${TARGET_HEADERS})
    endif()

    if (LINKAGE STREQUAL "UNKNOWN")
        message(FATAL_ERROR "Failed to determine linkage type for ${TARGET}")
    endif()

    if(TARGET_LINK_LIBS)
        target_link_libraries(${TARGET} ${LINKAGE} ${TARGET_LINK_LIBS})
    endif()

    if(TARGET_INCLUDE_DIRS)
        target_include_directories(${TARGET} ${LINKAGE} ${TARGET_INCLUDE_DIRS})
    endif()

    target_compile_features(${TARGET} ${LINKAGE} cxx_std_17)

    set_property(TARGET ${TARGET} PROPERTY FOLDER ${TARGET_FOLDER})

    if(NOT TARGET_NO_DEFAULT_INCLUDES)
        target_include_directories(${TARGET} ${LINKAGE} ${CMAKE_CURRENT_SOURCE_DIR}/include)
    endif()

    add_dependencies(vkb__components ${TARGET})
endfunction()

# Enable testing within the components
macro(vkb__enable_testing)
    if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING) OR VKB_BUILD_TESTS)
        include(CTest)
        enable_testing()
    endif()
endmacro()

# Bucket target for all tests
add_custom_target(vkb__tests)
set_target_properties(vkb__tests PROPERTIES FOLDER "CMake/CustomTargets")

# Register a new test
# Adds the test to the vkb__tests target
# Uses Catch2 by default. If NO_CATCH2 is set, then the test must provide its own main function
function(vkb__register_tests)
    set(options NO_CATCH2)
    set(oneValueArgs COMPONENT NAME)
    set(multiValueArgs SRC HEADERS LINK_LIBS INCLUDE_DIRS)

    if(NOT((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING) OR VKB_BUILD_TESTS))
        return() # testing not enabled
    endif()

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(TARGET_NAME STREQUAL "")
        message(FATAL_ERROR "NAME must be defined in vkb__register_tests")
    endif()

    if(NOT TARGET_SRC)
        message(FATAL_ERROR "One or more source files must be added to vkb__register_tests")
    endif()

    set(TARGET_FOLDER "components")
    set(TARGET_NAME "test__${TARGET_NAME}")

    message(STATUS "TEST: ${TARGET_NAME}")

    if(TARGET_NO_CATCH2 AND WIN32)
        add_executable(${TARGET_NAME} WIN32 ${TARGET_SRC})
    else()
        add_executable(${TARGET_NAME} ${TARGET_SRC})
    endif()

    if(NOT NO_CATCH2)
        target_link_libraries(${TARGET_NAME} PUBLIC Catch2::Catch2WithMain)
    endif()

    target_compile_definitions(${TARGET_NAME} PUBLIC VKB_BUILD_TESTS)

    set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER ${TARGET_FOLDER})

    set_target_properties(${TARGET_NAME}
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/${CMAKE_BUILD_TYPE}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/${CMAKE_BUILD_TYPE}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/${CMAKE_BUILD_TYPE}"
    )

    if(TARGET_LINK_LIBS)
        target_link_libraries(${TARGET_NAME} PUBLIC ${TARGET_LINK_LIBS})
    endif()

    if(${VKB_WARNINGS_AS_ERRORS})
        message(STATUS "Warnings as Errors Enabled")
        if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
            target_compile_options(${TARGET_NAME} PRIVATE -Werror)
        elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
            target_compile_options(${TARGET_NAME} PRIVATE /W3 /WX)
        endif()
    endif()

    add_test(NAME ${TARGET_NAME}
             COMMAND ${TARGET_NAME}
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    add_dependencies(vkb__tests ${TARGET_NAME})
endfunction()