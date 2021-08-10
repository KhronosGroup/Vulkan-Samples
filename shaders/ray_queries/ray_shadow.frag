#version 460
#extension GL_EXT_ray_query : enable

/* Copyright (c) 2021 Holochip Corporation
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

layout(location = 0) in vec4 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec4 in_scene_pos;

layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;

layout(set = 0, binding = 1) uniform GlobalUniform
{
	mat4 model;
	mat4 view_proj;
	vec3 camera_position;
	vec3 light_position;
}
global_uniform;

/**
Apply ray tracing to determine whether the point intersects light
*/
bool intersects_light(vec3 light_origin, vec3 pos)
{
	const float tmin = 0.01, tmax = 1000;
	const vec3  direction = light_origin - pos;
	const float distance  = sqrt(dot(direction, direction));

	rayQueryEXT query;

	// The following runs the actual ray query
	// For performance, use gl_RayFlagsTerminateOnFirstHitEXT, since we only need to know
	// whether an intersection exists, and not necessarily any particular intersection
	rayQueryInitializeEXT(query, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, pos, tmin, direction.xyz, distance);
	// The following is the cononical way of using ray Queries from the fragment shader when
	// there's more than one bounce or hit to traverse:
	// while (rayQueryProceedEXT(query)) { }
	// This sample has set flags to gl_RayFlagsTerminateOnFirstHitEXT which means that there
	// will never be a bounce and no need for an expensive while loop.  (i.e. we only need to call it once).
	rayQueryProceedEXT(query);
	if (rayQueryGetIntersectionTypeEXT(query, true) != gl_RayQueryCommittedIntersectionNoneEXT)
	{
		// e.g. to get distance:
		// const float dist = rayQueryGetIntersectionTEXT(query, false);
		return true;
	}
	return false;
}

void main(void)
{
	// this is where we apply the shadow
	o_color = intersects_light(global_uniform.light_position, in_scene_pos.xyz) ? vec4(0.2, 0.2, 0.2, 1) : vec4(0.9, 0.9, 0.9, 1);
}
