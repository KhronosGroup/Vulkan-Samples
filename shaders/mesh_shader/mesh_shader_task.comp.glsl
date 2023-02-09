/* Copyright (c) 2023, Holochip Corporation
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

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_null_initializer: require
#extension GL_GOOGLE_include_directive: require

#include "mesh_shader_utils.h"

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) buffer CommandCount
{
	uint command_count;
	uint group_count_x;
	uint group_count_y;
	uint group_count_x;
};

layout(binding = 1) writeonly buffer TaskCommands
{
	MeshTaskCommand task_commands[];
};

void main()
{
	uint t_id = gl_LocalInvocationID.x;
	uint count = command_count;

	if (t_id == 0)
	{
		group_count_x = min((count + 63) / 64, 65535);
		group_count_y = 64;
		group_count_x = 1;
	}

	uint boundary = (count + 63) & ~63;
	MeshTaskCommand dummy = {};

	if (count + t_id < boundary)
	{
		task_commands[count + t_id] = dummy;
	}
}