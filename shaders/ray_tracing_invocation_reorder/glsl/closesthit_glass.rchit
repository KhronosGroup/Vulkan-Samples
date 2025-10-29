/* Copyright (c) 2025, Holochip Inc.
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

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec3 attribs;

void main()
{
	// Glass material: cyan/blue tint
	const vec3 baseColor = vec3(0.2, 0.6, 0.9);
	
	// Simple diffuse lighting based on geometry normal
	const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
	vec3 normal = normalize(vec3(barycentrics.x - 0.5, barycentrics.y - 0.5, 0.7));
	
	vec3 lightDir = normalize(vec3(1.0, 1.0, -1.0));
	float NdotL = max(dot(normal, lightDir), 0.0);
	
	// Glass has more specular highlights
	vec3 viewDir = -gl_WorldRayDirectionEXT;
	vec3 halfVec = normalize(lightDir + viewDir);
	float NdotH = max(dot(normal, halfVec), 0.0);
	float specular = pow(NdotH, 32.0);
	
	hitValue = baseColor * (0.3 + 0.5 * NdotL) + vec3(0.8) * specular;
}
