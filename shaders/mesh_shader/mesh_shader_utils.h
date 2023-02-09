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

#define TASK_WG_SIZE 64
#define MESH_WG_SIZE 64

// Structures

struct Vertex
{
	float     vx{};
	float     vy{};
	float     vz{};
	uint8_t   nx{};
	uint8_t   ny{};
	uint8_t   nz{};
	uint8_t   nw{};
	float16_t tu{};
	float16_t tv{};
};

struct Meshlet
{
	vec3   center{};
	float  radius{};
	int8_t cone_axis[3];
	int8_t cone_cutoff{};

	uint    data_offset{};
	uint8_t vertex_count{};
	uint8_t triangle_count{};
};

struct Globals
{
	mat4  projection{};
	float screen_width{};
	float screen_height{};
	float z_near{};
	float z_far{};
	float frustum[4]{};
	float pyramid_width{};
	float pyramid_height{};
	int   cluster_occlusion_enabled{};
};

struct DrawCullData
{
	float P00{};
	float P11{};
	float z_near{};
	float z_far{};
	float frustum[4]{};
	float LoD_base{};
	float LoD_Step{};
	float pyramid_width{};
	float pyramid_height{};

	uint draw_count{};

	int culling_enabled{};
	int LoD_enabled{};
	int occlusion_enabled{};
	int cluster_occlusion_enabled{};
};

struct MeshLoD
{
	uint index_offset{};
	uint index_count{};
	uint meshlet_offset{};
	uint meshlet_count{};
};

struct Mesh
{
	vec3  center{};
	float radius{};

	uint vertex_offset{};
	uint vertex_count{};

	uint    LoD_count{};
	MeshLoD LoDs[8]{};
};

struct MeshDraw
{
	vec3  position{};
	float scale{};
	vec4  orientation{};

	uint mesh_index{};
	uint vertex_offset{};
	uint meshlet_visibility_offset{};
};

struct MeshDrawCommand
{
	uint draw_id{};
	uint index_count{};
	uint instance_count{};
	uint first_Index{};
	uint vertex_offset{};
	uint first_instance{};
};

struct MeshTaskCommand
{
	uint draw_id{};
	uint task_offset{};
	uint task_count{};
	uint late_draw_visibility{};
	uint meshlet_visibility_offset{};
};

struct MeshTaskPayload
{
	uint draw_id{};
	uint meshlet_indices[TASK_WG_SIZE]{};
};

// projectSphere
bool project_sphere(vec3 center, float radius, float z_near, float global_projection_00, float global_projection_01, out vec4 AABB)
{
	if (center.z < radius + z_near)
	{
		return false;
	}
	else
	{
		vec3  cr         = center * radius;
		float czr_square = center.z * center.z - radius * radius;

		float visible_x = sqrt(center.x * center.x + czr_square);
		float x_min     = (visible_x * center.x - cr.z) / (visible_x * center.z + cr.x);
		float x_max     = (visible_x * center.x + cr.z) / (visible_x * center.z - cr.x);

		float visible_y = sqrt(center.y * center.y + czr_square);
		float y_min     = (visible_y * center.y - cr.z) / (visible_y * center.z + cr.y);
		float y_max     = (visible_y * center.y + cr.z) / (visible_y * center.z - cr.y);

		AABB = vec4(x_min * global_projection_00, y_min * global_projection_11, x_max * global_projection_00, y_max * global_projection_11);
		AABB = AABB.xwzy * vec4(0.5f, -0.5f, 0.5f, -0.5f) + vec4(0.5f);

		return true;
	}
}

// coneCull
bool cone_cull(vec3 center, float radius, vec3 cone_axis, float cone_cutoff, vec3 camera_position)
{
	bool return_me = dot(center - camera_position, cone_axis) >= (cone_cutoff * length(center - camera_position) + radius);

	return return_me;
}

// rotateQuat
vec3 rotate_quaternion(vec3 v, vec4 q)
{
	vec3 return_me = v + 2.0f * cross(q.xyz, cross(q.xyz, v) + q.w * v);

	return return_me;
}