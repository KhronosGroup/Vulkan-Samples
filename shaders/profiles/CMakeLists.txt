vkb__register_shader(
    NAME "profiles_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/profiles.vert
)

vkb__register_shader(
    NAME "profiles_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/profiles.frag
)
