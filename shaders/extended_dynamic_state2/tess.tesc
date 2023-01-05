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

layout(set = 0, binding = 0) uniform UBO
{
	mat4  projection;
	mat4  modelview;
	vec4  lightPos;
	float tessellationFactor;
}
ubo;

layout(vertices = 3) out;

layout(location = 0) in vec3 inPos[];
layout(location = 1) in vec3 inNormal[];

layout(location = 0) out vec3 outPos[3];
layout(location = 1) out vec3 outNormal[3];


void main()
{
	if (gl_InvocationID == 0)
	{
		if (ubo.tessellationFactor > 0.0)
		{
			gl_TessLevelInner[0] = ubo.tessellationFactor;
			gl_TessLevelInner[1] = ubo.tessellationFactor;
			gl_TessLevelOuter[0] = ubo.tessellationFactor;
			gl_TessLevelOuter[1] = ubo.tessellationFactor;
			gl_TessLevelOuter[2] = ubo.tessellationFactor;
			gl_TessLevelOuter[3] = ubo.tessellationFactor;
		}
		else
		{
			gl_TessLevelInner[0] = 1;
			gl_TessLevelInner[1] = 1;
			gl_TessLevelOuter[0] = 1;
			gl_TessLevelOuter[1] = 1;
			gl_TessLevelOuter[2] = 1;
			gl_TessLevelOuter[3] = 1;
		}
		
	}

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	outNormal[gl_InvocationID]          = inNormal[gl_InvocationID];
	outPos[gl_InvocationID] = inPos[gl_InvocationID];
}