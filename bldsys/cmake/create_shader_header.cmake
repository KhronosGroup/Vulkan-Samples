# Copyright 2024 Bradley Austin Davis
#
# Licensed under the Apache License, Version 2.0 (the "License");
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


# A CMake script to convert SPIR-V files to C++ header files
# Inputs:
#   SHADER_SPIRV: The full path of the SPIRV compiled file to convert
#   SHADER_HEADER: The full path of the header file to generate
#   SHADER_FILE: The original shader file, relative to the shaders folder
#   HEADER_TEMPLATE: The full path of the header template file to use

# Spirv headers are little-endian, so if we want to
# construct a header file that contains the spirv
# data as an uint32_t literal array, we need to swap the
# endianness bytes to construct the proper 0xAAAAAAAA literal.
macro(ENDIAN_SWAP)
    set(endian_swap_temp "")
    while (NOT ("${dword}" STREQUAL ""))
        # Take the first 8 characters from the string
        string(SUBSTRING ${dword} 0 2 endian_swap_byte)
        # Remove the first 8 characters from the string
        string(SUBSTRING ${dword} 2 -1 dword)
        string(PREPEND endian_swap_temp "${endian_swap_byte}")
    endwhile()
    set(dword "${endian_swap_temp}")
endmacro()



function(GET_SHADER_STAGE_FOR_TYPE SHADER_TYPE OUTPUT_NAME)
    if(${SHADER_TYPE} STREQUAL "vert")
        set(${OUTPUT_NAME} "VK_SHADER_STAGE_VERTEX_BIT")
    elseif(${SHADER_TYPE} STREQUAL "tesc")
        set(${OUTPUT_NAME} "VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT")
    elseif(${SHADER_TYPE} STREQUAL "tese")
        set(${OUTPUT_NAME} "VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT")
    elseif(${SHADER_TYPE} STREQUAL "geom")
        set(${OUTPUT_NAME} "VK_SHADER_STAGE_GEOMETRY_BIT")
    elseif(${SHADER_TYPE} STREQUAL "geo")
        set(${OUTPUT_NAME} "VK_SHADER_STAGE_GEOMETRY_BIT")
    elseif(${SHADER_TYPE} STREQUAL "frag")
        set(${OUTPUT_NAME} "VK_SHADER_STAGE_FRAGMENT_BIT")
    elseif(${SHADER_TYPE} STREQUAL "comp")
        set(${OUTPUT_NAME} "VK_SHADER_STAGE_COMPUTE_BIT")
    elseif(${SHADER_TYPE} STREQUAL "rgen")
        set(${OUTPUT_NAME} "VK_SHADER_STAGE_RAYGEN_BIT_KHR")
    elseif(${SHADER_TYPE} STREQUAL "rchit")
        set(${OUTPUT_NAME} "VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR")
    elseif(${SHADER_TYPE} STREQUAL "rmiss")
        set(${OUTPUT_NAME} "VK_SHADER_STAGE_MISS_BIT_KHR")
    elseif(${SHADER_TYPE} STREQUAL "rint")
        set(${OUTPUT_NAME} "VK_SHADER_STAGE_INTERSECTION_BIT_KHR")
    elseif(${SHADER_TYPE} STREQUAL "rahit")
        set(${OUTPUT_NAME} "VK_SHADER_STAGE_ANY_HIT_BIT_KHR")
    elseif(${SHADER_TYPE} STREQUAL "mesh")
        set(${OUTPUT_NAME} "VK_SHADER_STAGE_MESH_BIT_EXT")
    elseif(${SHADER_TYPE} STREQUAL "task")
        set(${OUTPUT_NAME} "VK_SHADER_STAGE_TASK_BIT_EXT")
    elseif(${SHADER_TYPE} STREQUAL "rcall")
        set(${OUTPUT_NAME} "VK_SHADER_STAGE_CALLABLE_BIT_KHR")
    else()
        message(FATAL_ERROR "Unknown shader type ${SHADER_TYPE}")
    endif()
    set(${OUTPUT_NAME} "${${OUTPUT_NAME}}" PARENT_SCOPE)
endfunction()


macro(READ_CODE)
    message(STATUS "Reading SPIRV file: ${SHADER_SPIRV}")
    file(READ ${SHADER_SPIRV} input_hex HEX)
    string(LENGTH ${input_hex} SPIRV_NIBBLE_COUNT)
    math(EXPR SHADER_TOKENS "${SPIRV_NIBBLE_COUNT} / 8")

    set(SHADER_CODE "")
    set(COUNTER 0)
    # Iterate through each of the 32 bit tokens from the source file
    while (NOT ("${input_hex}" STREQUAL ""))
        if (COUNTER GREATER 8)
            # Write a newline so that all of the array initializer
            # gets spread across multiple lines.
            string(APPEND SHADER_CODE "\n                    ")
            set(COUNTER 0)
        endif()

        # Take the first 8 characters from the string
        string(SUBSTRING ${input_hex} 0 8 dword)
        # Remove the first 8 characters from the string
        string(SUBSTRING ${input_hex} 8 -1 input_hex)
        ENDIAN_SWAP()
        # Write the hex string to the line with an 0x prefix
        string(APPEND SHADER_CODE "0x${dword},")
        # Increment the element counter before the newline.
        math(EXPR COUNTER "${COUNTER}+1")
    endwhile()
endmacro()


READ_CODE()

get_filename_component(SHADER_FOLDER ${SHADER_FILE} DIRECTORY)
get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)
get_filename_component(SHADER_TYPE ${SHADER_FILE} EXT)
# Remove the leading dot from the extension to get the shader type
string(SUBSTRING ${SHADER_TYPE} 1 -1 SHADER_TYPE)
set(SHADER_SOURCE_FILE "${SHADER_BASE_PATH}/${SHADER_FILE}")
file(READ ${SHADER_SOURCE_FILE} SHADER_SOURCE)
get_shader_stage_for_type(${SHADER_TYPE} SHADER_STAGE)
configure_file("${HEADER_TEMPLATE}" ${SHADER_HEADER} @ONLY)


