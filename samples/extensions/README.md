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

# Extension samples <!-- omit in toc -->

## Contents <!-- omit in toc -->

- [Introduction](#introduction)
- [Samples](#samples)

## Introduction

This area of the repository contains samples that are meant to demonstrate Vulkan extension. The goal of these samples is to demonstrate how to use a given extension at the API level with as little abstraction as possible.

## Tutorials
- [Using the Debug Utilities for advanced Vulkan debugging](./debug_utils/debug_utils_tutorial.md)<br/>

## Samples
- [Conservative Rasterization](./conservative_rasterization)<br/>
**Extension**: [```VK_EXT_conservative_rasterization```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_EXT_conservative_rasterization)<br/>
Uses conservative rasterization to change the way fragments are generated. Enables overestimation to generate fragments for every pixel touched instead of only pixels that are fully covered.

- [Push Descriptors](./push_descriptors)<br/>
**Extension**: [```VK_KHR_push_descriptor```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_push_descriptor)<br/>
Push descriptors apply the push constants concept to descriptor sets. Instead of creating per-object descriptor sets, this example passes descriptors at command buffer creation time.

- [Debug Utilities](./debug_utils)<br/>
**Extension**: [```VK_EXT_debug_utils```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_EXT_debug_utils)<br/>
Uses the debug utilities extension to name and group Vulkan objects (command buffers, images, etc.). This information makes debugging in tools like RenderDoc significantly easier.

- [Basic Ray Tracing](./raytracing_basic)<br/>
**Extension**: [```VK_KHR_ray_tracing```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_ray_tracing)<br/>
Render a simple triangle using the official cross-vendor ray tracing extension. Shows how to setup acceleration structures, ray tracing pipelines and the shaders required to do the actual ray tracing.<br/>
**Note**:  This extension is not yet finalized and currently considered a BETA extension. This means that it is not yet production ready and subject to change until it's finalized. In order to use this sample you may also need special developer drivers.

- [OpenGL interoperability](./open_gl_interop)<br/>
**Extensions**: [```VK_KHR_external_memory```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_KHR_external_memory.html), , [```VK_KHR_external_semaphore```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_KHR_external_semaphore.html)<br/>
Render a procedural image using OpenGL and incorporate that rendered content into a Vulkan scene.  Demonstrates using the same backing memory for a texture in both OpenGL and Vulkan and how to synchronize the APIs using shared semaphores and barriers.

