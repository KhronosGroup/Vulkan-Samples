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

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inViewVec;
layout (location = 3) in vec3 inLightVec;
layout (location = 4) flat in vec4 inBaseColorFactor;
layout (location = 5) flat in float inMetallicFactor;
layout (location = 6) flat in float inRoughnessFactor;
layout (location = 7) flat in uint inBaseTextureIndex;
layout (location = 8) flat in uint inNormalTextureIndex;
layout (location = 9) flat in uint inMetallicRoughnessTextureIndex;

layout(binding = 1, set = 0) uniform sampler2D textures[15];

layout (location = 0) out vec4 outFragColor;

vec4 getColor()
{
    vec4 color = inBaseColorFactor;

	if (inBaseTextureIndex != -1)
		color = texture(textures[uint(round(inBaseTextureIndex))], inUV);

	return color;
}

vec3 getNormal()
{
    vec3 normal = inNormal;

	if (inNormalTextureIndex != -1)
		normal = texture(textures[uint(round(inNormalTextureIndex))], inUV).xyz * 2.0 - 1.0;

	return normal;
}

vec3 getPBR()
{
	vec3 pbr = vec3(inMetallicFactor, inRoughnessFactor, 0.0);

	if (inMetallicRoughnessTextureIndex != -1)
		pbr = texture(textures[uint(round(inMetallicRoughnessTextureIndex))], inUV).xyz;

	return pbr;
}

void main()
{
	vec3 N = normalize(getNormal());
	vec3 L = normalize(inLightVec);
	vec3 V = normalize(inViewVec);
	vec3 R = reflect(-L, N);
	vec3 ambient = vec3(0.25);
	vec3 diffuse = max(dot(N, L), 0.0) * vec3(0.75) * getPBR();
	vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * vec3(0.75);

	vec4 base_color = getColor();
	outFragColor = vec4(ambient * base_color.rgb + specular, base_color.a);
}
