/* Copyright (c) 2025-2026, Arm Limited and Contributors
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

static const int MAX_LIGHT_COUNT = 48;

[[vk::binding(0, 0)]] Sampler2D base_color_texture;

struct FSInput
{
    [[vk::location(0)]] float4 in_pos;
    [[vk::location(1)]] float2 in_uv;
    [[vk::location(2)]] float3 in_normal;
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

float3 apply_point_light(Light light, float3 pos, float3 normal)
{
    float3 world_to_light = light.position.xyz - pos;
    float dist = length(world_to_light) * 0.005;
    float atten = 1.0 / (dist * dist);
    world_to_light = normalize(world_to_light);
    float ndotl = clamp(dot(normal, world_to_light), 0.0, 1.0);
    return ndotl * light.color.w * atten * light.color.rgb;
}

float3 apply_spot_light(Light light, float3 pos, float3 normal)
{
    float3 light_to_pixel = normalize(pos - light.position.xyz);
    float theta = dot(light_to_pixel, normalize(light.direction.xyz));
    float inner_cone_angle = light.info.x;
    float outer_cone_angle = light.info.y;
    float intensity = (theta - outer_cone_angle) / (inner_cone_angle - outer_cone_angle);
    return smoothstep(0.0, 1.0, intensity) * light.color.w * light.color.rgb;
}

struct LightsInfo
{
    Light directional_lights[MAX_LIGHT_COUNT];
    Light point_lights[MAX_LIGHT_COUNT];
    Light spot_lights[MAX_LIGHT_COUNT];
};

[[vk::binding(4, 0)]] ConstantBuffer<LightsInfo> lights_info;

[[vk::constant_id(0)]] const uint DIRECTIONAL_LIGHT_COUNT = 0U;
[[vk::constant_id(1)]] const uint POINT_LIGHT_COUNT = 0U;
[[vk::constant_id(2)]] const uint SPOT_LIGHT_COUNT = 0U;

[[vk::binding(5, 0)]] Sampler2DShadow shadowmap_texture;

struct ShadowUniform
{
    column_major float4x4 light_matrix;
};

[[vk::binding(6, 0)]] ConstantBuffer<ShadowUniform> shadow_uniform;

float calculate_shadow(float4 in_pos)
{
    float4 projected_coord = mul(shadow_uniform.light_matrix, float4(in_pos.xyz, 1.0));

    projected_coord /= projected_coord.w;

    projected_coord.xy = 0.5 * projected_coord.xy + 0.5;

    return shadowmap_texture.SampleCmp(projected_coord.xy, projected_coord.z);
}

[shader("fragment")]
float4 main(FSInput input) : SV_Target0
{
    float3 normal = normalize(input.in_normal);

    float3 light_contribution = float3(0.0);

    for (uint i = 0U; i < DIRECTIONAL_LIGHT_COUNT; ++i)
    {
        light_contribution += apply_directional_light(lights_info.directional_lights[i], normal);

        // Shadows are enabled for the light source #0
        if (i == 0U)
        {
            light_contribution *= calculate_shadow(input.in_pos);
        }
    }

    for (uint i = 0U; i < POINT_LIGHT_COUNT; ++i)
    {
        light_contribution += apply_point_light(lights_info.point_lights[i], input.in_pos.xyz, normal);
    }

    for (uint i = 0U; i < SPOT_LIGHT_COUNT; ++i)
    {
        light_contribution += apply_spot_light(lights_info.spot_lights[i], input.in_pos.xyz, normal);
    }

    float4 base_color = base_color_texture.Sample(input.in_uv);

    float3 ambient_color = float3(0.2) * base_color.xyz;

    return float4(ambient_color + light_contribution * base_color.xyz, base_color.w);
}
