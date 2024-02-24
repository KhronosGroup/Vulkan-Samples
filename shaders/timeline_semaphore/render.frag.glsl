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
#version 450

layout(set = 0, binding = 0) uniform sampler2D in_image;
layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 out_color;

vec4 convert_color(vec4 value)
{
    float gray = dot(value.rgb, vec3(0.3, 0.6, 0.1));
    return vec4(mix(vec3(gray), value.rgb, value.a), 1.0);
}

void main()
{
    out_color = convert_color(textureLod(in_image, in_uv, 0.0));
}
