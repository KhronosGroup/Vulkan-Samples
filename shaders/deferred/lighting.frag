#version 450
/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

layout(input_attachment_index = 0, binding = 0) uniform subpassInput i_depth;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput i_albedo;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput i_normal;

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 3) uniform GlobalUniform
{
    mat4 inv_view_proj;
    vec2 inv_resolution;
}
global_uniform;

struct Light
{
    vec4 position;         // position.w represents type of light
    vec4 color;            // color.w represents light intensity
    vec4 direction;        // direction.w represents range
    vec2 info;             // (only used for spot lights) info.x represents light inner cone angle, info.y represents light outer cone angle
};

layout(set = 0, binding = 4) uniform LightsInfo
{
    Light lights[MAX_DEFERRED_LIGHT_COUNT];
}
lights;

layout (constant_id = 0) const uint LIGHT_COUNT = 0U;
layout (constant_id = 1) const bool HAS_DIRECTIONAL_LIGHTS = false;
layout (constant_id = 2) const bool HAS_POINT_LIGHTS = false;
layout (constant_id = 3) const bool HAS_SPOT_LIGHTS = false;

vec3 apply_directional_light(uint index, vec3 normal)
{
    vec3 world_to_light = -lights.lights[index].direction.xyz;
    world_to_light = normalize(world_to_light);
    float ndotl = clamp(dot(normal, world_to_light), 0.0, 1.0);
    return ndotl * lights.lights[index].color.w * lights.lights[index].color.rgb;
}

vec3 apply_point_light(uint index, vec3 pos, vec3 normal)
{
    vec3 world_to_light = lights.lights[index].position.xyz - pos;
    float dist = length(world_to_light) * 0.005;
    float atten = 1.0 / (dist * dist);
    world_to_light = normalize(world_to_light);
    float ndotl = clamp(dot(normal, world_to_light), 0.0, 1.0);
    return ndotl * lights.lights[index].color.w * atten * lights.lights[index].color.rgb;
}

vec3 apply_spot_light(uint index, vec3 pos, vec3 normal)
{
    vec3 light_to_pixel = normalize(pos - lights.lights[index].position.xyz);
    float theta = dot(light_to_pixel, normalize(lights.lights[index].direction.xyz));
    float inner_cone_angle = lights.lights[index].info.x;
    float outer_cone_angle = lights.lights[index].info.y;
    float intensity = (theta - outer_cone_angle) / (inner_cone_angle - outer_cone_angle);
    return smoothstep(0.0, 1.0, intensity) * lights.lights[index].color.w * lights.lights[index].color.rgb;
}

void main()
{
    // Retrieve position from depth
    vec4  clip         = vec4(in_uv * 2.0 - 1.0, subpassLoad(i_depth).x, 1.0);
    highp vec4 world_w = global_uniform.inv_view_proj * clip;
    highp vec3 pos     = world_w.xyz / world_w.w;

    vec4 albedo = subpassLoad(i_albedo);

    // Transform from [0,1] to [-1,1]
    vec3 normal = subpassLoad(i_normal).xyz;
    normal      = normalize(2.0 * normal - 1.0);

    // Calculate lighting
    vec3 L = vec3(0.0);
    for (uint i = 0U; i < LIGHT_COUNT; i++)
    {
        if (HAS_DIRECTIONAL_LIGHTS && lights.lights[i].position.w == DIRECTIONAL_LIGHT)
        {
            L += apply_directional_light(i, normal);
        }
        else if (HAS_POINT_LIGHTS && lights.lights[i].position.w == POINT_LIGHT)
        {
            L += apply_point_light(i, pos, normal);
        }
        else if (HAS_SPOT_LIGHTS && lights.lights[i].position.w == SPOT_LIGHT)
        {
            L += apply_spot_light(i, pos, normal);
        }
    }

    vec3 ambient_color = vec3(0.2) * albedo.xyz;
    
    o_color = vec4(ambient_color + L * albedo.xyz, 1.0);
}