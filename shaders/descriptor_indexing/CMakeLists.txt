vkb__register_shader(
    NAME "nonuniform_quads_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/nonuniform-quads.vert
)

vkb__register_shader(
    NAME "nonuniform_quads_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/nonuniform-quads.frag
)

vkb__register_shader(
    NAME "update_after_bind_quads_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/update-after-bind-quads.vert
)

vkb__register_shader(
    NAME "update_after_bind_quads_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/update-after-bind-quads.frag
)