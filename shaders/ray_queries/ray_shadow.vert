#version 460
#extension GL_EXT_ray_query : enable

/* Copyright (c) 2021 Holochip Corporation
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

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;

layout(set = 0, binding = 1) uniform GlobalUniform
{
	mat4 view;
	mat4 proj;
	vec3 camera_position;
	vec3 light_position;
}
global_uniform;

layout (location = 0) out vec4 o_pos;
layout (location = 1) out vec3 o_normal;
layout (location = 2) out vec4 scene_pos; // scene with respect to BVH coordinates

void main(void)
{
	// We want to be able to perform ray tracing, so don't apply any matrix to scene_pos
	scene_pos = vec4(position, 1);

    o_pos = global_uniform.view * vec4(position, 1);

    o_normal = normal;

    gl_Position = global_uniform.proj * global_uniform.view * vec4(position, 1.0);
}
