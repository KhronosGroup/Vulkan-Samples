#version 320 es
/* Copyright (c) 2025, Arm Limited and Contributors
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

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec3 i_normal;
layout(location = 2) in vec2 i_texcoord_0;

layout(set = 0, binding = 0) uniform GlobalUniform
{
	mat4 projection;
	mat4 modelview;
}
global_uniform;

layout(location = 0) out vec2 o_uv;
layout(location = 1) out vec3 o_normal;

void main(void)
{
	o_uv = i_texcoord_0;

	o_normal = mat3(global_uniform.modelview) * i_normal;

	gl_Position = global_uniform.projection * global_uniform.modelview * vec4(i_position, 1.0);
}
