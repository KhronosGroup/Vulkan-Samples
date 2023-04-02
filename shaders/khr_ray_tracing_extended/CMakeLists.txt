vkb__register_shader(
    NAME "ray_tracing_extended_miss"
    LANGUAGE GLSL
    SHADER_TYPE Miss
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/miss.rmiss
    INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/shaders"
)

vkb__register_shader(
    NAME "ray_tracing_extended_hit"
    LANGUAGE GLSL
    SHADER_TYPE ClosestHit
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/closesthit.rchit
    INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/shaders"
)

vkb__register_shader(
    NAME "ray_tracing_extended_gen"
    LANGUAGE GLSL
    SHADER_TYPE RayGeneration
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/raygen.rgen
    INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/shaders"
)
