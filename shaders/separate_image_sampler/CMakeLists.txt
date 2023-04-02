vkb__register_shader(
    NAME "separate_image_sampler_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/separate_image_sampler.vert
)

vkb__register_shader(
    NAME "separate_image_sampler_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/separate_image_sampler.frag
)
