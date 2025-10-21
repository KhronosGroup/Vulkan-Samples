/* Copyright (c) 2023-2025, Mobica Limited
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

struct UBO
{
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};
[[vk::binding(2, 0)]] ConstantBuffer<UBO> ubo;

struct s_meshlet
{
  uint vertices[64];
  uint indices[126];
  uint vertex_count;
  uint index_count;
};
[[vk::binding(3, 0)]] StructuredBuffer<s_meshlet> meshlets;

struct s_vertex
{
    float4 position;
    float4 normal;
};
[[vk::binding(4, 0)]] StructuredBuffer<s_vertex> vertices;

struct VertexOut {
    float4 Pos : SV_Position;
    float4 color;
}

[shader("mesh")]
[outputtopology("line")]
[numthreads(1, 1, 1)]
void main(out indices uint2 outIndices[64], out vertices VertexOut outVertices[126], uint3 groupId: SV_GroupID, uint3 groupThreadId: SV_GroupThreadID)
{
    uint meshlet_index = groupId.x;
    uint thread_id = groupThreadId.x;
    uint vertex_count = meshlets[meshlet_index].vertex_count;
    uint index_count = meshlets[meshlet_index].index_count;
    uint primitive_count = index_count/3;
    SetMeshOutputCounts(primitive_count * 2, primitive_count);

    float normalLength = 0.1;

    float4x4 MVP = mul(ubo.proj, mul(ubo.view, ubo.model));

    uint j = 0;
    uint k = 0;
    for (uint i = 0; i < primitive_count; ++i)
    {
        //triangle indices
        uint vi1 = meshlets[meshlet_index].indices[j];
        uint vi2 = meshlets[meshlet_index].indices[j + 1];
        uint vi3 = meshlets[meshlet_index].indices[j + 2];

        //middle point of triangle
        float3 pos = (vertices[vi1].position.xyz + vertices[vi2].position.xyz + vertices[vi3].position.xyz) / 3.0;
        float3 normal = vertices[vi1].normal.xyz;

        // line vertices
        outVertices[k].Pos = mul(MVP, float4(pos, 1.0));
        outVertices[k].color = float4(0.0, 0.0, 1.0, 1.0);

        outVertices[k + 1].Pos = mul(MVP, float4(pos + normal * normalLength, 1.0));
        outVertices[k + 1].color = float4(1.0, 0.0, 0.0, 1.0);

        k = k + 2;
        j= j + 3;
    }

    //indices for line vertices
    k = 0;
    for (uint i = 0; i < primitive_count; i++)
    {
        outIndices[i] =  uint2(k, k+1);
        k = k + 2;
    }
}
