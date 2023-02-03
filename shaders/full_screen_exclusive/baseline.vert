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

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;

layout(binding = 0) uniform UBO
{
	mat4  projection;
	mat4  view;
	vec4  ambientLightColor;
	vec4  lightPosition;
	vec4  lightColor;
	float lightIntensity;
}
ubo;

layout(push_constant) uniform Push_Constants
{
	mat4 model;
	vec4 color;
}
push_constants;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec4 fragColor;
layout(location = 4) out vec3 outLightVec;
layout(location = 5) out vec3 outLightColor[2];
layout(location = 7) out vec3 outViewVec;
layout(location = 8) out float outLightIntensity;

layout(constant_id = 0) const int type = 0;
out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	fragColor          = push_constants.color;
	vec4 localPos      = ubo.view * push_constants.model * vec4(inPos, 1.0);
	gl_Position        = ubo.projection * localPos;
	outNormal          = mat3(ubo.view * push_constants.model) * inNormal;
	vec4 positionWorld = push_constants.model * vec4(inPos, 1.0);
	outLightVec        = ubo.lightPosition.xyz - positionWorld.xyz;
	outLightColor[0]   = ubo.lightColor.xyz * ubo.lightColor.w;
	outLightColor[1]   = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
	outViewVec         = -localPos.xyz;
	outLightIntensity  = ubo.lightIntensity;
}
