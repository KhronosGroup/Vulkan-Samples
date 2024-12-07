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
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
    float4x4 viewProjectionInverse;
};
[[vk::binding(0, 0)]]
ConstantBuffer<UBO> ubo : register(b0);

struct VSOutput
{
    float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 nearPoint : VECTOR0;
[[vk::location(1)]] float3 farPoint : VECTOR1;
[[vk::location(2)]] float4x4 view : MATRIX0;
[[vk::location(6)]] float4x4 projection : MATRIX1;
};

float3 unprojectPoint(float x, float y, float z, float4x4 viewProjectionInverse) {
    float4 clipSpacePos = float4(x, y, z, 1.0);
    float4 eyeSpacePos = mul(viewProjectionInverse, clipSpacePos);
    return eyeSpacePos.xyz / eyeSpacePos.w;
}

static float3 gridPlane[6] = {
    float3(1, 1, 0), float3(-1, -1, 0), float3(-1, 1, 0),
    float3(-1, -1, 0), float3(1, 1, 0), float3(1, -1, 0)
};

VSOutput main(VSInput input, uint VertexIndex : SV_VertexID)
{
    float3 pos = gridPlane[VertexIndex].xyz;

    VSOutput output = (VSOutput) 0;
    output.nearPoint = unprojectPoint(pos.x, pos.y, 0.0, ubo.viewProjectionInverse);
    output.farPoint = unprojectPoint(pos.x, pos.y, 1.0, ubo.viewProjectionInverse);
    output.view = ubo.view;
    output.projection = ubo.projection;
    output.Pos = float4(pos, 1.0);
    return output;
}