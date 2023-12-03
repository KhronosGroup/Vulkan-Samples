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
#extension GL_EXT_shader_explicit_arithmetic_types_float16: require

layout(location = 0) out f16vec4 o_color;
layout(location = 0) in f16vec3 in_color;
layout(location = 1) in f16vec3 in_normal;
layout(location = 2) in f16vec3 in_delta_pos;

void main()
{
    vec3 normal_color = vec3(in_normal) * 0.5 + 0.5;
    vec4 color = vec4(mix(vec3(in_color), normal_color, 0.3), 1.0);
    color.rgb -= 0.2 * fract(2.0 * vec3(in_delta_pos));
    o_color = f16vec4(color);
}
