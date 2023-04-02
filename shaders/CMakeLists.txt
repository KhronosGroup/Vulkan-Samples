set(MAX_LIGHT_COUNT 20)
set(MAX_FORWARD_LIGHT_COUNT 16)

set(DIRECTIONAL_LIGHT 0.0)
set(POINT_LIGHT 1.0)

add_subdirectory(16bit_arithmetic)
add_subdirectory(descriptor_indexing)
add_subdirectory(khr_ray_tracing_extended)
add_subdirectory(shadows)
add_subdirectory(16bit_storage_input_output)
add_subdirectory(dynamic_rendering)
add_subdirectory(multi_draw_indirect)
add_subdirectory(specialization_constants)
add_subdirectory(async_compute)
add_subdirectory(dynamic_uniform_buffers)
# add_subdirectory(open_cl_interop) OpenCL Shaders are not supported
add_subdirectory(synchronization_2)
add_subdirectory(buffer_device_address)
add_subdirectory(fragment_shading_rate)
add_subdirectory(postprocessing)
add_subdirectory(terrain_tessellation)
add_subdirectory(compute_nbody)
add_subdirectory(fragment_shading_rate_dynamic)
add_subdirectory(profiles)
add_subdirectory(texture_compression_basisu)
add_subdirectory(conservative_rasterization)
add_subdirectory(hdr)
add_subdirectory(push_descriptors)
add_subdirectory(texture_loading)
add_subdirectory(constant_data)
add_subdirectory(hlsl_shaders)
add_subdirectory(ray_queries)
add_subdirectory(texture_mipmap_generation)
add_subdirectory(debug_utils)
add_subdirectory(instancing)
add_subdirectory(ray_tracing_reflection)
add_subdirectory(timeline_semaphore)
add_subdirectory(deferred)
add_subdirectory(khr_ray_tracing_basic)
add_subdirectory(separate_image_sampler)
add_subdirectory(uioverlay)

vkb__register_shader(
    NAME "base_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/base.vert
)

vkb__register_shader(
    NAME "base_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/base.frag
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
      "MAX_LIGHT_COUNT=${MAX_LIGHT_COUNT}"
    VARIANT_DEFINES
      "HAS_BASE_COLOR_TEXTURE"
)

vkb__register_shader(
    NAME "imgui_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/imgui.vert
)

vkb__register_shader(
    NAME "imgui_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/imgui.frag
)

vkb__register_shader(
    NAME "pbr_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/pbr.vert
    DEFINES
      "MAX_LIGHT_COUNT=${MAX_LIGHT_COUNT}"
      "MAX_FORWARD_LIGHT_COUNT=${MAX_FORWARD_LIGHT_COUNT}"
      "DIRECTIONAL_LIGHT=${DIRECTIONAL_LIGHT}"
      "POINT_LIGHT=${POINT_LIGHT}"
)

vkb__register_shader(
    NAME "pbr_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/pbr.frag
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
      "MAX_LIGHT_COUNT=${MAX_LIGHT_COUNT}"
      "MAX_FORWARD_LIGHT_COUNT=${MAX_FORWARD_LIGHT_COUNT}"
      "DIRECTIONAL_LIGHT=${DIRECTIONAL_LIGHT}"
      "POINT_LIGHT=${POINT_LIGHT}"
    VARIANT_DEFINES
      "HAS_BASE_COLOR_TEXTURE"
      "HAS_NORMAL_TEXTURE"
      "HAS_METALLIC_ROUGHNESS_TEXTURE"
)

vkb__register_shader(
    NAME "triangle_vertex"
    LANGUAGE GLSL
    SHADER_TYPE Vertex
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/triangle.vert
)

vkb__register_shader(
    NAME "triangle_fragment"
    LANGUAGE GLSL
    SHADER_TYPE Fragment
    SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/triangle.frag
)
