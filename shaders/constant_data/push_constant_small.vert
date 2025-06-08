#version 320 es
/* Copyright (c) 2019-2025, Arm Limited and Contributors
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

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord_0;
layout(location = 2) in vec3 normal;

layout(push_constant, std430) uniform MVPUniform
{
	mat4 model;
	mat4 view_proj;
} mvp_uniform;

layout (location = 0) out vec4 o_pos;
layout (location = 1) out vec2 o_uv;
layout (location = 2) out vec3 o_normal;

void main(void)
{
    o_pos = mvp_uniform.model * vec4(position, 1.0);
    o_uv = texcoord_0;
    o_normal = mat3(mvp_uniform.model) * normal;
    gl_Position = mvp_uniform.view_proj * o_pos;
}
