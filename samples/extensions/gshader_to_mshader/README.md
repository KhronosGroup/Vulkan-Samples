<!--
- Copyright (c) 2023, Mobica Limited
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

# Geometry shader to mesh shader

![Sample](./images/visualization_of_normals.png)

## Overview

This sample demonstrates how to use mesh shader instead of geometry shader to achieve same results. It contains geometry and mesh shader pipelines visualizing normals in teapot model.

## Meshlets

To access model vertices from within mesh shader it needs to be stored within storage buffer (SSBO). Indices need to be divided into meshlets and also stored within SSBO so each work item can work on single meshlet.
This sample adds fuction to framwerok `load_model_to_storage_buffer()` that is based on `load_model()`. It reads model from file, stores vertices positions and normals to SSBO using `VertexAlligned` strcuture (because of std430 memory layout), after that indices are divided into mehslets based on `Meshlet` structure and stored to SSBO.

## Enabling the Extension

To enable mesh shader VulkanAPI 1.1 is required.
The device extension is provided by `VK_EXT_MESH_SHADER_EXTENSION_NAME`.
It also requires `VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME`.
SPIRV needs to be set to 1.4.

```C++
set_api_version(VK_API_VERSION_1_1);
add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
add_device_extension(VK_EXT_MESH_SHADER_EXTENSION_NAME);
add_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
add_device_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
vkb::GLSLCompiler::set_target_environment(glslang::EShTargetSpv, glslang::EShTargetSpv_1_4);
```
The `VkPhysicalDeviceMeshShaderFeaturesEXT` structure needs to be included in the pNext chain of the `VkPhysicalDeviceFeatures2` structure passed to vkGetPhysicalDeviceFeatures2

```C++
auto &requested_vertex_input_features      = gpu.request_extension_features<VkPhysicalDeviceMeshShaderFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV);
requested_vertex_input_features.meshShader = VK_TRUE;
```
