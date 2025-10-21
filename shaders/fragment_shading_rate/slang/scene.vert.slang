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
    float3 Normal;
    float2 UV;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float2 UV;
    float3 ViewVec;
    float3 LightVec;
}; 

struct UBO
{
    float4x4 projection;
    float4x4 modelview;
    float4x4 skybox_modelview;
    int color_shading_rates;
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput main(VSInput input, uniform float4 offset, uniform int object_type)
{
    VSOutput output;
    float3 outPos;

    switch (object_type) {
    case 0: // Skysphere
        outPos = mul((float3x3)ubo.skybox_modelview, input.Pos);
        output.Pos = float4(mul(ubo.projection, float4(outPos, 1.0)));
        break;
    case 1: // Object
        float3 localPos = input.Pos + offset.xyz;
        outPos = mul((float3x3)ubo.modelview, localPos);
        output.Pos = mul(ubo.projection, mul(ubo.modelview, float4(localPos, 1.0)));
        break;
    }
    output.UV = input.UV;
    output.Normal = mul((float3x3)ubo.modelview, input.Normal);
    float3 lightPos = mul((float3x3)ubo.modelview, float3(0.0, -10.0, -10.0));
    output.LightVec = lightPos.xyz - outPos.xyz;
    output.ViewVec = -outPos.xyz;
    return output;
}