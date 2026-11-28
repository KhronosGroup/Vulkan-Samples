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

struct FSInput
{
    [[vk::location(0)]] float2 in_uv;
};

[[vk::binding(0, 0)]] Sampler2D hdr_tex;
[[vk::binding(1, 0)]] Sampler2D bloom_tex;

[shader("fragment")]
float4 main(FSInput input) : SV_TARGET0
{
    // The most basic tonemap possible.
    float3 col = hdr_tex.SampleLevel(input.in_uv, 0.0).rgb + bloom_tex.SampleLevel(input.in_uv, 0.0).rgb;
    col = col / (1.0 + col);
    return float4(col, 1.0);
}
