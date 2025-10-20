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


#	extension GL_EXT_fragment_invocation_density : enable

vec3 get_fdm_debug_color(vec3 original_color)
{
		const float debug_fdm_factor = 0.2f;
		original_color *= (1.0f - debug_fdm_factor);

		const vec3 red     = vec3(debug_fdm_factor, 0.0, 0.0);
		const vec3 green   = vec3(0.0, debug_fdm_factor, 0.0);
		const vec3 blue    = vec3(0.0, 0.0, debug_fdm_factor);
		const vec3 cyan    = vec3(0.0, debug_fdm_factor, debug_fdm_factor);
		const vec3 magenta = vec3(debug_fdm_factor, 0.0, debug_fdm_factor);
		const vec3 yellow  = vec3(debug_fdm_factor, debug_fdm_factor, 0.0);
		const vec3 orange  = vec3(debug_fdm_factor, 0.5 * debug_fdm_factor, 0.0);
		const vec3 purple  = vec3(0.5 * debug_fdm_factor, 0.0, 0.5 * debug_fdm_factor);
		const vec3 pink    = vec3(debug_fdm_factor, 0.0, 0.5 * debug_fdm_factor);

		if (all(equal(gl_FragSizeEXT, ivec2(1))))
		{
			original_color += green;
		}
		else if (all(equal(gl_FragSizeEXT, ivec2(1, 2))))
		{
			original_color += red;
		}
		else if (all(equal(gl_FragSizeEXT, ivec2(2, 1))))
		{
			original_color += blue;
		}
		else if (all(equal(gl_FragSizeEXT, ivec2(2))))
		{
			original_color += cyan;
		}
		else if (all(equal(gl_FragSizeEXT, ivec2(2, 4))))
		{
			original_color += magenta;
		}
		else if (all(equal(gl_FragSizeEXT, ivec2(4, 2))))
		{
			original_color += yellow;
		}
		else if (all(equal(gl_FragSizeEXT, ivec2(4))))
		{
			original_color += orange;
		}
		else if (any(lessThan(gl_FragSizeEXT, ivec2(1))))
		{
			original_color += purple;
		}
		else
		{
			original_color += pink;
		}
	return original_color;
}
