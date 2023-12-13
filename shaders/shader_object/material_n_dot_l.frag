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

layout (binding = 1) uniform sampler2D samplerColorMap;


layout (binding = 0) uniform UBO {
	mat4 projection;
	mat4 view;
	mat4 proj_view;
} ubo;

layout (location = 0) in VertexData {
	vec2 uv;
	vec3 normal;
	vec3 world_pos;
	vec3 object_pos;
	flat uint texture_index;
} vertex_in;

layout(push_constant) uniform pushConstants
{
	mat4 model;
	vec3 camera_pos;
	float elapsed_time;
	float material_diffuse;
	float material_spec;
} push;

layout (location = 0) out vec4 outColor;

void main(void)
{
	vec3 l = normalize(vec3(0,-1,1));
	vec4 texture_color = vec4(texture(samplerColorMap, vertex_in.uv));
	outColor = push.material_diffuse * texture_color * dot(normalize(vertex_in.normal),l) + 0.1f * texture_color;
	outColor += push.material_spec * vec4(vec3(1,1,1) * dot(normalize(vertex_in.normal),l), 1);
}
