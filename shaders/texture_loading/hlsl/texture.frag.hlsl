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

[[vk::binding(1, 0)]]
Texture2D textureColor : register(t1);
[[vk::binding(1, 0)]]
SamplerState samplerColor : register(s1);

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float2 UV : TEXCOORD0;
    [[vk::location(1)]] float LodBias : FLOAT0;
    [[vk::location(2)]] float3 Normal : NORMAL0;
    [[vk::location(3)]] float3 ViewVec : VECTOR0;
    [[vk::location(4)]] float3 LightVec : VECTOR1;
};

float4 main(VSOutput input) : SV_TARGET0
{
    float4 color = textureColor.SampleBias(samplerColor, input.UV, input.LodBias);
    float3 N = normalize(input.Normal);
    float3 L = normalize(input.LightVec);
    float3 V = normalize(input.ViewVec);
    float3 R = reflect(-L, N);
    float3 diffuse = max(dot(N, L), 0.0).xxx;
    float3 specular = (pow(max(dot(R, V), 0.0), 16.0)).xxx * color.a;
    return float4(diffuse * color.rgb + specular, 1.0);
}