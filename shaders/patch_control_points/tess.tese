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
#version 450

layout(binding = 0) uniform UBO
{
	mat4 projection;
	mat4 view;
}
ubo;

layout(location = 0) in vec3 inColor[gl_MaxPatchVertices];
layout(location = 0) out vec4 color;

layout(triangles, equal_spacing, cw) in;

vec4 interpolate3D(vec4 v0, vec4 v1, vec4 v2)
{
	return vec4(gl_TessCoord.x) * v0 + vec4(gl_TessCoord.y) * v1 + vec4(gl_TessCoord.z) * v2;
}

void main()
{
	vec4 pos    = interpolate3D(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position);
	gl_Position = ubo.projection * ubo.view * pos;
	color       = vec4(inColor[0], 1.0f);
}
