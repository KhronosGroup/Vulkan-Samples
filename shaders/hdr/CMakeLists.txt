vkb__register_shader(
    NAME "hdr_bloom_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bloom.vert
)

vkb__register_shader(
    NAME "hdr_bloom_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bloom.frag
)

vkb__register_shader(
    NAME "hdr_composition_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/composition.vert
)

vkb__register_shader(
    NAME "hdr_composition_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/composition.frag
)

vkb__register_shader(
    NAME "hdr_debug_utils_gbuffer_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/gbuffer.vert
)

vkb__register_shader(
    NAME "hdr_debug_utils_gbuffer_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/gbuffer.frag
)