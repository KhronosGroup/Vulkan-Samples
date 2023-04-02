vkb__register_shader(
    NAME "geometry_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/geometry.vert
)

vkb__register_shader(
    NAME "geometry_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/geometry.frag
    VARIANT_DEFINES
        "HAS_BASE_COLOR_TEXTURE"
)

vkb__register_shader(
    NAME "lighting_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lighting.vert
)

vkb__register_shader(
    NAME "lighting_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lighting.frag
    INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/shaders"
    DEFINES
        "MAX_LIGHT_COUNT=${MAX_LIGHT_COUNT}"
)