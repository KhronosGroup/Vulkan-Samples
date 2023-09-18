#version 450
/* Copyright (c) 2021, Sascha Willems
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

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec4 inVel;

layout (location = 0) out float outGradientPos;

layout (binding = 2) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
	vec2 screendim;
} ubo;

void main () 
{
	const float spriteSize = 0.005 * inPos.w; // Point size influenced by mass (stored in inPos.w);

	vec4 eyePos = ubo.modelview * vec4(inPos.x, inPos.y, inPos.z, 1.0); 
	vec4 projectedCorner = ubo.projection * vec4(0.5 * spriteSize, 0.5 * spriteSize, eyePos.z, eyePos.w);
	gl_PointSize = clamp(ubo.screendim.x * projectedCorner.x / projectedCorner.w, 1.0, 128.0);
	
	gl_Position = ubo.projection * eyePos;

	outGradientPos = inVel.w;
}