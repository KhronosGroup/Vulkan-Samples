/* Copyright (c) 2023-2024, Mobica Limited
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
#extension GL_ARB_sparse_texture2 : enable

layout(binding = 1) uniform sampler2D texSampler;

layout(binding = 2) uniform settingsData {
	bool color_highlight;
	int  minLOD;
	int  maxLOD;
} settings;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 fragOutColor;

vec3 color_blend_table[5] = 
{
	{1.00, 1.00, 1.00},
	{0.80, 0.60, 0.40},
	{0.60, 0.80, 0.60},
	{0.40, 0.60, 0.80},
	{0.20, 0.20, 0.20},
};

void main() {

	vec4 color = vec4(0.0);

	int lod = settings.minLOD;
	int residencyCode = sparseTextureLodARB(texSampler, fragTexCoord, lod, color);

	for(++lod; (lod <= settings.maxLOD) && !sparseTexelsResidentARB(residencyCode); ++lod)
	{
		residencyCode = sparseTextureLodARB(texSampler, fragTexCoord, lod, color);
	}	

	if (settings.color_highlight)
	{
		lod -= 1;
		color.xyz = (color.xyz * color_blend_table[lod]);
	}

	fragOutColor = color;
}