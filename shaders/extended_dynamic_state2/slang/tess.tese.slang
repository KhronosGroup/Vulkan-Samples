/* Copyright (c) 2023-2025, Mobica Limited
 * Copyright (c) 2025, Sascha Willems
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
    float3 LocPos;
    float3 Normal;
};

struct HSOutput
{
    float3 Pos : SV_POSITION;
    float3 Normal;
};

struct DSOutput
{
    float4 Pos : SV_POSITION;
};

struct ConstantsHSOutput
{
    float TessLevelOuter[3] : SV_TessFactor;
    float TessLevelInner[2] : SV_InsideTessFactor;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
};
ConstantBuffer<UBO> ubo;

float3 interpolate3D(float3 v0, float3 v1, float3 v2, float3 tessCoord)
{
    return float3(tessCoord.x) * v0 + float3(tessCoord.y) * v1 + float3(tessCoord.z) * v2;
}

[domain("tri")]
[shader("domain")]
DSOutput main(ConstantsHSOutput input, float3 tessCoord: SV_DomainLocation, const OutputPatch<HSOutput, 3> patch, uniform float4x4 model, uniform float3 color)
{
    DSOutput output;
    float4 pos = float4(interpolate3D(patch[0].Pos, patch[1].Pos, patch[2].Pos, tessCoord), 1.0);
    output.Pos = mul(ubo.projection, mul(ubo.view, mul(model, pos)));
    return output;
}
