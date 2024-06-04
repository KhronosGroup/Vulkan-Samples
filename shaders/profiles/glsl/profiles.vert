#version 450
/* Copyright (c) 2022-2024, Sascha Willems
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
layout (location = 1) in vec2 inUV;
layout (location = 2) in int inTexIndex;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 view;
} ubo;

layout (location = 0) out vec2 outUV;
layout (location = 1) flat out int outTexIndex;

void main() 
{
	outUV = inUV;
	outTexIndex = inTexIndex;
	gl_Position = ubo.projection * ubo.view * vec4(inPos.xyz, 1.0);
}
