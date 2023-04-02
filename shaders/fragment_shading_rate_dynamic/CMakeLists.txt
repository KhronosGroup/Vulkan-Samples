vkb__register_shader(
    NAME "dynamic_scene_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/scene.vert
)

vkb__register_shader(
    NAME "dynamic_scene_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/scene.frag
)

vkb__register_shader(
    NAME "generate_shading_rate"
    LANGUAGE GLSL
    SHADER_TYPE Compute
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/generate_shading_rate.comp
)
