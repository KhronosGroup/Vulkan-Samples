/* Copyright (c) 2019-2026, Arm Limited and Contributors
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

[[vk::binding(0, 0)]] Sampler2D base_color_texture;

struct FSInput
{
    [[vk::location(0)]] float4 in_pos;
    [[vk::location(1)]] float2 in_uv;
    [[vk::location(2)]] float3 in_normal;
    [[vk::location(3)]] float4 in_shadow_clip;
};

struct GlobalUniform
{
    column_major float4x4 model;
    column_major float4x4 view_proj;
    float3 camera_position;
};

[[vk::binding(1, 0)]] ConstantBuffer<GlobalUniform> global_uniform;

// Push constants come with a limitation in the size of data.
// The standard requires at least 128 bytes
struct PBRMaterialUniform
{
    float4 base_color_factor;
    float metallic_factor;
    float roughness_factor;
};

[[vk::push_constant]] PBRMaterialUniform pbr_material_uniform;

struct Light
{
    float4 position;         // position.w represents type of light
    float4 color;            // color.w represents light intensity
    float4 direction;        // direction.w represents range
    float2 info;             // (only used for spot lights) info.x represents light inner cone angle, info.y represents light outer cone angle
};

float3 apply_directional_light(Light light, float3 normal)
{
    float3 world_to_light = normalize(-light.direction.xyz);
    float ndotl = clamp(dot(normal, world_to_light), 0.0, 1.0);
    return ndotl * light.color.w * light.color.rgb;
}

struct LightsInfo
{
    Light directional_light;
};

[[vk::binding(4, 0)]] ConstantBuffer<LightsInfo> lights_info;

[[vk::binding(6, 0)]] Sampler2DShadow tex_shadow;

[shader("fragment")]
float4 main(FSInput input) : SV_TARGET0
{
    float3 normal = normalize(input.in_normal);

    float3 light_contribution = apply_directional_light(lights_info.directional_light, normal);

    float3 shadow_coord = input.in_shadow_clip.xyz / input.in_shadow_clip.w;
    float shadow = 0.0;
    shadow += tex_shadow.SampleCmpLevelZero(shadow_coord.xy, shadow_coord.z) / 4.0;
    shadow += tex_shadow.SampleCmpLevelZero(shadow_coord.xy, shadow_coord.z, int2(-1, 0)) / 8.0;
    shadow += tex_shadow.SampleCmpLevelZero(shadow_coord.xy, shadow_coord.z, int2(1, 0)) / 8.0;
    shadow += tex_shadow.SampleCmpLevelZero(shadow_coord.xy, shadow_coord.z, int2(0, -1)) / 8.0;
    shadow += tex_shadow.SampleCmpLevelZero(shadow_coord.xy, shadow_coord.z, int2(0, 1)) / 8.0;
    shadow += tex_shadow.SampleCmpLevelZero(shadow_coord.xy, shadow_coord.z, int2(-1, -1)) / 16.0;
    shadow += tex_shadow.SampleCmpLevelZero(shadow_coord.xy, shadow_coord.z, int2(1, 1)) / 16.0;
    shadow += tex_shadow.SampleCmpLevelZero(shadow_coord.xy, shadow_coord.z, int2(-1, 1)) / 16.0;
    shadow += tex_shadow.SampleCmpLevelZero(shadow_coord.xy, shadow_coord.z, int2(1, -1)) / 16.0;
    light_contribution *= shadow;

    float4 base_color = base_color_texture.Sample(input.in_uv);

    float3 ambient_color = float3(0.25) * base_color.xyz;

    return float4(ambient_color + light_contribution * base_color.xyz, base_color.w);
}
