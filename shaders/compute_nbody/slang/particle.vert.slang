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

struct VSInput
{
    float4 Pos;
    float4 Vel;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float PointSize : SV_PointSize;
    float GradientPos;
};

struct UBO
{
    float4x4 projection;
    float4x4 modelview;
    float2 screendim;
};
[[vk::binding(2, 0)]] ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;
    const float spriteSize = 0.005 * input.Pos.w; // Point size influenced by mass (stored in input.Pos.w);
    float4 eyePos = mul(ubo.modelview, float4(input.Pos.x, input.Pos.y, input.Pos.z, 1.0));
    // Use projection to make sure point sizes uniformly scale independent of current projection
    float4 projectedCorner = mul(ubo.projection, float4(0.5 * spriteSize, 0.5 * spriteSize, eyePos.z, eyePos.w));
    output.PointSize = clamp(ubo.screendim.x * projectedCorner.x / projectedCorner.w, 1.0, 128.0);
    output.Pos = mul(ubo.projection, eyePos);
    output.GradientPos = input.Vel.w;
    return output;
}