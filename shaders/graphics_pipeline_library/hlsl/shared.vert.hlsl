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
    [[vk::location(2)]] float2 UV : TEXCOORD0;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4 lightPos;
};
[[vk::binding(0, 0)]]
ConstantBuffer<UBO> ubo : register(b0);

struct PushConstants
{
    float4 color;
};
[[vk::push_constant]] PushConstants pushConsts;

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float3 Normal : NORMAL0;
    [[vk::location(1)]] float3 Color : COLOR0;
    [[vk::location(2)]] float3 ViewVec : VECTOR0;
    [[vk::location(3)]] float3 LightVec : VECTOR1;
    [[vk::location(4)]] nointerpolation float3 FlatNormal : NORMAL1;
};

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    float3 vecPos = mul((float3x3) ubo.model, input.Pos.xyz).xyz;

    output.Color = pushConsts.color.rgb;
    output.Normal = mul((float3x3) ubo.model, input.Normal);
    output.Pos = mul(ubo.projection, mul(ubo.model, float4(input.Pos.xyz, 1.0)));
    output.LightVec = ubo.lightPos.xyz - vecPos;
    output.ViewVec = -vecPos;
    
    return output;
}