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
#version 450
#define PI 3.14159
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outNormal;

layout(binding = 0) uniform UBO
{
	mat4  projection;
	mat4  view;
}
ubo;

layout(push_constant) uniform Push_Constants
{
	vec3 direction;
}
push_constants;


void main(void)
{
	mat4 rotX = mat4(1.0f);
	rotX[1][1] = cos(PI);
	rotX[1][2] = sin(PI);
	rotX[2][1] = -sin(PI);
	rotX[2][2] = cos(PI);
	vec4 pos = rotX * vec4(inPos + push_constants.direction, 1.0f);
	gl_Position = pos;
	outNormal   = mat3(ubo.view) * inNormal;
	outPos      = vec3(pos.xyz) + push_constants.direction;
}
