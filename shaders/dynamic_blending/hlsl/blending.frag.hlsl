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

Texture2D textureColor : register(t1);
SamplerState samplerColor : register(s1);

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float2 UV : TEXCOORD0;
    [[vk::location(1)]] uint ColorOffset : TEXCOORD1;
};

struct Col
{
    float4 data[8];
};
[[vk::binding(1, 0)]]
ConstantBuffer<Col> color : register(b1);

float4 sampleTexture(VSOutput input)
{
    float4 c00 = color.data[0 + input.ColorOffset];
    float4 c01 = color.data[1 + input.ColorOffset];
    float4 c02 = color.data[2 + input.ColorOffset];
    float4 c03 = color.data[3 + input.ColorOffset];

    float4 b0 = lerp(c00, c01, input.UV.x);
    float4 b1 = lerp(c02, c03, input.UV.x);

    float4 p0 = lerp(b0, b1, input.UV.y);

    return p0;
}

float4 main(VSOutput input) : SV_TARGET0
{
    return sampleTexture(input);
}