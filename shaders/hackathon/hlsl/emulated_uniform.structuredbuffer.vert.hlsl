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
    [[vk::location(1)]] float3 Color : COLOR0;
};

struct UBOView
{
    float4x4 projection;
    float4x4 view;
};
[[vk::binding(0, 0)]]
ConstantBuffer<UBOView> uboView;

[[vk::binding(1, 0)]]
StructuredBuffer<float> emulatedUboInstance;

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float3 Color : COLOR0;
};

VSOutput main(VSInput input)
{
    float4x4 model = {
        emulatedUboInstance[0], emulatedUboInstance[4], emulatedUboInstance[8], emulatedUboInstance[12],
        emulatedUboInstance[1], emulatedUboInstance[5], emulatedUboInstance[9], emulatedUboInstance[13],
        emulatedUboInstance[2], emulatedUboInstance[6], emulatedUboInstance[10], emulatedUboInstance[14],
        emulatedUboInstance[3], emulatedUboInstance[7], emulatedUboInstance[11], emulatedUboInstance[15],
    };

    VSOutput output = (VSOutput) 0;
    output.Color = input.Color;
    output.Pos = mul(uboView.projection, mul(uboView.view, mul(model, float4(input.Pos, 1.0))));
    return output;
}