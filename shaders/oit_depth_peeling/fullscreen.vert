#version 450
/* Copyright (c) 2024, Google
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

layout (location = 0) out vec2 outUV;

////////////////////////////////////////

void main()
{
	const vec4 fullscreen_triangle[] =
	{
		vec4(-1.0f,  3.0f, 0.0f, 2.0f),
		vec4(-1.0f, -1.0f, 0.0f, 0.0f),
		vec4( 3.0f, -1.0f, 2.0f, 0.0f),
	};
	const vec4 vertex = fullscreen_triangle[gl_VertexIndex % 3];
	gl_Position = vec4(vertex.xy, 0.0f, 1.0f);
	outUV = vertex.zw;
}

