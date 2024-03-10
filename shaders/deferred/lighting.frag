#version 450
/* Copyright (c) 2019-2020, Arm Limited and Contributors
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
precision highp float;

layout(input_attachment_index = 0, binding = 0) uniform subpassInput i_depth;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput i_albedo;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput i_normal;

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 3) uniform GlobalUniform
{
    mat4 inv_view_proj;
    vec2 inv_resolution;
}
global_uniform;

// Deferred shading can have a larger number of lights than forward rendering!
#define MAX_LIGHT_COUNT 32

#include "../lighting.h"

layout(set = 0, binding = 4) uniform LightsInfo
{
	Light directional_lights[MAX_LIGHT_COUNT];
	Light point_lights[MAX_LIGHT_COUNT];
	Light spot_lights[MAX_LIGHT_COUNT];
}
lights_info;

layout(constant_id = 0) const uint DIRECTIONAL_LIGHT_COUNT = 0U;
layout(constant_id = 1) const uint POINT_LIGHT_COUNT       = 0U;
layout(constant_id = 2) const uint SPOT_LIGHT_COUNT        = 0U;

void main()
{
	// Retrieve position from depth
	vec4  clip         = vec4(in_uv * 2.0 - 1.0, subpassLoad(i_depth).x, 1.0);
	vec4 world_w = global_uniform.inv_view_proj * clip;
	vec3 pos     = world_w.xyz / world_w.w;
	vec4 albedo = subpassLoad(i_albedo);
	// Transform from [0,1] to [-1,1]
	vec3 normal = subpassLoad(i_normal).xyz;
	normal      = normalize(2.0 * normal - 1.0);
	// Calculate lighting
	vec3 L = vec3(0.0);
	for (uint i = 0U; i < DIRECTIONAL_LIGHT_COUNT; ++i)
	{
		L += apply_directional_light(lights_info.directional_lights[i], normal);
	}
	for (uint i = 0U; i < POINT_LIGHT_COUNT; ++i)
	{
		L += apply_point_light(lights_info.point_lights[i], pos, normal);
	}
	for (uint i = 0U; i < SPOT_LIGHT_COUNT; ++i)
	{
		L += apply_spot_light(lights_info.spot_lights[i], pos, normal);
	}
	vec3 ambient_color = vec3(0.2) * albedo.xyz;
	
	o_color = vec4(ambient_color + L * albedo.xyz, 1.0);
}