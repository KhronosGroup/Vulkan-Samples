#version 450
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

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (set = 0, binding = 0) uniform UBO {
	mat4 projection;
	mat4 view;
} ubo;

layout(push_constant) uniform Push_Constants {
	mat4 model;

	vec4 base_color_factor;
	float metallic_factor;
	float roughness_factor;

	uint baseTextureIndex;
	uint normalTextureIndex;
	uint metallicRoughnessTextureIndex;
} push_constants;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outViewVec;
layout (location = 3) out vec3 outLightVec;
layout (location = 4) out vec4 outBaseColorFactor;
layout (location = 5) out float outMetallicFactor;
layout (location = 6) out float outRoughnessFactor;
layout (location = 7) out uint outBaseTextureIndex;
layout (location = 8) out uint outNormalTextureIndex;
layout (location = 9) out uint outMetallicRoughnessTextureIndex;

void main()
{
	outUV = inUV;
	vec4 localPos = ubo.view * push_constants.model * vec4(inPos, 1.0);
	gl_Position = ubo.projection * localPos;
	outNormal = mat3(ubo.view * push_constants.model) * inNormal;
	vec3 lightPos = vec3(10.0, -10.0, 10.0);
	outLightVec = lightPos.xyz;
	outViewVec = -localPos.xyz;
	outBaseColorFactor = push_constants.base_color_factor;
	outMetallicFactor = push_constants.metallic_factor;
	outRoughnessFactor = push_constants.roughness_factor;
	outBaseTextureIndex = push_constants.baseTextureIndex;
	outNormalTextureIndex = push_constants.normalTextureIndex;
	outMetallicRoughnessTextureIndex = push_constants.metallicRoughnessTextureIndex;
}
