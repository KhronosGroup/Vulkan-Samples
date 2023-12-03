#version 450
/* Copyright (c) 2019-2023, Sascha Willems
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
layout (location = 2) in vec2 inUV;

layout (set = 0, binding = 0) uniform UBOScene {
	mat4 projection;
	mat4 view;
} uboCamera;

layout (set = 0, binding = 1) uniform UBOModel {
	mat4 local;
} uboModel;

layout (location = 0) out vec2 outUV;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() 
{
	outUV = inUV;
	gl_Position = uboCamera.projection * uboCamera.view * uboModel.local * vec4(inPos.xyz, 1.0);
}