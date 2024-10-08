#version 450
/* Copyright (c) 2023, Mobica Limited
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

#define LAMBERT_VAL 0.3183098861837907f

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_uv;

layout (location = 0) out vec4 outFragColor;

layout (binding = 0) uniform Ubo
{
    mat4 projection;
    mat4 view;
    mat4 model;
} ubo;

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


void main()
{

    // TODO: implement BRDF reflections; code below is temporary solution
    vec3 normal = normalize(texture(fft_normal_map, in_uv).rgb);
    vec3 ambient = ocean_ubo.light_color * ocean_ubo.ocean_color;
    vec3 light_dir = normalize(ocean_ubo.light_position - in_pos);
    float diff = max(dot(normal, light_dir), 0.0f);
    vec3 diffuse = ambient * diff;

    float n_dot_l = max(0.0f, dot(normal, light_dir));
    vec3 view_dir = normalize(cam.position.xyz - in_pos.xyz);

    vec3 halfVec = normalize(light_dir + view_dir);
    float n_dot_h = clamp(dot(normal, halfVec), 0.0f, 1.0f);
    float spec = pow(n_dot_h, 50.0f) * 0.5f;
    vec4 col = vec4(ambient * n_dot_l + spec, 1.0f);
    col.r = pow(col.r, 1.0/ 2.2);
    col.g = pow(col.g, 1.0/ 2.2);
    col.b = pow(col.b, 1.0/ 2.2);

    outFragColor = col; // texture(fft_normal_map, in_uv); // vec4(diffuse, 1.0f);
}
