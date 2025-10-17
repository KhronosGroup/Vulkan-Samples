/* Copyright (c) 2025, Mobica Limited
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

[[vk::binding(1, 0)]] Sampler2D Textures[15];

float4 getColor(in VSOutput input)
{
    float4 color = input.BaseColorFactor;

    if (input.BaseTextureIndex != -1)
        color = Textures[uint(round(input.BaseTextureIndex))].Sample(input.UV);

    return color;
}

float3 getNormal(in VSOutput input)
{
    float3 normal = input.Normal;

    if (input.NormalTextureIndex != -1)
        normal = Textures[uint(round(input.NormalTextureIndex))].Sample(input.UV).xyz * 2.0 - 1.0;

    return normal;
}

float3 getPBR(in VSOutput input)
{
    float3 pbr = float3(input.MetallicFactor, input.RoughnessFactor, 0.0);

    if (input.MetallicRoughnessTextureIndex != -1)
        pbr = Textures[uint(round(input.MetallicRoughnessTextureIndex))].Sample(input.UV).xyz;

    return pbr;
}

[shader("fragment")]
float4 main(VSOutput input)
{
    float3 N = normalize(getNormal(input));
    float3 L = normalize(input.LightVec);
    float3 V = normalize(input.ViewVec);
    float3 R = reflect(-L, N);
    float3 ambient = float3(0.25);
    float3 diffuse = max(dot(N, L), 0.0) * float3(0.75) * getPBR(input);
    float3 specular = pow(max(dot(R, V), 0.0), 16.0) * float3(0.75);

    float4 base_color = getColor(input);
    return float4(ambient * base_color.rgb + specular, base_color.a);
}
