/* Copyright (c) 2021-2023, Arm Limited and Contributors
 * Copyright (c) 2023, Sascha Willems
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

// Generate an image with a random pattern based on a timer
__kernel void generate_texture(__read_write image2d_t dst_img, float time)
{
	int x      = get_global_id(0);
	int y      = get_global_id(1);
	int width  = get_global_size(0);
	int height = get_global_size(1);

	float dx = (x / (float) width - 0.5f) * 2.0f;
	float dy = (y / (float) height - 0.5f) * 2.0f;

	float dist = sqrt(dx * dx + dy * dy);

	uint4 val;
	val.s0 = (cos(dist * 25.0f - time * 5.0f) / 2.0f + 0.5f) * 255;
	val.s1 = (cos(dx * 50.0f) / 2.0f + 0.5f) * 255;
	val.s2 = (cos(dy * 50.0f) / 2.0f + 0.5f) * 255;
	val.s3 = 255;
	write_imageui(dst_img, (int2) (x, y), val);
}