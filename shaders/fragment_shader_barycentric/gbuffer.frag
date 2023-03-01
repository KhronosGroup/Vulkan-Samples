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

layout (binding = 1) uniform samplerCube samplerEnvMap;
layout (binding = 2) uniform sampler2D samplerColorMap;

layout (location = 0) in vec3 inUVW;
layout (location = 1) in vec3 inPos;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec2 inUV; // needed for 2D texture

layout (location = 0) out vec4 outColor0;

layout (constant_id = 0) const int type = 0;

#define PI 3.1415926
#define TwoPI (2.0 * PI)


void main() 
{
	vec4 color;

	switch (type) {
		case 0: // Skybox			
			{
				vec3 normal = normalize(inUVW);
				color = texture(samplerEnvMap, normal);
				// Color with manual exposure into attachment 0
				const float exposure = 1.f;
				outColor0.rgb = vec3(1.0) - exp(-color.rgb * exposure);
			}
			break;
		case 1: // object
			{
				 // outColor0 = texture(samplerColorMap, gl_BaryCoordNoPerspEXT.xy);
				 vec3 baryCoord = gl_BaryCoordNoPerspEXT;
                float edge = min(baryCoord.x, min(baryCoord.y, baryCoord.z));
                float edgeThickness = 0.25f;
                float wireframe = smoothstep(0, edgeThickness, edge);
                if (wireframe > edgeThickness) discard;
                outColor0 = vec4(wireframe, wireframe, wireframe, 1.0f);
			}
			break;
	}
}