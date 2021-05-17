#version 320 es
/* Copyright (c) 2019-2021, Arm Limited and Contributors
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

precision highp float;

#ifdef HAS_BASE_COLOR_TEXTURE
layout(set = 0, binding = 0) uniform sampler2D base_color_texture;
#endif

layout(location = 0) in vec4 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec4 in_shadow_clip;

layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 1) uniform GlobalUniform
{
    mat4 model;
    mat4 view_proj;
    vec3 camera_position;
}
global_uniform;

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
    Light directional_light;
} lights_info;

layout(set = 0, binding = 6) uniform highp sampler2DShadow tex_shadow;

void main(void)
{
    vec3 normal = normalize(in_normal);

    vec3 light_contribution = apply_directional_light(lights_info.directional_light, normal);

    float shadow = 0.0;
    shadow += textureProjLod(tex_shadow, in_shadow_clip, 0.0) / 4.0;
    shadow += textureProjLodOffset(tex_shadow, in_shadow_clip, 0.0, ivec2(-1, 0)) / 8.0;
    shadow += textureProjLodOffset(tex_shadow, in_shadow_clip, 0.0, ivec2(1, 0)) / 8.0;
    shadow += textureProjLodOffset(tex_shadow, in_shadow_clip, 0.0, ivec2(0, -1)) / 8.0;
    shadow += textureProjLodOffset(tex_shadow, in_shadow_clip, 0.0, ivec2(0, 1)) / 8.0;
    shadow += textureProjLodOffset(tex_shadow, in_shadow_clip, 0.0, ivec2(-1, -1)) / 16.0;
    shadow += textureProjLodOffset(tex_shadow, in_shadow_clip, 0.0, ivec2(1, 1)) / 16.0;
    shadow += textureProjLodOffset(tex_shadow, in_shadow_clip, 0.0, ivec2(-1, 1)) / 16.0;
    shadow += textureProjLodOffset(tex_shadow, in_shadow_clip, 0.0, ivec2(1, -1)) / 16.0;
    light_contribution *= shadow;

    vec4 base_color = vec4(1.0, 0.0, 0.0, 1.0);

    #ifdef HAS_BASE_COLOR_TEXTURE
    base_color = texture(base_color_texture, in_uv);
    #else
    base_color = pbr_material_uniform.base_color_factor;
    #endif

    vec3 ambient_color = vec3(0.25) * base_color.xyz;

    o_color = vec4(ambient_color + light_contribution * base_color.xyz, base_color.w);
}
