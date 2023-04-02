vkb__register_shader(
    NAME "dynamic_uniform_buffers_base_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/base.vert
)

vkb__register_shader(
    NAME "dynamic_uniform_buffers_base_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/base.frag
)