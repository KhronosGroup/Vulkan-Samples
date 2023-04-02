vkb__register_shader(
    NAME "fullscreen_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/fullscreen.vert
)

vkb__register_shader(
    NAME "fullscreen_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/fullscreen.frag
)

vkb__register_shader(
    NAME "triangle_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/triangle.vert
)

vkb__register_shader(
    NAME "triangle_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/triangle.frag
)

vkb__register_shader(
    NAME "triangle_overlay_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/triangle.vert
)

vkb__register_shader(
    NAME "triangle_overlay_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/triangle_overlay.frag
)