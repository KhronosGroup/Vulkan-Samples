/* Copyright (c) 2021-2026, Arm Limited and Contributors
 * Copyright (c) 2026, Sascha Willems
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

[[vk::binding(0, 0)]] Sampler2D in_tex;
[[vk::image_format("rgba16f")]] [[vk::binding(1, 0)]] RWTexture2D<float4> out_tex;

struct Registers
{
    uint2 resolution;
    float2 inv_resolution;
    float2 inv_input_resolution;
};

[[vk::push_constant]] Registers registers;

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 globalInvocationID : SV_DispatchThreadID)
{
    if (all(globalInvocationID.xy < registers.resolution))
    {
        float2 uv = (float2(globalInvocationID.xy) + 0.5) * registers.inv_resolution;
        float3 rgb = in_tex.SampleLevel(uv, 0.0).rgb;
        rgb = 0.2 * max(rgb - 1.0, float3(0.0));
        out_tex[int2(globalInvocationID.xy)] = float4(rgb, 1.0);
    }
}
