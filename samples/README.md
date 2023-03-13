<!--
- Copyright (c) 2020-2023, Arm Limited and Contributors
-
- SPDX-License-Identifier: Apache-2.0
-
- Licensed under the Apache License, Version 2.0 the "License";
- you may not use this file except in compliance with the License.
- You may obtain a copy of the License at
-
-     http://www.apache.org/licenses/LICENSE-2.0
-
- Unless required by applicable law or agreed to in writing, software
- distributed under the License is distributed on an "AS IS" BASIS,
- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
- See the License for the specific language governing permissions and
- limitations under the License.
-
-->

# Samples overview <!-- omit in toc -->

## Contents <!-- omit in toc -->

- [Introduction](#introduction)
- [Performance samples](#performance-samples)
- [API Samples](#api-samples)
- [Extension Samples](#extension-samples)
- [Tooling Samples](#tooling-samples)

## Introduction

This readme lists all Vulkan samples currently available in this repository. They are grouped into multiple categories. Many samples come with a tutorial, which can be found in their respective folders.

## Performance samples

The goal of these samples is to demonstrate how to use certain features and functions to achieve optimal performance. To visualize this, they also include real-time profiling information.

### [AFBC](./performance/afbc)<br/>
AFBC (Arm Frame Buffer Compression) is a real-time lossless compression algorithm found in Arm Mali GPUs, designed to tackle the ever-growing demand for higher resolution graphics. This format is applied to the framebuffers that are to be written to the GPU. This technology can offer bandwidth reductions of [up to 50%](https://developer.arm.com/technologies/graphics-technologies/arm-frame-buffer-compression).

### [Command buffer usage](./performance/command_buffer_usage)<br/>
This sample demonstrates how to use and manage secondary command buffers, and how to record them concurrently. Implementing multi-threaded recording of draw calls can help reduce CPU frame time.

### [Constant data](./performance/constant_data)<br/>
The Vulkan API exposes a few different ways in which we can send uniform data into our shaders. There are enough methods that it raises the question "Which one is fastest?", and more often than not the answer is "It depends". The main issue for developers is that the fastest methods may differ between the various vendors, so often there is no "one size fits all" solution. This sample aims to highlight this issue, and help move the Vulkan ecosystem to a point where we are better equipped to solve this for developers. This is done by having an interactive way to toggle different constant data methods that the Vulkan API expose to us. This can then be run on a platform of the developers choice to see the performance implications that each of them bring.

### [Descriptor management](./performance/descriptor_management)<br/>
An application using Vulkan will have to implement a system to manage descriptor pools and sets. The most straightforward and flexible approach is to re-create them for each frame, but doing so might be very inefficient, especially on mobile platforms. The problem of descriptor management is intertwined with that of buffer management, that is choosing how to pack data in `VkBuffer` objects. This sample will explore a few options to improve both descriptor and buffer management.

### [HPP Swapchain images](./performance/hpp_swapchain_images)<br/>
A transcoded version of the Performance sample [Swapchain images](#swapchain_images) that illustrates the usage of the C++ bindings of vulkan provided by vulkan.hpp.

### [Layout transitions](./performance/layout_transitions)<br/>
Vulkan requires the application to manage image layouts, so that all render pass attachments are in the correct layout when the render pass begins. This is usually done using pipeline barriers or the `initialLayout` and `finalLayout` parameters of the render pass. If the rendering pipeline is complex, transitioning each image to its correct layout is not trivial, as it requires some sort of state tracking. If previous image contents are not needed, there is an easy way out, that is setting `oldLayout`/`initialLayout` to `VK_IMAGE_LAYOUT_UNDEFINED`. While this is functionally correct, it can have performance implications as it may prevent the GPU from performing some optimizations. This sample will cover an example of such optimizations and how to avoid the performance overhead from using sub-optimal layouts.

### [MSAA](./performance/msaa)<br/>
Aliasing is the result of under-sampling a signal. In graphics this means computing the color of a pixel at a resolution that results in artifacts, commonly jaggies at model edges. Multisample anti-aliasing (MSAA) is an efficient technique that reduces pixel sampling error.

### [Multi-threaded recording with multiple render passes](./performance/multithreading_render_passes)<br/>

Ideally you render all stages of your frame in a single render pass. However, in some cases different stages can't be performed in the same render pass. This sample shows how multi-threading can help to boost performance when using multiple render passes to render a single frame. 

### [Pipeline barriers](./performance/pipeline_barriers)<br/>
Vulkan gives the application significant control over memory access for resources. Pipeline barriers are particularly convenient for synchronizing memory accesses between render passes. Having barriers is required whenever there is a memory dependency - the application should not assume that render passes are executed in order. However, having too many or too strict barriers can affect the application's performance. This sample will cover how to set up pipeline barriers efficiently, with a focus on pipeline stages.

### [Pipeline cache](./performance/pipeline_cache)<br/>
Vulkan gives applications the ability to save internal representation of a pipeline (graphics or compute) to enable recreating the same pipeline later. This sample will look in detail at the implementation and performance implications of the pipeline creation, caching and management.

### [Render passes](./performance/render_passes)<br/>
Vulkan render-passes use attachments to describe input and output render targets. This sample shows how loading and storing attachments might affect performance on mobile. During the creation of a render-pass, you can specify various color attachments and a depth-stencil attachment. Each of those is described by a [`VkAttachmentDescription`](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkAttachmentDescription.html) struct, which contains attributes to specify the [load operation](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkAttachmentLoadOp.html) (`loadOp`) and the [store operation](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkAttachmentStoreOp.html) (`storeOp`). This sample lets you choose between different combinations of these operations at runtime.

### [Specialization constants](./performance/specialization_constants)<br/>
Vulkan exposes a number of methods for setting values within shader code during run-time, this includes UBOs and Specialization Constants. This sample compares these two methods and the performance impact of them.

### [Sub passes](./performance/subpasses)<br/>
Vulkan introduces the concept of _subpasses_ to subdivide a single [render pass](./performance/render_passes/README.md) into separate logical phases. The benefit of using subpasses over multiple render passes is that a GPU is able to perform various optimizations. Tile-based renderers, for example, can take advantage of tile memory, which being on chip is decisively faster than external memory, potentially saving a considerable amount of bandwidth.

### [Surface rotation](./performance/surface_rotation)<br/>
Mobile devices can be rotated, therefore the logical orientation of the application window and the physical orientation of the display may not match. Applications then need to be able to operate in two modes: portrait and landscape. The difference between these two modes can be simplified to just a change in resolution. However, some display subsystems always work on the "native" (or "physical") orientation of the display panel. Since the device has been rotated, to achieve the desired effect the application output must also rotate. In this sample we focus on the rotation step, and analyze the performance implications of implementing it correctly with Vulkan.

### [Swapchain images](./performance/swapchain_images)<br/>
Vulkan gives the application some significant control over the number of swapchain images to be created. This sample analyzes the available options and their performance implications.

### [Wait idle](./performance/wait_idle)<br/>
This sample compares two methods for synchronizing between the CPU and GPU, ``WaitIdle`` and ``Fences`` demonstrating which one is the best option in order to avoid stalling.

### [16-bit storage InputOutput](./performance/16bit_storage_input_output)
This sample compares bandwidth consumption when using FP32 varyings compared to using FP16 varyings with `VK_KHR_16bit_storage`.

### [16-bit arithmetic](./performance/16bit_arithmetic)
This sample compares arithmetic throughput for 32-bit arithmetic operations and 16-bit arithmetic.
The sample also shows how to enable 16-bit storage for SSBOs and push constants.

### [Async compute](./performance/async_compute)
This sample demonstrates using multiple Vulkan queues to get better hardware utilization with compute post-processing workloads.

### [Basis Universal supercompressed GPU textures](./performance/texture_compression_basisu)
This sample demonstrates how to use Basis universal supercompressed GPU textures in a Vulkan application.

### [GPU Rendering and Multi-Draw Indirect](./performance/multi_draw_indirect) <br/>
This sample demonstrates how to reduce CPU usage by offloading draw call generation and frustum culling to the GPU.

### [Texture compression comparison](./performance/texture_compression_comparison)
This sample demonstrates how to use different types of compressed GPU textures in a Vulkan application, and shows 
the timing benefits of each.

## API samples

The goal of these samples is to demonstrate how to use a given Vulkan feature at the API level with as little abstraction as possible.

### [Compute shader N-Body simulation](./api/compute_nbody)<br/>
Compute shader example that uses two passes and shared compute shader memory for simulating a N-Body particle system.

### [Dynamic Uniform buffers](./api/dynamic_uniform_buffers)<br/>
Dynamic uniform buffers are used for rendering multiple objects with separate matrices stored in a single uniform buffer object, that are addressed dynamically.

### [High dynamic range](./api/hdr)<br/>
Implements a high dynamic range rendering pipeline using 16/32 bit floating point precision for all calculations.

### [Hello Triangle](./api/hello_triangle)<br/>
A self-contained (minimal use of framework) sample that illustrates the rendering of a triangle.

### [HPP Compute shader N-Body simulation](./api/hpp_compute_nbody)<br/>
A transcoded version of the API sample [Compute N-Body](#compute_nbody) that illustrates the usage of the C++ bindings of vulkan provided by vulkan.hpp.

### [HPP Dynamic Uniform Buffers](./api/hpp_dynamic_uniform_buffers)<br/>
A transcoded version of the API sample [Dynamic Uniform buffers](#dynamic_uniform_buffers) that illustrates the usage of the C++ bindings of vulkan provided by vulkan.hpp.

### [HPP High dynamic range](./api/hpp_hdr)<br/>
A transcoded version of the API sample [High dynamic range](#hdr)that illustrates the usage of the C++ bindings of vulkan provided by vulkan.hpp.

### [HPP Hello Triangle](./api/hpp_hello_triangle)<br/>
A transcoded version of the API sample [Hello Triangle](#hello_triangle) that illustrates the usage of the C++ bindings of vulkan provided by vulkan.hpp.

### [HPP HLSL shaders](./api/hpp_hlsl_shaders)<br/>
A transcoded version of the API sample [HLSL Shaders](#hlsl_shaders) that illustrates the usage of the C++ bindings of vulkan provided by vulkan.hpp.

### [HPP Instancing](./api/hpp_instancing)<br/>
A transcoded version of the API sample [Instancing](#instancing) that illustrates the usage of the C++ bindings of vulkan provided by vulkan.hpp.

### [HPP Separate image sampler](./api/hpp_separate_image_sampler)<br/>
A transcoded version of the API sample [Separate image sampler](#separate_image_sampler) that illustrates the usage of the C++ bindings of vulkan provided by vulkan.hpp.

### [HPP Terrain Tessellation](./api/hpp_terrain_tessellation)<br/>
A transcoded version of the API sample [Terrain Tessellation](#terrain_tessellation) that illustrates the usage of the C++ bindings of vulkan provided by vulkan.hpp.

### [HPP Texture Loading](./api/hpp_texture_loading)<br/>
A transcoded version of the API sample [Texture loading](#texture_loading) that illustrates the usage of the C++ bindings of vulkan provided by vulkan.hpp.

### [HPP Texture run-time mip-map generation](./api/hpp_texture_mipmap_generation)<br/>
A transcoded version of the API sample [Texture run-time mip-map generation](#texture_mipmap_generation) that illustrates the usage of the C++ bindings of vulkan provided by vulkan.hpp.

### [HPP Timestamp queries](./api/hpp_timestamp_queries/)<br/>
A transcoded version of the API sample [Timestamp queries](#timestamp_queries) that illustrates the usage of the C++ bindings of vulkan provided by vulkan.hpp.

### [Instancing](./api/instancing)<br/>
Uses the instancing feature for rendering many instances of the same mesh from a single vertex buffer with variable parameters and textures.

### [Separate image sampler](./api/separate_image_sampler)<br/>
Separate image and samplers, both in the application and the shaders. The sample demonstrates how to use different samplers for the same image without the need to recreate descriptors.

### [Terrain Tessellation](./api/terrain_tessellation)<br/>
Uses a tessellation shader for rendering a terrain with dynamic level-of-detail and frustum culling.

### [Texture loading](./api/texture_loading)<br/>
Loading and rendering of a 2D texture map from a file.

### [Texture run-time mip-map generation](./api/texture_mipmap_generation)<br/>
Generates a complete mip-chain for a texture at runtime instead of loading it from a file.

### [HLSL shaders](./api/hlsl_shaders)<br/>
Converts High Level Shading Language (HLSL) shaders to Vulkan-compatible SPIR-V.

### [Timestamp queries](./api/timestamp_queries/)<br/>
Using timestamp queries for profiling GPU workloads.

## Extension Samples

The goal of these samples is to demonstrate how to use a particular Vulkan extension at the API level with as little abstraction as possible.

### [Conservative Rasterization](./extensions/conservative_rasterization)<br/>
**Extension**: [```VK_EXT_conservative_rasterization```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_EXT_conservative_rasterization)<br/>
Uses conservative rasterization to change the way fragments are generated. Enables overestimation to generate fragments for every pixel touched instead of only pixels that are fully covered.

### [Dynamic Rendering](./extensions/dynamic_rendering) <br/>
**Extension**: [```VK_KHR_dynamic_rendering```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_dynamic_rendering) <br/>
Demonstrates how to use Dynamic Rendering.  Read the blog post here for discussion: (https://www.khronos.org/blog/streamlining-render-passes)

### [Push Descriptors](./extensions/push_descriptors)<br/>
**Extension**: [```VK_KHR_push_descriptor```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_push_descriptor)<br/>
Push descriptors apply the push constants concept to descriptor sets. Instead of creating per-object descriptor sets, this example passes descriptors at command buffer creation time.

### [Debug Utilities](./extensions/debug_utils)<br/>
**Extension**: [```VK_EXT_debug_utils```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_EXT_debug_utils)<br/>
Uses the debug utilities extension to name and group Vulkan objects (command buffers, images, etc.). This information makes debugging in tools like RenderDoc significantly easier.

### [Memory Budget](./extensions/memory_budget)<br/>
**Extension**: [```VK_EXT_memory_budget``](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_EXT_memory_budget)<br/>
Uses the memory budget extension to monitor the allocated memory in the GPU and demonstrates how to use it.

### [Basic ray queries](./extensions/ray_queries)<br/>
**Extensions**: [```VK_KHR_ray_query```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_ray_query), [```VK_KHR_acceleration_structure```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_acceleration_structure) <br/>
Render a sponza scene using the ray query extension. Shows how to set up all data structures required for ray queries, including the bottom and top level acceleration structures for the geometry and a standard vertex/fragment shader pipeline. Shadows are cast dynamically by ray queries being cast by the fragment shader.<br/>

### [Basic hardware accelerated ray tracing](./extensions/raytracing_basic)<br/>
**Extensions**: [```VK_KHR_ray_tracing_pipeline```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_ray_tracing_pipeline), [```VK_KHR_acceleration_structure```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_acceleration_structure)<br/>
Render a basic scene using the official cross-vendor ray tracing extension. Shows how to setup all data structures required for ray tracing, including the bottom and top level acceleration structures for the geometry, the shader binding table and the ray tracing pipelines with shader groups for ray generation, ray hits, and ray misses. After dispatching the rays, the final result is copied to the swapchain image.<br/>

### [Extended hardware accelerated ray tracing](./extensions/raytracing_extended)<br/>
**Extensions**: [```VK_KHR_ray_tracing_pipeline```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_ray_tracing_pipeline), [```VK_KHR_acceleration_structure```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_acceleration_structure)<br/>
Render Sponza with Ambient Occlusion.  Place a vase in center.  Generate a particle fire that 
demonstrates the TLAS (Top Level Acceleration Structure) animation for the same underlying geometry.
Procedurally generate a transparent quad and deform the geometry of the quad in the BLAS (Bottom Level Acceleration 
Structure) to demonstrate how to animate with deforming geometry.
Shows how to rebuild the acceleration structure and when to set it to fast rebuild vs fast traversal.

### [Mesh shading](./extensions/mesh_shading)<br/>
**Extensions**: [```VK_EXT_mesh_shader``](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VK_KHR_mesh_shader.html)<br/>
Renders a triangle with the most simple of all possible mesh shader pipeline examples.  There is no vertex shader, 
there is only a mesh and fragment shader.  The mesh shader creates the vertices for the triangle.  The mesh shading 
pipeline includes the task and mesh shaders before going into the fragment shader.  This replaces the vertex / 
geometry shader standard pipeline.

### [OpenGL interoperability](./extensions/open_gl_interop)<br/>
**Extensions**: [```VK_KHR_external_memory```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_KHR_external_memory.html), [```VK_KHR_external_semaphore```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_KHR_external_semaphore.html)<br/>
Render a procedural image using OpenGL and incorporate that rendered content into a Vulkan scene.  Demonstrates using the same backing memory for a texture in both OpenGL and Vulkan and how to synchronize the APIs using shared semaphores and barriers.

### [Arm OpenCL interoperability](./extensions/open_cl_interop)<br/>
**Extensions**: [```VK_ANDROID_EXTERNAL_MEMORY_ANDROID_HARDWARE_BUFFER_EXTENSION_NAME```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_ANDROID_external_memory_android_hardware_buffer.html)<br/>
This sample demonstrates usage of OpenCL extensions available on Arm devices. Fill a procedural texture using OpenCL and display it using Vulkan. In this sample data sharing between APIs is achieved using Android Hardware Buffers.

### [Timeline semaphore](./extensions/timeline_semaphore)<br/>
**Extensions**: [```VK_KHR_timeline_semaphore```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_KHR_timeline_semaphore.html)
Demonstrates various use cases which are enabled with timeline semaphores. The sample implements "Game of Life" in an esoteric way,
using out-of-order signal and wait, multiple waits on same semaphore in different queues, waiting and signalling semaphore on host.

### [Buffer device address](./extensions/buffer_device_address)<br/>
**Extensions**: [```VK_KHR_buffer_device_address```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_KHR_buffer_device_address.html)
Demonstrates how to use the buffer device address feature, which enables extreme flexibility in how buffer memory is accessed.

### [Synchronization2](./extensions/synchronization_2)<br/>
**Extension** [```VK_KHR_synchronization2```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_synchronization2)<br/>
Demonstrates the use of the reworked synchronization api introduced with `VK_KHR_synchronization2`. Based on the compute shading N-Body particle system, this sample uses the new extension to streamline the memory barriers used for the compute and graphics work submissions.

### [Descriptor indexing](./extensions/descriptor_indexing)<br/>
**Extensions**: [```VK_EXT_descriptor_indexing```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_EXT_descriptor_indexing.html)
Demonstrates how to use descriptor indexing to enable update-after-bind and non-dynamically uniform indexing of descriptors.

### [Fragment shading rate](./extensions/fragment_shading_rate)<br/>
**Extension**: [```VK_KHR_fragment_shading_rate```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_KHR_fragment_shading_rate.html)<br/>
Uses a special framebuffer attachment to control fragment shading rates for different framebuffer regions. This allows explicit control over the number of fragment shader invocations for each pixel covered by a fragment, which is e.g. useful for foveated rendering.

### [Fragment shading rate_dynamic](./extensions/fragment_shading_rate) <br/>
**Extension**: [```VK_KHR_fragment_shading_rate```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_KHR_fragment_shading_rate.html) <br/>
Render a simple scene showing the basics of shading rate dynamic.  This sample shows low and high frequency textures 
over several cubes.  It creates a sample rate map based upon this frequency every frame. Then it uses that dynamic 
sample rate map as a base for the next frame.

### [Ray tracing: reflection, shadow rays](./extensions/ray_tracing_reflection)<br/>
**Extensions**: [```VK_KHR_ray_tracing_pipeline```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_ray_tracing_pipeline), [```VK_KHR_acceleration_structure```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_acceleration_structure), [```VK_EXT_descriptor_indexing```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_EXT_descriptor_indexing.html), [```VK_EXT_scalar_block_layout```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_EXT_scalar_block_layout.html)
<br/>
Render a simple scene showing the basics of ray tracing, including reflection and shadow rays. The sample creates some geometries and create a bottom acceleration structure for each, then make instances of those, using different materials and placing them at different locations.
<br/>

### [Portability](./extensions/portability) <br/>
**Extensions**: [```VK_KHR_portability_subset```](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/vkspec.html#VK_KHR_portability_subset)
Demonstrate how to include non-conformant portable Vulkan implementations by using the portability extension to 
include those implementations in the device query.  An example of a non-conformant portable Vulkan implementation is 
MoltenVk: [MoltenVk](https://github.com/KhronosGroup/MoltenVK).  Also demonstrate use of beta extension which allows 
for querying which features of the full Vulkan spec are not currently supported by the non-conformant Vulkan 
implementation.

### [Graphics pipeline library](./extensions/graphics_pipeline_library)<br/>
**Extension**: [```VK_EXT_graphics_pipeline_library```](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VK_EXT_graphics_pipeline_library.html) <br/>
Uses the graphics pipeline library extensions to improve run-time pipeline creation. Instead of creating the whole pipeline at once, this sample makes use of that extension to pre-build shared pipeline parts such as vertex input state and fragment output state. These building blocks are then used to create pipelines at runtime, improving build times compared to traditional pipeline creation.

### [Conditional rendering](./extensions/conditional_rendering)
**Extension**: [```VK_EXT_conditional_rendering```](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VK_EXT_conditional_rendering.html) <br/>
Demonstrate how to do conditional rendering, dynamically discarding rendering commands without having to update command buffers. This is done by sourcing conditional rendering blocks from a dedicated buffer that can be updated without having to touch command buffers.

### [Vertex input dynamic state](./extensions/vertex_dynamic_state)
**Extension**: [```VK_EXT_vertex_input_dynamic_state```](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_vertex_input_dynamic_state.html) <br/>
Demonstrate how to use vertex input bindings and attribute descriptions dynamically, which can reduce the number of pipeline objects that are need to be created.

### [Logic operations dynamic state](./extensions/logic_op_dynamic_state)
**Extension**: [```VK_EXT_extended_dynamic_state2```](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_extended_dynamic_state2.html) <br/>
Demonstrate how to use logical operations dynamically, which can reduce the number of pipeline objects that are needed to be created or allow to change the pipeline state dynamically (change type of the logical operation).

## Tooling Samples

The goal of these samples is to demonstrate usage of tooling functions and libraries that are not directly part of the api.

### [Profiles Library](./tooling/profiles)
Use the [Vulkan Profiles library](https://github.com/KhronosGroup/Vulkan-Profiles) to simplify instance and device setup. The library defines a common baseline of features, extensions, etc.
