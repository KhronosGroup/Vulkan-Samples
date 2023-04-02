vkb__register_shader(
    NAME "hlsl_shader_vertex"
    LANGUAGE HLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_shader.vert
)

vkb__register_shader(
    NAME "hlsl_shader_fragment"
    LANGUAGE HLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_shader.frag
)

