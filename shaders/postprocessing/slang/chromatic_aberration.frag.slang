/* Copyright (c) 2024-2026, Arm Limited and Contributors
/* Copyright (c) 2026, Sascha Willems
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

[[vk::binding(1, 0)]] Sampler2D color_sampler;

struct FSInput
{
    [[vk::location(0)]] float2 in_uv;
};

struct PostprocessingUniform
{
    float sin_elapsed_time;
};

[[vk::binding(0, 0)]] ConstantBuffer<PostprocessingUniform> postprocessing_uniform;

static const float SCALE = 0.002;

static const int RED_OFFSET = 3;
static const int GREEN_OFFSET = -1;
static const int BLUE_OFFSET = 2;

[shader("fragment")]
float4 main(FSInput input) : SV_Target0
{
    float4 o_color = float4(1.0);

    const float2 center_coord = float2(0.5);
    const float2 direction = (input.in_uv - center_coord) * postprocessing_uniform.sin_elapsed_time * SCALE;

    o_color.r = color_sampler.Sample(input.in_uv - (direction * float2(RED_OFFSET))).r;
    o_color.g = color_sampler.Sample(input.in_uv - (direction * float2(GREEN_OFFSET))).g;
    o_color.b = color_sampler.Sample(input.in_uv - (direction * float2(BLUE_OFFSET))).b;

    return o_color;
}
