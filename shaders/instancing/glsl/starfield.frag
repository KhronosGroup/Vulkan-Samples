#version 450
/* Copyright (c) 2019-2024, Sascha Willems
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

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outFragColor;

#define HASHSCALE3 vec3(443.897, 441.423, 437.195)
#define STARFREQUENCY 0.01

// Hash function by Dave Hoskins (https://www.shadertoy.com/view/4djSRW)
float hash33(vec3 p3)
{
	p3 = fract(p3 * HASHSCALE3);
	p3 += dot(p3, p3.yxz+vec3(19.19));
	return fract((p3.x + p3.y)*p3.z + (p3.x+p3.z)*p3.y + (p3.y+p3.z)*p3.x);
}

vec3 starField(vec3 pos)
{
	vec3 color = vec3(0.0);
	float threshhold = (1.0 - STARFREQUENCY);
	float rnd = hash33(pos);
	if (rnd >= threshhold)
	{
		float starCol = pow((rnd - threshhold) / (1.0 - threshhold), 16.0);
		color += vec3(starCol);
	}	
	return color;
}

void main() 
{
	outFragColor = vec4(starField(inUVW), 1.0);
}