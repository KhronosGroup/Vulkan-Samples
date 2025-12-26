/* Copyright (c) 2023-2025, Mobica Limited
 * Copyright (c) 2025, Sascha Willems
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

struct VSOutput
{
    float4 Pos : SV_Position;
    float3 Normal;
};

struct GSOutput
{
    float4 Pos : SV_Position;
    float3 Color;
};

struct UBO {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};
[[vk::binding(1, 0)]] ConstantBuffer<UBO> ubo;

[shader("geometry")]
[maxvertexcount(2)]
void main(triangle VSOutput input[3], inout LineStream<GSOutput> outStream)
{
    float normalLength = 0.1f;

    // middle point of triangle
    float3 pos = (input[0].Pos.xyz + input[1].Pos.xyz + input[2].Pos.xyz) / 3.0;
    float3 normal = input[0].Normal.xyz;

    GSOutput output;

    // line vertices
    output.Pos = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(pos, 1.0))));
    output.Color = float3(1.0, 0.0, 0.0);
    outStream.Append(output);

    output.Pos = mul(ubo.proj, mul(ubo.view, mul(ubo.model,  float4(pos + normal * normalLength, 1.0))));
    output.Color = float3(0.0, 0.0, 1.0);
    outStream.Append(output);

    outStream.RestartStrip();
}
