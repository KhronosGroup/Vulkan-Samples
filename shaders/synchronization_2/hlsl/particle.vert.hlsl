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
    [[vk::location(0)]] float4 Pos : POSITION0;
    [[vk::location(1)]] float4 Vel : TEXCOORD0;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float GradientPos : POSITION3;
    [[vk::location(1)]] float2 CenterPos : POSITION1;
    [[vk::builtin("PointSize")]] float PSize : PSIZE;
    [[vk::location(2)]] float PointSize : TEXCOORD0;
};

struct UBO
{
    float4x4 projection;
    float4x4 modelview;
    float2 screendim;
};
[[vk::binding(2, 0)]]
ConstantBuffer<UBO> ubo : register(b2);

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    const float spriteSize = 0.005 * input.Pos.w; // Point size influenced by mass (stored in input.Pos.w);

    float4 eyePos = mul(ubo.modelview, float4(input.Pos.x, input.Pos.y, input.Pos.z, 1.0));
    float4 projectedCorner = mul(ubo.projection, float4(0.5 * spriteSize, 0.5 * spriteSize, eyePos.z, eyePos.w));
    output.PSize = output.PointSize = clamp(ubo.screendim.x * projectedCorner.x / projectedCorner.w, 1.0, 128.0);

    output.Pos = mul(ubo.projection, eyePos);
    output.CenterPos = ((output.Pos.xy / output.Pos.w) + 1.0) * 0.5 * ubo.screendim;

    output.GradientPos = input.Vel.w;
    return output;
}