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

[[vk::constant_id(0)]] const int type = 0;

struct UBO
{
    float4x4 projection;
    float4x4 modelview;
    float4x4 skyboxModelview;
    float4x4 inverseModelView;
    float modelscale;
};
[[vk::binding(0, 0)]]
ConstantBuffer<UBO> ubo : register(b0);

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float3 UVW : TEXCOORD0;
    [[vk::location(1)]] float3 Normal : NORMAL0;
    [[vk::location(2)]] float3 ViewVec : TEXCOORD1;
    [[vk::location(3)]] float3 LightVec : TEXCOORD2;
};

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    output.UVW = input.Pos;
    float3 worldPos;

    switch (type)
    {
        case 0: // Skybox
            worldPos = mul((float4x3) ubo.skyboxModelview, input.Pos).xyz;
            output.Pos = mul(ubo.projection, float4(worldPos, 1.0));
            break;
        case 1: // Object
            worldPos = mul(ubo.modelview, float4(input.Pos * ubo.modelscale, 1.0)).xyz;
            output.Pos = mul(ubo.projection, mul(ubo.modelview, float4(input.Pos.xyz * ubo.modelscale, 1.0)));
            break;
    }
    output.Normal = mul((float4x3) ubo.modelview, input.Normal).xyz;
	
    float3 lightPos = float3(0.0f, -5.0f, 5.0f);
    output.LightVec = lightPos.xyz - worldPos.xyz;
    output.ViewVec = -worldPos.xyz;
    return output;
}
