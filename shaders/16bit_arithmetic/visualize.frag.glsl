#version 450
/* Copyright (c) 2020-2023, Arm Limited and Contributors
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

layout(location = 0) out vec4 o_color;
layout(location = 0) in vec2 in_uv;
layout(set = 0, binding = 0) uniform sampler2D tex;

void main()
{
    o_color = vec4(textureLod(tex, in_uv, 0.0).rgb, 1.0);
}
