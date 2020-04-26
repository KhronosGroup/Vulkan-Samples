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

# API samples <!-- omit in toc -->

## Contents <!-- omit in toc -->

- [Introduction](#introduction)
- [Tutorials](#tutorials)
- [Samples](#samples)

## Introduction

This area of the repository contains samples that are meant to demonstrate Vulkan API usage. The goal of these samples is to demonstrate how to use a given feature at the API level with as little abstraction as possible.

## Tutorials
- [Run-time mip-map generation](./texture_mipmap_generation/texture_mipmap_generation_tutorial.md)

## Samples
- [Compute shader N-Body simulation](./compute_nbody)<br/>
Compute shader example that uses two passes and shared compute shader memory for simulating a N-Body particle system.
- [Dynamic Uniform buffers](./dynamic_uniform_buffers)<br/>
Dynamic uniform buffers are used for rendering multiple objects with separate matrices stored in a single uniform buffer object, that are addressed dynamically.
- [High dynamic range](./hdr)<br/>
Implements a high dynamic range rendering pipeline using 16/32 bit floating point precision for all calculations.
- [Hello Triangle](./hello_triangle)<br/>
A self-contained (minimal use of framework) sample that illustrates the rendering of a triangle.
- [Instancing](./instancing)<br/>
Uses the instancing feature for rendering many instances of the same mesh from a single vertex buffer with variable parameters and textures.
- [Terrain Tessellation](./terrain_tessellation)<br/>
Uses a tessellation shader for rendering a terrain with dynamic level-of-detail and frustum culling.
- [Texture loading](./texture_loading)<br/>
Loading and rendering of a 2D texture map from a file.
- [Texture run-time mip-map generation](./texture_mipmap_generation)<br/>
Generates a complete mip-chain for a texture at runtime instead of loading it from a file.

