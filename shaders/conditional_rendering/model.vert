#version 450
/* Copyright (c) 2022, Sascha Willems
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

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;

layout (set = 0, binding = 0) uniform UBO {
	mat4 projection;
	mat4 view;
} ubo;

layout(push_constant) uniform Push_Constants {
	mat4 model;
	vec4 color;
} push_constants;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outViewVec;
layout (location = 3) out vec3 outLightVec;

void main() 
{
	outNormal = inNormal;
	outColor = push_constants.color.rgb;
	
	vec4 localPos = ubo.view * push_constants.model * vec4(inPos, 1.0);
	gl_Position = ubo.projection * localPos;
	outNormal = mat3(ubo.view * push_constants.model) * inNormal;
	vec3 lightPos = vec3(10.0, -10.0, 10.0);
	outLightVec = lightPos.xyz - localPos.xyz;
	outViewVec = -localPos.xyz;		
}
