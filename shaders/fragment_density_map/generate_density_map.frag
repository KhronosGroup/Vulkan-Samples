#version 450
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

layout(location = 0) out vec2 outFragColor;

// UBO with all parameters
layout(set = 0, binding = 0) uniform UBO
{
	vec4 eye_center;
	vec4 circle_radius;
}
u_ubo;

#include "generate_fdm.glsl"

void main()
{
	vec2 eye_distance_vec = vec2(gl_FragCoord.xy) - u_ubo.eye_center.xy;
	outFragColor          = get_fragment_size_from_eye_distance(eye_distance_vec, u_ubo.circle_radius);
}
