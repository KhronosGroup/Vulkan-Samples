vkb__register_shader(
    NAME "specialization_constants_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/specialization_constants.frag
    VARIANT_DEFINES
        "HAS_BASE_COLOR_TEXTURE"
    INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/shaders"
    DEFINES
        "MAX_LIGHT_COUNT=${MAX_LIGHT_COUNT}"
)

vkb__register_shader(
    NAME "UBOs_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/UBOs.frag
    VARIANT_DEFINES
        "HAS_BASE_COLOR_TEXTURE"
    INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/shaders"
    DEFINES
        "MAX_LIGHT_COUNT=${MAX_LIGHT_COUNT}"
)