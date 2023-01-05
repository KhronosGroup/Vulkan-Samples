#version 450
/* Copyright (c) 2019, Sascha Willems
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

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;

layout (location = 0) out vec3 outPos;
layout (location = 1) out vec3 outNormal;

layout (set = 0, binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
	vec4 lightPos;
	float tessellationFactor;

} ubo; 



void main(void)
{
	gl_Position =  vec4(inPos, 1.0);
	outNormal = mat3(ubo.modelview) * inNormal;
	outPos = inPos;
	
	
	
	
}