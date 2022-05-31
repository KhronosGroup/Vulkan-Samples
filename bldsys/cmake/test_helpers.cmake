# Copyright (c) 2022, Arm Limited and Contributors
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 the "License";
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
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

add_custom_target(vkb_tests)

function(vkb__register_tests)
    set(options)  
    set(oneValueArgs NAME)
    set(multiValueArgs SRC LIBS)

    if(NOT ((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING) OR VKB_BUILD_TESTS))
        return() # testing not enabled
    endif()

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}) 

    if (TARGET_NAME STREQUAL "")
        message(FATAL_ERROR "NAME must be defined in vkb__register_tests")
    endif()

    if (NOT TARGET_SRC)
        message(FATAL_ERROR "One or more source files must be added to vkb__register_tests")
    endif()

    add_executable(${TARGET_NAME} ${TARGET_SRC})
    target_link_libraries(${TARGET_NAME} PUBLIC Catch2::Catch2WithMain)

    if (TARGET_LIBS)
        target_link_libraries(${TARGET_NAME} PUBLIC ${TARGET_LIBS})
    endif()

    add_test(NAME ${TARGET_NAME}
             COMMAND ${TARGET_NAME})

    add_dependencies(vkb_tests ${TARGET_NAME})
endfunction()