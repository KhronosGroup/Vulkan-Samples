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

layout(location = 0) rayPayloadInEXT vec4 hitValue;
hitAttributeEXT vec3 attribs;

layout(binding = 3, set = 0) uniform RenderSettings
{
	uvec4 render_mode;
} render_settings;

vec3 heatmap(float value, float minValue, float maxValue)
{
  float scaled = min(max(value, minValue), maxValue) / maxValue;
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

void main()
{
  const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
  if (render_settings.render_mode[0] == 1){ // barycentric
    hitValue = vec4(barycentricCoords, 1);
  } else if (render_settings.render_mode[0] == 2){ // index
    hitValue = vec4(heatmap(gl_InstanceCustomIndexEXT, 0, 25), 1);
  } else if (render_settings.render_mode[0] == 3){ // distance
    hitValue = vec4(heatmap(log(1 + gl_HitTEXT), 0, log(1 + 25)), 1);
  } else if (render_settings.render_mode[0] == 4) { // global xyz
    hitValue = vec4(0, 0, 0, 1);
  }
}
