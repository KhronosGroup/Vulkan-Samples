/* Copyright (c) 2021-2026, Arm Limited and Contributors
/* Copyright (c) 2026, Sascha Willems
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

[[vk::binding(0, 0)]] Sampler2D in_tex;
[[vk::image_format("rgba16f")]] [[vk::binding(1, 0)]] RWTexture2D<float4> out_tex;

struct Registers
{
	uint2  resolution;
	float2 inv_resolution;
	float2 inv_input_resolution;
};

[[vk::push_constant]] Registers registers;

float2 get_uv(float2 uv, float x, float y, float scale)
{
	return uv + registers.inv_input_resolution * (float2(x, y) * scale);
}

float3 bloom_blur(float2 uv, float uv_scale)
{
	float3      rgb = float3(0.0);
	const float N   = -1.0;
	const float Z   = 0.0;
	const float P   = 1.0;
	rgb += 0.25 * in_tex.SampleLevel(get_uv(uv, Z, Z, uv_scale), 0.0).rgb;
	rgb += 0.0625 * in_tex.SampleLevel(get_uv(uv, N, P, uv_scale), 0.0).rgb;
	rgb += 0.0625 * in_tex.SampleLevel(get_uv(uv, P, P, uv_scale), 0.0).rgb;
	rgb += 0.0625 * in_tex.SampleLevel(get_uv(uv, N, N, uv_scale), 0.0).rgb;
	rgb += 0.0625 * in_tex.SampleLevel(get_uv(uv, P, N, uv_scale), 0.0).rgb;
	rgb += 0.125 * in_tex.SampleLevel(get_uv(uv, N, Z, uv_scale), 0.0).rgb;
	rgb += 0.125 * in_tex.SampleLevel(get_uv(uv, P, Z, uv_scale), 0.0).rgb;
	rgb += 0.125 * in_tex.SampleLevel(get_uv(uv, Z, N, uv_scale), 0.0).rgb;
	rgb += 0.125 * in_tex.SampleLevel(get_uv(uv, Z, P, uv_scale), 0.0).rgb;
	return rgb;
}

#endif