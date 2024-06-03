/* Copyright (c) 2024, Sascha Willems
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
[[vk::location(0)]] float4 color;
[[vk::location(1)]] float4 intersection; // {x, y, z, intersectionType}
[[vk::location(2)]] float4 normal; // {nx, ny, nz, distance}    
};

struct Attributes
{
    float2 bary;
};

StructuredBuffer<float4> vertex_buffer : register(t4);
StructuredBuffer<uint> index_buffer : register(t5);
StructuredBuffer<uint> data_map : register(t6);
Texture2D textures[26]: register(t7);
SamplerState samplers[26]: register(s7);
StructuredBuffer<float4> dynamic_vertex_buffer : register(t8);
StructuredBuffer<uint> dynamic_index_buffer : register(t9);

[[vk::constant_id(0)]] const int render_mode = RENDER_DEFAULT;

float3 heatmap(float value, float minValue, float maxValue)
{
  float scaled = (min(max(value, minValue), maxValue) - minValue) / (maxValue - minValue);
  float r = scaled * (3.14159265359 / 2.);
  return float3(sin(r), sin(2 * r), cos(r));
}

struct Vertex
{
  float3 pt;
  float3 normal;
  float2 coordinate;
};

Vertex getVertex(uint vertexOffset, uint index, bool is_static)
{
  uint base_index = 2 * (vertexOffset + index);
  float4 A = is_static ? vertex_buffer[base_index] : dynamic_vertex_buffer[base_index];
  float4 B = is_static ? vertex_buffer[base_index + 1] : dynamic_vertex_buffer[base_index + 1];

  Vertex v;
  v.pt = A.xyz;
  v.normal = float3(A.w, B.x, B.y);
  v.coordinate = float2(B.z, B.w);
  return v;
}

uint3 getIndices(uint triangle_offset, uint primitive_id, bool is_static)
{
    uint base_index = 3 * (triangle_offset + primitive_id);
    uint index0 = is_static ? index_buffer[base_index] : dynamic_index_buffer[base_index];
    uint index1 = is_static ? index_buffer[base_index + 1] : dynamic_index_buffer[base_index + 1];
    uint index2 = is_static ? index_buffer[base_index + 2] : dynamic_index_buffer[base_index + 2];

    return uint3(index0, index1, index2);
}

void handleDraw(inout Payload hitValue, float2 attribs)
{
    uint index = InstanceID();

    uint vertexOffset = data_map[4 * index];
    uint triangleOffset = data_map[4*index + 1];
    uint imageOffset = data_map[4 * index + 2];
    uint objectType = data_map[4 * index + 3];
    bool is_static = objectType != 1;

    uint3 indices = getIndices(triangleOffset, PrimitiveIndex(), is_static);
    Vertex A = getVertex(vertexOffset, indices.x, is_static), B = getVertex(vertexOffset, indices.y, is_static), C = getVertex(vertexOffset, indices.z, is_static);

    // interpolate and obtain world point
    const float3 barycentricCoords = float3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    float alpha = barycentricCoords.x, beta = barycentricCoords.y, gamma = barycentricCoords.z;
    float3 pt = alpha * A.pt + beta * B.pt + gamma * C.pt;
    float4x3 transform = WorldToObject4x3();
    float3 worldPt = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();//transform * float4(pt, 0) + float3(transform[3][0], transform[3][1], transform[3][2]);
    float3 normal = normalize(alpha * A.normal + beta * B.normal + gamma * C.normal);
    float3 worldNormal = normalize(cross(B.pt - A.pt, C.pt - A.pt));

    float2 texcoord = alpha * A.coordinate + beta * B.coordinate + gamma * C.coordinate;

    hitValue.intersection = float4(worldPt.xyz, objectType);
    hitValue.normal = float4(worldNormal.xyz, RayTCurrent());
    if (render_mode == RENDER_GLOBAL_XYZ) { // global xyz
      hitValue.color = float4(heatmap(worldPt.x, -10, 10), 1);
      return;
    }
    if ((objectType == 0 || objectType == 2)){
      if (imageOffset >= 26){
        return; // this shouldn't happen
      }
      // obtain texture coordinate
      // NB: texture() is valid here as well as mipmaps are not used in this demo.
      float4 tex_value = textures[NonUniformResourceIndex(imageOffset)].SampleLevel(samplers[NonUniformResourceIndex(imageOffset)], texcoord, 0);
      hitValue.color = tex_value;
    } else {
      // the refraction itself is colorless, so
      // encode the index of refraction in the color
      const float base_IOR = 1.01;
      const float x = texcoord.x, y = texcoord.y;
      const float t = min(min(min(min(x, 1-x), y), 1-y), 0.5) / 0.5;
      const float IOR = t * base_IOR + (1 - t) * 1;
      hitValue.color = float4(IOR, 0, 0, 0);
      hitValue.normal = float4(normal.x, normal.y, normal.z, RayTCurrent());
    } 
}

[shader("closesthit")]
void main(inout Payload hitValue, in Attributes Attribs)
{
  const float3 barycentricCoords = float3(1.0f - Attribs.bary.x - Attribs.bary.y, Attribs.bary.x, Attribs.bary.y);
  if (render_mode == RENDER_BARYCENTRIC ){
    hitValue.color = float4(barycentricCoords, 1);
  } else if (render_mode == RENDER_INSTANCE_ID){
    hitValue.color = float4(heatmap(InstanceID(), 0, 25), 1);
  } else if (render_mode == RENDER_DISTANCE){
    hitValue.color = float4(heatmap(log(1 + RayTCurrent()), 0, log(1 + 25)), 1);
  } else {
    handleDraw(hitValue, Attribs.bary);
  }
}
