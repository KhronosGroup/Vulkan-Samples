#version 450
/* Copyright (c) 2020-2023, Sascha Willems
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

layout (binding = 1) uniform sampler2D samplerEnvMap;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inPos;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inViewVec;
layout (location = 4) in vec3 inLightVec;

layout (location = 0) out vec4 outColor0;
layout (location = 1) out vec4 outColor1;

layout(push_constant) uniform Push_Constants {
	vec4 offset;
	vec4 color;
	int object_type;
} push_constants;

void main() 
{
	vec4 color;

	switch (push_constants.object_type) {
		case 0: // Skysphere			
			{
				color = texture(samplerEnvMap, vec2(inUV.s, 1.0 - inUV.t));
			}
			break;
		
		case 1: // Phong shading
			{
				vec3 ambient = push_constants.color.rgb * vec3(0.5);
				vec3 N = normalize(inNormal);
				vec3 L = normalize(inLightVec);
				vec3 V = normalize(inViewVec);
				vec3 R = reflect(-L, N);
				vec3 diffuse = max(dot(N, L), 0.0) * push_constants.color.rgb;
				vec3 specular = vec3(pow(max(dot(R, V), 0.0), 8.0));
				color = vec4(ambient + diffuse + specular, 1.0);	
			}
			break;
	}

	// Base color into attachment 0
	outColor0.rgb = color.rgb;

	// Bright parts for bloom into attachment 1
	float l = dot(outColor0.rgb, vec3(0.2126, 0.7152, 0.0722));
	float threshold = 0.75;
	outColor1.rgb = (l > threshold) ? outColor0.rgb : vec3(0.0);
	outColor1.a = 1.0;
}