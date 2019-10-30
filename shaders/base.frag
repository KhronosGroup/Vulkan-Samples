#version 320 es
/* Copyright (c) 2019, Arm Limited and Contributors
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
layout (set=0, binding=0) uniform sampler2D base_color_texture;
#endif

layout (location = 0) in vec4 in_pos;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec3 in_normal;

layout (location = 0) out vec4 o_color;

layout(set = 0, binding = 1) uniform GlobalUniform {
    mat4 model;
    mat4 view_proj;
    vec4 light_pos;
    vec4 light_color;
} global_uniform;

// Push constants are much quicker than uniforms
// but they come with a limitation in the size of data.
// The standard requires at least 128 bytes
layout(push_constant, std430) uniform PBRMaterialUniform {
    vec4 base_color_factor;
    float metallic_factor;
    float roughness_factor;
} pbr_material_uniform;

void main(void)
{
    vec3 normal = normalize(in_normal);

    vec3 world_to_light = global_uniform.light_pos.xyz - in_pos.xyz;

    float dist = length(world_to_light) * 0.0001;

    float atten = 1.0 - smoothstep(0.5 * global_uniform.light_pos.w, global_uniform.light_pos.w, dist);

    world_to_light = normalize(world_to_light);

    float ndotl = clamp(dot(normal, world_to_light), 0.0, 1.0);

    vec4 base_color = vec4(1.0, 0.0, 0.0, 1.0);

#ifdef HAS_BASE_COLOR_TEXTURE
    base_color = texture(base_color_texture, in_uv);
#else
    base_color = pbr_material_uniform.base_color_factor;    
#endif

    vec4 ambient_color = vec4(0.2, 0.2, 0.2, 1.0) * base_color;

    o_color = ambient_color + ndotl * atten * global_uniform.light_color * base_color;
}
