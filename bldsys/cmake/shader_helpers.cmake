
function(vkb__register_shader)
    set(options)
    set(oneValueArgs NAME LANGUAGE VERTEX FRAGMENT COMPUTE)
    set(multiValueArgs INCLUDE_DIRS)

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

    set(JSON_DATA "{ \"language\" : \"${SHADER_LANGUAGE}\"")


    if (TARGET_VERTEX)
        string(APPEND JSON_DATA ", \"vertex\" : \"${TARGET_VERTEX}\"")
    endif()

    if (TARGET_FRAGMENT)
        string(APPEND JSON_DATA ", \"fragment\" : \"${TARGET_FRAGMENT}\"")
    endif()

    if (TARGET_COMPUTE)
        string(APPEND JSON_DATA ", \"compute\" : \"${TARGET_COMPUTE}\"")
    endif()

    if(TARGET_INCLUDE_DIRS)
        string(APPEND JSON_DATA ", \"includeFolders\" : [")
        foreach(INCLUDE_DIR ${TARGET_INCLUDE_DIRS})
            string(APPEND JSON_DATA "\"${INCLUDE_DIR}\",")
        endforeach()
        string(REGEX REPLACE ",$" "" JSON_DATA ${JSON_DATA})
        string(APPEND JSON_DATA "]")
    endif()

    string(APPEND JSON_DATA "}")

    # final parsing
    string(REPLACE " " "" JSON_DATA ${JSON_DATA})
    string(REPLACE "\"" "\\\"" JSON_DATA ${JSON_DATA})
    set(JSON_DATA "\"${JSON_DATA}\"")

    add_custom_target(
        ${TARGET_NAME}
        COMMAND ${CMAKE_BINARY_DIR}/$<TARGET_FILE:shader_compiler> --json-schema=${JSON_DATA}
        DEPENDS shader_compiler
        COMMENT "Generating shader variants for ${TARGET_NAME}"
    )

endfunction()