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

function(generate_entrypoint)
    set(options)
    set(oneValueArgs ID NAME CREATE_FUNC INCLUDE_PATH OUTPUT_PATH)
    set(multiValueArgs)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT TARGET_CREATE_FUNC)
        set(TARGET_CREATE_FUNC ${TARGET_ID})
    endif()

    set(CONFIG_FILE ${SCRIPT_DIR}/template/entrypoint_main.cpp.in)

    if(EXISTS ${CONFIG_FILE})
        configure_file(${CONFIG_FILE} ${TARGET_OUTPUT_PATH})
    else()
        message(FATAL_ERROR "No template file exists at `${CONFIG_FILE}`")
    endif()
endfunction()

function(generate_samples_header)
    set(options)
    set(oneValueArgs OUTPUT_DIR)
    set(multiValueArgs SAMPLE_ID_LIST)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(SAMPLE_INCLUDE_FILES)
    set(SAMPLE_NAME_FUNC_PAIRS)
    set(SAMPLE_INFO_LIST)

    foreach(SAMPLE_ID ${TARGET_SAMPLE_ID_LIST})
        if ("${VKB_${SAMPLE_ID}}" AND TARGET ${SAMPLE_ID})
            get_target_property(SAMPLE_CATEGORY ${SAMPLE_ID} SAMPLE_CATEGORY)
            get_target_property(SAMPLE_AUTHOR ${SAMPLE_ID} SAMPLE_AUTHOR)
            get_target_property(SAMPLE_NAME ${SAMPLE_ID} SAMPLE_NAME)
            get_target_property(SAMPLE_DESCRIPTION ${SAMPLE_ID} SAMPLE_DESCRIPTION)
            get_target_property(SAMPLE_TAGS ${SAMPLE_ID} SAMPLE_TAGS)

            # Ensure we send in an empty C++ string as the vendor category, rather than a string with a space
            set(INCLUDE_DIR ${SAMPLE_CATEGORY}/${SAMPLE_ID})

            set(SAMPLE_TAGS_VECTOR)

            foreach(TAG ${SAMPLE_TAGS})
                set(SAMPLE_TAGS_VECTOR "${SAMPLE_TAGS_VECTOR}\"${TAG}\",")
            endforeach()

            list(APPEND SAMPLE_INFO_LIST "\tSampleInfo{\"${SAMPLE_ID}\", \"${SAMPLE_CATEGORY}\"\, \"${SAMPLE_AUTHOR}\"\, \"${SAMPLE_NAME}\"\, \"${SAMPLE_DESCRIPTION}\", {${SAMPLE_TAGS_VECTOR}}},")
            list(APPEND SAMPLE_INCLUDE_FILES "#include \"${INCLUDE_DIR}/${SAMPLE_ID}.h\"")
            list(APPEND SAMPLE_NAME_FUNC_PAIRS "\t{ \"${SAMPLE_ID}\", create_${SAMPLE_ID} },")
        endif()
    endforeach()

    string_join(
        GLUE "\n"
        INPUT ${SAMPLE_INFO_LIST}
        OUTPUT SAMPLE_INFO_LIST)

    string_join(
        GLUE "\n"
        INPUT ${SAMPLE_INCLUDE_FILES}
        OUTPUT SAMPLE_INCLUDE_FILES)

    string_join(
        GLUE "\n"
        INPUT ${SAMPLE_NAME_FUNC_PAIRS}
        OUTPUT SAMPLE_NAME_FUNC_PAIRS)

    set(CONFIG_FILE ${SCRIPT_DIR}/template/samples.h.in)

    if(EXISTS ${CONFIG_FILE})
        configure_file(${CONFIG_FILE} ${TARGET_OUTPUT_DIR}/samples.h)
    else()
        message(FATAL_ERROR "No template file exists at `${CONFIG_FILE}`")
    endif()
endfunction()

function(generate_tests_header)
    set(options)
    set(oneValueArgs OUTPUT_DIR)
    set(multiValueArgs TEST_ID_LIST)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(TEST_INCLUDE_FILES)
    set(TEST_NAME_FUNC_PAIRS)

    foreach(TEST_ID ${TARGET_TEST_ID_LIST})
        if (TARGET ${TEST_ID})
            list(APPEND TEST_INCLUDE_FILES "#include \"${TEST_ID}/${TEST_ID}.h\"")
            list(APPEND TEST_NAME_FUNC_PAIRS "\t{ \"${TEST_ID}\", create_${TEST_ID}_test },")
        endif()
    endforeach()

    string_join(
        GLUE "\n"
        INPUT ${TEST_INCLUDE_FILES}
        OUTPUT TEST_INCLUDE_FILES)

    string_join(
        GLUE "\n"
        INPUT ${TEST_NAME_FUNC_PAIRS}
        OUTPUT TEST_NAME_FUNC_PAIRS)

    set(CONFIG_FILE ${SCRIPT_DIR}/template/tests.h.in)

    if(EXISTS ${CONFIG_FILE})
        configure_file(${CONFIG_FILE} ${TARGET_OUTPUT_DIR}/tests.h)
    else()
        message(FATAL_ERROR "No template file exists at `${CONFIG_FILE}`")
    endif()
endfunction()

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

function(add_test_)
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

    if(NOT ANDROID AND ${VKB_ENTRYPOINTS})
        set(CREATE_FUNC_NAME ${TARGET_ID})
        if(${TARGET_TYPE} STREQUAL "Sample")
            # create sample app project
            project(${TARGET_ID}_app LANGUAGES C CXX)
        elseif(${TARGET_TYPE} STREQUAL "Test")
            # create sample app project
            project(${TARGET_ID}_test LANGUAGES C CXX)
            set(CREATE_FUNC_NAME ${TARGET_ID}_test)
        endif()

        # create entrypoint file for sample
        generate_entrypoint(
            ID ${TARGET_ID}
            NAME ${TARGET_NAME}
            CREATE_FUNC ${CREATE_FUNC_NAME}
            INCLUDE_PATH ${TARGET_ID}
            OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/main.cpp)
            
        # create list of project files (objects + entrypoint file)
        set(PROJECT_FILES 
            ${CMAKE_CURRENT_BINARY_DIR}/main.cpp 
            $<TARGET_OBJECTS:${TARGET_ID}>)

        source_group("\\" FILES ${PROJECT_FILES})

        add_executable(${PROJECT_NAME} WIN32 ${PROJECT_FILES})

        # inherit sample include directories from sample object target
        target_include_directories(${PROJECT_NAME} PUBLIC 
            $<TARGET_PROPERTY:${TARGET_ID},INCLUDE_DIRECTORIES>)

        target_link_libraries(${PROJECT_NAME} PUBLIC framework)

        if(${TARGET_TYPE} STREQUAL "Test")
            target_link_libraries(${PROJECT_NAME} PUBLIC test_framework)
        endif()

        # add sample app project to a folder
        set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "Entrypoints//${CATEGORY}")

        if(${VKB_SYMLINKS})
            create_symlink(
                NAME ${PROJECT_NAME}
                DIR ${CMAKE_SOURCE_DIR}/assets 
                LINK ${CMAKE_CURRENT_BINARY_DIR}/assets)
                
            create_symlink(
                NAME ${PROJECT_NAME}
                DIR ${CMAKE_SOURCE_DIR}/output
                LINK ${CMAKE_CURRENT_BINARY_DIR}/output)
        endif()

        if(MSVC)
            set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
            
            foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
                string(TOUPPER ${CONFIG_TYPE} SUFFIX)
                string(TOLOWER ${CONFIG_TYPE} CONFIG_DIR)
                set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${SUFFIX} ${CMAKE_CURRENT_BINARY_DIR}/bin/${CONFIG_DIR}/${TARGET_ARCH})
                set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${SUFFIX} ${CMAKE_CURRENT_BINARY_DIR}/lib/${CONFIG_DIR}/${TARGET_ARCH})
                set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${SUFFIX} ${CMAKE_CURRENT_BINARY_DIR}/lib/${CONFIG_DIR}/${TARGET_ARCH})
            endforeach()
        endif()
    endif()
endfunction()

function(order_sample_list)
    set(options)  
    set(oneValueArgs NAME)
    set(multiValueArgs INPUT ORDER OUTPUT)

    set(OUTPUT_LIST)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    list(REMOVE_DUPLICATES TARGET_ORDER)

    # Add samples based on the given order
    foreach(SAMPLE_ID ${TARGET_ORDER})
        list(FIND TARGET_INPUT ${SAMPLE_ID} FOUND_SAMPLE)
        if(NOT ${FOUND_SAMPLE} LESS 0)
            list(APPEND OUTPUT_LIST ${SAMPLE_ID})
        endif()
    endforeach()

    # Move the remaining samples
    foreach(SAMPLE_ID ${TARGET_INPUT})
        list(FIND OUTPUT_LIST ${SAMPLE_ID} FOUND_SAMPLE)
        if(${FOUND_SAMPLE} LESS 0)
            list(APPEND OUTPUT_LIST ${SAMPLE_ID})
        endif()
    endforeach()

    set(${TARGET_OUTPUT} ${OUTPUT_LIST} PARENT_SCOPE)
endfunction()
