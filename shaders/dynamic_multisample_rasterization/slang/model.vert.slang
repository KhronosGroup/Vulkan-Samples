/* Copyright (c) 2024-2025, Mobica Limited
 * Copyright (c) 2025, Sascha Willems
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
    float3 Pos : POSITION0;
    float3 Normal;
    float2 UV;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float2 UV;
    float3 ViewVec;
    float3 LightVec;
    nointerpolation float4 BaseColorFactor;
    nointerpolation float MetallicFactor;
    nointerpolation float RoughnessFactor;
    nointerpolation uint BaseTextureIndex;
    nointerpolation uint NormalTextureIndex;
    nointerpolation uint MetallicRoughnessTextureIndex;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
};
ConstantBuffer<UBO> ubo;

struct Push_Constants {
    float4x4 model;

    float4 base_color_factor;
    float metallic_factor;
    float roughness_factor;

    uint baseTextureIndex;
    uint normalTextureIndex;
    uint metallicRoughnessTextureIndex;
} ;

[shader("vertex")]
VSOutput main(VSInput input, uniform Push_Constants push_constants)
{
    VSOutput output;
    output.UV = input.UV;
    float4 localPos = mul(ubo.view, mul(push_constants.model, float4(input.Pos, 1.0)));
    output.Pos = mul(ubo.projection, localPos);
    output.Normal = mul(float3x3(mul(ubo.view, push_constants.model)), input.Normal);
    float3 lightPos = float3(10.0, -10.0, 10.0);
    output.LightVec = lightPos.xyz;
    output.ViewVec = -localPos.xyz;
    output.BaseColorFactor = push_constants.base_color_factor;
    output.MetallicFactor = push_constants.metallic_factor;
    output.RoughnessFactor = push_constants.roughness_factor;
    output.BaseTextureIndex = push_constants.baseTextureIndex;
    output.NormalTextureIndex = push_constants.normalTextureIndex;
    output.MetallicRoughnessTextureIndex = push_constants.metallicRoughnessTextureIndex;
    return output;
}
