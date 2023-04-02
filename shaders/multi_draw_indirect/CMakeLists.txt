vkb__register_shader(
    NAME "cull_address_compute"
    LANGUAGE GLSL
    SHADER_TYPE Compute
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cull_address.comp
)

vkb__register_shader(
    NAME "cull_comp"
    LANGUAGE GLSL
    SHADER_TYPE Compute
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cull.comp
)

vkb__register_shader(
    NAME "multi_draw_indirect_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/multi_draw_indirect.vert
)

vkb__register_shader(
    NAME "multi_draw_indirect_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/multi_draw_indirect.frag
)
