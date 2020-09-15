<!--
- Copyright (c) 2020, Arm Limited and Contributors
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

## Introduction

This readme lists all Vulkan samples currently available in this repository. They are grouped into different categories.

## Performance samples

These samples demonstrate performant Vulkan API usage. The goal of these is to demonstrate how to use certain features and functions to achieve optimal performance.

### [AFBC](./performance/afbc)<br/>
AFBC (Arm Frame Buffer Compression) is a real-time lossless compression algorithm found in Arm Mali GPUs, designed to tackle the ever-growing demand for higher resolution graphics. This format is applied to the framebuffers that are to be written to the GPU. This technology can offer bandwidth reductions of [up to 50%](https://developer.arm.com/technologies/graphics-technologies/arm-frame-buffer-compression).

### [Command buffer usage](./performance/command_buffer_usage)<br/>
This sample demonstrates how to use and manage secondary command buffers, and how to record them concurrently. Implementing multi-threaded recording of draw calls can help reduce CPU frame time.

### [Constant data](./performance/constant_data)<br/>
The Vulkan API exposes a few different ways in which we can send uniform data into our shaders. There are enough methods that it raises the question "Which one is fastest?", and more often than not the answer is "It depends". The main issue for developers is that the fastest methods may differ between the various vendors, so often there is no "one size fits all" solution. This sample aims to highlight this issue, and help move the Vulkan ecosystem to a point where we are better equipped to solve this for developers. This is done by having an interactive way to toggle different constant data methods that the Vulkan API expose to us. This can then be run on a platform of the developers choice to see the performance implications that each of them bring.

### [Descriptor management](./performance/descriptor_management)<br/>
An application using Vulkan will have to implement a system to manage descriptor pools and sets. The most straightforward and flexible approach is to re-create them for each frame, but doing so might be very inefficient, especially on mobile platforms. The problem of descriptor management is intertwined with that of buffer management, that is choosing how to pack data in `VkBuffer` objects. This sample will explore a few options to improve both descriptor and buffer management.

### [Layout transitions](./performance/layout_transitions)<br/>
Vulkan requires the application to manage image layouts, so that all render pass attachments are in the correct layout when the render pass begins. This is usually done using pipeline barriers or the `initialLayout` and `finalLayout` parameters of the render pass. If the rendering pipeline is complex, transitioning each image to its correct layout is not trivial, as it requires some sort of state tracking. If previous image contents are not needed, there is an easy way out, that is setting `oldLayout`/`initialLayout` to `VK_IMAGE_LAYOUT_UNDEFINED`. While this is functionally correct, it can have performance implications as it may prevent the GPU from performing some optimizations. This sample will cover an example of such optimizations and how to avoid the performance overhead from using sub-optimal layouts.

### [MSAA](./performance/msaa)<br/>
Aliasing is the result of under-sampling a signal. In graphics this means computing the color of a pixel at a resolution that results in artifacts, commonly jaggies at model edges. Multisample anti-aliasing (MSAA) is an efficient technique that reduces pixel sampling error.

### [Pipeline barriers](./performance/pipeline_barriers)<br/>
Vulkan gives the application significant control over memory access for resources. Pipeline barriers are particularly convenient for synchronizing memory accesses between render passes. Having barriers is required whenever there is a memory dependency - the application should not assume that render passes are executed in order. However, having too many or too strict barriers can affect the application's performance. This sample will cover how to set up pipeline barriers efficiently, with a focus on pipeline stages.

### [Pipeline cache](./performance/pipeline_cache)<br/>
Vulkan gives applications the ability to save internal representation of a pipeline (graphics or compute) to enable recreating the same pipeline later. This sample will look in detail at the implementation and performance implications of the pipeline creation, caching and management.

### [Render passes](./performance/render_passes)<br/>
Vulkan render-passes use attachments to describe input and output render targets. This sample shows how loading and storing attachments might affect performance on mobile. During the creation of a render-pass, you can specify various color attachments and a depth-stencil attachment. Each of those is described by a [`VkAttachmentDescription`](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkAttachmentDescription.html) struct, which contains attributes to specify the [load operation](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkAttachmentLoadOp.html) (`loadOp`) and the [store operation](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkAttachmentStoreOp.html) (`storeOp`). This sample lets you choose between different combinations of these operations at runtime.

### [Specialization constants](./performance/specialization_constants)<br/>
Vulkan exposes a number of methods for setting values within shader code during run-time, this includes UBOs and Specialization Constants. This sample compares these two methods and the performance impact of them.

### [Sub passes](./performance/subpasses)<br/>
Vulkan introduces the concept of _subpasses_ to subdivide a single [render pass](./performance/render_passes/render_passes_tutorial.md) into separate logical phases. The benefit of using subpasses over multiple render passes is that a GPU is able to perform various optimizations. Tile-based renderers, for example, can take advantage of tile memory, which being on chip is decisively faster than external memory, potentially saving a considerable amount of bandwidth.

### [Surface rotation](./performance/surface_rotation)<br/>
Mobile devices can be rotated, therefore the logical orientation of the application window and the physical orientation of the display may not match. Applications then need to be able to operate in two modes: portrait and landscape. The difference between these two modes can be simplified to just a change in resolution. However, some display subsystems always work on the "native" (or "physical") orientation of the display panel. Since the device has been rotated, to achieve the desired effect the application output must also rotate. In this sample we focus on the rotation step, and analyze the performance implications of implementing it correctly with Vulkan.

### [Swapchain images](./performance/swapchain_images)<br/>
Vulkan gives the application some significant control over the number of swapchain images to be created. This sample analyzes the available options and their performance implications.

### [Wait idle](./performance/wait_idle)<br/>
This sample compares two methods for synchronizing between the CPU and GPU, ``WaitIdle`` and ``Fences`` demonstrating which one is the best option in order to avoid stalling.

## API samples

These samples demonstrate Vulkan API usage. The goal of these is to demonstrate how to use a given feature at the API level with as little abstraction as possible.

### [Compute shader N-Body simulation](./api/compute_nbody)<br/>
Compute shader example that uses two passes and shared compute shader memory for simulating a N-Body particle system.
### [Dynamic Uniform buffers](./api/dynamic_uniform_buffers)<br/>
Dynamic uniform buffers are used for rendering multiple objects with separate matrices stored in a single uniform buffer object, that are addressed dynamically.
### [High dynamic range](./api/hdr)<br/>
Implements a high dynamic range rendering pipeline using 16/32 bit floating point precision for all calculations.
### [Hello Triangle](./api/hello_triangle)<br/>
A self-contained (minimal use of framework) sample that illustrates the rendering of a triangle.
### [Instancing](./api/instancing)<br/>
Uses the instancing feature for rendering many instances of the same mesh from a single vertex buffer with variable parameters and textures.
### [Terrain Tessellation](./api/terrain_tessellation)<br/>
Uses a tessellation shader for rendering a terrain with dynamic level-of-detail and frustum culling.
### [Texture loading](./api/texture_loading)<br/>
Loading and rendering of a 2D texture map from a file.
### [Texture run-time mip-map generation](./api/texture_mipmap_generation)<br/>
Generates a complete mip-chain for a texture at runtime instead of loading it from a file.

## Extension Samples

These samples demonstrate how to use a particular Vulkan extension. The goal of these is to demonstrate how to use a given extension at the API level with as little abstraction as possible.

### [Conservative Rasterization](./extensions/conservative_rasterization)<br/>
**Extension**: [```VK_EXT_conservative_rasterization```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_EXT_conservative_rasterization)<br/>
Uses conservative rasterization to change the way fragments are generated. Enables overestimation to generate fragments for every pixel touched instead of only pixels that are fully covered.

### [Push Descriptors](./extensions/push_descriptors)<br/>
**Extension**: [```VK_KHR_push_descriptor```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_push_descriptor)<br/>
Push descriptors apply the push constants concept to descriptor sets. Instead of creating per-object descriptor sets, this example passes descriptors at command buffer creation time.

### [Debug Utilities](./extensions/debug_utils)<br/>
**Extension**: [```VK_EXT_debug_utils```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_EXT_debug_utils)<br/>
Uses the debug utilities extension to name and group Vulkan objects (command buffers, images, etc.). This information makes debugging in tools like RenderDoc significantly easier.

### [Basic Ray Tracing](./extensions/raytracing_basic)<br/>
**Extension**: [```VK_KHR_ray_tracing```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_ray_tracing)<br/>
Render a simple triangle using the official cross-vendor ray tracing extension. Shows how to setup acceleration structures, ray tracing pipelines and the shaders required to do the actual ray tracing.<br/>
**Note**:  This extension is not yet finalized and currently considered a BETA extension. This means that it is not yet production ready and subject to change until it's finalized. In order to use this sample you may also need special developer drivers.

### [OpenGL interoperability](./extensions/open_gl_interop)<br/>
**Extensions**: [```VK_KHR_external_memory```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_KHR_external_memory.html), [```VK_KHR_external_semaphore```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_KHR_external_semaphore.html)<br/>
Render a procedural image using OpenGL and incorporate that rendered content into a Vulkan scene.  Demonstrates using the same backing memory for a texture in both OpenGL and Vulkan and how to synchronize the APIs using shared semaphores and barriers.