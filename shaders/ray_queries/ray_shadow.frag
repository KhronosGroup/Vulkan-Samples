#version 460
#extension GL_EXT_ray_query : enable

/* Copyright (c) 2021-2022 Holochip Corporation
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
Calculate ambient occlusion
*/
float calculate_ambient_occlusion(vec3 object_point, vec3 object_normal)
{
	const float ao_mult = 1;
	uint max_ao_each = 3;
	uint max_ao = max_ao_each * max_ao_each;
	const float max_dist = 2;
	const float tmin = 0.01, tmax = max_dist;
	float accumulated_ao = 0.f;
	vec3 u = abs(dot(object_normal, vec3(0, 0, 1))) > 0.9 ? cross(object_normal, vec3(1, 0, 0)) : cross(object_normal, vec3(0, 0, 1));
	vec3 v = cross(object_normal, u);
	float accumulated_factor = 0;
	for (uint j = 0; j < max_ao_each; ++j)
	{
		float phi = 0.5*(-3.14159 + 2 * 3.14159 * (float(j + 1) / float(max_ao_each + 2)));
		for (uint k = 0; k < max_ao_each; ++k){
			float theta =  0.5*(-3.14159 + 2 * 3.14159 * (float(k + 1) / float(max_ao_each + 2)));
			float x = cos(phi) * sin(theta);
			float y = sin(phi) * sin(theta);
			float z = cos(theta);
			vec3 direction = x * u + y * v + z * object_normal;

			rayQueryEXT query;
			rayQueryInitializeEXT(query, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, object_point, tmin, direction.xyz, tmax);
			rayQueryProceedEXT(query);
			float dist = max_dist;
			if (rayQueryGetIntersectionTypeEXT(query, true) != gl_RayQueryCommittedIntersectionNoneEXT)
			{
				dist = rayQueryGetIntersectionTEXT(query, true);
			}
			float ao = min(dist, max_dist);
			float factor = 0.2 + 0.8 * z * z;
			accumulated_factor += factor;
			accumulated_ao += ao * factor;
		}
	}
	accumulated_ao /= (max_dist * accumulated_factor);
	accumulated_ao *= accumulated_ao;
	accumulated_ao = max(min((accumulated_ao) * ao_mult, 1), 0);
	return accumulated_ao;
}

/**
Apply ray tracing to determine whether the point intersects light
*/
bool intersects_light(vec3 light_origin, vec3 pos)
{
	const float tmin = 0.01, tmax = 1000;
	const vec3  direction = light_origin - pos;

	rayQueryEXT query;

	// The following runs the actual ray query
	// For performance, use gl_RayFlagsTerminateOnFirstHitEXT, since we only need to know
	// whether an intersection exists, and not necessarily any particular intersection
	rayQueryInitializeEXT(query, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, pos, tmin, direction.xyz, 1.0);
	// The following is the canonical way of using ray Queries from the fragment shader when
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
	const float ao = calculate_ambient_occlusion(in_scene_pos.xyz, in_normal);
	const vec4 lighting = intersects_light(global_uniform.light_position, in_scene_pos.xyz) ? vec4(0.2, 0.2, 0.2, 1) : vec4(1, 1, 1, 1);
	o_color = lighting * vec4(ao * vec3(1, 1, 1), 1);
}
