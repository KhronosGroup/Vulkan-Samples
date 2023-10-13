#version 450
/* Copyright (c) 2023, Maximilien Dagois
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

void main()
{
    const vec2 fullscreen_triangle[] =
    {
        vec2(-1.0f,  3.0f),
        vec2(-1.0f, -1.0f),
        vec2( 3.0f, -1.0f),
    };
    const vec2 vertex = fullscreen_triangle[gl_VertexIndex % 3];
    gl_Position = vec4(vertex, 0.0f, 1.0f);
}

