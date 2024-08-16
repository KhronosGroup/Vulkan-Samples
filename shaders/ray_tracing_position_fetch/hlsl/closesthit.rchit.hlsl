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

struct Attributes
{
    float2 bary;
};

struct Payload
{
    [[vk::location(0)]] float3 hitValue;
};

struct UBO
{
    float4x4 viewInverse;
    float4x4 projInverse;
    int displayMode;
};
ConstantBuffer<UBO> ubo : register(b2);

// We need to use special syntax for SPIR-V inlines
#define HitTriangleVertexPositionsKHR 5335
#define RayTracingPositionFetchKHR 5336

[[vk::ext_builtin_input(HitTriangleVertexPositionsKHR)]]
const static float3 gl_HitTriangleVertexPositions[3];

[[vk::ext_extension("SPV_KHR_ray_tracing_position_fetch")]]
[[vk::ext_capability(RayTracingPositionFetchKHR)]]
[shader("closesthit")]
void main(inout Payload p, in Attributes attribs)
{
	// We need the barycentric coordinates to calculate data for the current position
    const float3 barycentricCoords = float3(1.0f - attribs.bary.x - attribs.bary.y, attribs.bary.x, attribs.bary.y);

	// With VK_KHR_ray_tracing_position_fetch we can access the vertices for the hit triangle in the shader

    float3 pos0 = gl_HitTriangleVertexPositions[0];
    float3 pos1 = gl_HitTriangleVertexPositions[1];
    float3 pos2 = gl_HitTriangleVertexPositions[2];
    float3 currentPos = pos0 * barycentricCoords.x + pos1 * barycentricCoords.y + pos2 * barycentricCoords.z;
   
    p.hitValue = float3(0.0, 0.0, 0.0);
    
    switch (ubo.displayMode)
    {
        case (0):{
			// Visualize the geometric normal
            float3 normal = normalize(cross(pos1 - pos0, pos2 - pos0));
            normal = normalize(mul(float4(normal, 1.0), WorldToObject4x3()));
            p.hitValue = normal;
            break;
        }
        case (1):{
			// Visualize the vertex position
            p.hitValue = currentPos;
            break;
        }
    }
}