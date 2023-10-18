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

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;

layout (location = 0) out vec4 outFragColor;

layout (binding = 3) uniform CameraPos
{
    vec4 position;
} cam;

layout (binding = 5) uniform OceanParamsUbo
{
	vec3 light_color;
	vec3 light_position;
	vec3 ocean_color;
} ocean_ubo;

void main() 
{
	vec3 normal = in_normal;
	vec3 light_pos = ocean_ubo.light_position;
	vec3 light_color = ocean_ubo.light_color;
	vec3 ocean_color = ocean_ubo.ocean_color;
	float ambient_strength = 0.91f;

	vec3 ambient = ambient_strength * light_color;

	vec3 light_dir = normalize(light_pos - in_pos);
	float diff = max(dot(normal, light_dir), 0.0f);
	vec3 diffuse = diff * light_color;

	float specular_strength = 0.5f;
	vec3 view_dir = normalize(vec3(cam.position.xyz - in_pos));
	vec3 ref_dir = reflect(-light_dir, normal);
	float spec = pow(max(dot(view_dir, ref_dir), 0.0f), 32);
	vec3 specular = specular_strength * spec * light_color;

	vec3 result = (ambient + diffuse + specular) * ocean_color;
	result = result / (result + vec3(1.0f));

	// gamma correction
	result = pow(result, vec3(1.0f / 2.2f));

	outFragColor = vec4(result, 1.0f);
}
