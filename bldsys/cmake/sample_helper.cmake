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
    set(oneValueArgs ID CATEGORY AUTHOR NAME DESCRIPTION)
    set(multiValueArgs FILES LIBS SHADER_FILES_GLSL SHADER_FILES_HLSL)

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
            ${TARGET_SHADER_FILES_GLSL}
        SHADER_FILES_HLSL
            ${TARGET_SHADER_FILES_HLSL}
        NO_SHADERS
            ${TARGET_NO_SHADERS})
endfunction()

function(add_sample_with_tags)
    set(options)
    set(oneValueArgs ID CATEGORY AUTHOR NAME DESCRIPTION NO_SHADERS)
    set(multiValueArgs TAGS FILES LIBS SHADER_FILES_GLSL SHADER_FILES_HLSL)

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
        SHADERS_GLSL
            ${TARGET_SHADER_FILES_GLSL}
        SHADERS_HLSL
            ${TARGET_SHADER_FILES_HLSL})

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

add_custom_target(vkb__shaders)

# TODO: Shaders to fix
set(SHADERS_TO_FIX
    buffer_array.frag
    buffer_array.vert
)

macro(get_hlsl_target_profile SHADER_FILE OUT_PROFILE)
    if(${SHADER_FILE} MATCHES ".*\\.vert$")
        set(${OUT_PROFILE} "vs_6_0")
    elseif(${SHADER_FILE} MATCHES ".*\\.frag$")
        set(${OUT_PROFILE} "ps_6_0")
    else()
        message(FATAL_ERROR "Unknown shader profile for ${SHADER_FILE}")
    endif()
endmacro()

function(compile_shaders)
    set(options)  
    set(oneValueArgs ID)
    set(multiValueArgs SHADER_FILES_GLSL SHADER_FILES_HLSL)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    find_program(GLSLC_EXECUTABLE glslc HINTS $ENV{VULKAN_SDK}/bin)
    if(NOT GLSLC_EXECUTABLE)
        message(FATAL_ERROR "glslc not found! Shaders with .glsl extension will not be compiled to SPIR-V. Please set VULKAN_SDK to the Vulkan SDK path.")
    endif()

    set(SHADER_DIR "${CMAKE_BINARY_DIR}/shaders")

    set(SHADER_FILES_SPV)

    # Compile GLSL shaders to SPIR-V
    if (GLSLC_EXECUTABLE)
        foreach(SHADER_FILE_GLSL ${TARGET_SHADER_FILES_GLSL})
            # Skip header and OpenCL files
            if(${SHADER_FILE_GLSL} MATCHES ".*\\.h$" OR ${SHADER_FILE_GLSL} MATCHES ".*\\.cl$")
                continue()
            endif()

            # TODO: Remove this when all shaders are fixed
            set(SKIP 0)
            foreach(SHADER_TO_FIX ${SHADERS_TO_FIX})
                if(${SHADER_FILE_GLSL} MATCHES ".*\\${SHADER_TO_FIX}$")
                    message(WARNING "Shader ${SHADER_FILE_GLSL} needs to be fixed")
                    set(SKIP 1)
                endif()
            endforeach()

            if(SKIP)
                continue()
            endif()

            set(SHADER_FILE_SPV "${SHADER_DIR}/${SHADER_FILE_GLSL}.spv")

            get_filename_component(SHADER_FILE_DIR ${SHADER_FILE_SPV} DIRECTORY)
            file(MAKE_DIRECTORY ${SHADER_FILE_DIR})

            add_custom_command(
                OUTPUT ${SHADER_FILE_SPV}
                COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}"
                COMMAND ${GLSLC_EXECUTABLE} -o "${SHADER_FILE_SPV}" --target-spv=spv1.6 ${PROJECT_SOURCE_DIR}/shaders/${SHADER_FILE_GLSL}
                COMMAND ${CMAKE_COMMAND} -E copy "${SHADER_FILE_SPV}" "${PROJECT_SOURCE_DIR}/shaders/${SHADER_FILE_GLSL}.spv"
                DEPENDS ${PROJECT_SOURCE_DIR}/shaders/${SHADER_FILE_GLSL}
                COMMENT "Compiling ${SHADER_FILE_GLSL} to SPIR-V"
                VERBATIM)
            list(APPEND SHADER_FILES_SPV ${SHADER_FILE_SPV})
        endforeach()
    endif()

    find_program(DXC_EXECUTABLE dxc HINTS $ENV{VULKAN_SDK}/bin)
    if(NOT DXC_EXECUTABLE)
        message(WARNING "dxc not found! Shaders with .hlsl extension will not be compiled to SPIR-V. Please set VULKAN_SDK to the Vulkan SDK path.")
    endif()

    # Compile HLSL shaders to SPIR-V
    if (DXC_EXECUTABLE)
        foreach(SHADER_FILE_HLSL ${TARGET_SHADER_FILES_HLSL})
            # Skip header and OpenCL files
            if(${SHADER_FILE_HLSL} MATCHES ".*\\.h$" OR ${SHADER_FILE_HLSL} MATCHES ".*\\.cl$")
                continue()
            endif()

            set(SHADER_FILE_SPV "${SHADER_DIR}/${SHADER_FILE_HLSL}.spv")

            get_filename_component(SHADER_FILE_DIR ${SHADER_FILE_SPV} DIRECTORY)
            file(MAKE_DIRECTORY ${SHADER_FILE_DIR})

            get_hlsl_target_profile(${SHADER_FILE_HLSL} SHADER_PROFILE)

            add_custom_command(
                OUTPUT ${SHADER_FILE_SPV}
                COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}"
                COMMAND ${DXC_EXECUTABLE} -spirv -fspv-target-env=vulkan1.3 -E main -T ${SHADER_PROFILE} -Fo "${SHADER_FILE_SPV}" ${PROJECT_SOURCE_DIR}/shaders/${SHADER_FILE_HLSL}
                COMMAND ${CMAKE_COMMAND} -E copy "${SHADER_FILE_SPV}" "${PROJECT_SOURCE_DIR}/shaders/${SHADER_FILE_HLSL}.spv"
                DEPENDS ${PROJECT_SOURCE_DIR}/shaders/${SHADER_FILE_HLSL}
                COMMENT "Compiling ${SHADER_FILE_HLSL} to SPIR-V"
                VERBATIM)
            list(APPEND SHADER_FILES_SPV ${SHADER_FILE_SPV})
        endforeach()
    endif()

    add_custom_target(${TARGET_ID}_shaders DEPENDS ${SHADER_FILES_SPV})
    add_dependencies(${TARGET_ID} ${TARGET_ID}_shaders)
    add_dependencies(vkb__shaders ${TARGET_ID}_shaders)
endfunction()

function(add_project)
    set(options)  
    set(oneValueArgs TYPE ID CATEGORY AUTHOR NAME DESCRIPTION)
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

    source_group("\\" FILES ${TARGET_FILES})

    # Add shaders to project group
    if (TARGET_SHADERS_GLSL)
        list(APPEND SOURCE_SHADER_FILES)
        foreach(GLSL_FILE ${TARGET_SHADERS_GLSL})
            list(APPEND SOURCE_SHADER_FILES "${PROJECT_SOURCE_DIR}/shaders/${GLSL_FILE}")
        endforeach()   
    endif()

    if (TARGET_SHADERS_HLSL)
        list(APPEND SOURCE_SHADER_FILES)
        foreach(HLSL_FILE ${TARGET_SHADERS_HLSL})
            list(APPEND SOURCE_SHADER_FILES "${PROJECT_SOURCE_DIR}/shaders/${HLSL_FILE}")
        endforeach()   
    endif()
    
    if (SOURCE_SHADER_FILES)
        source_group("\\Shaders" FILES ${SOURCE_SHADER_FILES})
    endif()

if(${TARGET_TYPE} STREQUAL "Sample")
    add_library(${TARGET_ID} OBJECT ${TARGET_FILES} ${SOURCE_SHADER_FILES})
elseif(${TARGET_TYPE} STREQUAL "Test")
    add_library(${TARGET_ID} STATIC ${TARGET_FILES} ${SOURCE_SHADER_FILES})
endif()
    set_target_properties(${TARGET_ID} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    # # inherit include directories from framework target
    target_include_directories(${TARGET_ID} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(${TARGET_ID} PRIVATE framework)

    # Link against extra project specific libraries
    if(TARGET_LIBS)
        target_link_libraries(${TARGET_ID} PUBLIC ${TARGET_LIBS})
    endif()

    # capitalise the first letter of the category  (performance -> Performance) 
    string(SUBSTRING ${TARGET_CATEGORY} 0 1 FIRST_LETTER)
    string(TOUPPER ${FIRST_LETTER} FIRST_LETTER)
    string(REGEX REPLACE "^.(.*)" "${FIRST_LETTER}\\1" CATEGORY "${TARGET_CATEGORY}")

    if(${TARGET_TYPE} STREQUAL "Sample")
        # set sample properties
        set_target_properties(${TARGET_ID}
            PROPERTIES 
                SAMPLE_CATEGORY ${TARGET_CATEGORY}
                SAMPLE_AUTHOR ${TARGET_AUTHOR}
                SAMPLE_NAME ${TARGET_NAME}
                SAMPLE_DESCRIPTION ${TARGET_DESCRIPTION}
                SAMPLE_TAGS "${TARGET_TAGS}")

        # add sample project to a folder
        set_property(TARGET ${TARGET_ID} PROPERTY FOLDER "Samples//${CATEGORY}")
    elseif(${TARGET_TYPE} STREQUAL "Test")
        # add test project to a folder
        set_property(TARGET ${TARGET_ID} PROPERTY FOLDER "Tests")
    endif()

    if(VKB_DO_CLANG_TIDY)
        set_target_properties(${TARGET_ID} PROPERTIES CXX_CLANG_TIDY "${VKB_DO_CLANG_TIDY}")
    endif()

    compile_shaders(
        ID ${TARGET_ID}
        SHADER_FILES_GLSL ${TARGET_SHADERS_GLSL}
        SHADER_FILES_HLSL ${TARGET_SHADERS_HLSL}
    )
endfunction()
