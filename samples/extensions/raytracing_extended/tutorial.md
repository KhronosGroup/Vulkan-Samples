<!--
- Copyright (c) 2019-2021, Holochip Corporation
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

## Ray-tracing: Extended features and dynamic objects

This code sample demonstrates how to incorporate animations into a ray-traced scene, and shows how to incorporate
different types of changing objects within the acceleration structures.

# Acceleration structures

The ray tracing acceleration structures are separated into two types: bottom-level acceleration structures (BLAS) and
top-level acceleration structures (TLAS). The BLAS contains information about each object's geometry within its own
coordinate system and is built using the vertex and index data stored in a GPU buffer. In contrast, the TLAS contains
information about each instance of the geometry and its transformation (i.e. scaling, rotation, translation, etc.).

Each object must be represented in the BLAS, but can have any number of instances, each with its own transformation.
This allows objects to be replicated without creating an acceleration structure for each instance.

# Objects: Static, moving, and changing

There are three categories of objects to consider when building acceleration structures: static, moving, and changing
geometry. Static geometry includes scene data. In this code sample, the Sponza scene has a single, non-moving instance.
In contrast, dynamic objects can have a changing transformation, changing geometry, or both. An example of
transformation-only dynamic objects in this code sample are given by the flame particle effect, which is achieved by
adjusting only the location and rotation of a square billboard -- the internal geometry (and thus the billboard's BLAS)
does not change. In contrast, the refraction effect is achieved by changing both the internal geometry each frame, and
the rotation (so that it faces the viewer).

Vulkan offers methods of optimizing the acceleration structures for each type of geometry.
The `VkAccelerationStructureBuildGeometryInfoKHR` struct has flags that can either toggle "fast trace", which optimizes
run-time performance at the expense of build time, or "fast build", which optimizes build time. When constructing large,
static objects such as the Sponza scene, for instance, the "fast trace"
bit (`VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR`) is selected because the build will occur once and the
model contains many points. When constructing dynamic objects such as the refraction model, which will need a BLAS
update every frame, the "fast build" bit (`VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR`) is selected.

Further optimization methods can be used. For instance, the refraction model is updated every frame by the CPU and thus
uses host-visible memory. However, because host-visible memory can incur a performance penalty, the Sponza and billboard
models use a staging buffer to copy to device-exclusive memory. An alternative method would be to use a "compute shader"
to generate the refraction model each frame, but that is outside the scope of this tutorial.

# Reference Object Data from a Closest-Hit Shader

Though the ray-tracing pipeline uses an acceleration structure to traverse the scene's geometry, the acceleration
structures themselves do not store user-defined information about the geometry and instead give the developer the
flexibility to define their own custom geometry information. This information can be encoded at the per-instance level,
per-object level, or per-primitive level.

*Per-instance level:* The top-level acceleration structure allows instance information to encode a custom ID (
per-instance level).

*Per-object level:* In this code sample, this custom ID then references a struct at the per-object level containing the
object ID , the index of the vertices in the vertex buffer, and the index of the (triangle) indices in the index buffer:

```
struct SceneInstanceData
{
    uint32_t vertex_index;
    uint32_t indices_index;
    uint32_t image_index;
    uint32_t object_type;
};
```

*Per-primitive level*  In this sample, each vertex is encoded with a per-vertex normal and texture coordinate, though
other applications may wish to provide other information at the per-vertex level. To allow the bottom-level acceleration
structure to reference geometry data with a custom-defined layout, the `VkAccelerationStructureGeometryKHR` provides the
ability to set geometry offsets and strides (i.e. `vertexStride`). In the code below, the
struct `acceleration_structure_geometry` of type `VkAccelerationStructureGeometryKHR` references the data layout
provided by NewVertex, which encodes the normal and texture coordinate:

```
acceleration_structure_geometry.geometry.triangles.vertexData    = vertex_data_device_address;
acceleration_structure_geometry.geometry.triangles.maxVertex     = model_buffer.num_vertices;
acceleration_structure_geometry.geometry.triangles.vertexStride  = sizeof(NewVertex);
acceleration_structure_geometry.geometry.triangles.indexType     = VK_INDEX_TYPE_UINT32;
acceleration_structure_geometry.geometry.triangles.indexData     = index_data_device_address;
acceleration_structure_geometry.geometry.triangles.transformData = transform_matrix_device_address;
```

This technique allows the closest-hit shader to access pre-calculated vertex information.

# Texture Binding and Shaders

In a traditional raster pipeline, it is possible to render each object separately and bind its appropriate texture
images during that pass. However, in a ray-tracing pipeline, each ray during a render pass could intersect with many
objects within the scene, and thus all textures must be available to the shader. In this code sample, an array of
textures (`Sampler2D[]`) is bound, and each object is associated with a given texture index. The texture ID information
is stored in the object data.

# Ambient Occlusion and Ray-Traced Shadows

This code sample explores two different ways to calculate lighting: ray-traced shadows and ambient occlusion, both of
which are updated each frame and are triggered when a primary ray intersects a scene object (i.e. an element of the
Sponza scene).

Ray-traced shadows are calculated by performing a test: a ray is shot from the object point in the direction of the
light. If the returned distance is less than the distance to the light source, then the object point is in a shadow. In
pseudocode:

```
direction = object_pt - light_pt
dist = trace_ray(object_pt, direction)
if (dist < distance(object_pt, light_pt)):
    color.rgb *= 0.2
```

The ambient occlusion effect is used to simulate the light diminishing effect of clustered geometry. It's simulated by
tracing rays distributed about a hemisphere centered at the intersection point with the object's normal. The
light-diminishing effect is estimated using the distance to the nearest ray intersection. In some implementations, a
hard threshold is used. In pseudocode:

```
for theta,phi in angles:
    hard_threshold = 10.f 
    direction = hemisphere_pt(object_pt, normal, theta, phi)
    dist = trace_ray(object_pt, direction)
    if (dist < hard_threshold):
        color.rgb *= 0.2
```

The code sample in this tutorial instead linearly interpolates up to the hard_threshold:

```
color.rgb *= min(dist, hard_threshold) / min_threshold
```

There are further optimizations that can be used. One common technique is to reduce the number of generated ambient
occlusion rays at each point, often shooting just a single ray. The resulting image can then be de-noised using a
separate de-noising pass, though this technique is outside the scope of this tutorial.

