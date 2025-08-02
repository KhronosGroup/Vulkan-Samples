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
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4x4 view;
    float4x4 inverseTranspose;
};
ConstantBuffer<UBO> ubo;

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal : NORMAL0;
    float4 Color : COLOR0;
    float3 WorldPos : POS0;
};

struct PushConsts {
    float4x4 Matrix;
    float4 Color;
};
[[vk::push_constant]] PushConsts pushConsts;

VSOutput main(VSInput input)
{
    float4x4 nodeMat = mul(ubo.model, pushConsts.Matrix);
    VSOutput output;
    output.Pos = mul(ubo.projection, mul(ubo.view, mul(pushConsts.Matrix, float4(input.Pos, 1.0))));
    // Vertex position in world space
    output.WorldPos = mul(pushConsts.Matrix, float4(input.Pos, 1.0)).xyz;
    // GL to Vulkan coord space
    output.WorldPos.y = -output.WorldPos.y;
    output.Normal = mul((float3x3)nodeMat, input.Normal);
    output.Color = pushConsts.Color;
    return output;
}
