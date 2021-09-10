#version 450
/* Copyright (c) 2020-2021, Holochip
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

#extension GL_EXT_fragment_shading_rate : require

layout (binding = 1) uniform sampler2D samplerEnvMap;
layout (binding = 2) uniform sampler2D samplerSphere;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inPos;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inViewVec;
layout (location = 4) in vec3 inLightVec;

layout (binding = 0) uniform UBO {
	mat4 projection;
	mat4 modelview;
	mat4 skybox_modelview;
	int color_shading_rates;
} ubo;

layout (location = 0) out vec4 outColor;
layout (location = 1) out uvec2 outFrequency;

layout(push_constant) uniform Push_Constants {
	vec4 offset;
	int object_type;
} push_constants;

void main() 
{
	vec4 color;
	float freq_x =0, freq_y = 0;

	switch (push_constants.object_type) {
		case 0: // Skysphere			
			{
				color = texture(samplerEnvMap, vec2(inUV.s, 1.0 - inUV.t));
				vec3 dx = dFdx(color.xyz);
				vec3 dy = dFdx(color.xyz);
				freq_x = dot(dx, dx);
				freq_y = dot(dy, dy);
			}
			break;
		
		case 1: // Phong shading
			{
				vec4 tex_value = texture(samplerSphere, inUV);
				vec3 ambient = tex_value.rgb;
				vec3 N = normalize(inNormal);
				vec3 L = normalize(inLightVec);
				vec3 V = normalize(inViewVec);
				vec3 R = reflect(-L, N);
				vec3 diffuse = vec3(max(dot(N, L), 0.0));
				vec3 specular = vec3(pow(max(dot(R, V), 0.0), 8.0));
				color = vec4(ambient + diffuse + specular, 1.0);
				vec3 dx = dFdx(color.xyz);
				vec3 dy = dFdy(color.xyz);
				freq_x = dot(dx, dx);
				freq_y = dot(dy, dy);
			}
			break;
	}

	if (ubo.color_shading_rates == 1) {
		// Visualize fragment shading rates

		int v = 1;
		int h = 1;
	
		if ((gl_ShadingRateEXT & gl_ShadingRateFlag2VerticalPixelsEXT) == gl_ShadingRateFlag2VerticalPixelsEXT) {
			v = 2;
		}
		if ((gl_ShadingRateEXT & gl_ShadingRateFlag4VerticalPixelsEXT) == gl_ShadingRateFlag4VerticalPixelsEXT) {
			v = 4;
		}
		if ((gl_ShadingRateEXT & gl_ShadingRateFlag2HorizontalPixelsEXT) == gl_ShadingRateFlag2HorizontalPixelsEXT) {
			h = 2;
		}
		if ((gl_ShadingRateEXT & gl_ShadingRateFlag4HorizontalPixelsEXT) == gl_ShadingRateFlag4HorizontalPixelsEXT) {
			h = 4;
		}

		outColor = vec4(vec3(1, 1, 1)*(1 - h * v / 16.0), 1);
		/*if (v == 1 && h == 1) {
			outColor = vec4(color.rrr * 1.0, 1.0);
		} else {
 			outColor = vec4(color.rrr * 1.0 - ((v+h) * 0.05), 1.0);
		}*/
	} else {
		outColor = vec4(color.rgb, 1.0);
	}
	outFrequency = uvec2(255 * freq_x, 255 * freq_y);
}