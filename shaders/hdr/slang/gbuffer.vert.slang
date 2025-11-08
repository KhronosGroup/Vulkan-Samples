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
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 UVW;
    float3 Normal;
    float3 ViewVec;
    float3 LightVec;
};

struct UBO {
    float4x4 projection;
    float4x4 modelview;
    float4x4 skyboxModelview;
    float4x4 inverseModelView;
    float modelscale;
};
ConstantBuffer<UBO> ubo;

[[SpecializationConstant]] const int type = 0;

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;
    output.UVW = input.Pos;
    float3 worldPos;
    switch (type) {
        case 0: // Skybox
            worldPos = mul((float3x3)ubo.skyboxModelview, input.Pos);
            output.Pos = mul(ubo.projection, float4(worldPos, 1.0));
            break;
        case 1: // Object
            worldPos = mul(ubo.modelview, float4(input.Pos * ubo.modelscale, 1.0)).xyz;
            output.Pos = mul(ubo.projection, mul(ubo.modelview, float4(input.Pos.xyz * ubo.modelscale, 1.0)));
            break;
    }
    output.Normal = mul((float3x3)ubo.modelview, input.Normal);
    float3 lightPos = float3(0.0f, -5.0f, 5.0f);
    output.LightVec = lightPos.xyz - worldPos.xyz;
    output.ViewVec = -worldPos.xyz;
    return output;
}