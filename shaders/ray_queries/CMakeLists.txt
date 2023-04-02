vkb__register_shader(
    NAME "ray_shadow_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/ray_shadow.vert
)

vkb__register_shader(
    NAME "ray_shadow_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/ray_shadow.frag
)
