#version 450
/* Copyright (c) 2026, Sascha Willems
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

layout(push_constant) uniform PushConsts {
	int samplerIndex;
	int frameIndex;
} pushConsts;

layout (set = 0, binding = 0) uniform UBO {
	mat4 projection;
	mat4 view;
	mat4 model[2];
} ubo[2];

layout (location = 0) out vec2 outUV;
layout (location = 1) flat out int outInstanceIndex;

void main() 
{
	outUV = inUV;
	gl_Position = ubo[pushConsts.frameIndex].projection * ubo[pushConsts.frameIndex].view * ubo[pushConsts.frameIndex].model[gl_InstanceIndex] * vec4(inPos.xyz, 1.0);
	outInstanceIndex = gl_InstanceIndex;
}