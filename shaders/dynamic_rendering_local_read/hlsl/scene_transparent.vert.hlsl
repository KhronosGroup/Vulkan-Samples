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
    float3 Pos : POSITION0;
    float3 Normal : NORMAL0;
    float2 UV : TEXCOORD;
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
    float3 Color : COLOR0;
    float2 UV : TEXCOORD0;
};

struct PushConsts {
    float4x4 Matrix;
    float4 Color;
};
[[vk::push_constant]] PushConsts pushConsts;

VSOutput main(VSInput input)
{
    VSOutput output;
    output.Color = pushConsts.Color.rgb;
    output.Pos = mul(ubo.projection, mul(ubo.view, mul(pushConsts.Matrix, float4(input.Pos, 1.0))));
    output.UV = input.UV;
    return output;
}