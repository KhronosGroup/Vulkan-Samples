# Copyright (c) 2020-2021, Arm Limited and Contributors
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
#

cmake_minimum_required(VERSION 3.12)
project(apps)

# Generate apps.cpp

set(APP_INFO_LIST)

set(SAMPLE_INCLUDE_FILES)
set(SAMPLE_INFO_LIST)

foreach(SAMPLE_ID ${TOTAL_SAMPLE_ID_LIST})
    if ("${VKB_${SAMPLE_ID}}" AND TARGET ${SAMPLE_ID})
        get_target_property(SAMPLE_CATEGORY ${SAMPLE_ID} SAMPLE_CATEGORY)
        get_target_property(SAMPLE_AUTHOR ${SAMPLE_ID} SAMPLE_AUTHOR)
        get_target_property(SAMPLE_NAME ${SAMPLE_ID} SAMPLE_NAME)
        get_target_property(SAMPLE_DESCRIPTION ${SAMPLE_ID} SAMPLE_DESCRIPTION)
        get_target_property(SAMPLE_TAGS ${SAMPLE_ID} SAMPLE_TAGS)

        # Ensure we send in an empty C++ string as the vendor category, rather than a string with a space
        set(INCLUDE_DIR ${SAMPLE_CATEGORY}/${SAMPLE_ID})

        set(SAMPLE_TAGS_VECTOR)

        list(JOIN SAMPLE_TAGS "\", \"" SAMPLE_TAGS_VECTOR)

        list(APPEND SAMPLE_INCLUDE_FILES "#include \"${INCLUDE_DIR}/${SAMPLE_ID}.h\"")
        list(APPEND SAMPLE_INFO_LIST "\tSampleInfo{\"${SAMPLE_ID}\", create_${SAMPLE_ID}, \"${SAMPLE_CATEGORY}\"\, \"${SAMPLE_AUTHOR}\"\, \"${SAMPLE_NAME}\"\, \"${SAMPLE_DESCRIPTION}\", {\"${SAMPLE_TAGS_VECTOR}\"}},")
        list(APPEND APP_INFO_LIST "\tAppInfo{\"${SAMPLE_ID}\", create_${SAMPLE_ID}},")
    endif()
endforeach()

set(TEST_INCLUDE_FILES)
set(TEST_INFO_LIST)
set(TEST_APP_INFO_LIST)

foreach(TEST_ID ${TOTAL_TEST_ID_LIST})
    if (TARGET ${TEST_ID})
        list(APPEND TEST_INCLUDE_FILES "#include \"${TEST_ID}/${TEST_ID}.h\"")
        list(APPEND TEST_INFO_LIST "\tTestInfo{ \"${TEST_ID}\", create_${TEST_ID}_test },")
        list(APPEND APP_INFO_LIST "\tAppInfo{ \"${TEST_ID}\", create_${TEST_ID}_test },")
    endif()
endforeach()

list(JOIN SAMPLE_INCLUDE_FILES "\n" SAMPLE_INCLUDE_FILES)
list(JOIN SAMPLE_INFO_LIST "\n" SAMPLE_INFO_LIST)
list(JOIN TEST_INCLUDE_FILES "\n" TEST_INCLUDE_FILES)
list(JOIN TEST_INFO_LIST "\n" TEST_INFO_LIST)
list(JOIN TEST_APP_INFO_LIST "\n" TEST_APP_INFO_LIST)
list(JOIN APP_INFO_LIST "\n" APP_INFO_LIST)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/apps.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/apps.cpp)

# Create apps library

set(SRC
    apps.h
    ${CMAKE_CURRENT_BINARY_DIR}/apps.cpp
)

add_library(${PROJECT_NAME} SHARED ${SRC})

target_link_libraries(${PROJECT_NAME} PUBLIC framework)
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/samples
    ${CMAKE_SOURCE_DIR}/tests/system_test/sub_tests
    ${CMAKE_SOURCE_DIR}/tests/system_test/test_framework)

# Link all samples and tests
set(PROJECT_ID_LIST ${TOTAL_SAMPLE_ID_LIST} ${TOTAL_TEST_ID_LIST})
foreach(ID ${PROJECT_ID_LIST})
    if(TARGET ${ID})
#        get_target_property(TARGET_TYPE ${ID} TYPE)
#
#        if(TARGET_TYPE STREQUAL "STATIC_LIBRARY")
            target_link_libraries(${PROJECT_NAME} PUBLIC ${ID})
#        endif()
    endif()
endforeach()