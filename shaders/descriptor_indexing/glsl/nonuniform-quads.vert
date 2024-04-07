#version 450
/* Copyright (c) 2021-2024, Arm Limited and Contributors
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

layout(location = 0) out vec2 out_uv;
layout(location = 1) flat out int out_texture_index;

layout(push_constant) uniform Registers
{
    float phase;
} registers;

void main()
{
    vec2 local_offset = vec2(gl_VertexIndex & 1, gl_VertexIndex >> 1);
    out_uv = local_offset;

    // A lazy quad rotation, could easily have been precomputed on CPU.
    float cos_phase = cos(registers.phase);
    float sin_phase = sin(registers.phase);
    local_offset = mat2(cos_phase, -sin_phase, sin_phase, cos_phase) * (local_offset - 0.5);

    // To keep the sample as simple as possible, use gl_InstanceIndex to move the quads around on screen.
    int instance_x = gl_InstanceIndex % 8;
    int instance_y = gl_InstanceIndex / 8;
    vec2 instance_offset = vec2(instance_x, instance_y) / vec2(15.0, 7.0);
    instance_offset = 2.1 * (instance_offset - 0.5);

    gl_Position = vec4((0.10 * local_offset) + instance_offset, 0.0, 1.0);

    // Pass down an index which we will use to index into the descriptor array.
    out_texture_index = gl_InstanceIndex;
}
