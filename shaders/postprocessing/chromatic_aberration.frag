#version 450
/* Copyright (c) 2024, Arm Limited and Contributors
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

layout(set = 0, binding = 1) uniform sampler2D color_sampler;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 0) uniform PostprocessingUniform
{
	float sin_elapsed_time;
}
postprocessing_uniform;

#define SCALE 0.002

#define RED_OFFSET 3
#define GREEN_OFFSET -1
#define BLUE_OFFSET 2

void main(void)
{
	o_color = vec4(1.0);

	const vec2 center_coord = vec2(0.5);
	const vec2 direction    = (in_uv - center_coord) * postprocessing_uniform.sin_elapsed_time * SCALE;

	o_color.r = texture(color_sampler, in_uv - (direction * vec2(RED_OFFSET))).r;
	o_color.g = texture(color_sampler, in_uv - (direction * vec2(GREEN_OFFSET))).g;
	o_color.b = texture(color_sampler, in_uv - (direction * vec2(BLUE_OFFSET))).b;
}
