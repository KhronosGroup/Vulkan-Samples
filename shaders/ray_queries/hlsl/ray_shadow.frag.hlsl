/* Copyright (c) 2024, Sascha Willems
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

[[vk::binding(0, 0)]]
RaytracingAccelerationStructure topLevelAS : register(t0);

struct GlobalUniform
{
	float4x4 view;
	float4x4 proj;
	float4 camera_position;
	float4 light_position;
};
[[vk::binding(1, 0)]]
ConstantBuffer<GlobalUniform> global_uniform : register(b1);

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float4 O_Pos : TEXCOORD0;
    [[vk::location(1)]] float3 O_Normal : NORMAL0;
	[[vk::location(2)]] float4 Scene_pos : TEXCOORD1;	// scene with respect to BVH coordinates
};

/**
Calculate ambient occlusion
*/
float calculate_ambient_occlusion(float3 object_point, float3 object_normal)
{
	const float ao_mult = 1;
	uint max_ao_each = 3;
	uint max_ao = max_ao_each * max_ao_each;
	const float max_dist = 2;
	const float tmin = 0.01, tmax = max_dist;
	float accumulated_ao = 0.f;
	float3 u = abs(dot(object_normal, float3(0, 0, 1))) > 0.9 ? cross(object_normal, float3(1, 0, 0)) : cross(object_normal, float3(0, 0, 1));
	float3 v = cross(object_normal, u);
	float accumulated_factor = 0;
	for (uint j = 0; j < max_ao_each; ++j)
	{
		float phi = 0.5*(-3.14159 + 2 * 3.14159 * (float(j + 1) / float(max_ao_each + 2)));
		for (uint k = 0; k < max_ao_each; ++k){
			float theta =  0.5*(-3.14159 + 2 * 3.14159 * (float(k + 1) / float(max_ao_each + 2)));
			float x = cos(phi) * sin(theta);
			float y = sin(phi) * sin(theta);
			float z = cos(theta);
			float3 direction = x * u + y * v + z * object_normal;

			RayDesc ray;
			ray.TMin = tmin;
			ray.TMax = tmax;
			ray.Origin = object_point;
			ray.Direction = direction.xyz;

			RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;
			query.TraceRayInline(topLevelAS, 0, 0xFF, ray);
			query.Proceed();
			float dist = max_dist;
			if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
			{
				dist = query.CommittedRayT();
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
bool intersects_light(float3 light_origin, float3 pos)
{
	const float tmin = 0.01, tmax = 1.0;
	const float3 direction = light_origin - pos;

	RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;

	RayDesc ray;
	ray.TMin = tmin;
	ray.TMax = tmax;
	ray.Origin = pos;
	ray.Direction = direction.xyz;

	// The following runs the actual ray query
	// For performance, use gl_RayFlagsTerminateOnFirstHitEXT, since we only need to know
	// whether an intersection exists, and not necessarily any particular intersection
	query.TraceRayInline(topLevelAS, 0, 0xFF, ray);

	// The following is the canonical way of using ray Queries from the fragment shader when
	// there's more than one bounce or hit to traverse:
	// while (rayQueryProceedEXT(query)) { }
	// This sample has set flags to gl_RayFlagsTerminateOnFirstHitEXT which means that there
	// will never be a bounce and no need for an expensive while loop.  (i.e. we only need to call it once).
	query.Proceed();
	if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
	{
		// e.g. to get distance:
		// const float dist = rayQueryGetIntersectionTEXT(query, false);
		return true;		
	}

	return false;
}

float4 main(VSOutput input) : SV_TARGET0
{
	// this is where we apply the shadow
	const float ao = calculate_ambient_occlusion(input.Scene_pos.xyz, input.O_Normal);
	const float4 lighting = intersects_light(global_uniform.light_position.xyz, input.Scene_pos.xyz) ? float4(0.2, 0.2, 0.2, 1.0) : float4(1.0, 1.0, 1.0, 1.0);
	return lighting * float4(ao * float3(1, 1, 1), 1);
}
