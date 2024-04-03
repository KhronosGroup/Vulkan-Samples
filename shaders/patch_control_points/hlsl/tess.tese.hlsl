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

struct UBO
{
	float4x4 projection;
	float4x4 view;
};
[[vk::binding(0, 0)]]
ConstantBuffer<UBO> ubo : register(b0);

struct HSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Color : COLOR0;
};

struct ConstantsHSOutput
{
    float TessLevelOuter[4] : SV_TessFactor;
    float TessLevelInner[2] : SV_InsideTessFactor;
};

struct DSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Color : COLOR0;
};

/// layout(triangles, equal_spacing, cw) in;

float4 interpolate3D(float4 v0, float4 v1, float4 v2, float3 TessCoord)
{
	return TessCoord.x * v0 + TessCoord.y * v1 + TessCoord.z * v2;
}

[domain("tri")]
DSOutput main(ConstantsHSOutput input, float3 TessCoord : SV_DomainLocation, const OutputPatch<HSOutput, 3> patch)
{
	DSOutput output = (DSOutput)0;
	
	float4 pos = interpolate3D(patch[0].Pos, patch[1].Pos, patch[2].Pos, TessCoord);
	
	output.Pos = mul(ubo.projection, mul(ubo.view, pos));
	output.Color = patch[0].Color;

	return output;
}