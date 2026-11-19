/* Copyright (c) 2019-2026, Arm Limited and Contributors
/* Copyright (c) 2026, Sascha Willems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
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

[[vk::binding(0, 0)]] Sampler2D base_color_texture;

struct FSInput
{
    [[vk::location(0)]] float4 in_pos;
    [[vk::location(1)]] float2 in_uv;
    [[vk::location(2)]] float3 in_normal;
};

struct FSOutput
{
    float4 o_albedo : SV_Target0;
    float4 o_normal : SV_Target1;
};

struct GlobalUniform
{
    column_major float4x4 model;
    column_major float4x4 view_proj;
    float3 camera_position;
};

[[vk::binding(1, 0)]] ConstantBuffer<GlobalUniform> global_uniform;

struct PBRMaterialUniform
{
    float4 base_color_factor;
    float metallic_factor;
    float roughness_factor;
};

[[vk::push_constant]] PBRMaterialUniform pbr_material_uniform;

[shader("fragment")]
FSOutput main(FSInput input)
{
    FSOutput output;
    float3 normal = normalize(input.in_normal);

    output.o_normal = float4(0.5 * normal + 0.5, 1.0);
    output.o_albedo = base_color_texture.Sample(input.in_uv);

    return output;
}
