vkb__register_shader(
    NAME "ray_tracing_miss"
    LANGUAGE GLSL
    SHADER_TYPE Miss
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/miss.rmiss
)

vkb__register_shader(
    NAME "ray_tracing_hit"
    LANGUAGE GLSL
    SHADER_TYPE ClosestHit
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/closesthit.rchit
)

vkb__register_shader(
    NAME "ray_tracing_gen"
    LANGUAGE GLSL
    SHADER_TYPE RayGeneration
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/raygen.rgen
)
