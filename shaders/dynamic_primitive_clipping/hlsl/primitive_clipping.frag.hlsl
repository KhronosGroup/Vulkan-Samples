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

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float ClipDistance : SV_ClipDistance0;
    [[vk::location(0)]] float3 Normal : NORMAL0;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
    float4 colorTransformation;
    int2 sceneTransformation;
    float usePrimitiveClipping;
};
[[vk::binding(0, 0)]]
ConstantBuffer<UBO> ubo : register(b0);

float4 main(VSOutput input) : SV_TARGET0
{   
    float4 outColor = float4(0.5 * input.Normal + 0.5.xxx, 1.0);

    outColor.xyz = ubo.colorTransformation.x * outColor.xyz + ubo.colorTransformation.yyy;

    return outColor;
}
