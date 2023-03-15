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

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inViewVec;
layout (location = 3) in vec3 inLightVec;
layout (location = 4) flat in vec3 inFlatNormal;

layout (constant_id = 0) const int LIGHTING_MODEL = 0;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	switch (LIGHTING_MODEL) {
		case 0: // Phong			
		{
			vec3 ambient = inColor * vec3(0.25);
			vec3 N = normalize(inNormal);
			vec3 L = normalize(inLightVec);
			vec3 V = normalize(inViewVec);
			vec3 R = reflect(-L, N);
			vec3 diffuse = max(dot(N, L), 0.0) * inColor;
			vec3 specular = pow(max(dot(R, V), 0.0), 32.0) * vec3(0.75);
			outFragColor = vec4(ambient + diffuse * 1.75 + specular, 1.0);		
			break;
		}
		case 1: // Toon
		{

			vec3 N = normalize(inNormal);
			vec3 L = normalize(inLightVec);
			float intensity = dot(N,L);
			vec3 color;
			if (intensity > 0.98)
				color = inColor * 1.5;
			else if  (intensity > 0.9)
				color = inColor * 1.0;
			else if (intensity > 0.5)
				color = inColor * 0.6;
			else if (intensity > 0.25)
				color = inColor * 0.4;
			else
				color = inColor * 0.2;
			// Desaturate a bit
			color = vec3(mix(color, vec3(dot(vec3(0.2126,0.7152,0.0722), color)), 0.25));	
			outFragColor.rgb = color;
			break;
		}
		case 2: // No shading
		{
			outFragColor.rgb = inColor;
			break;
		}
	}
}