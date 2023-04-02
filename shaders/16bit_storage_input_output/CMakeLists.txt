vkb__register_shader(
    NAME "storage_input_output_disable_16bit_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/16bit_storage_input_output_disabled.vert
    INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}/shaders
    DEFINES
        "MAX_LIGHT_COUNT=${MAX_LIGHT_COUNT}"
)

vkb__register_shader(
    NAME "storage_input_output_disable_16bit_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/16bit_storage_input_output_disabled.frag
    INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}/shaders
    DEFINES
        "MAX_LIGHT_COUNT=${MAX_LIGHT_COUNT}"
)

vkb__register_shader(
    NAME "storage_input_output_enabled_16bit_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/16bit_storage_input_output_enabled.vert
    INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}/shaders
    DEFINES
        "MAX_LIGHT_COUNT=${MAX_LIGHT_COUNT}"
)

vkb__register_shader(
    NAME "storage_input_output_enabled_16bit_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/16bit_storage_input_output_enabled.frag
    INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}/shaders
    DEFINES
        "MAX_LIGHT_COUNT=${MAX_LIGHT_COUNT}"
)