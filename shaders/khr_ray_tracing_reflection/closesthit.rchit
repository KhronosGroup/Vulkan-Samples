/* Copyright (c) 2019-2020, Sascha Willems
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

#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) rayPayloadInEXT vec3 hitValue;
layout(location = 1) rayPayloadEXT bool isShadowed;

hitAttributeEXT vec3 attribs;

struct WaveFrontMaterial
{
	vec3  diffuse;
	float shininess;
};

struct Vertex
{
	vec4 pos;
	vec4 nrm;
};

// clang-format off
layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 0, binding = 3, scalar) buffer _materials { WaveFrontMaterial m[]; } materials[];
layout(set = 0, binding = 4, scalar) buffer _vertices { Vertex v[]; } vertices[];
layout(set = 0, binding = 5)         buffer _indices { uint i[]; } indices[];
// clang-format on

void main()
{
	int objId = gl_InstanceCustomIndexEXT;

	WaveFrontMaterial mat = materials[objId].m[0];

	// Indices of the triangle
	ivec3 ind = ivec3(indices[nonuniformEXT(objId)].i[3 * gl_PrimitiveID + 0],         //
	                  indices[nonuniformEXT(objId)].i[3 * gl_PrimitiveID + 1],         //
	                  indices[nonuniformEXT(objId)].i[3 * gl_PrimitiveID + 2]);        //
	// Vertex of the triangle
	Vertex v0 = vertices[nonuniformEXT(objId)].v[ind.x];
	Vertex v1 = vertices[nonuniformEXT(objId)].v[ind.y];
	Vertex v2 = vertices[nonuniformEXT(objId)].v[ind.z];

	const vec3 barycentrics = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

	// Computing the normal at hit position
	vec3 N = v0.nrm.xyz * barycentrics.x + v1.nrm.xyz * barycentrics.y + v2.nrm.xyz * barycentrics.z;
	N      = normalize(vec3(N.xyz * gl_WorldToObjectEXT));        // Transforming the normal to world space

	// Computing the coordinates of the hit position
	vec3 P = v0.pos.xyz * barycentrics.x + v1.pos.xyz * barycentrics.y + v2.pos.xyz * barycentrics.z;
	P      = vec3(gl_ObjectToWorldEXT * vec4(P, 1.0));        // Transforming the position to world space

	// Hardocded (to) light direction
	vec3 L = normalize(vec3(1, 1, 1));

	float NdotL = dot(N, L);

	// Fake Lambertian to avoid black
	vec3 result = mat.diffuse * max(NdotL, 0.1);

	//	// Tracing shadow ray only if the light is visible from the surface
	//	if (NdotL > 0)
	//	{
	//		float tMin   = 0.001;
	//		float tMax   = 1e32;        // infinite
	//		vec3  origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
	//		vec3  rayDir = L;
	//		uint  flags  = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
	//		isShadowed   = true;
	//		traceRayEXT(topLevelAS,        // acceleration structure
	//		            flags,             // rayFlags
	//		            0xFF,              // cullMask
	//		            0,                 // sbtRecordOffset
	//		            0,                 // sbtRecordStride
	//		            1,                 // missIndex
	//		            origin,            // ray origin
	//		            tMin,              // ray min range
	//		            rayDir,            // ray direction
	//		            tMax,              // ray max range
	//		            1                  // payload (location = 1)
	//		);
	//
	//		if (isShadowed)
	//		{
	//			result *= 0.3;
	//		}
	//	}

	hitValue = result;
}
