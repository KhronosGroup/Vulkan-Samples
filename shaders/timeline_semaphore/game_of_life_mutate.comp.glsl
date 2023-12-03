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
#version 450
layout(local_size_x = 8, local_size_y = 8) in;

layout(rgba8, set = 0, binding = 0) writeonly uniform image2D ImageOutput;
layout(set = 1, binding = 0) uniform sampler2D ImageInput;

layout(push_constant) uniform Registers
{
    float counter;
} registers;

void main()
{
    vec4 v = texelFetch(ImageInput, ivec2(gl_GlobalInvocationID.xy), 0);

    // Increase intensity over time until the cell dies.
    if (any(notEqual(v.rgb, vec3(0.0))))
        v.w = max(v.w, registers.counter);
    else
        v.w = 0.0;

    imageStore(ImageOutput, ivec2(gl_GlobalInvocationID.xy), v);
}
