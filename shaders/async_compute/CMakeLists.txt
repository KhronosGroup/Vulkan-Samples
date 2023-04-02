vkb__register_shader(
    NAME "blur_down"
    LANGUAGE GLSL
    SHADER_TYPE Compute
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/blur_down.comp
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        "MAX_LIGHT_COUNT=${MAX_LIGHTS}"
)

vkb__register_shader(
    NAME "blur_up"
    LANGUAGE GLSL
    SHADER_TYPE Compute
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/blur_up.comp
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
)

vkb__register_shader(
    NAME "composite_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/composite.vert
)

vkb__register_shader(
    NAME "composite_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/composite.frag
)


vkb__register_shader(
    NAME "forward_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/forward.vert
    INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}/shaders
    VARIANT_DEFINES
        "HAS_BASE_COLOR_TEXTURE=1"
)

vkb__register_shader(
    NAME "forward_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/forward.frag
    INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}/shaders
    VARIANT_DEFINES
        "HAS_BASE_COLOR_TEXTURE=1"
)

vkb__register_shader(
    NAME "shadow_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/shadow.vert
)

vkb__register_shader(
    NAME "shadow_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/shadow.frag
)

vkb__register_shader(
    NAME "threshold"
    LANGUAGE GLSL
    SHADER_TYPE Compute
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/threshold.comp
)
