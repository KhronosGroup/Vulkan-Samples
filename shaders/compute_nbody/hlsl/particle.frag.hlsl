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

Texture2D textureColorMap : register(t0);
SamplerState samplerColorMap : register(s0);
Texture2D textureGradientRamp : register(t1);
SamplerState samplerGradientRamp : register(s1);

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float GradientPos : POSITION0;
    [[vk::location(1)]] float2 CenterPos : POSITION1;
    [[vk::builtin("PointSize")]] float PSize : PSIZE;
    [[vk::location(2)]] float PointSize : TEXCOORD0;
};

float4 main(VSOutput input) : SV_TARGET
{
    float3 color = textureGradientRamp.Sample(samplerGradientRamp, float2(input.GradientPos, 0.0)).rgb;
    float2 PointCoord = (input.Pos.xy - input.CenterPos.xy) / input.PointSize + 0.5;
    return float4(textureColorMap.Sample(samplerColorMap, PointCoord).rgb * color, 1);
}