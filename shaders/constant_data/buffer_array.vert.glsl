#version 320 es
/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

 #define SCENE_MESH_COUNT 245

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord_0;
layout(location = 2) in vec3 normal;

struct MVPUniform
{
	mat4 model;
	mat4 view_proj;

#ifdef PUSH_CONSTANT_LIMIT_256
	mat4 scale;
	mat4 padding;
#endif
};

layout(set = 0, binding = 1) buffer MVPUniformArray
{
	MVPUniform uniform_data[SCENE_MESH_COUNT];
} mvp_array;

layout (location = 0) out vec4 o_pos;
layout (location = 1) out vec2 o_uv;
layout (location = 2) out vec3 o_normal;

void main(void)
{
#ifdef PUSH_CONSTANT_LIMIT_256
    o_pos = mvp_array.uniform_data[gl_InstanceIndex].model * mvp_array.uniform_data[gl_InstanceIndex].scale * vec4(position, 1.0);
#else
    o_pos = mvp_array.uniform_data[gl_InstanceIndex].model * vec4(position, 1.0);
#endif

    o_uv = texcoord_0;

    o_normal = mat3(mvp_array.uniform_data[gl_InstanceIndex].model) * normal;

    gl_Position = mvp_array.uniform_data[gl_InstanceIndex].view_proj * o_pos;
}
