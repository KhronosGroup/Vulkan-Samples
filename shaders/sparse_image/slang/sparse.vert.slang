/* Copyright (c) 2023-2025, Mobica Limited
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
    float2 Pos : POSITION0;
    float2 UV;
};

struct MvpTransform
{
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};
ConstantBuffer<MvpTransform> mvp;

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
};

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;
    output.UV = input.UV;
    output.Pos = mul(mvp.proj, mul(mvp.view, mul(mvp.model, float4(input.Pos, 0.0, 1.0))));
    return output;
}