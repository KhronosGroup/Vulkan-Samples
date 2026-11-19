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

[[vk::binding(0, 0)]] Sampler2D base_color_texture;

struct FSInput
{
    float4 in_pos;
    float2 in_uv;
    float3 in_normal;
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

struct Light
{
    float4 position;
    float4 color;
    float4 direction;
    float2 info;
};

struct LightsInfo
{
    Light directional_lights[48];
    Light point_lights[48];
    Light spot_lights[48];
};

[[vk::binding(4, 0)]] ConstantBuffer<LightsInfo> lights_info;

[[vk::constant_id(0)]] const uint DIRECTIONAL_LIGHT_COUNT = 0U;
[[vk::constant_id(1)]] const uint POINT_LIGHT_COUNT = 0U;
[[vk::constant_id(2)]] const uint SPOT_LIGHT_COUNT = 0U;

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

[shader("fragment")]
float4 main(FSInput input) : SV_TARGET0
{
    float3 normal = normalize(input.in_normal);
    float3 light_contribution = float3(0.0);

    for (uint i = 0U; i < DIRECTIONAL_LIGHT_COUNT; ++i)
    {
        light_contribution += apply_directional_light(lights_info.directional_lights[i], normal);
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
