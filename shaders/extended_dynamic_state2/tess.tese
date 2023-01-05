#version 450
/* Copyright (c) 2019, Sascha Willems
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

layout (set = 0, binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
	vec4 lightPos;
	float tessellationFactor;
} ubo; 

layout(push_constant) uniform Push_Constants {
	mat4 model;
	vec4 color;
} push_constants;

layout(triangles, equal_spacing, cw) in;

layout (location = 0) in vec3 inPos[];
layout (location = 1) in vec3 inNormal[];
 
layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outViewVec;
layout (location = 2) out vec3 outLightVec;
layout (location = 3) out vec3 outEyePos;
layout (location = 4) out vec3 outWorldPos;


vec2 interpolate3D(vec2 v0, vec2 v1, vec2 v2)
{
    return vec2(gl_TessCoord.x) * v0 + vec2(gl_TessCoord.y) * v1 + vec2(gl_TessCoord.z) * v2;
}

vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2)
{
    return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}

vec4 interpolate3D(vec4 v0, vec4 v1, vec4 v2)
{
    return vec4(gl_TessCoord.x) * v0 + vec4(gl_TessCoord.y) * v1 + vec4(gl_TessCoord.z) * v2;
}


void main()
{
	outNormal = interpolate3D(inNormal[0], inNormal[1], inNormal[2]);

	vec4 pos = interpolate3D(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position);
	
	gl_Position = ubo.projection * ubo.modelview * push_constants.model * pos ;

	// Calculate vectors for lighting based on tessellated position
	outViewVec = -pos.xyz;
	outLightVec = normalize(ubo.lightPos.xyz + outViewVec);
	outWorldPos = pos.xyz;
	outEyePos = vec3(ubo.modelview * pos);
}