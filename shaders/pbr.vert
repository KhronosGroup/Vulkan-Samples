#version 320 es
/* Copyright (c) 2019, Arm Limited and Contributors
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

#define MAX_FORWARD_LIGHT_COUNT 16

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord_0;
layout(location = 2) in vec3 normal;

layout(set = 0, binding = 1) uniform GlobalUniform
{
	mat4 model;
	mat4 view_proj;
	vec3 camera_position;
}
global_uniform;

struct Light
{
	vec4 position;
	vec4 color;
};

layout(set = 0, binding = 4) uniform LightsInfo
{
	uint  count;
	Light lights[MAX_FORWARD_LIGHT_COUNT];
}
lights;

layout(location = 0) out vec3 o_pos;
layout(location = 1) out vec2 o_uv;
layout(location = 2) out vec3 o_normal;

void main(void)
{
	o_pos = vec3(global_uniform.model * vec4(position, 1.0));

	o_uv = texcoord_0;

	o_normal = mat3(global_uniform.model) * normal;

	gl_Position = global_uniform.view_proj * global_uniform.model * vec4(position, 1.0);
}
