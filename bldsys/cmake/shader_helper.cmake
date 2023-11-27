add_custom_target(shaders)
add_custom_target(shader_reflections)
add_custom_target(shaders_full)
add_dependencies(shaders_full shaders shader_reflections)

# Require Python 3 for calling shader compile script
find_package (Python3 COMPONENTS Interpreter Development)

# We need to know the target language for the shader
set(TARGET_LANGUAGE_VALUES glsl hlsl)

set(SHADER_COMPILER_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/compile_shader.py)
set(SHADER_VARIANT_FILE ${CMAKE_SOURCE_DIR}/generated/shader_variants.json)
set(SHADER_ATLAS_FILE ${CMAKE_SOURCE_DIR}/generated/shader_atlas.json)
set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(ASSET_DIR ${CMAKE_SOURCE_DIR}/assets)

add_custom_target(
    generate_shader_variants
    COMMAND variant_reflector --output ${SHADER_VARIANT_FILE} --shader-dir ${SHADER_DIR} --asset-dir ${ASSET_DIR}
    COMMAND_EXPAND_LISTS
)

# configure_shader(
#   NAME <target_name>
#   SOURCE <shader>.<stage>.<language>
#   OUTPUT <output_file>
#)
function(configure_shader)
    set(options )
    set(oneValueArgs NAME SOURCE OUTPUT)
    set(multiValueArgs)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT TARGET_SOURCE)
        message(FATAL_ERROR "No source shader file specified")
    endif()

    if (NOT TARGET_OUTPUT)
        message(FATAL_ERROR "No output file specified")
    endif()

    string(REGEX MATCH "^(.*)\\.(.*)\\.(.*)$" SOURCE_MATCH ${TARGET_SOURCE})
    if (NOT SOURCE_MATCH)
        message(FATAL_ERROR "Invalid source shader file specified")
    endif()

    set(SOURCE_NAME ${CMAKE_MATCH_1})
    set(STAGE ${CMAKE_MATCH_2})
    set(LANGUAGE ${CMAKE_MATCH_3})

    if (NOT ${LANGUAGE} IN_LIST TARGET_LANGUAGE_VALUES)
        message(FATAL_ERROR "No valid shader language specified. Valid values are: ${TARGET_LANGUAGE_VALUES}")
    endif()

    string(TOUPPER ${LANGUAGE} CAPITALIZED_LANGAUGE)
    
    set(SHADER_TARGET_NAME ${CAPITALIZED_LANGAUGE}__${TARGET_NAME})
    message(STATUS "Shader ${SHADER_TARGET_NAME}")

    set(SHADER_SPIRV ${CMAKE_SOURCE_DIR}/generated/${LANGUAGE}/${TARGET_OUTPUT})

    add_custom_target(
        ${SHADER_TARGET_NAME}
        COMMAND ${PYTHON_EXECUTABLE} ${SHADER_COMPILER_SCRIPT} ${TARGET_SOURCE} ${SHADER_SPIRV} --language ${LANGUAGE} --variants ${SHADER_VARIANT_FILE} --atlas ${SHADER_ATLAS_FILE}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS ${TARGET_SOURCE} generate_shader_variants
        COMMENT "Compiling shader ${SOURCE_NAME}"
    )

    add_custom_target(
        ${SHADER_TARGET_NAME}_reflection
        COMMAND shader_reflector ${SHADER_SPIRV} ${CMAKE_SOURCE_DIR}/generated/${LANGUAGE}/${TARGET_NAME}.json ${STAGE}
        DEPENDS ${SHADER_TARGET_NAME} shader_reflector
        COMMENT "Generating shader reflection for ${SHADER_SPIRV}"
    )

    add_dependencies(shaders ${SHADER_TARGET_NAME})
    add_dependencies(shader_reflections ${SHADER_TARGET_NAME}_reflection)
endfunction(configure_shader)