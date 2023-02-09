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
#extension GL_GOOGLE_include_directive: require

#include "mesh_shader_utils.h"

layout (constant_id = 0) const bool LATE = false;
layout (constant_id = 1) const bool TASK = false;

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout(push_constant) uniform block
{
	DrawCullData cull_data;
};

layout(binding = 0) readonly buffer Draws
{
	MeshDraw draws[];
};

layout(binding = 1) readonly buffer Meshes
{
	Mesh meshes[];
};

layout(binding = 2) writeonly buffer DrawCommands
{
	MeshDrawCommand draw_commands[];
};

layout(binding = 2) writeonly buffer TaskCommands
{
	MeshTaskCommand task_commands[];
};

layout(binding = 3) buffer CommandCount
{
	uint command_count;
};

layout(binding = 4) buffer DrawVisibility
{
	uint draw_visibility[];
};

layout(binding = 5) uniform sampler2D depth_pyramid;





void main()
{
	uint d_index = gl_GlobalInvocationID.x;

	if (d_index >= cull_data.drawCount)
		return;

	if (!LATE && draw_visibility[d_index] == 0)
		return;



	uint mesh_index = draws[d_index].mesh_index;
	Mesh mesh = meshes[mesh_index];

	vec3 center = rotate_quaternion(mesh.center, draws[d_index].orientation) * draws[d_index].scale + draws[d_index].position;
	float radius = mesh.radius * draws[d_index].scale;

	bool visible = true;

	visible = visible && center.z * cull_data.frustum[1] - abs(center.x) * cull_data.frustum[0] > -radius;
	visible = visible && center.z * cull_data.frustum[3] - abs(center.y) * cull_data.frustum[2] > -radius;

	visible = visible && center.z + radius > cull_data.z_near && center.z - radius < cull_data.z_far;

	visible = visible || cull_data.culling_enabled == 0;

//////////

	if (LATE && visible && cull_data.occlusion_enabled == 1)
	{
		vec4 aabb;
		if (project_sphere(center, radius, cull_data.z_near, cull_data.P00, cull_data.P11, aabb))
		{
			float width = (aabb.z - aabb.x) * cull_data.pyramid_width;
			float height = (aabb.w - aabb.y) * cull_data.pyramid_height;

			float level = floor(log2(max(width, height)));

			float depth = textureLod(depth_pyramid, (aabb.xy + aabb.zw) * 0.5f, level).x;
			float depth_sphere = cull_data.z_near / (center.z - radius);

			visible = visible && depth_sphere > depth;
		}
	}

	if (visible && (!LATE || cull_data.cluster_occlusion_enabled == 1 || draw_visibility[d_index] == 0))
	{
		float LoD_index_float = log2( length( center ) / cull_data.LoD_base ) / log2( cull_data.LoD_step );
		uint LoD_index = min( uint ( max( LoD_index_float + 1.0f, 0.0f ) ), mesh.LoD_count - 1 );

		LoD_index = cull_data.lod_enabled == 1 ? LoD_index : 0;

		MeshLoD LoD = meshes[mesh_index].LoDs[LoD_index];

		if (TASK)
		{
			uint task_groups = (LoD.meshletCount + TASK_WG_SIZE - 1) / TASK_WG_SIZE;
			uint dc_index = atomicAdd(command_count, task_groups);

			uint late_draw_visibility = draw_visibility[d_index];
			uint meshlet_visibility_offset = draws[d_index].meshlet_visibility_offset;

			for (uint i = 0; i < task_groups; ++i)
			{
				task_commands[dc_index + i].draw_id = d_index;
				task_commands[dc_index + i].task_offset = LoD.meshletOffset + i * TASK_WG_SIZE;
				task_commands[dc_index + i].task_count = min(TASK_WG_SIZE, LoD.meshletCount - i * TASK_WG_SIZE);
				task_commands[dc_index + i].late_draw_visibility = late_draw_visibility;
				task_commands[dc_index + i].meshlet_visibility_offset = meshlet_visibility_offset + i * TASK_WG_SIZE;
			}
		}
		else
		{
			uint dc_index = atomicAdd(command_count, 1);

			draw_commands[dc_index].draw_id = d_index;
			draw_commands[dc_index].index_count = LoD.indexCount;
			draw_commands[dc_index].instance_count = 1;
			draw_commands[dc_index].first_index = LoD.indexOffset;
			draw_commands[dc_index].vertex_offset = mesh.vertexOffset;
			draw_commands[dc_index].first_instance = 0;
		}
	}

	if (LATE)
		draw_visibility[di] = visible ? 1 : 0;
}
