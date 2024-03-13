#version 460
/*
 * Copyright 2023 Nintendo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
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

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (binding = 0) uniform UBO {
	mat4 projection;
	mat4 view;
	mat4 proj_view;
} ubo;

layout(push_constant) uniform pushConstants
{
	mat4 model;
	vec3 camera_pos;
	float elapsed_time;
	float material_diffuse;
	float material_spec;
} push;

layout (location = 0) out VertexData {
	vec2 uv;
	vec3 normal;
	vec3 world_pos;
	vec3 object_pos;
	flat uint texture_index;
} vertex_out;

void main()
{
	float theta = push.elapsed_time * 0.1f;
	mat3 rotate = {{cos(theta),0,-sin(theta)},{0, 1, 0},{sin(theta), 0, cos(theta)}};
	vertex_out.uv = inUV;
	vertex_out.normal = mat3(inverse(transpose(push.model))) * inNormal;
	vertex_out.object_pos = inPos;
	vertex_out.world_pos = (push.model * vec4(rotate * inPos.xyz, 1.0f)).xyz;

	gl_Position = ubo.proj_view * vec4(vertex_out.world_pos, 1.0f);
}
