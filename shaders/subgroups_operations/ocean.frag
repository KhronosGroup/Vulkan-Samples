#version 450
/* Copyright (c) 2024, Mobica Limited
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

layout (location = 0) in vec4 in_pos;
layout (location = 1) in vec2 in_uv;

layout (location = 0) out vec4 outFragColor;

layout (binding = 0) uniform Ubo
{
    mat4 projection;
    mat4 view;
    mat4 model;
} ubo;

layout (binding = 1, rgba32f) uniform image2D fft_displacement_map;

layout (binding = 3) uniform CameraPos
{
    vec4 position;
} cam;

layout (binding = 4) uniform sampler2D fft_normal_map;

layout (binding = 5) uniform OceanParamsUbo
{
    vec3 light_color;
    vec3 light_position;
    vec3 ocean_color;
} ocean_ubo;

layout (binding = 6) uniform samplerCube skybox_texture_map;

const float fresnel_approx_pow_factor = 2.0;
const float specular_power = 16.0;
const float specular_scale = 0.75;
const float dyna_range = 0.8f;
const vec3 ocean_dark = vec3(0.03, 0.06, 0.135);

void main() 
{
    vec3 result = vec3(0.0f);
    ivec2 normal_texture_size = textureSize(fft_normal_map, 0);
    vec2 offset_scale = vec2(4.0f / normal_texture_size.x, 4.0f / normal_texture_size.y);

    vec3 n0 = texture(fft_normal_map, in_uv + offset_scale).xyz;
    vec3 n1 = texture(fft_normal_map, in_uv + vec2(-offset_scale.x, offset_scale.y)).xyz;
    vec3 n2 = texture(fft_normal_map, in_uv - offset_scale).xyz;
    vec3 n3 = texture(fft_normal_map, in_uv - vec2(-offset_scale.x, offset_scale.y)).xyz;

    float f0 = clamp(abs(dot(n0, n2) * (-0.5f) + 0.5f), 0.0f, 1.0f);
    float f1 = clamp(abs(dot(n1, n3) * (-0.5f) + 0.5f), 0.0f, 1.0f);

    f0 = pow(f0 * 5.0f, 2.0f);
    f1 = pow(f1 * 5.0f, 2.0f);

    vec4 normal_map_data = texture(fft_normal_map, in_uv);

    float fac = (normal_map_data.z / 125.0f) * clamp(max(f0, f1), 0.0f, 1.0f);

    mat3 normal_matrix = mat3(ubo.model);
    vec3 normal = normal_matrix * normal_map_data.xyz;

    vec3 light_dir = normalize(normal_matrix * ocean_ubo.light_position);
    vec3 view_dir = normalize(in_pos.xyz);

    vec3 specular = vec3(0.0f);
    float n_dot_vp = max(0.0f, dot(normal, light_dir));
    float n_dot_d = dot(normal, -view_dir);
    float diffuse = clamp(dot(normal, light_dir), 0.0, 1.0);

    if (n_dot_vp > 0.0f)
    {
        vec3 D = -view_dir;
        vec3 R = normalize(reflect(-light_dir, normal));

        float dir_scale = mix(pow(abs(n_dot_d), 8.0f), 1.0f - pow(abs(1.0f - n_dot_d), 4.0f), n_dot_d);
        specular = vec3(0.8f) * vec3(pow(max(dot(R, D), 0.0f), specular_power) * specular_scale * dir_scale)  * ocean_ubo.light_color;
    }

    float fresnel = clamp(pow(1.0f + n_dot_d, -fresnel_approx_pow_factor) * dyna_range, 0.0f, 1.0f);
    vec3 ambient = fresnel * ocean_ubo.ocean_color;
    vec3 water_color = (1.0f - fresnel) *  ocean_ubo.ocean_color * ocean_dark * diffuse;
    result = ambient + water_color + specular;

    // gamma correction
    result.r = pow(result.r, 1.0f / 2.2f);
    result.g = pow(result.g, 1.0f / 2.2f);
    result.b = pow(result.b, 1.0f / 2.2f);

    outFragColor = vec4(result, 1.0f);
}