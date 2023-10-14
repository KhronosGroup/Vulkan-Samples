#version 450
/* Copyright (c) 2023, Mobica Limited
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

layout(triangles) in;

layout(location = 0) in vec3 inPostion[];
layout(location = 1) in vec2 inUv[];

layout (binding = 0) uniform Ubo 
{
	mat4 projection;
	mat4 view;
	mat4 model;
} ubo;

layout (binding = 1) uniform sampler2D fftDisplacementMap; 


vec2 interpolate_2d(vec2 v0, vec2 v1, vec2 v2)
{
	return vec2(gl_TessCoord.x) * v0 + vec2(gl_TessCoord.y) * v1 + vec2(gl_TessCoord.z) * v2;
}

vec3 interpolate_3d(vec3 v0, vec3 v1, vec3 v2)
{
	return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}

void main()
{

	vec3 world_pos = interpolate_3d(inPostion[0], inPostion[1], inPostion[2]);

	vec2 tex_coord = interpolate_2d(inUv[0], inUv[1], inUv[2]);
	vec4 fft_texel = vec4(1.0f); // texture2D(fftDisplacementMap, tex_coord);

	world_pos.y += fft_texel.y;
	world_pos.x -= fft_texel.x;
	world_pos.z -= fft_texel.z;

	
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(world_pos, 1.0f);
}