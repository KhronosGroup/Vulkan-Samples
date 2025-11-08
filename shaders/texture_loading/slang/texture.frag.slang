/* Copyright (c) 2025, Sascha Willems
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

[[vk::binding(1, 0)]] Sampler2D samplerColor;

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
    float LodBias;
    float3 Normal;
    float3 ViewVec;
    float3 LightVec;
};

[shader("fragment")]
float4 main(VSOutput input)
{
    float4 color = samplerColor.SampleBias(input.UV, input.LodBias);
    float3 N = normalize(input.Normal);
    float3 L = normalize(input.LightVec);
    float3 V = normalize(input.ViewVec);
    float3 R = reflect(-L, N);
    float3 diffuse = max(dot(N, L), 0.0);
    float3 specular = (pow(max(dot(R, V), 0.0), 16.0)) * color.a;
    return float4(diffuse * color.rgb + specular, 1.0);
}