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

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;

layout(binding = 0) uniform UBO
{
	mat4 projection;
	mat4 view;
}
ubo;

layout(location = 0) out vec3 outUVW;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	outUVW      = inPos;
	gl_Position = ubo.projection * vec4(mat3(ubo.view) * (inPos * 10), 1.0);
}
