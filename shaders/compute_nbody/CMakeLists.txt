vkb__register_shader(
    NAME "particle_calculate"
    LANGUAGE GLSL
    SHADER_TYPE Compute
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/particle_calculate.comp
)

vkb__register_shader(
    NAME "particle_integrate"
    LANGUAGE GLSL
    SHADER_TYPE Compute
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/particle_integrate.comp
)

vkb__register_shader(
    NAME "particle_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/particle.vert
)

vkb__register_shader(
    NAME "particle_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/particle.frag
)