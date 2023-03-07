#version 450
/* Copyright (c) 2021-2022, Holochip
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
layout (location = 0) in pervertexEXT vec3 in_color[]; 

layout (location = 0) out vec4 outColor0;

layout( push_constant ) uniform Push_Constants{
  int type;
} PushConstant;
void main() 
{
	switch (PushConstant.type) {
		case 0: // Object
		{
    outColor0.rgb = in_color[0].rgb * gl_BaryCoordEXT.x +
                    in_color[1].rgb * gl_BaryCoordEXT.y +
                    in_color[2].rgb * gl_BaryCoordEXT.z;
		break;
		}
    case 1: 
    {
    outColor0.x = gl_BaryCoordEXT.x - gl_BaryCoordNoPerspEXT.x;
		outColor0.y = gl_BaryCoordEXT.y - gl_BaryCoordNoPerspEXT.y;
		outColor0.z = gl_BaryCoordEXT.z - gl_BaryCoordNoPerspEXT.z;
		const float exposure = 10.f;
		outColor0 = vec4(vec3(1.0) - exp(-outColor0.rgb * exposure), 1.0);
    break;
    }
    case 2:
    {
    if(gl_BaryCoordEXT.x < 0.01 || gl_BaryCoordEXT.y < 0.01 || gl_BaryCoordEXT.z < 0.01) 
		  outColor0 = vec4(0.0, 0.0, 0.0, 1.0);
		else
		  outColor0 = vec4(0.5, 0.5, 0.5, 1.0);
    break;
    }
    case 3:
    {
    if (gl_BaryCoordEXT.x <= gl_BaryCoordEXT.y && gl_BaryCoordEXT.x <= gl_BaryCoordEXT.z)
      outColor0 = vec4(in_color[0].rgb * gl_BaryCoordEXT.x, 1.0); 
    else if (gl_BaryCoordEXT.y < gl_BaryCoordEXT.x && gl_BaryCoordEXT.y <= gl_BaryCoordEXT.z)
      outColor0 = vec4(in_color[1].rgb * gl_BaryCoordEXT.y, 1.0); 
    else
      outColor0 = vec4(in_color[2].rgb * gl_BaryCoordEXT.z, 1.0); 
    break;
    }
    case 4:
    {
      // outColor0 = vec4(texture(samplerColorMap, gl_PointCoord.xy));
    }
	}
}