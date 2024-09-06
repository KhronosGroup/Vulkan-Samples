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

layout (input_attachment_index = 0, binding = 0) uniform subpassInput positionDepthAttachment;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput normalAttachment;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput albedoAttachment;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

struct Light {
	vec4 position;
	vec3 color;
	float radius;
};

layout(std140, binding = 3) readonly buffer LightsBuffer
{
   Light lights[ ];
};


void main() 
{
	// Read G-Buffer values from previous sub pass
	vec3 fragPos = subpassLoad(positionDepthAttachment).rgb;
	vec3 normal = subpassLoad(normalAttachment).rgb;
	vec4 albedo = subpassLoad(albedoAttachment);

	#define ambient 0.005
	
	// Ambient part
	vec3 fragcolor  = albedo.rgb * ambient;
	
	for(int i = 0; i < lights.length(); ++i)
	{
		vec3 L = lights[i].position.xyz - fragPos;
		float dist = length(L);

		float attenuation = lights[i].radius / (pow(dist, 8.0) + 1.0);

		float NdotL = max(0.0, dot(normalize(normal), normalize(L)));
		vec3 diffuse = lights[i].color * albedo.rgb * NdotL * attenuation;

		fragcolor += diffuse;
	}    	
   
	outColor = vec4(fragcolor, 1.0);
}