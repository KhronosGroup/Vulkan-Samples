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
    [[vk::location(1)]] float2 UV : TEXCOORD0;
    [[vk::location(2)]] float3 Normal : NORMAL0;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4 viewPos;
    float lodBias;
};
[[vk::binding(0, 0)]]
ConstantBuffer<UBO> ubo : register(b0);

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float2 UV : TEXCOORD0;
    [[vk::location(1)]] float LodBias : FLOAT0;
    [[vk::location(2)]] float3 Normal : NORMAL0;
    [[vk::location(3)]] float3 ViewVec : VECTOR0;
    [[vk::location(4)]] float3 LightVec : VECTOR1;
};

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    float3 vecPos = mul((float3x3) ubo.model, input.Pos.xyz).xyz;
    float3 lightPos = mul((float3x3) ubo.model, (1.0).xxx);
    
    output.UV = input.UV;
    output.LodBias = ubo.lodBias;
    output.Pos = mul(ubo.projection, mul(ubo.model, float4(input.Pos.xyz, 1.0)));
    output.Normal = mul((float3x3) ubo.model, input.Normal);
    output.LightVec = lightPos - vecPos;
    output.ViewVec = ubo.viewPos.xyz - vecPos;
    
    return output;
}