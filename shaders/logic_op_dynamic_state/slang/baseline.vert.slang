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
    float3 Normal;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
};
ConstantBuffer<UBO> ubo;

struct UBOBaseline
{
    float4 ambientLightColor;
    float4 lightPosition;
    float4 lightColor;
    float lightIntensity;
};
ConstantBuffer<UBOBaseline> ubo_baseline;

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float4 Color;
    float3 LightVec;
    float3 LightColor[2];
    float3 ViewVec;
    float LightIntensity;
};

[shader("vertex")]
VSOutput main(VSInput input, uniform float4x4 model, uniform float4 color)
{
    VSOutput output;

    float4 localPos        = mul(ubo.view, mul(model, float4(input.Pos, 1.0)));
    float4 positionWorld   = mul(model, float4(input.Pos, 1.0));

    output.Color           = color;
    output.Pos             = mul(ubo.projection, localPos);
    output.Normal          = mul((float3x3)(mul(ubo.view, model)), input.Normal);
    output.LightVec        = ubo_baseline.lightPosition.xyz - positionWorld.xyz;
    output.LightColor[0]   = ubo_baseline.lightColor.xyz * ubo_baseline.lightColor.w;
    output.LightColor[1]   = ubo_baseline.ambientLightColor.xyz * ubo_baseline.ambientLightColor.w;
    output.ViewVec         = -localPos.xyz;
    output.LightIntensity  = ubo_baseline.lightIntensity;

    return output;
}
