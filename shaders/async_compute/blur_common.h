/* Copyright (c) 2021, Arm Limited and Contributors
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

#ifndef BLUR_COMMON_H_
#define BLUR_COMMON_H_

layout(set = 0, binding = 0) uniform sampler2D in_tex;
layout(rgba16f, set = 0, binding = 1) writeonly uniform image2D out_tex;

layout(push_constant) uniform Registers
{
	uvec2 resolution;
	vec2 inv_resolution;
	vec2 inv_input_resolution;
} registers;

vec2 get_uv(vec2 uv, float x, float y, float scale)
{
	return uv + registers.inv_input_resolution * (vec2(x, y) * scale);
}

vec3 bloom_blur(vec2 uv, float uv_scale)
{
	vec3 rgb = vec3(0.0);
	const float N = -1.0;
	const float Z = 0.0;
	const float P = 1.0;
	rgb += 0.25 * textureLod(in_tex, get_uv(uv, Z, Z, uv_scale), 0.0).rgb;
	rgb += 0.0625 * textureLod(in_tex, get_uv(uv, N, P, uv_scale), 0.0).rgb;
	rgb += 0.0625 * textureLod(in_tex, get_uv(uv, P, P, uv_scale), 0.0).rgb;
	rgb += 0.0625 * textureLod(in_tex, get_uv(uv, N, N, uv_scale), 0.0).rgb;
	rgb += 0.0625 * textureLod(in_tex, get_uv(uv, P, N, uv_scale), 0.0).rgb;
	rgb += 0.125 * textureLod(in_tex, get_uv(uv, N, Z, uv_scale), 0.0).rgb;
	rgb += 0.125 * textureLod(in_tex, get_uv(uv, P, Z, uv_scale), 0.0).rgb;
	rgb += 0.125 * textureLod(in_tex, get_uv(uv, Z, N, uv_scale), 0.0).rgb;
	rgb += 0.125 * textureLod(in_tex, get_uv(uv, Z, P, uv_scale), 0.0).rgb;
	return rgb;
}

#endif