vkb__register_shader(
    NAME "compute_buffer_fp16"
    LANGUAGE GLSL
    SHADER_TYPE Compute
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/compute_buffer_fp16.comp
    VARIANT_DEFINES
        "PUSH_CONSTANT_16=1"
)

vkb__register_shader(
    NAME "compute_buffer"
    LANGUAGE GLSL
    SHADER_TYPE Compute
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/compute_buffer.comp
)

vkb__register_shader(
    NAME "visualize_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/visualize.vert
)

vkb__register_shader(
    NAME "visualize_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/visualize.frag
)