vkb__register_shader(
    NAME "outline_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/outline.frag
    VARIANT_DEFINES
        "MS_DEPTH"
        "OUTLINE_ONLY"
)

vkb__register_shader(
    NAME "post_processing_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/postprocessing.vert
)
