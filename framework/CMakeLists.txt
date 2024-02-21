# Copyright (c) 2019-2024, Arm Limited and Contributors
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 the "License";
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.16)

project(framework LANGUAGES C CXX)

set(FRAMEWORK_FILES
    # Header Files
    gui.h
    glsl_compiler.h
    spirv_reflection.h
    gltf_loader.h
    buffer_pool.h
    debug_info.h
    fence_pool.h
    heightmap.h
    semaphore_pool.h
    resource_binding_state.h
    resource_cache.h
    resource_record.h
    resource_replay.h
    vulkan_sample.h
    api_vulkan_sample.h
    timer.h
    camera.h
    hpp_api_vulkan_sample.h
    hpp_buffer_pool.h
    hpp_fence_pool.h
    hpp_glsl_compiler.h
    hpp_gltf_loader.h
    hpp_gui.h
    hpp_resource_binding_state.h
    hpp_resource_cache.h
    hpp_resource_record.h
    hpp_resource_replay.h
    hpp_semaphore_pool.h
    hpp_vulkan_sample.h
    # Source Files
    gui.cpp
    glsl_compiler.cpp
    spirv_reflection.cpp
    gltf_loader.cpp
    debug_info.cpp
    buffer_pool.cpp
    fence_pool.cpp
    heightmap.cpp
    semaphore_pool.cpp
    resource_binding_state.cpp
    resource_cache.cpp
    resource_record.cpp
    resource_replay.cpp
    vulkan_sample.cpp
    api_vulkan_sample.cpp
    timer.cpp
    camera.cpp
    hpp_gui.cpp
    hpp_api_vulkan_sample.cpp
    hpp_resource_cache.cpp
	hpp_vulkan_sample.cpp)

set(COMMON_FILES
    # Header Files
    common/ktx_common.h
    common/vk_common.h
    common/vk_initializers.h
    common/glm_common.h 
    common/resource_caching.h
    common/logging.h
    common/helpers.h
    common/error.h
    common/utils.h
    common/strings.h
    common/tags.h
    common/hpp_error.h
    common/hpp_resource_caching.h
    common/hpp_strings.h
    common/hpp_utils.h
    common/hpp_vk_common.h
    # Source Files
    common/error.cpp
    common/ktx_common.cpp
    common/vk_common.cpp
    common/utils.cpp
    common/strings.cpp)

set(GEOMETRY_FILES
    # Header Files
    geometry/frustum.h
    # Source Files
    geometry/frustum.cpp)

set(RENDERING_FILES
    # Header files
    rendering/pipeline_state.h
    rendering/postprocessing_pipeline.h
    rendering/postprocessing_pass.h
    rendering/postprocessing_renderpass.h
    rendering/postprocessing_computepass.h
    rendering/render_context.h
    rendering/render_frame.h
    rendering/render_pipeline.h
    rendering/render_target.h
    rendering/subpass.h
    rendering/hpp_pipeline_state.h
    rendering/hpp_render_context.h
    rendering/hpp_render_frame.h
    rendering/hpp_render_pipeline.h
    rendering/hpp_render_target.h
    rendering/hpp_subpass.h
    # Source files
    rendering/pipeline_state.cpp
    rendering/postprocessing_pipeline.cpp
    rendering/postprocessing_pass.cpp
    rendering/postprocessing_renderpass.cpp
    rendering/postprocessing_computepass.cpp
    rendering/render_context.cpp
    rendering/render_frame.cpp
    rendering/render_pipeline.cpp
    rendering/render_target.cpp
    rendering/subpass.cpp
    rendering/hpp_render_context.cpp
    rendering/hpp_render_frame.cpp
    rendering/hpp_render_target.cpp)

set(RENDERING_SUBPASSES_FILES
    # Header files
    rendering/subpasses/forward_subpass.h
    rendering/subpasses/lighting_subpass.h
    rendering/subpasses/geometry_subpass.h
    rendering/subpasses/hpp_forward_subpass.h
    # Source files
    rendering/subpasses/forward_subpass.cpp
    rendering/subpasses/lighting_subpass.cpp
    rendering/subpasses/geometry_subpass.cpp)

set(SCENE_GRAPH_FILES
    # Header Files
    scene_graph/component.h
    scene_graph/node.h
    scene_graph/scene.h
    scene_graph/script.h
    # Source Files
    scene_graph/component.cpp
    scene_graph/node.cpp
    scene_graph/scene.cpp
    scene_graph/script.cpp)

set(SCENE_GRAPH_COMPONENT_FILES
    # Header Files
    scene_graph/components/aabb.h
    scene_graph/components/camera.h
    scene_graph/components/perspective_camera.h
    scene_graph/components/orthographic_camera.h
    scene_graph/components/image.h
    scene_graph/components/light.h
    scene_graph/components/material.h
    scene_graph/components/mesh.h
    scene_graph/components/pbr_material.h
    scene_graph/components/sampler.h
    scene_graph/components/sub_mesh.h
    scene_graph/components/texture.h
    scene_graph/components/transform.h
    scene_graph/components/image/astc.h
    scene_graph/components/image/ktx.h
    scene_graph/components/image/stb.h
    scene_graph/components/hpp_image.h
    scene_graph/components/hpp_sub_mesh.h
    # Source Files
    scene_graph/components/aabb.cpp
    scene_graph/components/camera.cpp
    scene_graph/components/perspective_camera.cpp
    scene_graph/components/orthographic_camera.cpp
    scene_graph/components/image.cpp
    scene_graph/components/light.cpp
    scene_graph/components/material.cpp
    scene_graph/components/mesh.cpp
    scene_graph/components/pbr_material.cpp
    scene_graph/components/sampler.cpp
    scene_graph/components/sub_mesh.cpp
    scene_graph/components/texture.cpp
    scene_graph/components/transform.cpp
    scene_graph/components/image/astc.cpp
    scene_graph/components/image/ktx.cpp
    scene_graph/components/image/stb.cpp
    scene_graph/components/hpp_image.cpp)

set(SCENE_GRAPH_SCRIPTS_FILES
    # Header Files
    scene_graph/scripts/free_camera.h
    scene_graph/scripts/node_animation.h
    scene_graph/scripts/animation.h
    # Source Files
    scene_graph/scripts/free_camera.cpp
    scene_graph/scripts/node_animation.cpp
    scene_graph/scripts/animation.cpp)

set(STATS_FILES
    # Header Files
    stats/stats.h
    stats/stats_common.h
    stats/stats_provider.h
    stats/frame_time_stats_provider.h
    stats/vulkan_stats_provider.h
    stats/hpp_stats.h

    # Source Files
    stats/stats.cpp
    stats/stats_provider.cpp
    stats/frame_time_stats_provider.cpp
    stats/vulkan_stats_provider.cpp)

set(CORE_FILES
    # Header Files
    core/instance.h
    core/physical_device.h
    core/device.h
    core/debug.h
    core/shader_module.h
    core/pipeline_layout.h
    core/pipeline.h
    core/descriptor_set_layout.h
    core/descriptor_pool.h
    core/descriptor_set.h
    core/queue.h
    core/command_pool.h
    core/swapchain.h
    core/command_buffer.h
    core/buffer.h
    core/image.h
    core/image_view.h
    core/sampled_image.h
    core/instance.h
    core/sampler.h
    core/framebuffer.h
    core/render_pass.h
    core/query_pool.h
    core/scratch_buffer.h
    core/acceleration_structure.h
    core/shader_binding_table.h
    core/hpp_buffer.h
    core/hpp_command_buffer.h
    core/hpp_command_pool.h
    core/hpp_debug.h
    core/hpp_descriptor_pool.h
    core/hpp_descriptor_set.h
    core/hpp_descriptor_set_layout.h
    core/hpp_device.h
    core/hpp_framebuffer.h
    core/hpp_image.h
    core/hpp_image_view.h
    core/hpp_instance.h
    core/hpp_physical_device.h
    core/hpp_pipeline.h
    core/hpp_pipeline_layout.h
    core/hpp_query_pool.h
    core/hpp_queue.h
	core/hpp_render_pass.h
    core/hpp_sampler.h
    core/hpp_shader_module.h
    core/hpp_swapchain.h
    core/hpp_vulkan_resource.h
    core/vulkan_resource.h
    # Source Files
    core/instance.cpp
    core/physical_device.cpp
    core/device.cpp
    core/debug.cpp
    core/image.cpp
    core/shader_module.cpp
    core/pipeline_layout.cpp
    core/pipeline.cpp
    core/descriptor_set_layout.cpp
    core/descriptor_pool.cpp
    core/descriptor_set.cpp
    core/queue.cpp
    core/command_pool.cpp
    core/swapchain.cpp
    core/command_buffer.cpp
    core/buffer.cpp
    core/image.cpp
    core/image_view.cpp
    core/sampled_image.cpp
    core/instance.cpp
    core/sampler.cpp
    core/framebuffer.cpp
    core/render_pass.cpp
    core/query_pool.cpp
    core/scratch_buffer.cpp
    core/acceleration_structure.cpp
    core/shader_binding_table.cpp
    core/vulkan_resource.cpp
    core/hpp_buffer.cpp
    core/hpp_command_buffer.cpp
    core/hpp_command_pool.cpp
    core/hpp_debug.cpp
    core/hpp_device.cpp
    core/hpp_image.cpp
    core/hpp_image_view.cpp
    core/hpp_instance.cpp
    core/hpp_physical_device.cpp
    core/hpp_pipeline_layout.cpp
    core/hpp_queue.cpp
    core/hpp_sampler.cpp
    core/hpp_swapchain.cpp
)

set(PLATFORM_FILES
    # Header Files
    platform/application.h
    platform/platform.h
    platform/window.h
    platform/filesystem.h
    platform/input_events.h
    platform/configuration.h
    platform/parser.h
    platform/headless_window.h
    platform/plugins/plugin.h
    platform/plugins/plugin_base.h

    # Source Files
    platform/headless_window.cpp
    platform/application.cpp
    platform/platform.cpp
    platform/window.cpp
    platform/filesystem.cpp
    platform/input_events.cpp
    platform/configuration.cpp
    platform/parser.cpp
    platform/plugins/plugin.cpp)

set(GLFW_FILES
    # Header Files
    platform/glfw_window.h
    # Source Files
    platform/glfw_window.cpp
)

set(PARSER_FILES
    # Header Files
    platform/parsers/CLI11.h
    platform/parsers/help_formatter.h
    # Source Files
    platform/parsers/CLI11.cpp
    platform/parsers/help_formatter.cpp
)

set(ANDROID_FILES
    # Header Files
    platform/android/android_platform.h
    platform/android/android_window.h
    stats/hwcpipe_stats_provider.h
    # Source Files
    platform/android/android_platform.cpp
    platform/android/android_window.cpp
    stats/hwcpipe_stats_provider.cpp)

set(WINDOWS_FILES
    # Header Files
    platform/windows/windows_platform.h
    # Source Files
    platform/windows/windows_platform.cpp)

set(UNIX_FILES
    # Header Files
    platform/unix/unix_platform.h
    # Source Files
    platform/unix/unix_platform.cpp)

set(LINUX_D2D_FILES
    # Header Files
    platform/unix/unix_d2d_platform.h
    platform/unix/direct_window.h
    # Source Files
    platform/unix/unix_d2d_platform.cpp
    platform/unix/direct_window.cpp) 

source_group("\\" FILES ${FRAMEWORK_FILES})
source_group("common\\" FILES ${COMMON_FILES})
source_group("platform\\" FILES ${PLATFORM_FILES})
source_group("platform\\" FILES ${GLFW_FILES})
source_group("platform\\parsers" FILES ${PARSER_FILES})
source_group("platform\\windows" FILES ${WINDOWS_FILES})
source_group("platform\\android" FILES ${ANDROID_FILES})
source_group("platform\\unix" FILES ${UNIX_FILES})
source_group("platform\\unix" FILES ${LINUX_D2D_FILES})
source_group("core\\" FILES ${CORE_FILES})
source_group("geometry\\" FILES ${GEOMETRY_FILES})
source_group("rendering\\" FILES ${RENDERING_FILES})
source_group("rendering\\subpasses" FILES ${RENDERING_SUBPASSES_FILES})
source_group("scene_graph\\" FILES ${SCENE_GRAPH_FILES})
source_group("scene_graph\\components\\" FILES ${SCENE_GRAPH_COMPONENT_FILES})
source_group("scene_graph\\scripts\\" FILES ${SCENE_GRAPH_SCRIPTS_FILES})
source_group("stats\\" FILES ${STATS_FILES})

set(PROJECT_FILES
    ${PLATFORM_FILES}
    ${PARSER_FILES}
    ${COMMON_FILES}
    ${FRAMEWORK_FILES}
    ${CORE_FILES}
    ${GEOMETRY_FILES}
    ${RENDERING_FILES}
    ${RENDERING_SUBPASSES_FILES}
    ${SCENE_GRAPH_FILES}
    ${SCENE_GRAPH_COMPONENT_FILES}
    ${SCENE_GRAPH_SCRIPTS_FILES}
    ${STATS_FILES})

# No need for explicit casts from vk::HandleType to VkHandleType on ANDROID
if(ANDROID)
    add_definitions(-DVULKAN_HPP_TYPESAFE_CONVERSION=1)
endif()

# Add files based on platform
if(ANDROID)
    list(APPEND PROJECT_FILES ${ANDROID_FILES})
    message(STATUS "Android platform detected")
else()
    if(DIRECT_TO_DISPLAY)
        list(APPEND PROJECT_FILES ${LINUX_D2D_FILES})
        message(STATUS "Unix d2d platform detected")
    else()
        list(APPEND PROJECT_FILES ${GLFW_FILES})
        if(WIN32)
            list(APPEND PROJECT_FILES ${WINDOWS_FILES})
            message(STATUS "Windows platform detected")
        else()
            list(APPEND PROJECT_FILES ${UNIX_FILES})
            message(STATUS "Unix platform detected")
        endif()
    endif()
endif()

# mask out the min/max macros from minwindef.h
if(MSVC)
    add_definitions(-DNOMINMAX)
endif()

#NB: switch this to shared library and things stop working. (there is likely two copies of volk somewhere.
add_library(${PROJECT_NAME} OBJECT ${PROJECT_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

# compiler flags based on compiler type
if(NOT MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC -fexceptions)
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC /MP)
endif()

if(${VKB_VALIDATION_LAYERS})
    target_compile_definitions(${PROJECT_NAME} PUBLIC VKB_VALIDATION_LAYERS)
endif()

# GPU assisted validation layers are not available on macOS.
if(${VKB_VALIDATION_LAYERS_GPU_ASSISTED})
    if (APPLE)
        message(WARNING "GPU assisted validation layers are not currently available on macOS.")
    else()
        target_compile_definitions(${PROJECT_NAME} PUBLIC VKB_VALIDATION_LAYERS)
        target_compile_definitions(${PROJECT_NAME} PUBLIC VKB_VALIDATION_LAYERS_GPU_ASSISTED)
    endif()
endif()

if(${VKB_VALIDATION_LAYERS_BEST_PRACTICES})
    target_compile_definitions(${PROJECT_NAME} PUBLIC VKB_VALIDATION_LAYERS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC VKB_VALIDATION_LAYERS_BEST_PRACTICES)
endif()

if(${VKB_VALIDATION_LAYERS_SYNCHRONIZATION})
    target_compile_definitions(${PROJECT_NAME} PUBLIC VKB_VALIDATION_LAYERS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC VKB_VALIDATION_LAYERS_SYNCHRONIZATION)
endif()

if(${VKB_VULKAN_DEBUG})
    target_compile_definitions(${PROJECT_NAME} PUBLIC VKB_VULKAN_DEBUG)
endif()

if(${VKB_ENABLE_PORTABILITY})
    message(STATUS "Vulkan Portability extension is enabled")
    target_compile_definitions(${PROJECT_NAME} PUBLIC VKB_ENABLE_PORTABILITY)
endif()

if(${VKB_WARNINGS_AS_ERRORS})
    message(STATUS "Warnings as Errors Enabled")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX)
    endif()
endif()

target_include_directories(${PROJECT_NAME} PUBLIC $<TARGET_PROPERTY:plugins,INCLUDE_DIRECTORIES> ${CMAKE_CURRENT_SOURCE_DIR})

# Link third party libraries
target_link_libraries(${PROJECT_NAME} PUBLIC
    vkb__core
    volk
    ktx
    stb
    astc
    imgui
    tinygltf
    glm
    glslang
    SPIRV
    vma
    spirv-cross-glsl
    glslang-default-resource-limits
    spdlog
    ctpl
    CLI11::CLI11
    plugins)

if(${NEED_LINK_ATOMIC})
    target_link_libraries(${PROJECT_NAME} PUBLIC atomic)
endif()

# Link platform specific libraries
if(ANDROID)
    # Import game-activity static lib inside the game-activity_static prefab module.
    find_package(game-activity REQUIRED CONFIG)
    target_link_libraries(${PROJECT_NAME} PUBLIC hwcpipe log android game-activity::game-activity_static)
else()
    if (DIRECT_TO_DISPLAY)
        target_link_libraries(${PROJECT_NAME} PRIVATE dl)
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
    endif()
endif()

if(VKB_DO_CLANG_TIDY)
    set_target_properties(framework PROPERTIES CXX_CLANG_TIDY "${VKB_DO_CLANG_TIDY}")
endif()