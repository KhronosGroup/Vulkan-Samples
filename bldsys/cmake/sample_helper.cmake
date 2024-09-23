#[[
 Copyright (c) 2019-2024, Arm Limited and Contributors
 Copyright (c) 2024, Mobica Limited
 Copyright (c) 2024, Sascha Willems

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
    set(oneValueArgs ID CATEGORY AUTHOR NAME DESCRIPTION DXC_ADDITIONAL_ARGUMENTS)
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
            ${TARGET_FILES}
        LIBS
            ${TARGET_LIBS}
        SHADER_FILES_GLSL
            ${TARGET_SHADER_FILES_GLSL}
        SHADER_FILES_HLSL
            ${TARGET_SHADER_FILES_HLSL}
        DXC_ADDITIONAL_ARGUMENTS ${TARGET_DXC_ADDITIONAL_ARGUMENTS})
endfunction()

function(add_sample_with_tags)
    set(options)
    set(oneValueArgs ID CATEGORY AUTHOR NAME DESCRIPTION DXC_ADDITIONAL_ARGUMENTS)
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
    foreach(SHADER_FILE_GLSL ${TARGET_SHADER_FILES_GLSL})
        list(APPEND SHADERS_GLSL "${PROJECT_SOURCE_DIR}/shaders/${SHADER_FILE_GLSL}")
    endforeach()

    # Add HLSL shader files for this sample
    foreach(SHADER_FILE_HLSL ${TARGET_SHADER_FILES_HLSL})
        list(APPEND SHADERS_HLSL "${PROJECT_SOURCE_DIR}/shaders/${SHADER_FILE_HLSL}")
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
        DXC_ADDITIONAL_ARGUMENTS ${TARGET_DXC_ADDITIONAL_ARGUMENTS})

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
    set(oneValueArgs TYPE ID CATEGORY AUTHOR NAME DESCRIPTION DXC_ADDITIONAL_ARGUMENTS)
    set(multiValueArgs TAGS FILES LIBS SHADERS_GLSL SHADERS_HLSL)

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

if(${TARGET_TYPE} STREQUAL "Sample")
    add_library(${PROJECT_NAME} OBJECT ${TARGET_FILES} ${SHADERS_GLSL} ${SHADERS_HLSL})
elseif(${TARGET_TYPE} STREQUAL "Test")
    add_library(${PROJECT_NAME} STATIC ${TARGET_FILES} ${SHADERS_GLSL} ${SHADERS_HLSL})
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

    if(Vulkan_dxc_EXECUTABLE AND DEFINED SHADERS_HLSL)
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
            add_custom_command( PRE_BUILD
                    OUTPUT ${OUTPUT_FILE}
                    COMMAND ${Vulkan_dxc_EXECUTABLE} -spirv -T ${DXC_PROFILE} -E main ${TARGET_DXC_ADDITIONAL_ARGUMENTS} ${SHADER_FILE_HLSL} -Fo ${OUTPUT_FILE}
                    COMMAND ${CMAKE_COMMAND} -E copy ${OUTPUT_FILE} ${directory}
                    MAIN_DEPENDENCY ${SHADER_FILE_HLSL}
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            )
            if(NOT TARGET ${bare_name}-${extension})
                add_custom_target(${bare_name}-${extension} DEPENDS ${OUTPUT_FILE})
            endif()
            add_dependencies(${PROJECT_NAME} ${bare_name}-${extension})
            set_source_files_properties(${OUTPUT_FILE} PROPERTIES
                    MACOSX_PACKAGE_LOCATION Resources
            )
            set_property(TARGET ${bare_name}-${extension} PROPERTY FOLDER "HLSL_Shaders")
        endforeach()
    endif()
endfunction()