vkb__register_shader(
    NAME "render_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/render.vert
)

vkb__register_shader(
    NAME "render_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/render.frag
)

vkb__register_shader(
    NAME "game_of_life_init_compute"
    LANGUAGE GLSL
    SHADER_TYPE Compute
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/game_of_life_init.comp
)

vkb__register_shader(
    NAME "game_of_life_update_compute"
    LANGUAGE GLSL
    SHADER_TYPE Compute
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/game_of_life_update.comp
)

vkb__register_shader(
    NAME "game_of_life_mutate_compute"
    LANGUAGE GLSL
    SHADER_TYPE Compute
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/game_of_life_mutate.comp
)
