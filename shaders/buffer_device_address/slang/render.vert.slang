/* Copyright (c) 2021-2025, Arm Limited and Contributors
 * Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

struct VSOutput
{
    float4 Pos : SV_Position;
    nointerpolation float4 Color;
}

struct Registers
{
    float4x4 view_projection;
    // This is a pointer to an array of pointers, essentially:
    // const VBO * const *vbos
    float2 **positions;
};

[shader("vertex")]
VSOutput main(uint instanceIndex: SV_InstanceID, uint vertexIndex: SV_VertexID, uniform Registers registers)
{
    int slice = instanceIndex;

    // One VBO per instance, load the VBO pointer.
    // The cool thing here is that a compute shader could hypothetically
    // write the pointer list where vertices are stored.
    // With vertex attributes we do not have the luxury to modify VBO bindings on the GPU.
    // The best we can do is to just modify the vertexOffset in an indirect draw call,
    // but that's not always flexible enough, and enforces a very specific engine design to work.
    // We can even modify the attribute layout per slice here, since we can just cast the pointer
    // to something else if we want.
    float2* positions = registers.positions[slice];

    // Load the vertex based on VertexIndex instead of an attribute. Fully flexible.
    // Only downside is that we do not get format conversion for free like we do with normal vertex attributes.
    float2 pos = positions[vertexIndex] * 2.5;

    // Place the quad meshes on screen and center it.
    pos += 3.0 * (float2(slice % 8, slice / 8) - 3.5);

    VSOutput output;
    // Normal projection.
    output.Pos = mul(registers.view_projection, float4(pos, 0.0, 1.0));

    // Color the vertex. Use a combination of a wave and checkerboard, completely arbitrary.
    int index_x = vertexIndex % 16;
    int index_y = vertexIndex / 16;

    float r = 0.5 + 0.3 * sin(float(index_x));
    float g = 0.5 + 0.3 * sin(float(index_y));

    int checkerboard = (index_x ^ index_y) & 1;
    r *= float(checkerboard) * 0.8 + 0.2;
    g *= float(checkerboard) * 0.8 + 0.2;

    output.Color = float4(r, g, 0.15, 1.0);

    return output;
}
