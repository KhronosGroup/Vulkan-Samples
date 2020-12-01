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

set(SCRIPT_DIR ${CMAKE_CURRENT_LIST_DIR})

function(add_sample)
    set(options)  
    set(oneValueArgs ID CATEGORY AUTHOR NAME DESCRIPTION)
    set(multiValueArgs FILES LIBS)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(SRC_FILES
        ${TARGET_ID}.h
        ${TARGET_ID}.cpp
    )

    # Append extra files if present
    if (TARGET_FILES)
        list(APPEND SRC_FILES ${TARGET_FILES})
    endif()

    add_project(
        TYPE "Sample"
        ID ${TARGET_ID}
        CATEGORY ${TARGET_CATEGORY}
        AUTHOR ${TARGET_AUTHOR}
        NAME ${TARGET_NAME}
        DESCRIPTION ${TARGET_DESCRIPTION}
        TAGS 
            "any"
        FILES
            ${SRC_FILES}
        LIBS
            ${TARGET_LIBS})
endfunction()

function(add_sample_with_tags)
    set(options)
    set(oneValueArgs ID CATEGORY AUTHOR NAME DESCRIPTION)
    set(multiValueArgs TAGS FILES LIBS)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    list(APPEND TARGET_TAGS "any")

    set(SRC_FILES
        ${TARGET_ID}.h
        ${TARGET_ID}.cpp
    )

    # Append extra files if present
    if (TARGET_FILES)
        list(APPEND SRC_FILES ${TARGET_FILES})
    endif()


    add_project(
        TYPE "Sample"
        ID ${TARGET_ID}
        CATEGORY ${TARGET_CATEGORY}
        AUTHOR ${TARGET_AUTHOR}
        NAME ${TARGET_NAME}
        DESCRIPTION ${TARGET_DESCRIPTION}
        TAGS 
            ${TARGET_TAGS}
        FILES
            ${SRC_FILES}
        LIBS
            ${TARGET_LIBS})

endfunction()

function(vkb_add_test)
    set(options)
    set(oneValueArgs ID)
    set(multiValueArgs)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_project(
        TYPE "Test"
        ID ${TARGET_ID}
        CATEGORY "Tests"
        AUTHOR " "
        NAME ${TARGET_ID}
        DESCRIPTION " "
        VENDOR_TAG " "
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_ID}.h
            ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_ID}.cpp)
endfunction()

function(add_project)
    set(options)  
    set(oneValueArgs TYPE ID CATEGORY AUTHOR NAME DESCRIPTION)
    set(multiValueArgs TAGS FILES LIBS)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(${TARGET_TYPE} STREQUAL "Sample")
        set("VKB_${TARGET_ID}" ON CACHE BOOL "Build sample ${TARGET_ID}")
    endif()

    if(NOT ${VKB_${TARGET_ID}})
        message(STATUS "${TARGET_TYPE} `${TARGET_ID}` - DISABLED")
        return()
    endif()

    message(STATUS "${TARGET_TYPE} `${TARGET_ID}` - BUILD")

    # create project (object target - reused by app target)
    project(${TARGET_ID} LANGUAGES C CXX)

    source_group("\\" FILES ${TARGET_FILES})

    add_library(${PROJECT_NAME} STATIC ${TARGET_FILES})
    
    # inherit compile definitions from framework target
    target_compile_definitions(${PROJECT_NAME} PUBLIC $<TARGET_PROPERTY:framework,COMPILE_DEFINITIONS>)

    # # inherit include directories from framework target
    target_include_directories(${PROJECT_NAME} PUBLIC 
        $<TARGET_PROPERTY:framework,INCLUDE_DIRECTORIES>
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

    target_link_libraries(${PROJECT_NAME} PRIVATE framework)

    # Link against extra project specific libraries
    if(TARGET_LIBS)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${TARGET_LIBS})
    endif()

    # if test then add test framework as well
    if(${TARGET_TYPE} STREQUAL "Test")
        target_compile_definitions(${PROJECT_NAME} PUBLIC $<TARGET_PROPERTY:test_framework,COMPILE_DEFINITIONS>)
        target_include_directories(${PROJECT_NAME} PUBLIC $<TARGET_PROPERTY:test_framework,INCLUDE_DIRECTORIES>)
        target_link_libraries(${PROJECT_NAME} PRIVATE test_framework)
    endif()

    # capitalise the first letter of the category  (performance -> Performance) 
    string(SUBSTRING ${TARGET_CATEGORY} 0 1 FIRST_LETTER)
    string(TOUPPER ${FIRST_LETTER} FIRST_LETTER)
    string(REGEX REPLACE "^.(.*)" "${FIRST_LETTER}\\1" CATEGORY "${TARGET_CATEGORY}")

    if(${TARGET_TYPE} STREQUAL "Sample")
        # set sample properties
        set_target_properties(${PROJECT_NAME}
            PROPERTIES 
                SAMPLE_CATEGORY ${TARGET_CATEGORY}
                SAMPLE_AUTHOR ${TARGET_AUTHOR}
                SAMPLE_NAME ${TARGET_NAME}
                SAMPLE_DESCRIPTION ${TARGET_DESCRIPTION}
                SAMPLE_TAGS "${TARGET_TAGS}")

        # add sample project to a folder
        set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "Samples//${CATEGORY}")
    elseif(${TARGET_TYPE} STREQUAL "Test")
        # add test project to a folder
        set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "Tests")
    endif()
endfunction()
