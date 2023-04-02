vkb__register_shader(
    NAME "shadows_main_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/main.vert
)

vkb__register_shader(
    NAME "shadows_main_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/main.frag
    VARIANT_DEFINES
        "HAS_BASE_COLOR_TEXTURE"
    INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/shaders"
    DEFINES
        "MAX_LIGHT_COUNT=${MAX_LIGHT_COUNT}"
)

vkb__register_shader(
    NAME "shadowmap_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/shadowmap.vert
)

vkb__register_shader(
    NAME "shadowmap_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/shadowmap.frag
)