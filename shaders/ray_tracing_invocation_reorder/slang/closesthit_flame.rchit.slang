/* Copyright (c) 2025-2026, Holochip Inc.
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

#define RENDER_DEFAULT 0
#define RENDER_BARYCENTRIC 1
#define RENDER_INSTANCE_ID 2
#define RENDER_DISTANCE 3
#define RENDER_GLOBAL_XYZ 4
#define RENDER_SHADOW_MAP 5
#define RENDER_AO 6

struct Payload
{
    float4 color;
    float4 intersection; // {x, y, z, intersectionType}
    float4 normal;       // {nx, ny, nz, distance}
};

struct Attributes
{
    float2 bary;
};

// Buffers/textures layout must match descriptor set layout (set=0, bindings 4..9)
StructuredBuffer<float4> vertex_buffer        : register(t4);
StructuredBuffer<uint>   index_buffer         : register(t5);
StructuredBuffer<uint>   data_map             : register(t6);
Texture2D                textures[26]         : register(t7);
SamplerState             samplers[26]         : register(s7);

[[vk::constant_id(0)]] const uint render_mode = RENDER_DEFAULT;

float3 heatmap(float value, float minValue, float maxValue)
{
    float scaled = (min(max(value, minValue), maxValue) - minValue) / (maxValue - minValue);
    float r = scaled * (3.14159265359 / 2.0);
    return float3(sin(r), sin(2.0 * r), cos(r));
}

struct Vertex
{
    float3 pt;
    float3 normal;
    float2 coordinate;
};

Vertex getVertex(uint vertexOffset, uint index)
{
    uint base_index = 2 * (vertexOffset + index);
    float4 A = vertex_buffer[base_index];
    float4 B = vertex_buffer[base_index + 1];

    Vertex v;
    v.pt = A.xyz;
    v.normal = float3(A.w, B.x, B.y);
    v.coordinate = float2(B.z, B.w);
    return v;
}

uint3 getIndices(uint triangle_offset, uint primitive_id)
{
    uint base_index = 3 * (triangle_offset + primitive_id);
    uint index0 = index_buffer[base_index];
    uint index1 = index_buffer[base_index + 1];
    uint index2 = index_buffer[base_index + 2];
    return uint3(index0, index1, index2);
}

[shader("closesthit")]
void main(inout Payload hitValue, in Attributes attribs)
{
    const float3 barycentricCoords = float3(1.0f - attribs.bary.x - attribs.bary.y, attribs.bary.x, attribs.bary.y);

    if (render_mode == RENDER_BARYCENTRIC)
    {
        hitValue.color = float4(barycentricCoords, 1);
        return;
    }
    else if (render_mode == RENDER_INSTANCE_ID)
    {
        hitValue.color = float4(heatmap(InstanceID(), 0, 25), 1);
        return;
    }
    else if (render_mode == RENDER_DISTANCE)
    {
        hitValue.color = float4(heatmap(log(1 + RayTCurrent()), 0, log(1 + 25)), 1);
        return;
    }

    uint index = InstanceID();

    uint vertexOffset   = data_map[4 * index];
    uint triangleOffset = data_map[4 * index + 1];
    uint imageOffset    = data_map[4 * index + 2];
    const uint objectType = 2; // OBJECT_FLAME

    uint3 indices = getIndices(triangleOffset, PrimitiveIndex());
    Vertex A = getVertex(vertexOffset, indices.x);
    Vertex B = getVertex(vertexOffset, indices.y);
    Vertex C = getVertex(vertexOffset, indices.z);

    // interpolate and obtain world point
    float alpha = barycentricCoords.x, beta = barycentricCoords.y, gamma = barycentricCoords.z;
    float3 pt = alpha * A.pt + beta * B.pt + gamma * C.pt;
    float3 worldPt = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    float3 worldNormal = normalize(cross(B.pt - A.pt, C.pt - A.pt));

    float2 texcoord = alpha * A.coordinate + beta * B.coordinate + gamma * C.coordinate;

    hitValue.intersection = float4(worldPt.xyz, objectType);
    hitValue.normal       = float4(worldNormal.xyz, RayTCurrent());

    if (render_mode == RENDER_GLOBAL_XYZ)
    {
        hitValue.color = float4(heatmap(worldPt.x, -10, 10), 1);
        return;
    }

    // Flame/emission material - texture lookup with emission boost
    if (imageOffset < 26)
    {
        float4 tex_value = textures[NonUniformResourceIndex(imageOffset)].SampleLevel(samplers[NonUniformResourceIndex(imageOffset)], texcoord, 0);
        float flicker = 0.8 + 0.2 * sin(worldPt.x * 10.0 + worldPt.y * 15.0);
        float emission = 2.0 * flicker;
        hitValue.color = float4(tex_value.rgb * emission, tex_value.a);
    }
}
