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
	float4x4 modelview;
	float4 lightPos;
	float4 frustumPlanes[6];
	float displacementFactor;
	float tessellationFactor;
	float2 viewportDim;
	float tessellatedEdgeSize;
};
[[vk::binding(0, 0)]]
ConstantBuffer<UBO> ubo : register(b0);

Texture2D displacementMapTexture : register(t1);
SamplerState displacementMapSampler : register(s1);

struct HSOutput
{
[[vk::location(2)]]	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
};

struct ConstantsHSOutput
{
    float TessLevelOuter[4] : SV_TessFactor;
    float TessLevelInner[2] : SV_InsideTessFactor;
};

struct DSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
[[vk::location(2)]] float3 ViewVec : TEXCOORD1;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
[[vk::location(4)]] float3 EyePos : POSITION1;
[[vk::location(5)]] float3 WorldPos : POSITION0;
};

[domain("quad")]
DSOutput main(ConstantsHSOutput input, float2 TessCoord : SV_DomainLocation, const OutputPatch<HSOutput, 4> patch)
{
	// Interpolate UV coordinates
	DSOutput output = (DSOutput)0;
	float2 uv1 = lerp(patch[0].UV, patch[1].UV, TessCoord.x);
	float2 uv2 = lerp(patch[3].UV, patch[2].UV, TessCoord.x);
	output.UV = lerp(uv1, uv2, TessCoord.y);

	float3 n1 = lerp(patch[0].Normal, patch[1].Normal, TessCoord.x);
	float3 n2 = lerp(patch[3].Normal, patch[2].Normal, TessCoord.x);
	output.Normal = lerp(n1, n2, TessCoord.y);

	// Interpolate positions
	float4 pos1 = lerp(patch[0].Pos, patch[1].Pos, TessCoord.x);
	float4 pos2 = lerp(patch[3].Pos, patch[2].Pos, TessCoord.x);
	float4 pos = lerp(pos1, pos2, TessCoord.y);
	// Displace
	pos.y -= displacementMapTexture.SampleLevel(displacementMapSampler, output.UV, 0.0).r * ubo.displacementFactor;
	// Perspective projection
	output.Pos = mul(ubo.projection, mul(ubo.modelview, pos));

	// Calculate vectors for lighting based on tessellated position
	output.ViewVec = -pos.xyz;
	output.LightVec = normalize(ubo.lightPos.xyz + output.ViewVec);
	output.WorldPos = pos.xyz;
	output.EyePos = mul(ubo.modelview, pos).xyz;
	return output;
}