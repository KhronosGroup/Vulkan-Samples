/* Copyright (c) 2026, Sascha Willems
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
    [[vk::location(0)]] float3 Pos : POSITION0;
    [[vk::location(1)]] float2 UV : TEXCOORD0;
};

struct UBOCamera
{
    float4x4 projection;
    float4x4 view;
    float4x4 model[2];
};
[[vk::binding(0, 0)]] ConstantBuffer<UBOCamera> ubo[2] : register(b0, space0);

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float2 UV : TEXCOORD0;
    [[vk::location(1)]] int InstanceIndex : TEXCOORD1;
};

struct PushConsts
{
    int samplerIndex;
    int frameIndex;
};
[[vk::push_constant]] PushConsts pushConsts;

VSOutput main(VSInput input, uint InstanceIndex: SV_InstanceID)
{
    VSOutput output;
    output.UV = input.UV;
    output.Pos = mul(ubo[pushConsts.frameIndex].projection, mul(ubo[pushConsts.frameIndex].view, mul(ubo[pushConsts.frameIndex].model[InstanceIndex], float4(input.Pos.xyz, 1.0))));
    output.InstanceIndex = InstanceIndex;
    return output;
};