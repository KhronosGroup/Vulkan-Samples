#[[
 Copyright (c) 2019-2024, Arm Limited and Contributors

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

function(COMPILE_SPIRV_SHADER SHADER_FILE SHADER_OUTPUT)
    message(TRACE "Compiling shader\n\t${SHADER_FILE}\n\t${SHADER_OUTPUT}")
    if (NOT DEFINED GLSLANG_EXECUTABLE)
        find_program(GLSLANG_EXECUTABLE glslangValidator HINTS "$ENV{VULKAN_SDK}/bin")
    endif()
    add_custom_command(OUTPUT ${SHADER_OUTPUT}
        COMMAND ${GLSLANG_EXECUTABLE} -V ${SHADER_FILE} -o ${SHADER_OUTPUT}
        DEPENDS ${SHADER_FILE}
        COMMENT "Compiling shader ${SHADER_FILE} to SPIRV ${SHADER_OUTPUT}"
        VERBATIM
    )
endfunction()


function(OPTIMIZE_SPIRV SPIRV_FILE OPTIMIZED_OUTPUT)
    message(TRACE "Optimizing shader\n\t${SPIRV_FILE}\n\t${OPTIMIZED_OUTPUT}")
    if (NOT DEFINED SPIRV_OPT_EXECUTABLE)
        find_program(SPIRV_OPT_EXECUTABLE spirv-opt HINTS "$ENV{VULKAN_SDK}/bin")
    endif()
    add_custom_command(OUTPUT ${OPTIMIZED_OUTPUT}
        COMMAND ${SPIRV_OPT_EXECUTABLE} -O ${SPIRV_FILE} -o ${OPTIMIZED_OUTPUT}
        DEPENDS ${SPIRV_FILE}
        COMMENT "Optimizing SPIRV ${SPIRV_FILE} to SPIRV ${OPTIMIZED_OUTPUT}"
        VERBATIM
    )
endfunction()

function(GENERATE_HEADER SHADER_FILE SPIRV_FILE HEADER_FILE)
    message(TRACE "Generating C++ header for\n\t${SPIRV_FILE}\n\t${HEADER_FILE}")
    set(HEADER_TEMPLATE "${CMAKE_SOURCE_DIR}/bldsys/cmake/ShaderData.inl.in")
    set(SHADER_HEADERGEN_SCRIPT "${CMAKE_SOURCE_DIR}/bldsys/cmake/create_shader_header.cmake")
    set(SHADER_BASE_PATH "${CMAKE_SOURCE_DIR}/shaders")

    #   SHADER_SPIRV: The full path of the SPIRV compiled file to convert
    #   SHADER_HEADER: The full path of the header file to generate
    #   SHADER_FILE: The original shader file, relative to the shaders folder
    #   HEADER_TEMPLATE: The full path of the header template file to use
    add_custom_command(OUTPUT ${SHADER_HEADER}
        COMMAND ${CMAKE_COMMAND} -DHEADER_TEMPLATE=${HEADER_TEMPLATE} -DSHADER_BASE_PATH=${CMAKE_SOURCE_DIR}/shaders -DSHADER_FILE=${SHADER_FILE} -DSHADER_SPIRV=${SPIRV_FILE} -DSHADER_HEADER=${SHADER_HEADER} -P ${SHADER_HEADERGEN_SCRIPT}
        DEPENDS ${SPIRV_FILE}
        COMMENT "Generating header ${HEADER_FILE} from SPIRV ${SPIRV_FILE}"
        VERBATIM
    )
endfunction()

function(add_sample)
    set(options)
    set(oneValueArgs ID CATEGORY AUTHOR NAME DESCRIPTION)
    set(multiValueArgs FILES LIBS SHADER_FILES_GLSL)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_sample_with_tags(
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
            ${TARGET_LIBS}
        SHADER_FILES_GLSL
            ${TARGET_SHADER_FILES_GLSL})
endfunction()

function(add_sample_with_tags)
    set(options)
    set(oneValueArgs ID CATEGORY AUTHOR NAME DESCRIPTION)
    set(multiValueArgs TAGS FILES LIBS SHADER_FILES_GLSL)

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

    # Add GLSL shader files for this sample
    if (TARGET_SHADER_FILES_GLSL)
        list(APPEND SHADER_FILES_GLSL ${TARGET_SHADER_FILES_GLSL})
        foreach(SHADER_FILE_GLSL ${SHADER_FILES_GLSL})
            list(APPEND SHADERS_GLSL "${PROJECT_SOURCE_DIR}/shaders/${SHADER_FILE_GLSL}")
        endforeach()
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
            ${TARGET_LIBS}
        SHADERS_GLSL
            ${SHADERS_GLSL})

endfunction()

function(vkb_add_test)
    set(options)
    set(oneValueArgs ID)
    set(multiValueArgs)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(SRC_FILES
        ${TARGET_ID}.h
        ${TARGET_ID}.cpp
    )

    add_project(
        TYPE "Test"
        ID ${TARGET_ID}
        CATEGORY "Tests"
        AUTHOR " "
        NAME ${TARGET_ID}
        DESCRIPTION " "
        TAGS " "
        FILES ${SRC_FILES}
        LIBS test_framework)
endfunction()

function(add_project)
    set(options)
    set(oneValueArgs TYPE ID CATEGORY AUTHOR NAME DESCRIPTION)
    set(multiValueArgs TAGS FILES LIBS SHADERS_GLSL)

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

    # Add shaders to project group
    if (SHADERS_GLSL)
        source_group("\\Shaders" FILES ${SHADERS_GLSL})
    endif()

if(${TARGET_TYPE} STREQUAL "Sample")
    add_library(${PROJECT_NAME} OBJECT ${TARGET_FILES} ${SHADERS_GLSL})
elseif(${TARGET_TYPE} STREQUAL "Test")
    add_library(${PROJECT_NAME} STATIC ${TARGET_FILES} ${SHADERS_GLSL})
endif()
    set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    # # inherit include directories from framework target
    target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE framework)

    # Add shaders to project group
    if (SHADERS_GLSL AND ("OfflineShaders" IN_LIST TARGET_TAGS))
        foreach(SHADER_ABSOLUTE_PATH  ${SHADERS_GLSL})
            file(RELATIVE_PATH SHADER_FILE ${CMAKE_SOURCE_DIR}/shaders ${SHADER_ABSOLUTE_PATH})
            set(SHADER_DEBUG_SPIRV ${CMAKE_BINARY_DIR}/shaders/${SHADER_FILE}.debug.spv)
            set(SHADER_SPIRV ${CMAKE_BINARY_DIR}/shaders/${SHADER_FILE}.spv)
            set(SHADER_HEADER ${CMAKE_BINARY_DIR}/shaders/${SHADER_FILE}.inl)
            compile_spirv_shader(${SHADER_ABSOLUTE_PATH} ${SHADER_DEBUG_SPIRV})
            optimize_spirv(${SHADER_DEBUG_SPIRV} ${SHADER_SPIRV})
            generate_header(${SHADER_FILE} ${SHADER_SPIRV} ${SHADER_HEADER})
            target_sources(${PROJECT_NAME} PRIVATE ${SHADER_HEADER})
            source_group("\\Shaders\\Compiled" FILES ${SHADER_HEADER})
        endforeach()
        target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_BINARY_DIR}/shaders")
    endif()

    # Link against extra project specific libraries
    if(TARGET_LIBS)
        target_link_libraries(${PROJECT_NAME} PUBLIC ${TARGET_LIBS})
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

    if(VKB_DO_CLANG_TIDY)
        set_target_properties(${PROJECT_NAME} PROPERTIES CXX_CLANG_TIDY "${VKB_DO_CLANG_TIDY}")
    endif()
endfunction()
