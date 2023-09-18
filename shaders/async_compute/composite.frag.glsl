#version 450
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

layout(location = 0) out vec4 o_color;
layout(location = 0) in vec2 in_uv;
layout(set = 0, binding = 0) uniform sampler2D hdr_tex;
layout(set = 0, binding = 1) uniform sampler2D bloom_tex;

void main()
{
    // The most basic tonemap possible.
    vec3 col = textureLod(hdr_tex, in_uv, 0.0).rgb + textureLod(bloom_tex, in_uv, 0.0).rgb;
    col = col / (1.0 + col);
    o_color = vec4(col, 1.0);
}
