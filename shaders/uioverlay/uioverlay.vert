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

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec4 inColor;

layout (push_constant) uniform PushConstants {
	mat4 transform;
} pushConstants;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec4 outColor;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	outUV = inUV;
	outColor = inColor;
	gl_Position = pushConstants.transform * vec4(inPos.xy, 0.0, 1.0);
}
