add_custom_target(vkb__shaders)

set(VALID_SHADERS
    Vertex
    Fragment
    Compute
    Geometry
    TessellationControl
    TessellationEvaluation
    RayGeneration
    Intersection
    AnyHit
    ClosestHit
    Miss
    Callable
    Task
    Mesh
)

string(JOIN ", " VALID_SHADERS ${VALID_SHADERS})

function(vkb__register_shader)
    set(options)
    set(oneValueArgs NAME LANGUAGE SHADER_TYPE SHADER_PATH ENTRY_POINT)
    set(multiValueArgs INCLUDE_DIRS DEFINES VARIANT_DEFINES)

    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT TARGET_NAME OR TARGET_NAME STREQUAL "")
        message(FATAL_ERROR "Shader name not specified")
    endif()

    if (NOT TARGET_LANGUAGE)
        message(FATAL_ERROR "Shader language not specified: GLSL, HLSL")
    endif()

    set(SHADER_LANGUAGE "")
    if (TARGET_LANGUAGE STREQUAL "GLSL" OR TARGET_LANGUAGE STREQUAL "HLSL")
        set(SHADER_LANGUAGE ${TARGET_LANGUAGE})
    endif()

    if(SHADER_LANGUAGE STREQUAL "")
        message(FATAL_ERROR "Shader language not supported: ${TARGET_LANGUAGE}")
    endif()

    if (NOT TARGET_SHADER_TYPE)
        message(FATAL_ERROR "Use a valid shader type:\n\n${VALID_SHADERS}\n\n")
    endif()

    if (NOT TARGET_SHADER_PATH)
        message(FATAL_ERROR "Shader path not specified")
    endif()

    set(ENTRY_POINT "main")
    if (TARGET_ENTRY_POINT)
        set(ENTRY_POINT ${TARGET_ENTRY_POINT})
    endif()

    set(JSON_DATA "{ \"name\" : \"${TARGET_NAME}\"")
    string(APPEND JSON_DATA ", \"language\" : \"${SHADER_LANGUAGE}\"")
    string(APPEND JSON_DATA ", \"type\" : \"${TARGET_SHADER_TYPE}\"")
    string(APPEND JSON_DATA ", \"path\": \"${TARGET_SHADER_PATH}\"")
    string(APPEND JSON_DATA ", \"entry_point\" : \"${ENTRY_POINT}\"")

    if(TARGET_INCLUDE_DIRS)
        string(APPEND JSON_DATA ", \"include_folders\" : [")
        foreach(INCLUDE_DIR ${TARGET_INCLUDE_DIRS})
            string(APPEND JSON_DATA "\"${INCLUDE_DIR}\",")
        endforeach()
        string(REGEX REPLACE ",$" "" JSON_DATA ${JSON_DATA})
        string(APPEND JSON_DATA "]")
    endif()

    if(TARGET_DEFINES)
        string(APPEND JSON_DATA ", \"defines\" : [")
        foreach(DEFINES ${TARGET_DEFINES})
            string(APPEND JSON_DATA "\"${DEFINES}\",")
        endforeach()
        string(REGEX REPLACE ",$" "" JSON_DATA ${JSON_DATA})
        string(APPEND JSON_DATA "]")
    endif()

    if(TARGET_VARIANT_DEFINES)
        string(APPEND JSON_DATA ", \"variant_defines\" : [")
        foreach(DEFINES ${TARGET_VARIANT_DEFINES})
            string(APPEND JSON_DATA "\"${DEFINES}\",")
        endforeach()
        string(REGEX REPLACE ",$" "" JSON_DATA ${JSON_DATA})
        string(APPEND JSON_DATA "]")
    endif()

    string(APPEND JSON_DATA "}")

    # final parsing
    string(REPLACE " " "" JSON_DATA ${JSON_DATA})
    string(REPLACE "\"" "\\\"" JSON_DATA ${JSON_DATA})
    set(JSON_DATA "\"${JSON_DATA}\"")

    set(OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.generated.hpp")

    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND $<TARGET_FILE:shader_compiler> --json-schema=${JSON_DATA} --output=${OUTPUT_FILE}
        DEPENDS shader_compiler ${TARGET_SHADER_PATH}
        COMMENT "Generating shader header for ${TARGET_NAME}"
    )

    add_custom_target("shader__${TARGET_NAME}" DEPENDS ${OUTPUT_FILE})

    add_dependencies("shader__${TARGET_NAME}" shader_compiler)
    add_dependencies(vkb__shaders "shader__${TARGET_NAME}")

endfunction()