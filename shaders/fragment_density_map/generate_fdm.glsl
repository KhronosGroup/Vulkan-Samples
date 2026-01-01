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

// We want the current fragment density map to look like concentric circles
vec2 get_fragment_size_from_eye_distance(vec2 eye_distance_vec, vec4 circle_radius)
{
	float eye_distance = length(eye_distance_vec);
	if (eye_distance < circle_radius.x)
	{
		return vec2(1.0);
	}
	else if (eye_distance < circle_radius.y)
	{
		if (abs(eye_distance_vec.x) < abs(eye_distance_vec.y))
		{
			return vec2(0.49, 1.0);
		}
		else
		{
			return vec2(1.0, 0.49);
		}
	}
	else if (eye_distance < circle_radius.z)
	{
		return vec2(0.49);
	}
	else if (eye_distance < circle_radius.w)
	{
		if (abs(eye_distance_vec.x) < abs(eye_distance_vec.y))
		{
			return vec2(0.24, 0.49);
		}
		else
		{
			return vec2(0.49, 0.24);
		}
	}
	else
	{
		return vec2(0.24);
	}
}
