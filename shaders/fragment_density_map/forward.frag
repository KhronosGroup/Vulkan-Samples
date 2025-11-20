#version 320 es
/* Copyright (c) 2025, Arm Limited and Contributors
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

precision mediump float;

precision highp float;

layout(set = 0, binding = 1) uniform sampler2D base_color_texture;

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_normal;

layout(location = 0) out vec4 o_color;

#include "lighting.h"

void main(void)
{
	vec3 normal = normalize(in_normal);

	Light directional_light;
	directional_light.color     = vec4(0.95, 0.87, 0.85, 1);
	directional_light.direction = vec4(normalize(vec3(1)), 0);

	vec3 light_contribution = apply_directional_light(directional_light, normal);

	vec4 base_color = vec4(1.0, 0.0, 0.0, 1.0);

	base_color = texture(base_color_texture, in_uv);

	vec3 ambient_color = vec3(0.25) * base_color.xyz;

	o_color     = vec4(ambient_color + light_contribution * base_color.xyz, base_color.w);
}
