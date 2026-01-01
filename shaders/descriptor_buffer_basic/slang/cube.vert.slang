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
    float2 UV;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
};

struct UBOCamera {
    float4x4 projection;
    float4x4 view;
};
ConstantBuffer<UBOCamera> uboCamera;

struct UBOModel {
    float4x4 local;
};
[[vk::binding(0, 1)]] ConstantBuffer<UBOModel> uboModel;

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;
    output.UV = input.UV;
    output.Pos = mul(uboCamera.projection, mul(uboCamera.view, mul(uboModel.local, float4(input.Pos.xyz, 1.0))));
    return output;
}