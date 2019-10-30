# Copyright (c) 2019, Arm Limited and Contributors
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

cmake_minimum_required(VERSION 3.10)

project(framework LANGUAGES C CXX)

set(FRAMEWORK_FILES
    # Header Files
    gui.h
    stats.h
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
    # Source Files
    gui.cpp
    stats.cpp
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
    camera.cpp)

set(COMMON_FILES
    # Header Files
    common/vk_common.h
    common/vk_initializers.h
    common/logging.h
    common/helpers.h
    common/error.h
    common/utils.h
    # Source Files
    common/vk_common.cpp
    common/error.cpp
    common/utils.cpp)

set(GEOMETRY_FILES
    # Header Files
    geometry/frustum.h
    # Source Files
    geometry/frustum.cpp)

set(RENDERING_FILES
    # Header files
    rendering/pipeline_state.h
    rendering/render_context.h
    rendering/render_frame.h
    rendering/render_pipeline.h
    rendering/render_target.h
    rendering/subpass.h
    # Source files
    rendering/pipeline_state.cpp
    rendering/render_context.cpp
    rendering/render_frame.cpp
    rendering/render_pipeline.cpp
    rendering/render_target.cpp
    rendering/subpass.cpp)

set(RENDERING_SUBPASSES_FILES
    # Header files
    rendering/subpasses/scene_subpass.h
    rendering/subpasses/lighting_subpass.h
    # Source files
    rendering/subpasses/scene_subpass.cpp
    rendering/subpasses/lighting_subpass.cpp)

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
    scene_graph/components/image.h
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
    # Source Files
    scene_graph/components/aabb.cpp
    scene_graph/components/camera.cpp
    scene_graph/components/perspective_camera.cpp
    scene_graph/components/image.cpp
    scene_graph/components/material.cpp
    scene_graph/components/mesh.cpp
    scene_graph/components/pbr_material.cpp
    scene_graph/components/sampler.cpp
    scene_graph/components/sub_mesh.cpp
    scene_graph/components/texture.cpp
    scene_graph/components/transform.cpp
    scene_graph/components/image/astc.cpp
    scene_graph/components/image/ktx.cpp
    scene_graph/components/image/stb.cpp)

set(SCENE_GRAPH_SCRIPTS_FILES
    # Header Files
    scene_graph/scripts/free_camera.h
    scene_graph/scripts/node_animation.h
    # Source Files
    scene_graph/scripts/free_camera.cpp
    scene_graph/scripts/node_animation.cpp)

set(CORE_FILES
    # Header Files
    core/device.h
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
    core/sampler.h
    core/framebuffer.h
    core/render_pass.h
    # Source Files
    core/device.cpp
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
    core/sampler.cpp
    core/framebuffer.cpp
    core/render_pass.cpp)

set(PLATFORM_FILES
    # Header Files
    platform/application.h
    platform/options.h
    platform/platform.h
    platform/filesystem.h
    platform/input_events.h
    platform/configuration.h
    # Source Files
    platform/application.cpp
    platform/options.cpp
    platform/platform.cpp
    platform/filesystem.cpp
    platform/input_events.cpp
    platform/configuration.cpp)

# Add files based on platform
if(ANDROID)
    list(APPEND PLATFORM_FILES
        # Header Files
        platform/android/android_platform.h
        # Source Files
        platform/android/android_platform.cpp)
elseif(DIRECT_TO_DISPLAY)
    list(APPEND PLATFORM_FILES
        # Header Files
        platform/linux/linux_d2d_platform.h
        # Source Files
        platform/linux/linux_d2d_platform.cpp)
else()
    list(APPEND PLATFORM_FILES
        # Header Files
        platform/glfw_platform.h
        # Source Files
        platform/glfw_platform.cpp)

    if(WIN32)
        list(APPEND PLATFORM_FILES
            # Header Files
            platform/windows/windows_platform.h
            # Source Files
            platform/windows/windows_platform.cpp)
    else()
        list(APPEND PLATFORM_FILES
            # Header Files
            platform/linux/linux_platform.h
            # Source Files
            platform/linux/linux_platform.cpp)
    endif()
endif()

source_group("\\" FILES ${FRAMEWORK_FILES})
source_group("common\\" FILES ${COMMON_FILES})
source_group("platform\\" FILES ${PLATFORM_FILES})
source_group("core\\" FILES ${CORE_FILES})
source_group("geometry\\" FILES ${GEOMETRY_FILES})
source_group("rendering\\" FILES ${RENDERING_FILES})
source_group("rendering\\subpasses" FILES ${RENDERING_SUBPASSES_FILES})
source_group("scene_graph\\" FILES ${SCENE_GRAPH_FILES})
source_group("scene_graph\\components\\" FILES ${SCENE_GRAPH_COMPONENT_FILES})
source_group("scene_graph\\scripts\\" FILES ${SCENE_GRAPH_SCRIPTS_FILES})

set(PROJECT_FILES
    ${PLATFORM_FILES}
    ${COMMON_FILES}
    ${FRAMEWORK_FILES}
    ${CORE_FILES}
    ${GEOMETRY_FILES}
    ${RENDERING_FILES}
    ${RENDERING_SUBPASSES_FILES}
    ${SCENE_GRAPH_FILES}
    ${SCENE_GRAPH_COMPONENT_FILES}
    ${SCENE_GRAPH_SCRIPTS_FILES})

vulkan_samples_pch(PROJECT_FILES pch.cpp)

if(MSVC)
  # Enable parallel build
  add_definitions(/MP)
endif()

add_library(${PROJECT_NAME} STATIC ${PROJECT_FILES})

# compiler flags based on compiler type
if(NOT MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC -fexceptions)
endif()

if(${VKB_VALIDATION_LAYERS})
    target_compile_definitions(${PROJECT_NAME} PUBLIC VKB_VALIDATION_LAYERS)
endif()

if(${VKB_WARNINGS_AS_ERRORS})
    message(STATUS "Warnings as Errors Enabled")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX)
    endif()
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link third party libraries
target_link_libraries(${PROJECT_NAME}
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
    hwcpipe
    spirv-cross-glsl
    glslang-default-resource-limits
    spdlog
    ctpl
    docopt)

# Link platform specific libraries
if(ANDROID)
    target_link_libraries(${PROJECT_NAME} log android native_app_glue)
else()
    if (DIRECT_TO_DISPLAY)
        target_link_libraries(${PROJECT_NAME} dl)
    else()
        target_link_libraries(${PROJECT_NAME} glfw)
    endif()
endif()
