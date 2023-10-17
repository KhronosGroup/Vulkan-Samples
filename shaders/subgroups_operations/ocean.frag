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

void main() 
{
	vec3 light_pos = vec3(0.0f, 0.0f, -1.0f);
	vec3 ocean_color = vec3(0.0f, 0.2423423f, 0.434335435f);
	vec3 view_dir = normalize(vec3(cam.position.xyz - in_pos));
	vec3 ref_dir = reflect(-view_dir, in_normal);

	vec3 light_dir = normalize(light_pos - in_pos);

	float dot_l = max(dot(in_normal, light_dir), 0.0f);

	vec3 result = dot_l * ocean_color;
	result = result / (result + vec3(1.0f));
	// gamma correction
	result = pow(result, vec3(1.0f / 2.2f));

	outFragColor = vec4(result, 1.0f);
}
