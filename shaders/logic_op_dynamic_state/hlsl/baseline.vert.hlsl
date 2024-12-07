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
 
struct VSInput
{
    [[vk::location(0)]] float3 Pos : POSITION0;
    [[vk::location(1)]] float3 Normal : NORMAL0;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
};
[[vk::binding(0, 0)]]
ConstantBuffer<UBO> ubo : register(b0);

struct UBOBaseline
{
	float4 ambientLightColor;
	float4 lightPosition;
	float4 lightColor;
	float lightIntensity;
};
[[vk::binding(1, 0)]]
ConstantBuffer<UBOBaseline> ubo_baseline : register(b1);

struct PushConstants
{
    float4x4 model;
    float4 color;
};
[[vk::push_constant]] PushConstants push_constants;

struct VSOutput
{
    float4 Pos : SV_POSITION;
	float3 Normal : NORMAL0;
	float4 Color : TEXCOORD0;
	float3 LightVec : TEXCOORD1;
	float3 LightColor[2] : TEXCOORD2;
	float3 ViewVec : TEXCOORD3;
	float LightIntensity : TEXCOORD4;
};

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

	float4 localPos        = mul(ubo.view, mul(push_constants.model, float4(input.Pos, 1.0)));
	float4 positionWorld   = mul(push_constants.model, float4(input.Pos, 1.0));

	output.Color           = push_constants.color;
    output.Pos             = mul(ubo.projection, localPos);
	output.Normal          = mul((float3x3)(mul(ubo.view, push_constants.model)), input.Normal);
	output.LightVec        = ubo_baseline.lightPosition.xyz - positionWorld.xyz;
	output.LightColor[0]   = ubo_baseline.lightColor.xyz * ubo_baseline.lightColor.w;
	output.LightColor[1]   = ubo_baseline.ambientLightColor.xyz * ubo_baseline.ambientLightColor.w;
	output.ViewVec         = -localPos.xyz;
	output.LightIntensity  = ubo_baseline.lightIntensity;

    return output;
}
