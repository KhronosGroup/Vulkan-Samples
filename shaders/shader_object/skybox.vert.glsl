#version 450
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
} push;

layout (location = 0) out vec3 outUVWorldPos;
layout (location = 1) out vec3 outPos;
layout (location = 2) out vec3 outNormal;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
	outUVWorldPos = vec3(inUV,0.f);
	
	outPos = vec3(mat3(ubo.view * push.model) * inPos);
	gl_Position = vec4(ubo.projection * vec4(outPos, 1.0));

	outNormal = mat3(push.model * ubo.view) * inNormal;
}
