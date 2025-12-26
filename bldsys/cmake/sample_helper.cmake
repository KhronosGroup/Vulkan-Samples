#[[
 Copyright (c) 2019-2025, Arm Limited and Contributors
 Copyright (c) 2024-2025, Mobica Limited
 Copyright (c) 2024-2025, Sascha Willems

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
    set(oneValueArgs ID CATEGORY AUTHOR NAME DESCRIPTION DXC_ADDITIONAL_ARGUMENTS GLSLC_ADDITIONAL_ARGUMENTS)
    set(multiValueArgs FILES LIBS SHADER_FILES_GLSL SHADER_FILES_HLSL SHADER_FILES_SLANG)

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
            ${TARGET_FILES}
        LIBS
            ${TARGET_LIBS}
        SHADER_FILES_GLSL
            ${TARGET_SHADER_FILES_GLSL}
        SHADER_FILES_HLSL
            ${TARGET_SHADER_FILES_HLSL}
        SHADER_FILES_SLANG
            ${TARGET_SHADER_FILES_SLANG}
        DXC_ADDITIONAL_ARGUMENTS ${TARGET_DXC_ADDITIONAL_ARGUMENTS}
        GLSLC_ADDITIONAL_ARGUMENTS ${TARGET_GLSLC_ADDITIONAL_ARGUMENTS})
endfunction()

function(add_sample_with_tags)
    set(options)
    set(oneValueArgs ID CATEGORY AUTHOR NAME DESCRIPTION DXC_ADDITIONAL_ARGUMENTS GLSLC_ADDITIONAL_ARGUMENTS)
    set(multiValueArgs TAGS FILES LIBS SHADER_FILES_GLSL SHADER_FILES_HLSL SHADER_FILES_SLANG SHADER_FILES_SPVASM)

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
    foreach(SHADER_FILE_GLSL ${TARGET_SHADER_FILES_GLSL})
        list(APPEND SHADERS_GLSL "${PROJECT_SOURCE_DIR}/shaders/${SHADER_FILE_GLSL}")
    endforeach()

    # Add HLSL shader files for this sample
    foreach(SHADER_FILE_HLSL ${TARGET_SHADER_FILES_HLSL})
        list(APPEND SHADERS_HLSL "${PROJECT_SOURCE_DIR}/shaders/${SHADER_FILE_HLSL}")
    endforeach()

    # Add slang shader files for this sample
    foreach(SHADER_FILE_SLANG ${TARGET_SHADER_FILES_SLANG})
        list(APPEND SHADERS_SLANG "${PROJECT_SOURCE_DIR}/shaders/${SHADER_FILE_SLANG}")
    endforeach()

    # Add spvasm shader files for this sample
    foreach(SHADER_FILE_SPVASM ${TARGET_SHADER_FILES_SPVASM})
        list(APPEND SHADERS_SPVASM "${PROJECT_SOURCE_DIR}/shaders/${SHADER_FILE_SPVASM}")
    endforeach()

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
            ${SHADERS_GLSL}
        SHADERS_HLSL
            ${SHADERS_HLSL}
        SHADERS_SLANG
            ${SHADERS_SLANG}
        SHADERS_SPVASM
            ${SHADERS_SPVASM}
        DXC_ADDITIONAL_ARGUMENTS ${TARGET_DXC_ADDITIONAL_ARGUMENTS}
        GLSLC_ADDITIONAL_ARGUMENTS ${TARGET_GLSLC_ADDITIONAL_ARGUMENTS})
endfunction()

function(add_project)
    set(options)
    set(oneValueArgs TYPE ID CATEGORY AUTHOR NAME DESCRIPTION DXC_ADDITIONAL_ARGUMENTS GLSLC_ADDITIONAL_ARGUMENTS)
    set(multiValueArgs TAGS FILES LIBS SHADERS_GLSL SHADERS_HLSL SHADERS_SLANG SHADERS_SPVASM)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(${TARGET_TYPE} STREQUAL "Sample")
        set("VKB_${TARGET_ID}" ON CACHE BOOL "Build sample ${TARGET_ID}")
    endif()

    if(NOT ${VKB_${TARGET_ID}})
        # message(STATUS "${TARGET_TYPE} `${TARGET_ID}` - DISABLED")
        return()
    endif()

    message(STATUS "${TARGET_TYPE} `${TARGET_ID}` - BUILD")

    # create project (object target - reused by app target)
    project(${TARGET_ID} LANGUAGES C CXX)

    source_group("\\" FILES ${TARGET_FILES})

    # Add shaders to project group
    if (TARGET_SHADERS_GLSL)
        source_group("\\Shaders\\glsl" FILES ${TARGET_SHADERS_GLSL})
    endif()
    if (TARGET_SHADERS_HLSL)
        source_group("\\Shaders\\hlsl" FILES ${TARGET_SHADERS_HLSL})
        # Disable automatic compilation of HLSL shaders for MSVC
        set_source_files_properties(SOURCE ${SHADERS_HLSL} PROPERTIES VS_SETTINGS "ExcludedFromBuild=true")
    endif()
    if (TARGET_SHADERS_SLANG)
        source_group("\\Shaders\\slang" FILES ${TARGET_SHADERS_SLANG})
    endif()
    if (TARGET_SHADERS_SPVASM)
        source_group("\\Shaders\\spvasm" FILES ${TARGET_SHADERS_SPVASM})
    endif()

if(${TARGET_TYPE} STREQUAL "Sample")
    add_library(${PROJECT_NAME} OBJECT ${TARGET_FILES} ${SHADERS_GLSL} ${SHADERS_HLSL} ${SHADERS_SLANG} ${SHADERS_SPVASM})
elseif(${TARGET_TYPE} STREQUAL "Test")
    add_library(${PROJECT_NAME} STATIC ${TARGET_FILES} ${SHADERS_GLSL} ${SHADERS_HLSL} ${SHADERS_SLANG} ${SHADERS_SPVASM})
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

    # HLSL compilation via DXC
    if(Vulkan_dxc_EXECUTABLE AND DEFINED SHADERS_HLSL)
        set(OUTPUT_FILES "")
        set(HLSL_TARGET_NAME ${PROJECT_NAME}-HLSL)
        foreach(SHADER_FILE_HLSL ${TARGET_SHADERS_HLSL})
            get_filename_component(HLSL_SPV_FILE ${SHADER_FILE_HLSL} NAME_WLE)
            get_filename_component(bare_name ${HLSL_SPV_FILE} NAME_WLE)
            get_filename_component(extension ${HLSL_SPV_FILE} LAST_EXT)
            get_filename_component(directory ${SHADER_FILE_HLSL} DIRECTORY)
            string(REGEX REPLACE "[.]+" "" extension ${extension})
            set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shader-spv")
            set(OUTPUT_FILE ${OUTPUT_DIR}/${bare_name}.${extension}.spv)
            file(MAKE_DIRECTORY ${OUTPUT_DIR})

            set(DXC_PROFILE_TYPE "")
            set(DXC_PROFILE_VER "6_1")
            if(${extension} STREQUAL "vert")
                set(DXC_PROFILE_TYPE "vs")
            elseif(${extension} STREQUAL "frag")
                set(DXC_PROFILE_TYPE "ps")
                set(DXC_PROFILE_VER "6_4")
            elseif(${extension} STREQUAL "rgen" OR ${extension} STREQUAL "rmiss" OR ${extension} STREQUAL "rchit")
                list(APPEND TARGET_DXC_ADDITIONAL_ARGUMENTS "-fspv-extension=SPV_KHR_ray_tracing -fspv-target-env=vulkan1.1spirv1.4")
                set(DXC_PROFILE_TYPE "lib")
                set(DXC_PROFILE_VER "6_3")
            elseif(${extension} STREQUAL "comp")
                set(DXC_PROFILE_TYPE "cs")
            elseif(${extension} STREQUAL "geom")
                set(DXC_PROFILE_TYPE "gs")
            elseif(${extension} STREQUAL "tesc")
                set(DXC_PROFILE_TYPE "hs")
            elseif(${extension} STREQUAL "tese")
                set(DXC_PROFILE_TYPE "ds")
            elseif(${extension} STREQUAL "mesh")
                list(APPEND TARGET_DXC_ADDITIONAL_ARGUMENTS "-fspv-target-env=vulkan1.1spirv1.4")
                set(DXC_PROFILE_TYPE "ms")
                set(DXC_PROFILE_VER "6_6")
            endif()
            set(DXC_PROFILE ${DXC_PROFILE_TYPE}_${DXC_PROFILE_VER})
            if(NOT "${TARGET_DXC_ADDITIONAL_ARGUMENTS}" STREQUAL "")
                string(REPLACE " " ";" TARGET_DXC_ADDITIONAL_ARGUMENTS "${TARGET_DXC_ADDITIONAL_ARGUMENTS}")
            endif ()
            add_custom_command(
                OUTPUT ${OUTPUT_FILE}
                COMMAND ${Vulkan_dxc_EXECUTABLE} -spirv -T ${DXC_PROFILE} -E main ${TARGET_DXC_ADDITIONAL_ARGUMENTS} ${SHADER_FILE_HLSL} -Fo ${OUTPUT_FILE}
                COMMAND ${CMAKE_COMMAND} -E copy ${OUTPUT_FILE} ${directory}
                MAIN_DEPENDENCY ${SHADER_FILE_HLSL}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            )
            list(APPEND OUTPUT_FILES ${OUTPUT_FILE})
            set_source_files_properties(${OUTPUT_FILE} PROPERTIES
                MACOSX_PACKAGE_LOCATION Resources
            )
        endforeach()
        add_custom_target(${HLSL_TARGET_NAME} DEPENDS ${OUTPUT_FILES})
        set_property(TARGET ${HLSL_TARGET_NAME} PROPERTY FOLDER "Shaders-HLSL")
        add_dependencies(${PROJECT_NAME} ${HLSL_TARGET_NAME})
    endif()

    # Slang shader compilation
    # Skip on MacOS/iOS due to CI/CD using potentially broken slang compiler versions from the SDK
    # Might revisit once Slang shipped with the SDK is usable
    set(SLANG_SKIP_COMPILE false)
    if (($ENV{CI} MATCHES true) AND ((CMAKE_SYSTEM_NAME MATCHES "Darwin") OR (CMAKE_SYSTEM_NAME MATCHES "iOS")))
        set(SLANG_SKIP_COMPILE true)
    endif()
    if(VKB_SKIP_SLANG_SHADER_COMPILATION)
        set(SLANG_SKIP_COMPILE true)
    endif()
    if(NOT SLANG_SKIP_COMPILE AND Vulkan_slang_EXECUTABLE AND DEFINED SHADERS_SLANG)
        set(OUTPUT_FILES "")
        set(SLANG_TARGET_NAME ${PROJECT_NAME}-SLANG)
        foreach(SHADER_FILE_SLANG ${TARGET_SHADERS_SLANG})
            get_filename_component(SLANG_SPV_FILE ${SHADER_FILE_SLANG} NAME_WLE)
            get_filename_component(bare_name ${SLANG_SPV_FILE} NAME_WLE)
            get_filename_component(extension ${SLANG_SPV_FILE} LAST_EXT)
            get_filename_component(directory ${SHADER_FILE_SLANG} DIRECTORY)
            string(REGEX REPLACE "[.]+" "" extension ${extension})
            set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shader-slang-spv")
            set(OUTPUT_FILE ${OUTPUT_DIR}/${bare_name}.${extension}.spv)
            file(MAKE_DIRECTORY ${OUTPUT_DIR})
            set(SLANG_PROFILE "spirv_1_4")
            set(SLANG_ENTRY_POINT "main")
            add_custom_command(
                OUTPUT ${OUTPUT_FILE}
                COMMAND ${Vulkan_slang_EXECUTABLE} ${SHADER_FILE_SLANG} -profile ${SLANG_PROFILE} -matrix-layout-column-major -target spirv -o ${OUTPUT_FILE} -entry ${SLANG_ENTRY_POINT}
                COMMAND ${CMAKE_COMMAND} -E copy ${OUTPUT_FILE} ${directory}
                MAIN_DEPENDENCY ${SHADER_FILE_SLANG}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            )
            list(APPEND OUTPUT_FILES ${OUTPUT_FILE})
            set_source_files_properties(${OUTPUT_FILE} PROPERTIES
                MACOSX_PACKAGE_LOCATION Resources
            )
        endforeach()
        add_custom_target(${SLANG_TARGET_NAME} DEPENDS ${OUTPUT_FILES})
        set_property(TARGET ${SLANG_TARGET_NAME} PROPERTY FOLDER "Shaders-SLANG")
        add_dependencies(${PROJECT_NAME} ${SLANG_TARGET_NAME})
    endif()

    # GLSL shader compilation
    if(Vulkan_glslc_EXECUTABLE AND DEFINED SHADERS_GLSL)
        set(GLSL_TARGET_NAME ${PROJECT_NAME}-GLSL)
        set(OUTPUT_FILES "")
        foreach(SHADER_FILE_GLSL ${TARGET_SHADERS_GLSL})
            get_filename_component(GLSL_SPV_FILE ${SHADER_FILE_GLSL} NAME_WLE)
            get_filename_component(bare_name ${GLSL_SPV_FILE} NAME_WLE)
            get_filename_component(extension ${SHADER_FILE_GLSL} LAST_EXT)
            get_filename_component(directory ${SHADER_FILE_GLSL} DIRECTORY)
            # Skip extensions that can't be compiled (cl, inlcudes)
            if(${extension} STREQUAL ".cl" OR ${extension} STREQUAL ".h")
                continue()
            endif()
            set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shader-glsl-spv")
            set(OUTPUT_FILE ${OUTPUT_DIR}/${bare_name}${extension}.spv)
            file(MAKE_DIRECTORY ${OUTPUT_DIR})

            # NOTE: Vulkan SDK has old glslc but new glslang. We must use glslang to compile shaders from `tensor_and_data_graph`.
            # TODO: Remove workaround once glslc is updated.
            if ("${CMAKE_CURRENT_BINARY_DIR}" MATCHES "tensor_and_data_graph" AND NOT ${SHADER_FILE_GLSL} MATCHES "base")
                #  glslang (NOT glslangValidator, as that also seems to be old)
                string(REPLACE "glslangValidator" "glslang" GLSLANG_EXECUTABLE ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE})
                SET(COMPILE_COMMAND ${GLSLANG_EXECUTABLE} ${SHADER_FILE_GLSL} -o ${OUTPUT_FILE} -V -I"${CMAKE_SOURCE_DIR}/shaders/includes/glsl" ${TARGET_GLSLC_ADDITIONAL_ARGUMENTS})
            else()
                # glslc
                SET(COMPILE_COMMAND ${Vulkan_glslc_EXECUTABLE} ${SHADER_FILE_GLSL} -o ${OUTPUT_FILE} -I "${CMAKE_SOURCE_DIR}/shaders/includes/glsl" ${TARGET_GLSLC_ADDITIONAL_ARGUMENTS})
            endif()


            add_custom_command(
                OUTPUT ${OUTPUT_FILE}
                COMMAND ${COMPILE_COMMAND}
                COMMAND ${CMAKE_COMMAND} -E copy ${OUTPUT_FILE} ${directory}
                MAIN_DEPENDENCY ${SHADER_FILE_GLSL}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            )
            list(APPEND OUTPUT_FILES ${OUTPUT_FILE})
            set_source_files_properties(${OUTPUT_FILE} PROPERTIES
                MACOSX_PACKAGE_LOCATION Resources
            )
        endforeach()
        add_custom_target(${GLSL_TARGET_NAME} DEPENDS ${OUTPUT_FILES})
        set_property(TARGET ${GLSL_TARGET_NAME} PROPERTY FOLDER "Shaders-GLSL")
        add_dependencies(${PROJECT_NAME} ${GLSL_TARGET_NAME})
    endif()

    # spvasm shader compilation
    if(Vulkan_spirvas_EXECUTABLE AND DEFINED SHADERS_SPVASM)
        set(SPVASM_TARGET_NAME ${PROJECT_NAME}-SPVASM)
        set(OUTPUT_FILES "")
        foreach(SHADER_FILE_SPVASM ${TARGET_SHADERS_SPVASM})
            get_filename_component(SPVASM_FILE ${SHADER_FILE_SPVASM} NAME_WLE)
            get_filename_component(bare_name ${SPVASM_FILE} NAME_WLE)
            get_filename_component(extension ${SHADER_FILE_SPVASM} LAST_EXT)
            get_filename_component(directory ${SHADER_FILE_SPVASM} DIRECTORY)
            set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shader-spvasm-spv")
            set(OUTPUT_FILE ${OUTPUT_DIR}/${bare_name}${extension}.spv)
            file(MAKE_DIRECTORY ${OUTPUT_DIR})
            add_custom_command(
                OUTPUT ${OUTPUT_FILE}
                COMMAND ${Vulkan_spirvas_EXECUTABLE} ${SHADER_FILE_SPVASM} -o ${OUTPUT_FILE} ${TARGET_SPVASM_ADDITIONAL_ARGUMENTS}
                COMMAND ${CMAKE_COMMAND} -E copy ${OUTPUT_FILE} ${directory}
                MAIN_DEPENDENCY ${SHADER_FILE_SPVASM}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            )
            list(APPEND OUTPUT_FILES ${OUTPUT_FILE})
            set_source_files_properties(${OUTPUT_FILE} PROPERTIES
                MACOSX_PACKAGE_LOCATION Resources
            )
        endforeach()
        add_custom_target(${SPVASM_TARGET_NAME} DEPENDS ${OUTPUT_FILES})
        set_property(TARGET ${SPVASM_TARGET_NAME} PROPERTY FOLDER "Shaders-SPVASM")
        add_dependencies(${PROJECT_NAME} ${SPVASM_TARGET_NAME})

    endif()

endfunction()