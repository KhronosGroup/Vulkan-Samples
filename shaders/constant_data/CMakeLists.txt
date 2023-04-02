vkb__register_shader(
    NAME "buffer_array_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/buffer_array.vert
    DEFINES
        "SCENE_MESH_COUNT=250"
    VARIANT_DEFINES
        "PUSH_CONSTANT_LIMIT_256"
)

vkb__register_shader(
    NAME "buffer_array_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/buffer_array.frag
    INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/shaders"
    DEFINES
        "MAX_LIGHT_COUNT=${MAX_LIGHT_COUNT}"
        "SCENE_MESH_COUNT=250"
    VARIANT_DEFINES
        "HAS_BASE_COLOR_TEXTURE"
        "PUSH_CONSTANT_LIMIT_256"
)

vkb__register_shader(
    NAME "push_constant_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/push_constant.vert
    VARIANT_DEFINES
        "PUSH_CONSTANT_LIMIT_256"
)

vkb__register_shader(
    NAME "push_constant_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/push_constant.frag
    INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/shaders"
    DEFINES
        "MAX_LIGHT_COUNT=${MAX_LIGHT_COUNT}"
    VARIANT_DEFINES
        "HAS_BASE_COLOR_TEXTURE"
        "PUSH_CONSTANT_LIMIT_256"
)

vkb__register_shader(
    NAME "ubo_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/ubo.vert
    VARIANT_DEFINES
        "PUSH_CONSTANT_LIMIT_256"
)

vkb__register_shader(
    NAME "ubo_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/ubo.frag
    INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/shaders"
    DEFINES
        "MAX_LIGHT_COUNT=${MAX_LIGHT_COUNT}"
    VARIANT_DEFINES
        "HAS_BASE_COLOR_TEXTURE"
        "PUSH_CONSTANT_LIMIT_256"
)