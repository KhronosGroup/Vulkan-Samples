add_custom_target(vkb__shaders)
set_target_properties(vkb__shaders PROPERTIES FOLDER "CMake\\CustomTargets" POSITION_INDEPENDENT_CODE ON)

if (NOT DEFINED DXC_EXECUTABLE)
    find_program(DXC_EXECUTABLE dxc HINTS $ENV{VULKAN_SDK}/bin)
    if(NOT DXC_EXECUTABLE)
        message(WARNING "dxc not found! Shaders with .hlsl extension will not be compiled to SPIR-V. Please set VULKAN_SDK to the Vulkan SDK path.")
    endif()
endif()

if (NOT DEFINED SPV_VALIDATOR_EXECUTABLE)
    find_program(SPV_VALIDATOR_EXECUTABLE spirv-val HINTS $ENV{VULKAN_SDK}/bin)
    if(NOT SPV_VALIDATOR_EXECUTABLE)
        message(WARNING "spirv-val not found! SPIR-V validation will not be performed. Please set VULKAN_SDK to the Vulkan SDK path.")
    endif()
endif()

if (NOT DEFINED GLSLC_EXECUTABLE)
    find_program(GLSLC_EXECUTABLE glslc HINTS $ENV{VULKAN_SDK}/bin)
    if(NOT GLSLC_EXECUTABLE)
        message(WARNING "glslc not found! Shaders with .glsl extension will not be compiled to SPIR-V. Please set VULKAN_SDK to the Vulkan SDK path.")
    endif()
endif()

set(SHADERS_PROCESSED)

macro(get_hlsl_target_profile SHADER_FILE OUT_PROFILE)
    if(${SHADER_FILE} MATCHES ".*\\.vert$")
        set(${OUT_PROFILE} "vs_6_0")
    elseif(${SHADER_FILE} MATCHES ".*\\.frag$")
        set(${OUT_PROFILE} "ps_6_0")
    else()
        message(FATAL_ERROR "Unknown shader profile for ${SHADER_FILE}")
    endif()
endmacro()

macro(filter_shadsers TYPE OUT_VAR)
    set(${OUT_VAR})
    foreach(SHADER_FILE ${ARGN})
        # Skip header and OpenCL files
        if(${SHADER_FILE} MATCHES ".*\\.h$" OR ${SHADER_FILE} MATCHES ".*\\.cl$")
            continue()
        endif()
        list(APPEND ${OUT_VAR} "${CMAKE_SOURCE_DIR}/shaders/${SHADER_FILE}")
    endforeach()

    source_group("\\Shaders\\${TYPE}" FILES ${${OUT_VAR}})
    target_sources(${TARGET_ID} PRIVATE ${${OUT_VAR}})
endmacro()


macro(compile_shader SHADER_FILE TYPE SPIRV_VERSION)
    set(SHADER_BUILD_DIR "${CMAKE_BINARY_DIR}/shaders")
    set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/shaders")
    file(RELATIVE_PATH SHADER_FILE_REL "${CMAKE_SOURCE_DIR}/shaders" ${SHADER_FILE})
    set(SHADER_FILE_SPV "${SHADER_BUILD_DIR}/${SHADER_FILE_REL}.spv")
    set(STORED_SHADER_FILE_SPV "${SHADER_SOURCE_DIR}/${SHADER_FILE_REL}.spv")

    get_filename_component(SHADER_FILE_DIR ${SHADER_FILE_SPV} DIRECTORY)
    file(MAKE_DIRECTORY ${SHADER_FILE_DIR})

    if (${TYPE} STREQUAL "GLSL")
        add_custom_command(
            OUTPUT ${SHADER_FILE_SPV}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}"
            COMMAND ${GLSLC_EXECUTABLE} -o "${SHADER_FILE_SPV}" --target-spv=spv${SPIRV_VERSION} ${SHADER_FILE}
            COMMAND ${CMAKE_COMMAND} -E copy "${SHADER_FILE_SPV}" "${STORED_SHADER_FILE_SPV}"
            COMMAND ${SPV_VALIDATOR_EXECUTABLE} "${SHADER_FILE_SPV}" || true
            DEPENDS ${SHADER_FILE}
            COMMENT "Compiling GLSL ${SHADER_FILE_REL} to SPIR-V"
            VERBATIM)
    elseif(${TYPE} STREQUAL "HLSL")
        get_hlsl_target_profile(${SHADER_FILE} SHADER_PROFILE)
        add_custom_command(
            OUTPUT ${SHADER_FILE_SPV}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}"
            COMMAND ${DXC_EXECUTABLE} -spirv -fspv-target-env=vulkan1.3 -E main -T ${SHADER_PROFILE} -Fo "${SHADER_FILE_SPV}" ${SHADER_FILE}
            COMMAND ${CMAKE_COMMAND} -E copy "${SHADER_FILE_SPV}" "${STORED_SHADER_FILE_SPV}"
            COMMAND ${SPV_VALIDATOR_EXECUTABLE} "${SHADER_FILE_SPV}" || true
            DEPENDS ${SHADER_FILE}
            COMMENT "Compiling HLSL ${SHADER_FILE_REL} to SPIR-V"
            VERBATIM)
    else()
        message(FATAL_ERROR "Unknown shader type ${TYPE}")
    endif()
    # CMake doesn't like if you reference a generated file in a different scope than you created it with
    # add_custom_command, so all use of `target_sources` referencing the output of the above custom commands
    # needs to be concentrated here.
    target_sources(vkb__shaders PRIVATE ${SHADER_FILE_SPV})
endmacro()


# Because the `compile_shaders` command is called at various different scopes
# we want to push the list of all shaders into a global variable, so during a
# final compile_all_shaders pass we can dedupe them.
macro(queue_shader SHADER_FILE TYPE SPIRV_VERSION)
    get_property(SHADERS_QUEUED GLOBAL PROPERTY SHADERS_QUEUED)
    list(APPEND SHADERS_QUEUED "${SHADER_FILE}|${TYPE}|${SPIRV_VERSION}")
    set_property(GLOBAL PROPERTY SHADERS_QUEUED "${SHADERS_QUEUED}")
endmacro()

function(compile_shaders)
    set(options)
    set(oneValueArgs ID SPIRV_VERSION)
    set(multiValueArgs SHADER_FILES_GLSL SHADER_FILES_HLSL)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT TARGET_SPIRV_VERSION)
        set(TARGET_SPIRV_VERSION "1.0")
    endif()

    filter_shadsers(GLSL SHADER_FILES_GLSL ${TARGET_SHADER_FILES_GLSL})
    # Compile GLSL shaders to SPIR-V
    if (GLSLC_EXECUTABLE)
        foreach(SHADER_FILE ${SHADER_FILES_GLSL})
            queue_shader(${SHADER_FILE} GLSL ${TARGET_SPIRV_VERSION})
        endforeach()
    endif()

    # Compile HLSL shaders to SPIR-V
    filter_shadsers(HLSL SHADER_FILES_HLSL ${TARGET_SHADER_FILES_HLSL})
    if (DXC_EXECUTABLE)
        foreach(SHADER_FILE ${SHADER_FILES_HLSL})
            queue_shader(${SHADER_FILE} HLSL ${TARGET_SPIRV_VERSION})
        endforeach()
    endif()
    add_dependencies(${TARGET_ID} vkb__shaders)
endfunction()

function(compile_all_shaders)
    get_property(SHADERS_QUEUED GLOBAL PROPERTY SHADERS_QUEUED)

    # Ninja doesn't like if there's more than one definition to create a given output file, so we need to dedupe the
    # shaders that may be referenced from more than one example.
    # Note, this means that queueing the same shader with different type or different SPIRV_VERSIONS will cause an
    # error
    list(REMOVE_DUPLICATES SHADERS_QUEUED)

    foreach(SHADER_QUEUED ${SHADERS_QUEUED})
        string(REPLACE "|" ";" SHADERR_LIST ${SHADER_QUEUED})
        list(GET SHADERR_LIST 0 SHADER_FILE)
        list(GET SHADERR_LIST 1 TYPE)
        list(GET SHADERR_LIST 2 SPIRV_VERSION)
        compile_shader(${SHADER_FILE} ${TYPE} ${SPIRV_VERSION})
    endforeach()
endfunction()