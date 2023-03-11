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

#extension GL_EXT_fragment_shader_barycentric : require

layout (binding = 1) uniform sampler2D samplerColorMap;
layout (location = 0) in pervertexEXT vec3 inColor[]; 

layout (location = 0) out vec4 outColor;

layout( push_constant ) uniform Push_Constants{
	int type;
} pushConstant;

void main() 
{
	switch (pushConstant.type) {
		case 0:
		{
			outColor.rgb = inColor[0].rgb * gl_BaryCoordEXT.x +
				inColor[1].rgb * gl_BaryCoordEXT.y +
				inColor[2].rgb * gl_BaryCoordEXT.z;
			outColor.a = 1.0;
			break;
		}
		case 1: 
		{
			outColor.rgb = gl_BaryCoordEXT - gl_BaryCoordNoPerspEXT;
			const float exposure = 10.f;
			outColor = vec4(vec3(1.0) - exp(-outColor.rgb * exposure), 1.0);
			break;
		}
		case 2:
		{
			if(gl_BaryCoordEXT.x < 0.01 || gl_BaryCoordEXT.y < 0.01 || gl_BaryCoordEXT.z < 0.01) 
				outColor = vec4(0.0, 0.0, 0.0, 1.0);
			else
				outColor = vec4(0.5, 0.5, 0.5, 1.0);
			break;
		}
		case 3:
		{
			if (gl_BaryCoordEXT.x <= gl_BaryCoordEXT.y && gl_BaryCoordEXT.x <= gl_BaryCoordEXT.z)
				outColor = vec4(inColor[0].rgb * gl_BaryCoordEXT.x, 1.0); 
			else if (gl_BaryCoordEXT.y < gl_BaryCoordEXT.x && gl_BaryCoordEXT.y <= gl_BaryCoordEXT.z)
				outColor = vec4(inColor[1].rgb * gl_BaryCoordEXT.y, 1.0); 
			else
				outColor = vec4(inColor[2].rgb * gl_BaryCoordEXT.z, 1.0); 
			break;
		}
		case 4:
		{
			outColor = texture(samplerColorMap, vec2(sin(gl_BaryCoordEXT.x) + cos(2 * gl_BaryCoordEXT.z), sin(gl_BaryCoordEXT.x) + cos(2 * gl_BaryCoordEXT.y)));
		}
	}
}
