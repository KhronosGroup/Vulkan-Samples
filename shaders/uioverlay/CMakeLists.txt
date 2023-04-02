vkb__register_shader(
    NAME "uioverlay_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/uioverlay.vert
)

vkb__register_shader(
    NAME "uioverlay_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/uioverlay.frag
)
