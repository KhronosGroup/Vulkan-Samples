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

macro(vkb__enable_testing)
    if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING) OR VKB_BUILD_TESTS)
        include(CTest)
        enable_testing()
    endif()
endmacro()

add_custom_target(vkb__tests)

set_property(TARGET vkb__tests PROPERTY FOLDER "tests")

function(vkb__register_tests)
    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs SRC LIBS INCLUDE_DIRS)

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

    message(STATUS "TEST: ${TARGET_NAME}")

    add_executable(${TARGET_NAME} ${TARGET_SRC})
    target_link_libraries(${TARGET_NAME} PUBLIC Catch2::Catch2WithMain)

    target_compile_definitions(${TARGET_NAME} PUBLIC VKB_BUILD_TESTS)

    set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "tests")

    set_target_properties(${TARGET_NAME}
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    if(TARGET_INCLUDE_DIRS)
        target_include_directories(${TARGET_NAME} PUBLIC ${TARGET_INCLUDE_DIRS})
    endif()

    if(TARGET_LIBS)
        target_link_libraries(${TARGET_NAME} PUBLIC ${TARGET_LIBS})
    endif()

    if(${VKB_WARNINGS_AS_ERRORS})
        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
            target_compile_options(${TARGET_NAME} PRIVATE -Werror)
        elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
            target_compile_options(${TARGET_NAME} PRIVATE /W3 /WX)
        endif()
    endif()

    add_test(NAME ${TARGET_NAME}
        COMMAND ${TARGET_NAME}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    add_dependencies(vkb__tests ${TARGET_NAME})
endfunction()

function(vkb__register_tests_no_catch2)
    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs SRC LIBS)

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

    if(WIN32)
        add_executable(${TARGET_NAME} WIN32 ${TARGET_SRC})
    else()
        add_executable(${TARGET_NAME} ${TARGET_SRC})
    endif()

    if(TARGET_LIBS)
        target_link_libraries(${TARGET_NAME} PUBLIC ${TARGET_LIBS})
    endif()

    set_target_properties(${TARGET_NAME}
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )

    if(${VKB_WARNINGS_AS_ERRORS})
        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
            target_compile_options(${TARGET_NAME} PRIVATE -Werror)
        elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
            target_compile_options(${TARGET_NAME} PRIVATE /W3 /WX)
        endif()
    endif()

    add_test(
        NAME ${TARGET_NAME}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${TARGET_NAME})

    add_dependencies(vkb__tests ${TARGET_NAME})
endfunction()

function(vkb__register_gpu_tests)
    # we arent building gpu tests - dont register them
    if(NOT VKB_BUILD_GPU_TESTS)
        return()
    endif()

    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs SRC LIBS)

    if(NOT((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING) OR VKB_BUILD_TESTS))
        return() # testing not enabled
    endif()

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    vkb__register_tests(
        NAME ${TARGET_NAME}
        SRC ${TARGET_SRC}
        LIBS ${TARGET_LIBS}
    )
endfunction()