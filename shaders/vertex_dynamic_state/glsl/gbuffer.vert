/* Copyright (c) 2022-2024, Mobica Limited
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

layout(constant_id = 0) const int type = 0;

layout(binding = 0) uniform UBO
{
	mat4  projection;
	mat4  modelview;
	mat4  skybox_modelview;
	mat4  inverse_modelview;
	float modelscale;
}ubo;

layout(location = 0) out vec3 outUVW;
layout(location = 1) out vec3 outPos;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outViewVec;
layout(location = 4) out vec3 outLightVec;

void main()
{
	outUVW = inPos;

	switch (type)
	{
		case 0:        // Skybox
			outPos      = vec3(mat3(ubo.skybox_modelview) * inPos);
			gl_Position = vec4(ubo.projection * vec4(outPos, 1.0));
			break;
		case 1:        // Object
			outPos      = vec3(ubo.modelview * vec4(inPos * ubo.modelscale, 1.0));
			gl_Position = ubo.projection * ubo.modelview * vec4(inPos.xyz * ubo.modelscale, 1.0);
			break;
	}
	outNormal = mat3(ubo.modelview) * inNormal;

	vec3 lightPos = vec3(0.0f, -5.0f, 5.0f);
	outLightVec   = lightPos.xyz - outPos.xyz;
	outViewVec    = -outPos.xyz;
}
