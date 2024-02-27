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
    set(oneValueArgs ID SPIRV_VERSION)
    set(multiValueArgs SHADER_FILES_GLSL SHADER_FILES_HLSL)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT TARGET_SPIRV_VERSION)
        set(TARGET_SPIRV_VERSION "1.0")
    endif()

    find_program(SPV_VALIDATOR_EXECUTABLE spirv-val HINTS $ENV{VULKAN_SDK}/bin)
    if(NOT SPV_VALIDATOR_EXECUTABLE)
        message(WARNING "spirv-val not found! SPIR-V validation will not be performed. Please set VULKAN_SDK to the Vulkan SDK path.")
    endif()

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
            set(STORED_SHADER_FILE_SPV "${PROJECT_SOURCE_DIR}/shaders/${SHADER_FILE_GLSL}.spv")

            get_filename_component(SHADER_FILE_DIR ${SHADER_FILE_SPV} DIRECTORY)
            file(MAKE_DIRECTORY ${SHADER_FILE_DIR})

            add_custom_command(
                OUTPUT ${SHADER_FILE_SPV}
                COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}"
                COMMAND ${GLSLC_EXECUTABLE} -o "${SHADER_FILE_SPV}" --target-spv=spv${TARGET_SPIRV_VERSION} ${PROJECT_SOURCE_DIR}/shaders/${SHADER_FILE_GLSL}
                COMMAND ${CMAKE_COMMAND} -E copy "${SHADER_FILE_SPV}" "${STORED_SHADER_FILE_SPV}"
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
            set(STORED_SHADER_FILE_SPV "${PROJECT_SOURCE_DIR}/shaders/${SHADER_FILE_GLSL}.spv")

            get_filename_component(SHADER_FILE_DIR ${SHADER_FILE_SPV} DIRECTORY)
            file(MAKE_DIRECTORY ${SHADER_FILE_DIR})

            get_hlsl_target_profile(${SHADER_FILE_HLSL} SHADER_PROFILE)

            add_custom_command(
                OUTPUT ${SHADER_FILE_SPV}
                COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}"
                COMMAND ${DXC_EXECUTABLE} -spirv -fspv-target-env=vulkan1.3 -E main -T ${SHADER_PROFILE} -Fo "${SHADER_FILE_SPV}" ${PROJECT_SOURCE_DIR}/shaders/${SHADER_FILE_HLSL}
                COMMAND ${CMAKE_COMMAND} -E copy "${SHADER_FILE_SPV}" "${STORED_SHADER_FILE_SPV}"
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