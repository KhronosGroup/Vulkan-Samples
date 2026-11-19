/* Copyright (c) 2019-2026, Arm Limited and Contributors
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

struct VSInput
{
    [[vk::location(0)]] float3 position : POSITION0;
    [[vk::location(1)]] float2 texcoord_0;
    [[vk::location(2)]] float3 normal;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    [[vk::location(0)]] float4 o_pos;
    [[vk::location(1)]] float2 o_uv;
    [[vk::location(2)]] float3 o_normal;
};

struct MVPUniform
{
    column_major float4x4 model;
    column_major float4x4 view_proj;
};

[[vk::push_constant]] MVPUniform mvp_uniform;

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;
    output.o_pos = mul(mvp_uniform.model, float4(input.position, 1.0));
    output.o_uv = input.texcoord_0;
    output.o_normal = mul(float3x3(mvp_uniform.model), input.normal);
    output.position = mul(mvp_uniform.view_proj, output.o_pos);
    return output;
}
