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

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

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

layout (location = 0) in VertexData
{
	vec2 uv;
	vec3 normal;
	vec3 world_pos;
	vec3 object_pos;
	flat uint texture_index;
} vertex_in[3];

layout (location = 0) out VertexData
{
	vec2 uv;
	vec3 normal;
	vec3 world_pos;
	vec3 object_pos;
	flat uint texture_index;
} vertex_out;

void main() 
{	
	//A,B,C -> B - A cross C - A
	//B - A
	vec3 AB = vertex_in[1].object_pos - vertex_in[0].object_pos;
	//C - A
	vec3 AC = vertex_in[2].object_pos - vertex_in[0].object_pos;
	vec3 normal = mat3(inverse(transpose(push.model))) * cross(AB, AC);

	for(int i = 0; i < gl_in.length(); ++i)
	{
		gl_Position = gl_in[i].gl_Position;
		vertex_out.uv = vertex_in[i].uv;
		vertex_out.normal = normal;
		vertex_out.world_pos = vertex_in[i].world_pos;
		vertex_out.object_pos = vertex_in[i].object_pos;
		vertex_out.texture_index = vertex_in[i].texture_index;
		EmitVertex();
	}
	EndPrimitive();
}
