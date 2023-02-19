<!--
- Copyright (c) 2023, Holochip Corporation
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

# Mesh Shader Culling

## Overview

This sample demonstrates how to incorporate the Vulkan extension ```VK_EXT_mesh_shader_exclusive ```, and
introduces mesh shader culling using meshlets.

## Introduction

This sample provides a detailed procedure to 
1) [enable the mesh shader extension](#enabling-mesh-shading) 
2) [create a mesh shading graphic
pipeline](#creating-pipeline) 
3) [generate a simple mesh using meshlets](#mesh-shader) 
4) [establish a basic cull logic for the meshlets.](#mesh-shader-culling)

## Enabling mesh shading

To enable the mesh shading feature, the following 
extensions are required:

1) ```VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2```
2) ```VK_KHR_SPIRV_1_4```
3) ```VK_EXT_MESH_SHADER```
4) ```VK_API_VERSION_1_1```

To enable task shaders and mesh shaders enable the 
following flags from the 
```VkPhysicalDeviceMeshShaderFeaturesEXT``` feature.
1) ```taskShader```
2) ```meshShader```

## Creating pipeline

When working with Mesh shader pipelines, Vertex Input 
State and Input Assembly state are ignored.  This is 
because the mesh pipeline has the responsibility of 
defining/creating the vertex information that the 
standard fragment pipeline utilizes

The mesh pipeline can create its own vertices as is 
done in this sample.  Or it can receive them from the 
application the same way one would for compute shaders 
when working with models.

Thus, we disable the ```pVertexInputState``` and 
```pInputAssemblyState```.

It is expected the normal use case would be to send 
model information to the mesh / task shaders and allow 
them to create the proper vertex information.

## Linking resources

In this sample, we use a UBO (Uniform Buffer Object) to 
set the settings for the culling.

```cpp
	struct UBO
	{
		float cull_center_x   = 0.0f;
		float cull_center_y   = 0.0f;
		float cull_radius     = 1.75f;
		float meshlet_density = 2.0f;
	} ubo_cull{};
```

* ```cull_center_x``` and ```cull_center_y``` determines 
the translation of the cull mask
* ```cull_radius```defines the size of the cull mask. 
* ```meshlet_density``` defines the total number of meshlets used for the sample.

## Task Shader

A simple task shader processes the data from the uniform 
buffer, and then send it to its associated mesh
shader.

One should notice that, mesh shading allows data sharing from the task shader to its associated mesh shader

```glsl
// Example of the data shared with its associated mesh shader:
// 1) define some structure if more than one variable data sharing is desired:
struct SharedData
{
    vec4  positionTransformation;
    int   N;
    int   meshletsNumber;
    float subDimension;
    float cullRadius;
};
// 2) use the following command to "establish the connection" to its associated mesh shader:
taskPayloadSharedEXT SharedData sharedData;
```

The ```sharedData``` declared in the task shader can be directly accessed from the mesh shader, if the same data
structure is defined in the mesh shader as well. Where, in its associated mesh shader:

```glsl
// Example of how to read shared data from its associated task shader:
// 1) the same structure must be defined (name can vary however):
struct SharedData
{
    vec4  positionTransformation;
    int   N;
    int   meshletsNumber;
    float subDimension;
    float cullRadius;
};
// 2) using the following command to "establish the connection" (variable name can vary):
taskPayloadSharedEXT SharedData sharedData;
```

In this simple task shader, it merely processes the uniform data, sharing them to its associated mesh shader, and then
emits a simple mesh task.

In general, the Mesh pipeline refers to a new pipeline 
which replaces everything before the fragment shader 
with a task shading stage that can call other mesh 
shading stages.

* A task shader is used to emit mesh shader(s) (or not)
* A mesh shader has the responsibility to generate 
  primatives.

More details about emitting a mesh task can be found in the attached article:

[Introduction to Turing Mesh Shaders](https://developer.nvidia.com/blog/introduction-turing-mesh-shaders/)

GPU manufactures have recommended best practices for 
their hardware in setting the work group and mesh size 
number. Further reading can be found here:

* [Meshlet Size tradeoffs](https://zeux.io/2023/01/16/meshlet-size-tradeoffs/)
* NVIDIA - [Mesh Shaders in Turing](https://on-demand.gputechconf.com/gtc-eu/2018/pdf/e8515-mesh-shaders-in-turing.pdf)
* AMD - [Sampler feedback ultimate in Mesh shaders](https://gpuopen.com/wp-content/uploads/slides/AMD_RDNA2_DirectX12_Ultimate_SamplerFeedbackMeshShaders.pdf)
* [Timur's blog](https://timur.hu/blog/2022/mesh-and-task-shaders)

## Mesh Shader

A simple mesh shader generates vertices and indices 
based on the number of meshlets determined by its task
shader. The vertices and indices generation process can be 
found in the following code:

```glsl
// Vertices:
gl_MeshVerticesEXT[k * 4 + 0].gl_Position = vec4(2.0 * sharedData.subDimension * unitVertex_0, 0.0f, 1.0f) + sharedData.positionTransformation + displacement;
gl_MeshVerticesEXT[k * 4 + 1].gl_Position = vec4(2.0 * sharedData.subDimension * unitVertex_1, 0.0f, 1.0f) + sharedData.positionTransformation + displacement;
gl_MeshVerticesEXT[k * 4 + 2].gl_Position = vec4(2.0 * sharedData.subDimension * unitVertex_2, 0.0f, 1.0f) + sharedData.positionTransformation + displacement;
gl_MeshVerticesEXT[k * 4 + 3].gl_Position = vec4(2.0 * sharedData.subDimension * unitVertex_3, 0.0f, 1.0f) + sharedData.positionTransformation + displacement;
// Indices
gl_PrimitiveTriangleIndicesEXT[k * 2 + 0] = unitPrimitive_0 + k * uvec3(4);
gl_PrimitiveTriangleIndicesEXT[k * 2 + 1] = unitPrimitive_1 + k * uvec3(4);
// Assigning the color output:
vec3 color = vec3(1.0f, 0.0f, 0.0f) * (k + 1) / sharedData.meshletsNumber;
outColor[k * 4 + 0] = color;
outColor[k * 4 + 1] = color;
outColor[k * 4 + 2] = color;
outColor[k * 4 + 3] = color;
```

More details of meshlets generation can be found in the attached article:

[Using Mesh Shaders for Professional Graphics](https://developer.nvidia.com/blog/using-mesh-shaders-for-professional-graphics/)

## Meshlets culling

This sample uses a simple cull functionality from the mesh 
shader. In mesh shading, there is no command, or 
automated functions that helps to "cull" a geometry. The cull logic simply determines the condition,
whether a meshlet may be generated or not. 

In this sample, a circular visual zone is centered at the origin, with an adjustable radius, controlled by the gui.
Whenever a meshlet moves out of the visual zone, its generation process will be skipped.

```glsl
// the actual position of each meshlet:
vec4 position = displacement + sharedData.positionTransformation;
float squareRadius = position.x * position.x + position.y * position.y;
// Cull Logic: only is the meshlet center position is within the view circle defined by the cull radius,
// then this very meshlet will be generated. Otherwise, meshlet will NOT be generated.
if (squareRadius < sharedData.cullRadius * sharedData.cullRadius)
{
    // Generating meshlets
}
```

There are more adaptive theories for such purpose, and a 
commonly used cull logic can be found in the
following video:

[Culling with NVIDIA Mesh Shaders](https://www.youtube.com/watch?v=n3cnUHYGbpw)

## UI overlay

![Sample](./images/mesh_shader_culling_ui_overlay.png)

A simple UI overlay is created to help showcase the mesh shader cull feature. Where, the radius of the circular cull
mask can be adjusted using the "Cull Radius" slider, which ranges from ```0.5f``` to ```2.0f```. And the meshlet density
level can be controlled using the combo-box. Where, a selection of 4-by-4, 6-by-6, and 8-by-8 of density opinions is
provided. Where, users may use keyboard input ```W``` ```A``` ```S``` ```D``` to move the square mesh around the scene.
