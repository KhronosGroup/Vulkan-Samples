set(GLSL_SHADER_FILES
    uioverlay/uioverlay.frag
    uioverlay/uioverlay.vert
    dynamic_uniform_buffers/base.vert
    dynamic_uniform_buffers/base.frag
    hdr/bloom.vert
    hdr/composition.vert
    hdr/gbuffer.frag
    hdr/gbuffer.vert
    hdr/composition.frag
    hdr/bloom.frag
    triangle.frag
    multi_draw_indirect/cull.comp
    multi_draw_indirect/multi_draw_indirect.vert
    multi_draw_indirect/multi_draw_indirect.frag
    multi_draw_indirect/cull_address.comp
    graphics_pipeline_library/shared.vert
    graphics_pipeline_library/uber.frag
    ray_queries/ray_shadow.frag
    ray_queries/ray_shadow.vert
    khr_ray_tracing_extended/closesthit.rchit
    khr_ray_tracing_extended/raygen.rgen
    khr_ray_tracing_extended/miss.rmiss
    imgui.vert
    shadows/main.vert
    shadows/shadowmap.vert
    shadows/shadowmap.frag
    shadows/main.frag
    deferred/lighting.frag
    deferred/geometry.vert
    deferred/lighting.vert
    deferred/geometry.frag
    compute_nbody/particle.frag
    compute_nbody/particle_integrate.comp
    compute_nbody/particle_calculate.comp
    compute_nbody/particle.vert
    color_write_enable/triangle_separate_channels.vert
    color_write_enable/composition.vert
    color_write_enable/composition.frag
    color_write_enable/triangle_separate_channels.frag
    patch_control_points/tess.vert
    patch_control_points/tess.frag
    patch_control_points/tess.tese
    patch_control_points/tess.tesc
    terrain_tessellation/terrain.tesc
    terrain_tessellation/terrain.frag
    terrain_tessellation/skysphere.frag
    terrain_tessellation/terrain.tese
    terrain_tessellation/terrain.vert
    terrain_tessellation/skysphere.vert
    16bit_arithmetic/visualize.frag
    16bit_arithmetic/visualize.vert
    16bit_arithmetic/compute_buffer.comp
    16bit_arithmetic/compute_buffer_fp16.comp
    timeline_semaphore/game_of_life_mutate.comp
    timeline_semaphore/game_of_life_init.comp
    timeline_semaphore/render.frag
    timeline_semaphore/render.vert
    timeline_semaphore/game_of_life_update.comp
    synchronization_2/particle.frag
    synchronization_2/particle_integrate.comp
    synchronization_2/particle_calculate.comp
    synchronization_2/particle.vert
    dynamic_line_rasterization/grid.vert
    dynamic_line_rasterization/base.vert
    dynamic_line_rasterization/base.frag
    dynamic_line_rasterization/grid.frag
    conditional_rendering/model.frag
    conditional_rendering/model.vert
    fragment_shader_barycentric/object.vert
    fragment_shader_barycentric/skybox.vert
    fragment_shader_barycentric/object.frag
    fragment_shader_barycentric/skybox.frag
    pbr.frag
    debug_utils/bloom.vert
    debug_utils/composition.vert
    debug_utils/gbuffer.frag
    debug_utils/gbuffer.vert
    debug_utils/composition.frag
    debug_utils/bloom.frag
    extended_dynamic_state2/baseline.frag
    extended_dynamic_state2/baseline.vert
    extended_dynamic_state2/background.vert
    extended_dynamic_state2/background.frag
    extended_dynamic_state2/tess.vert
    extended_dynamic_state2/tess.frag
    extended_dynamic_state2/tess.tese
    extended_dynamic_state2/tess.tesc
    imgui.frag
    mesh_shader_culling/mesh_shader_culling.mesh
    mesh_shader_culling/mesh_shader_culling.frag
    mesh_shader_culling/mesh_shader_culling.task
    vertex_dynamic_state/gbuffer.frag
    vertex_dynamic_state/gbuffer.vert
    base.vert
    profiles/profiles.vert
    profiles/profiles.frag
    logic_op_dynamic_state/baseline.frag
    logic_op_dynamic_state/baseline.vert
    logic_op_dynamic_state/background.vert
    logic_op_dynamic_state/background.frag
    ray_tracing_reflection/closesthit.rchit
    ray_tracing_reflection/raygen.rgen
    ray_tracing_reflection/miss.rmiss
    ray_tracing_reflection/missShadow.rmiss
    mesh_shading/ms.mesh
    mesh_shading/ps.frag
    conservative_rasterization/triangle.frag
    conservative_rasterization/fullscreen.vert
    conservative_rasterization/triangleoverlay.frag
    conservative_rasterization/fullscreen.frag
    conservative_rasterization/triangle.vert
    buffer_device_address/render.frag
    buffer_device_address/update_vbo.comp
    buffer_device_address/render.vert
    constant_data/push_constant.vert
    constant_data/buffer_array.frag
    constant_data/push_constant.frag
    constant_data/ubo.frag
    constant_data/ubo.vert
    constant_data/buffer_array.vert
    descriptor_buffer_basic/cube.vert
    descriptor_buffer_basic/cube.frag
    fragment_shading_rate/scene.frag
    fragment_shading_rate/scene.vert
    push_descriptors/cube.vert
    push_descriptors/cube.frag
    texture_loading/texture.vert
    texture_loading/texture.frag
    gshader_to_mshader/gshader_to_mshader_base.frag
    gshader_to_mshader/gshader_to_mshader.geom
    gshader_to_mshader/gshader_to_mshader.frag
    gshader_to_mshader/gshader_to_mshader_mesh.frag
    gshader_to_mshader/gshader_to_mshader.vert
    gshader_to_mshader/gshader_to_mshader.mesh
    gshader_to_mshader/gshader_to_mshader_base.vert
    base.frag
    dynamic_rendering/gbuffer.frag
    dynamic_rendering/gbuffer.vert
    fragment_shading_rate_dynamic/scene.frag
    fragment_shading_rate_dynamic/scene.vert
    fragment_shading_rate_dynamic/generate_shading_rate.comp
    specialization_constants/specialization_constants.frag
    specialization_constants/UBOs.frag
    instancing/planet.frag
    instancing/instancing.frag
    instancing/starfield.frag
    instancing/instancing.vert
    instancing/planet.vert
    instancing/starfield.vert
    16bit_storage_input_output/16bit_storage_input_output_disabled.frag
    16bit_storage_input_output/16bit_storage_input_output_disabled.vert
    16bit_storage_input_output/16bit_storage_input_output_enabled.frag
    16bit_storage_input_output/16bit_storage_input_output_enabled.vert
    async_compute/threshold.comp
    async_compute/shadow.frag
    async_compute/forward.frag
    async_compute/composite.frag
    async_compute/forward.vert
    async_compute/shadow.vert
    async_compute/blur_down.comp
    async_compute/blur_up.comp
    async_compute/composite.vert
    texture_compression_basisu/texture.vert
    texture_compression_basisu/texture.frag
    descriptor_indexing/nonuniform-quads.vert
    descriptor_indexing/update-after-bind-quads.vert
    descriptor_indexing/update-after-bind-quads.frag
    descriptor_indexing/nonuniform-quads.frag
    texture_mipmap_generation/texture.vert
    texture_mipmap_generation/texture.frag
    khr_ray_tracing_basic/closesthit.rchit
    khr_ray_tracing_basic/raygen.rgen
    khr_ray_tracing_basic/miss.rmiss
    separate_image_sampler/separate_image_sampler.vert
    separate_image_sampler/separate_image_sampler.frag
    shader_object/material_scene.vert
    shader_object/material_texture.frag
    shader_object/basic_n_dot_l.frag
    shader_object/material_n_dot_l.frag
    shader_object/basic_uv.vert
    shader_object/terrain.frag
    shader_object/material_pass_through.geom
    shader_object/basic_uv.frag
    shader_object/basic_pos.frag
    shader_object/basic_pos.vert
    shader_object/material_scene.frag
    shader_object/material_rotates.vert
    shader_object/skybox.vert
    shader_object/post_process_invert.frag
    shader_object/material_wave_x.vert
    shader_object/post_process_FSQ.vert
    shader_object/post_process_quantize.frag
    shader_object/material_gen_normals.geom
    shader_object/basic_normals.vert
    shader_object/material_normals.frag
    shader_object/post_process_grayscale.frag
    shader_object/post_process_brighten.frag
    shader_object/basic_normals.frag
    shader_object/post_process_edge_detection.frag
    shader_object/material_wave_y.vert
    shader_object/basic_n_dot_l.vert
    shader_object/material_pass_sin_offset.geom
    shader_object/material_reflective.frag
    shader_object/skybox.frag
    shader_object/post_process_color_cycle.frag
    shader_object/material_wave_z.vert
    shader_object/terrain.vert
    postprocessing/postprocessing.vert
    postprocessing/outline.frag
    triangle.vert
    pbr.vert
)

list(TRANSFORM GLSL_SHADER_FILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)

foreach(SHADER_FILE ${GLSL_SHADER_FILES})
    string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/ "" SHADER_NAME ${SHADER_FILE})
    string(REPLACE "/" "_" SHADER_NAME ${SHADER_NAME})
    string(REPLACE "." "_" SHADER_NAME ${SHADER_NAME})
    string(REPLACE "\\" "_" SHADER_NAME ${SHADER_NAME})
    configure_shader(
        NAME ${SHADER_NAME}
        SOURCE ${SHADER_FILE}
        OUTPUT ${SHADER_NAME}.spv
        LANGUAGE glsl
    )
endforeach()

set (HLSL_SHADER_FILES
    hlsl_shaders/hlsl_shader.frag
    hlsl_shaders/hlsl_shader.vert
)

list(TRANSFORM HLSL_SHADER_FILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)

foreach(SHADER_FILE ${HLSL_SHADER_FILES})
    string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/ "" SHADER_NAME ${SHADER_FILE})
    string(REPLACE "/" "_" SHADER_NAME ${SHADER_NAME})
    string(REPLACE "." "_" SHADER_NAME ${SHADER_NAME})
    string(REPLACE "\\" "_" SHADER_NAME ${SHADER_NAME})
    # Not yet supported
    # configure_shader(
    #     NAME ${SHADER_NAME}
    #     SOURCE ${SHADER_FILE}
    #     OUTPUT ${SHADER_NAME}.spv
    #     LANGUAGE hlsl
    # )
endforeach()