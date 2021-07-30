/* Copyright (c) 2019-2020, Sascha Willems
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

layout(binding=8, set = 0) readonly buffer DynamicVertexBuffer
{
  vec4[] data;
} dynamic_vertex_buffer;

layout(binding=9, set = 0) readonly buffer DynamicIndexBuffer
{
  uint[] indices;
} dynamic_index_buffer;

layout (constant_id = 0) const uint render_mode = 0;

vec3 heatmap(float value, float minValue, float maxValue)
{
  float scaled = (min(max(value, minValue), maxValue) - minValue) / (maxValue - minValue);
  float r = scaled * (3.14159265359 / 2.);
  return vec3(sin(r), sin(2 * r), cos(r));
}

/*
// Geometry instance ids
in     int   gl_PrimitiveID;
in     int   gl_InstanceID;
in     int   gl_InstanceCustomIndexEXT;
in     int   gl_GeometryIndexEXT;
*/

struct Vertex
{
  vec3 pt;
  vec3 normal;
  vec2 coordinate;
};

Vertex getVertex(uint vertexOffset, uint index, bool is_static)
{
  uint base_index = 2 * (vertexOffset + index);
  vec4 A = is_static ? vertex_buffer.data[base_index] : dynamic_vertex_buffer.data[base_index];
  vec4 B = is_static ? vertex_buffer.data[base_index + 1] : dynamic_vertex_buffer.data[base_index + 1];

  Vertex v;
  v.pt = A.xyz;
  v.normal = vec3(A.w, B.x, B.y);
  v.coordinate = vec2(B.z, B.w);
  return v;
}

uvec3 getIndices(uint triangle_offset, uint primitive_id, bool is_static)
{
    uint base_index = 3 * (triangle_offset + primitive_id);
    uint index0 = is_static ? index_buffer.indices[base_index] : dynamic_index_buffer.indices[base_index];
    uint index1 = is_static ? index_buffer.indices[base_index + 1] : dynamic_index_buffer.indices[base_index + 1];
    uint index2 = is_static ? index_buffer.indices[base_index + 2] : dynamic_index_buffer.indices[base_index + 2];

    return uvec3(index0, index1, index2);
}

void handleDraw()
{
    uint index = gl_InstanceCustomIndexEXT;

    uint vertexOffset = data_map.indices[4 * index];
    uint triangleOffset = data_map.indices[4*index + 1];
    uint imageOffset = data_map.indices[4 * index + 2];
    uint objectType = data_map.indices[4 * index + 3];
    bool is_static = objectType != 1;

    uvec3 indices = getIndices(triangleOffset, gl_PrimitiveID, is_static);
    Vertex A = getVertex(vertexOffset, indices.x, is_static), B = getVertex(vertexOffset, indices.y, is_static), C = getVertex(vertexOffset, indices.z, is_static);

    // interpolate and obtain world point
    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    float alpha = barycentricCoords.x, beta = barycentricCoords.y, gamma = barycentricCoords.z;
    vec3 pt = alpha * A.pt + beta * B.pt + gamma * C.pt;
    mat4x3 transform = gl_WorldToObjectEXT;
    vec3 worldPt =  gl_WorldRayOriginEXT + gl_HitTEXT * gl_WorldRayDirectionEXT;//transform * vec4(pt, 0) + vec3(transform[3][0], transform[3][1], transform[3][2]);
    vec3 normal = normalize(alpha * A.normal + beta * B.normal + gamma * C.normal);
    vec3 worldNormal = normalize(cross(B.pt - A.pt, C.pt - A.pt));

    vec2 texcoord = alpha * A.coordinate + beta * B.coordinate + gamma * C.coordinate;

    hitValue.intersection = vec4(worldPt.xyz, objectType);
    hitValue.normal = vec4(worldNormal.xyz, gl_HitTEXT);
    if (render_mode == 4) { // global xyz
      hitValue.color = vec4(heatmap(worldPt.x, -10, 10), 1);
      return;
    }
    if ((objectType == 0 || objectType == 2)){
      if (imageOffset >= 26){
        return; // this shouldn't happen
      }
      // obtain texture coordinate
      vec4 tex_value = texture(textures[imageOffset], texcoord);
      hitValue.color = tex_value;
    } else {
      // the refraction itself is colorless, so
      // encode the index of refraction in the color
      const float base_IOR = 1.01;
      const float x = texcoord.x, y = texcoord.y;
      const float t = min(min(min(min(x, 1-x), y), 1-y), 0.5) / 0.5;
      const float IOR = t * base_IOR + (1 - t) * 1;
      hitValue.color = vec4(IOR, 0, 0, 0);
      //hitValue.normal = vec4(normal.x, normal.y, normal.z, gl_HitTEXT);
    }
    

}

void main()
{
  const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
  if (render_mode == 1 ){ // barycentric
    hitValue.color = vec4(barycentricCoords, 1);
  } else if (render_mode == 2){ // index
    hitValue.color = vec4(heatmap(gl_InstanceCustomIndexEXT, 0, 25), 1);
  } else if (render_mode == 3){ // distance
    hitValue.color = vec4(heatmap(log(1 + gl_HitTEXT), 0, log(1 + 25)), 1);
  } else {
    handleDraw();
  }
}
