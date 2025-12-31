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

struct ConstantsHSOutput
{
    float TessLevelOuter[3] : SV_TessFactor;
    float TessLevelInner[2] : SV_InsideTessFactor;
};

struct UBO
{
    float tessellationFactor;
};
[[vk::binding(1, 0)]] ConstantBuffer<UBO> ubo;

ConstantsHSOutput ConstantsHS(InputPatch<VSOutput, 3> patch)
{
    ConstantsHSOutput output;
    if (ubo.tessellationFactor > 0.0) {
        output.TessLevelInner[0] = ubo.tessellationFactor;
        output.TessLevelInner[1] = ubo.tessellationFactor;
        output.TessLevelOuter[0] = ubo.tessellationFactor;
        output.TessLevelOuter[1] = ubo.tessellationFactor;
        output.TessLevelOuter[2] = ubo.tessellationFactor;
    } else {
        output.TessLevelInner[0] = 1;
        output.TessLevelInner[1] = 1;
        output.TessLevelOuter[0] = 1;
        output.TessLevelOuter[1] = 1;
        output.TessLevelOuter[2] = 1;
    }
    return output;
}

[shader("hull")]
[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantsHS")]
HSOutput main(InputPatch<VSOutput, 3> patch, uint InvocationID: SV_OutputControlPointID)
{
    HSOutput output;
    output.Pos = patch[InvocationID].Pos.xyz;
    output.Normal = patch[InvocationID].Normal;
    return output;
}
