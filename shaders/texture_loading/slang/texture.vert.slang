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
    float2 UV;
    float3 Normal;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4 viewPos;
    float lodBias;
};
ConstantBuffer<UBO> ubo;

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
    float LodBias;
    float3 Normal;
    float3 ViewVec;
    float3 LightVec;
};

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;
    float3 vecPos = mul((float3x3) ubo.model, input.Pos);
    float3 lightPos = mul((float3x3) ubo.model, (1.0).xxx);
    output.UV = input.UV;
    output.LodBias = ubo.lodBias;
    output.Pos = mul(ubo.projection, mul(ubo.model, float4(input.Pos, 1.0)));
    output.Normal = mul((float3x3) ubo.model, input.Normal);
    output.LightVec = lightPos - vecPos;
    output.ViewVec = ubo.viewPos.xyz - vecPos;   
    return output;
}