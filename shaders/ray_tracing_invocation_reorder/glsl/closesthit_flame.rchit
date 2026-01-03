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

#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable

// Closest-hit shader for OBJECT_FLAME (type 2) - emission/flame surfaces
// This shader is intentionally separate to create shader divergence for SER demonstration

#define RENDER_DEFAULT 0
#define RENDER_BARYCENTRIC 1
#define RENDER_INSTANCE_ID 2
#define RENDER_DISTANCE 3
#define RENDER_GLOBAL_XYZ 4
#define RENDER_SHADOW_MAP 5
#define RENDER_AO 6

struct Payload
{
  vec4 color;
  vec4 intersection; // {x, y, z, intersectionType}
  vec4 normal; // {nx, ny, nz, distance}
};

layout(location = 0) rayPayloadInEXT Payload hitValue;
hitAttributeEXT vec3 attribs;

layout(binding=4, set = 0) readonly buffer VertexBuffer
{
  vec4[] data;
} vertex_buffer;

layout(binding=5, set = 0) readonly buffer IndexBuffer
{
  uint[] indices;
} index_buffer;

layout(binding=6, set = 0) readonly buffer DataMap
{
  uint[] indices;
} data_map;

layout(binding=7, set = 0) uniform sampler2D textures[26];

layout (constant_id = 0) const uint render_mode = RENDER_DEFAULT;

vec3 heatmap(float value, float minValue, float maxValue)
{
  float scaled = (min(max(value, minValue), maxValue) - minValue) / (maxValue - minValue);
  float r = scaled * (3.14159265359 / 2.);
  return vec3(sin(r), sin(2 * r), cos(r));
}

struct Vertex
{
  vec3 pt;
  vec3 normal;
  vec2 coordinate;
};

Vertex getVertex(uint vertexOffset, uint index)
{
  uint base_index = 2 * (vertexOffset + index);
  vec4 A = vertex_buffer.data[base_index];
  vec4 B = vertex_buffer.data[base_index + 1];

  Vertex v;
  v.pt = A.xyz;
  v.normal = vec3(A.w, B.x, B.y);
  v.coordinate = vec2(B.z, B.w);
  return v;
}

uvec3 getIndices(uint triangle_offset, uint primitive_id)
{
    uint base_index = 3 * (triangle_offset + primitive_id);
    uint index0 = index_buffer.indices[base_index];
    uint index1 = index_buffer.indices[base_index + 1];
    uint index2 = index_buffer.indices[base_index + 2];

    return uvec3(index0, index1, index2);
}

void main()
{
    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    
    if (render_mode == RENDER_BARYCENTRIC) {
        hitValue.color = vec4(barycentricCoords, 1);
        return;
    } else if (render_mode == RENDER_INSTANCE_ID) {
        hitValue.color = vec4(heatmap(gl_InstanceCustomIndexEXT, 0, 25), 1);
        return;
    } else if (render_mode == RENDER_DISTANCE) {
        hitValue.color = vec4(heatmap(log(1 + gl_HitTEXT), 0, log(1 + 25)), 1);
        return;
    }

    uint index = gl_InstanceCustomIndexEXT;

    uint vertexOffset = data_map.indices[4 * index];
    uint triangleOffset = data_map.indices[4*index + 1];
    uint imageOffset = data_map.indices[4 * index + 2];
    const uint objectType = 2; // OBJECT_FLAME

    uvec3 indices = getIndices(triangleOffset, gl_PrimitiveID);
    Vertex A = getVertex(vertexOffset, indices.x);
    Vertex B = getVertex(vertexOffset, indices.y);
    Vertex C = getVertex(vertexOffset, indices.z);

    // interpolate and obtain world point
    float alpha = barycentricCoords.x, beta = barycentricCoords.y, gamma = barycentricCoords.z;
    vec3 pt = alpha * A.pt + beta * B.pt + gamma * C.pt;
    vec3 worldPt = gl_WorldRayOriginEXT + gl_HitTEXT * gl_WorldRayDirectionEXT;
    vec3 worldNormal = normalize(cross(B.pt - A.pt, C.pt - A.pt));

    vec2 texcoord = alpha * A.coordinate + beta * B.coordinate + gamma * C.coordinate;

    hitValue.intersection = vec4(worldPt.xyz, objectType);
    hitValue.normal = vec4(worldNormal.xyz, gl_HitTEXT);
    
    if (render_mode == RENDER_GLOBAL_XYZ) {
        hitValue.color = vec4(heatmap(worldPt.x, -10, 10), 1);
        return;
    }

    // Flame/emission material - texture lookup with emission boost
    // This is different computation than diffuse or refraction, creating shader divergence
    if (imageOffset < 26) {
        vec4 tex_value = textureLod(textures[nonuniformEXT(imageOffset)], texcoord, 0);
        
        // Emission computation - flames glow and don't receive shadows
        // Add some procedural flame flickering based on position
        float flicker = 0.8 + 0.2 * sin(worldPt.x * 10.0 + worldPt.y * 15.0);
        
        // Boost emission intensity
        float emission = 2.0 * flicker;
        
        // Flame color with emission
        hitValue.color = vec4(tex_value.rgb * emission, tex_value.a);
    }
}
