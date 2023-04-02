vkb__register_shader(
    NAME "particle_integrate_compute"
    LANGUAGE GLSL
    SHADER_TYPE Compute
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/particle_integrate.comp
)

vkb__register_shader(
    NAME "particle_calculate_compute"
    LANGUAGE GLSL
    SHADER_TYPE Compute
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/particle_calculate.comp
)


vkb__register_shader(
    NAME "synchronization_2_particle_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/particle.vert
)

vkb__register_shader(
    NAME "synchronization_2_particle_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/particle.frag
)
