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

function(add_sample)
    set(options NO_SHADERS)  
    set(oneValueArgs ID CATEGORY AUTHOR NAME DESCRIPTION SPIRV_VERSION)
    set(multiValueArgs TAGS FILES LIBS SHADER_FILES_GLSL SHADER_FILES_HLSL)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})    

    set(options NO_SHADERS)
    set(oneValueArgs ID CATEGORY AUTHOR NAME DESCRIPTION SPIRV_VERSION)
    set(multiValueArgs TAGS FILES LIBS SHADER_FILES_GLSL SHADER_FILES_HLSL)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    list(APPEND TARGET_TAGS "any")

    if (NOT TARGET_SPIRV_VERSION)
        set(TARGET_SPIRV_VERSION "1.0")
    endif()

    set(SRC_FILES
        ${TARGET_ID}.h
        ${TARGET_ID}.cpp
    )

    message(STATUS "Adding shaders ${TARGET_ID} ${TARGET_SPIRV_VERSION}")

    # Append extra files if present
    if (TARGET_FILES)
        list(APPEND SRC_FILES ${TARGET_FILES})
    endif()

    # Add GLSL shader files for this sample
    if (NOT TARGET_SHADER_FILES_GLSL AND NOT TARGET_SHADER_FILES_HLSL AND NOT TARGET_NO_SHADERS)
        message(FATAL_ERROR "No GLSL or HLSL shader files provided for sample ${TARGET_ID}\n\tPlease provide at least one GLSL shader file under SHADER_FILES_GLSL or HLSL shader file under SHADER_FILES_HLSL\n\tIf the sample does not need shaders, set NO_SHADERS to ON")
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
        SPIRV_VERSION
            ${TARGET_SPIRV_VERSION}
        SHADERS_GLSL
            ${TARGET_SHADER_FILES_GLSL}
        SHADERS_HLSL
            ${TARGET_SHADER_FILES_HLSL})

    compile_shaders(
        ID ${TARGET_ID}
        SPIRV_VERSION ${TARGET_SPIRV_VERSION}
        SHADER_FILES_GLSL
            ${TARGET_SHADER_FILES_GLSL}
        SHADER_FILES_HLSL
            ${TARGET_SHADER_FILES_HLSL}
    )
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
    set(oneValueArgs TYPE ID CATEGORY AUTHOR NAME DESCRIPTION SPIRV_VERSION)
    set(multiValueArgs TAGS FILES LIBS SHADERS_GLSL SHADERS_HLSL)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(${TARGET_TYPE} STREQUAL "Sample")
        set("VKB_${TARGET_ID}" ON CACHE BOOL "Build sample ${TARGET_ID}")
    endif()

    if(NOT ${VKB_${TARGET_ID}})
        message(STATUS "${TARGET_TYPE} `${TARGET_ID}` - DISABLED")
        return()
    endif()

    message(STATUS "${TARGET_TYPE} `${TARGET_ID}` - BUILD")

    set(TARGET_DIR ${CMAKE_SOURCE_DIR})

    # create project (object target - reused by app target)
    project(${TARGET_ID} LANGUAGES C CXX)

    source_group("\\" FILES ${TARGET_FILES})

    # Add shaders to project group
    if (TARGET_SHADERS_GLSL)
        list(APPEND SOURCE_SHADER_FILES)
        foreach(GLSL_FILE ${TARGET_SHADERS_GLSL})
            list(APPEND SOURCE_SHADER_FILES "${TARGET_DIR}/shaders/${GLSL_FILE}")
        endforeach()   
    endif()

    if (TARGET_SHADERS_HLSL)
        list(APPEND SOURCE_SHADER_FILES)
        foreach(HLSL_FILE ${TARGET_SHADERS_HLSL})
            list(APPEND SOURCE_SHADER_FILES "${TARGET_DIR}/shaders/${HLSL_FILE}")
        endforeach()   
    endif()
    
    if (SOURCE_SHADER_FILES)
        source_group("\\Shaders" FILES ${SOURCE_SHADER_FILES})
    endif()

if(${TARGET_TYPE} STREQUAL "Sample")
    add_library(${PROJECT_NAME} OBJECT ${TARGET_FILES} ${SOURCE_SHADER_FILES})
elseif(${TARGET_TYPE} STREQUAL "Test")
    add_library(${PROJECT_NAME} STATIC ${TARGET_FILES} ${SOURCE_SHADER_FILES})
endif()
    set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    # # inherit include directories from framework target
    target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE framework)

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
