#version 450
/* Copyright (c) 2023, Mobica Limited
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

layout (input_attachment_index = 0, binding = 0) uniform subpassInput in_color_r;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput in_color_g;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput in_color_b;

layout (location = 0) out vec4 outColor;

void main()
{
    vec4 color_r = subpassLoad(in_color_r);
    vec4 color_g = subpassLoad(in_color_g);
    vec4 color_b = subpassLoad(in_color_b);

    outColor = color_r + color_g + color_b;
}
