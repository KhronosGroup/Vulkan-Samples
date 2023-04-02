vkb__register_shader(
    NAME "buffer_device_address_render_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/render.vert
)

vkb__register_shader(
    NAME "buffer_device_address_render_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/render.frag
)

vkb__register_shader(
    NAME "update_vbo"
    LANGUAGE GLSL
    SHADER_TYPE Compute
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/update_vbo.comp
)