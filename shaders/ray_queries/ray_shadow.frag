#version 460
#extension GL_EXT_ray_query : enable
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


layout(location = 0) in vec4 in_pos;
layout(location = 1) in vec3 in_normal;

layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 1) uniform GlobalUniform
{
	mat4 model;
	mat4 view_proj;
	vec3 camera_position;
	vec3 light_position;
}
global_uniform;

/**
This does not take into account shadows by itself
*/
vec3 apply_point_light(vec3 light_origin, vec4 light_color, vec3 pos, vec3 normal)
{
	vec3  world_to_light = light_origin.xyz - pos;
	float dist           = length(world_to_light) * 0.005;
	float atten          = 1.0 / (dist * dist);
	world_to_light       = normalize(world_to_light);
	float ndotl          = clamp(dot(normal, world_to_light), 0.0, 1.0);
	return ndotl * light_color.w * atten * light_color.rgb;
}


void main(void)
{
	const vec3 normal = normalize(in_normal);

	const vec3 origin = in_pos.xyz;

	// this is where we apply the shadow
	const bool intersects = false;
	vec3 light_contribution = vec3(0, 0, 0);
	vec4 outcolor = vec4(0.4, 0.4, 0.4, 1.0);
	if (!intersects)
	{
		light_contribution += apply_point_light(global_uniform.light_position, vec4(0.7, 0.9, 0.7, 1.0), origin, normal);
	}
	outcolor.rgb += light_contribution;
	o_color = outcolor;
}
