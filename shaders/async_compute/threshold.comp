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

layout(local_size_x = 8, local_size_y = 8) in;

layout(set = 0, binding = 0) uniform sampler2D in_tex;
layout(rgba16f, set = 0, binding = 1) writeonly uniform image2D out_tex;

layout(push_constant) uniform Registers
{
    uvec2 resolution;
    vec2 inv_resolution;
    vec2 inv_input_resolution;
} registers;

void main()
{
    if (all(lessThan(gl_GlobalInvocationID.xy, registers.resolution)))
    {
        vec2 uv = (vec2(gl_GlobalInvocationID.xy) + 0.5) * registers.inv_resolution;
        vec3 rgb = textureLod(in_tex, uv, 0.0).rgb;
        rgb = 0.2 * max(rgb - 1.0, vec3(0.0));
        imageStore(out_tex, ivec2(gl_GlobalInvocationID.xy), vec4(rgb, 1.0));
    }
}