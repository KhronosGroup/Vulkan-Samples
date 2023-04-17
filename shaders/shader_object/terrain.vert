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

layout (binding = 2) uniform sampler2D samplerHeight;

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
} push;

layout (location = 0) out TerrainVertexData {
	vec2 uv;
	vec3 pos;
	vec3 normal;
} vertex_out;

void main(void)
{
	vertex_out.uv = inUV;
	vertex_out.normal = inNormal;
	vertex_out.pos = vec3(inPos.x, textureLod(samplerHeight, inUV, 0.0).r * 256.0f, inPos.z);
	gl_Position = ubo.proj_view * push.model * vec4(vertex_out.pos, 1.0);
}
