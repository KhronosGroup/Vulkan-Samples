#version 450
/* Copyright (c) 2020-2021, Arm Limited and Contributors
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
#extension GL_EXT_shader_explicit_arithmetic_types_float16 : require

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(set = 0, binding = 1) uniform GlobalUniform {
    mat4 model;
    mat4 view_proj;
    vec3 camera_position;
} global_uniform;

layout (location = 0) out f16vec3 o_vertex_color;
layout (location = 1) out f16vec3 o_normal;
layout (location = 2) out f16vec3 o_delta_pos;

// Push constants come with a limitation in the size of data.
// The standard requires at least 128 bytes
layout(push_constant, std430) uniform PBRMaterialUniform
{
    vec4  base_color_factor;
    float metallic_factor;
    float roughness_factor;
}
pbr_material_uniform;

#include "lighting.h"

layout(set = 0, binding = 4) uniform LightsInfo
{
    Light directional_lights[MAX_LIGHT_COUNT];
    Light point_lights[MAX_LIGHT_COUNT];
    Light spot_lights[MAX_LIGHT_COUNT];
}
lights_info;

layout(constant_id = 0) const uint DIRECTIONAL_LIGHT_COUNT = 0U;
layout(constant_id = 1) const uint POINT_LIGHT_COUNT       = 0U;
layout(constant_id = 2) const uint SPOT_LIGHT_COUNT        = 0U;

void main(void)
{
    vec4 pos = global_uniform.model * vec4(position, 1.0);
    vec3 normal = mat3(global_uniform.model) * normal;
    gl_Position = global_uniform.view_proj * pos;

    vec3 light_contribution = vec3(0.0);
    for (uint i = 0U; i < DIRECTIONAL_LIGHT_COUNT; ++i)
    {
        light_contribution += apply_directional_light(lights_info.directional_lights[i], normal);
    }

    light_contribution += vec3(0.2);
    light_contribution *= pbr_material_uniform.base_color_factor.rgb;
    o_vertex_color = f16vec3(light_contribution);
    o_normal = f16vec3(normal);
    o_delta_pos = f16vec3(pos.xyz - global_uniform.camera_position);
}
