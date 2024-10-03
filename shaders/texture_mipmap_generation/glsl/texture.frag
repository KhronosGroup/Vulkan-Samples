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

layout (set = 0, binding = 1) uniform texture2D textureColor;
layout (set = 0, binding = 2) uniform sampler samplers[3];

layout (set = 0, binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
	float lodBias;
	int samplerIndex;
} ubo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{	
	vec3 color = texture(sampler2D(textureColor, samplers[ubo.samplerIndex]), inUV * vec2(2.0, 0.25), ubo.lodBias).rgb;
	outFragColor = vec4(color, 1.0);
}