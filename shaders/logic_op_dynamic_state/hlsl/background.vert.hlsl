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
    [[vk::location(0)]] float3 Pos : POSITION0;
    [[vk::location(1)]] float3 Normal : NORMAL0;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
};
[[vk::binding(0, 0)]]
ConstantBuffer<UBO> ubo : register(b0);

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float3 UVW : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
	output.UVW = input.Pos;
    output.Pos = mul(ubo.projection, mul(ubo.view, float4(input.Pos.xyz * 10.0, 1.0)));
    return output;
}