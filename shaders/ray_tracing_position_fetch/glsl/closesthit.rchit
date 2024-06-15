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

#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable

// This extension is required for fetching position data in the closest hit shader
#extension GL_EXT_ray_tracing_position_fetch : require

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec2 attribs;

layout(binding = 2, set = 0) uniform UBO 
{
	mat4 viewInverse;
	mat4 projInverse;
	int displayMode;
} ubo;

void main()
{
	// We need the barycentric coordinates to calculate data for the current position
	const vec3 barycentricCoords = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

	// With VK_KHR_ray_tracing_position_fetch we can access the vertices for the hit triangle in the shader
	vec3 pos0 = gl_HitTriangleVertexPositionsEXT[0];
	vec3 pos1 = gl_HitTriangleVertexPositionsEXT[1];
	vec3 pos2 = gl_HitTriangleVertexPositionsEXT[2];
	vec3 currentPos = pos0 * barycentricCoords.x + pos1 * barycentricCoords.y + pos2 * barycentricCoords.z;

	hitValue = vec3(0.0);

	switch (ubo.displayMode) {
		case(0): {
			// Visualize the geometric normal
			vec3 normal = normalize(cross(pos1 - pos0, pos2 - pos0));
			normal = normalize(vec3(normal * gl_WorldToObjectEXT));
			hitValue = normal;
			break;
		}
		case(1): {
			// Visualize the vertex position
			hitValue = currentPos;
			break;
		}
	}
}