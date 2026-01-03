/* Copyright (c) 2025-2026, Holochip Inc.
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

struct RayPayload
{
	float3 hitValue;
};

struct HitAttributes
{
	float2 attribs;
};

[shader("closesthit")]
void main(inout RayPayload payload, in HitAttributes attr)
{
	// Metal material: silver/gray color with slight blue tint
	const float3 baseColor = float3(0.75, 0.75, 0.8);
	
	// Simple diffuse lighting based on geometry normal
	const float3 barycentrics = float3(1.0 - attr.attribs.x - attr.attribs.y, attr.attribs.x, attr.attribs.y);
	float3 normal = normalize(float3(barycentrics.x - 0.5, barycentrics.y - 0.5, 0.7));
	
	float3 lightDir = normalize(float3(1.0, 1.0, -1.0));
	float NdotL = max(dot(normal, lightDir), 0.0);
	
	payload.hitValue = baseColor * (0.3 + 0.7 * NdotL);
}
