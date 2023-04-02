vkb__register_shader(
    NAME "gbuffer_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/gbuffer.vert
)

vkb__register_shader(
    NAME "gbuffer_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/gbuffer.frag
)