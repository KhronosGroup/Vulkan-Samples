add_custom_target(shaders)

# Require Python 3 for calling shader compile script
find_package(PythonInterp 3 REQUIRED)

# We need to know the target language for the shader
set(TARGET_LANGUAGE_VALUES glsl hlsl)

set(SHADER_COMPILER_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/compile_shader.py)

# configure_shader(
#   NAME <target_name>
#   SOURCE <source_shader_file>
#   OUTPUT <output_file>
#   LANGUAGE <glsl|hlsl>
#)
function(configure_shader)
    set(options )
    set(oneValueArgs NAME SOURCE OUTPUT LANGUAGE)
    set(multiValueArgs)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT TARGET_SOURCE)
        message(FATAL_ERROR "No source shader file specified")
    endif()

    if (NOT TARGET_OUTPUT)
        message(FATAL_ERROR "No output file specified")
    endif()

    if (NOT ${TARGET_LANGUAGE} IN_LIST TARGET_LANGUAGE_VALUES)
        message(FATAL_ERROR "No valid shader language specified. Valid values are: ${TARGET_LANGUAGE_VALUES}")
    endif()

    string(TOUPPER ${TARGET_LANGUAGE} CAPITALIZED_LANGAUGE)
    
    set(SHADER_TARGET_NAME ${CAPITALIZED_LANGAUGE}__${TARGET_NAME})
    message(STATUS "Shader ${SHADER_TARGET_NAME}")

    set(SHADER_SPIRV ${CMAKE_SOURCE_DIR}/generated/${TARGET_LANGUAGE}/${TARGET_OUTPUT})

    add_custom_target(
        ${SHADER_TARGET_NAME}
        COMMAND ${PYTHON_EXECUTABLE} ${SHADER_COMPILER_SCRIPT} ${TARGET_SOURCE} ${SHADER_SPIRV} --language ${TARGET_LANGUAGE}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS ${TARGET_SOURCE}
        COMMENT "Compiling shader ${SOURCE_NAME}"
    )

    add_custom_target(
        ${SHADER_TARGET_NAME}_reflection
        COMMAND shader_reflector ${SHADER_SPIRV} ${CMAKE_SOURCE_DIR}/generated/${TARGET_LANGUAGE}/${TARGET_NAME}.json
        DEPENDS ${SHADER_TARGET_NAME} shader_reflector
    )

    add_dependencies(shaders ${SHADER_TARGET_NAME} ${SHADER_TARGET_NAME}_reflection)
endfunction(configure_shader)