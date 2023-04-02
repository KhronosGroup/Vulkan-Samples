vkb__register_shader(
    NAME "skysphere_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/skysphere.vert
)

vkb__register_shader(
    NAME "skysphere_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/skysphere.frag
)


vkb__register_shader(
    NAME "terrain_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/terrain.vert
)

vkb__register_shader(
    NAME "terrain_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/terrain.frag
)

vkb__register_shader(
    NAME "terrain_tess_control"
    LANGUAGE GLSL
    SHADER_TYPE TessellationControl
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/terrain.tesc
)

vkb__register_shader(
    NAME "terrain_frtess_evaluation"
    LANGUAGE GLSL
    SHADER_TYPE TessellationEvaluation
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/terrain.tese
)
