#version 450
/* Copyright (c) 2024, Sascha Willems
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

// printf requires this extension
#extension GL_EXT_debug_printf : require

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (binding = 0) uniform UBO {
	mat4 projection;
	mat4 modelview;
	mat4 skybox_modelview;
	float modelscale;
} ubo;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outPos;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;

layout(push_constant) uniform Push_Constants {
	vec4 offset;
	vec4 color;
	int object_type;
} push_constants;

void main() 
{
	switch(push_constants.object_type) {
		case 0: // Skysphere
			outPos = vec3(mat3(ubo.skybox_modelview) * inPos);
			gl_Position = vec4(ubo.projection * vec4(outPos, 1.0));
			break;
		case 1: // Object
			vec3 localPos = inPos * ubo.modelscale + push_constants.offset.xyz;
			outPos = vec3(ubo.modelview * vec4(localPos, 1.0));
			gl_Position = ubo.projection * ubo.modelview * vec4(localPos, 1.0);
			break;
	}
	outUV = inUV;
	outNormal = mat3(ubo.modelview) * inNormal;	
	vec3 lightPos = mat3(ubo.modelview) * vec3(0.0, -10.0, -10.0);
	outLightVec = lightPos.xyz - outPos.xyz;
	outViewVec = -outPos.xyz;

	// Output the vertex position using debug printf
	if (gl_VertexIndex == 0) {
		debugPrintfEXT("Position = %v4f", outPos);
	}
}
