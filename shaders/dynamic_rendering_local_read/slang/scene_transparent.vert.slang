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
    float3 Pos;
    float3 Normal;
    float2 UV;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4x4 view;
};
[[vk::binding(1,0)]] ConstantBuffer<UBO> ubo;

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float4 Color;
    float2 UV;
};

[shader("vertex")]
VSOutput main(VSInput input, uniform float4x4 pushMatrix, uniform float4 pushColor)
{
    VSOutput output;
    output.Color = pushColor;
    output.Pos = mul(ubo.projection, mul(ubo.view, mul(pushMatrix, float4(input.Pos, 1.0))));
    output.UV = input.UV;
    return output;
}