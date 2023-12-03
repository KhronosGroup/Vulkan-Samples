#version 450
/* Copyright (c) 2021-2023, Arm Limited and Contributors
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

// Trivial shader that blits compute result to screen.

layout(location = 0) out vec2 o_uv;

void main()
{
    if (gl_VertexIndex == 0)
    gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);
    else if (gl_VertexIndex == 1)
    gl_Position = vec4(-1.0, 3.0, 0.0, 1.0);
    else
    gl_Position = vec4(3.0, -1.0, 0.0, 1.0);

    o_uv = gl_Position.xy * 0.5 + 0.5;
}
