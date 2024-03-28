/* Copyright (c) 2019, Sascha Willems
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

Texture2D textureColor : register(t1);
SamplerState samplers[3] : register(s2);

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float lodBias;
    int samplerIndex;
};
[[vk::binding(0, 0)]]
ConstantBuffer<UBO> ubo : register(b0);

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float2 UV : TEXCOORD0;
};

float4 main(VSOutput input) : SV_TARGET
{	
    float3 color = textureColor.SampleBias(samplers[ubo.samplerIndex], input.UV * float2(2.0, 0.25), ubo.lodBias).rgb;
    return float4(color, 1.0);
}