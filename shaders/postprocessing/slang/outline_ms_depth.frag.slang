/* Copyright (c) 2020-2026, Arm Limited and Contributors
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

[[vk::binding(1, 0)]] Sampler2DMS ms_depth_sampler;
[[vk::binding(2, 0)]] Sampler2D color_sampler;

struct FSInput
{
    float4 fragCoord : SV_Position;
    [[vk::location(0)]] float2 in_uv;
};

struct PostprocessingUniform
{
    float2 near_far;
};

[[vk::binding(0, 0)]] ConstantBuffer<PostprocessingUniform> postprocessing_uniform;

float linearizeDepth(float depth, float near, float far)
{
    return near * far / (far + depth * (near - far));
}

float getDepth(int2 frag_coord, int2 offset)
{
    float depth = ms_depth_sampler.Load(frag_coord + offset, 0).r;
    return linearizeDepth(depth, postprocessing_uniform.near_far.x, postprocessing_uniform.near_far.y);
}

[shader("fragment")]
float4 main(FSInput input) : SV_Target0
{
    float4 color = color_sampler.Sample(input.in_uv);
    int2 frag_coord = int2(input.fragCoord.xy);
    float depth = getDepth(frag_coord, int2(0, 0));

    float3 outline_color = float3(0.0, 0.0, 0.0);
    int thickness = 2;
    float outline = depth - getDepth(frag_coord, int2(-thickness, 0));
    outline += depth - getDepth(frag_coord, int2(0, thickness));
    outline += depth - getDepth(frag_coord, int2(thickness, 0));
    outline += depth - getDepth(frag_coord, int2(0, -thickness));

    return float4(lerp(color.rgb, outline_color, clamp(outline, 0.0, 1.0)), 1.0);
}
