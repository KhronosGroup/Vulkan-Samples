vkb__register_shader(
    NAME "texture_mipmap_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/texture.vert
)

vkb__register_shader(
    NAME "texture_mipmap_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/texture.frag
)
