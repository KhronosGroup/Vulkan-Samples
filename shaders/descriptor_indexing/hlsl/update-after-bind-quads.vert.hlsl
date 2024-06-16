/* Copyright (c) 2024, Sascha Willems
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

struct Registers
{
    float phase;
};
[[vk::push_constant]] Registers registers;

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float2 UV : TEXCOORD0;
};

VSOutput main(uint VertexIndex : SV_VertexID, uint InstanceIndex : SV_InstanceID)
{
    VSOutput output = (VSOutput) 0;

    float2 local_offset = float2(VertexIndex & 1, VertexIndex >> 1);
    output.UV = local_offset;

    // A lazy quad rotation, could easily have been precomputed on CPU.
    float cos_phase = cos(registers.phase);
    float sin_phase = sin(registers.phase);
    local_offset = mul(float2x2(cos_phase, -sin_phase, sin_phase, cos_phase), (local_offset - 0.5));

    // To keep the sample as simple as possible, use InstanceIndex to move the quads around on screen.
    int instance_x = InstanceIndex % 8 + 8;
    int instance_y = InstanceIndex / 8;
    float2 instance_offset = float2(instance_x, instance_y) / float2(15.0, 7.0);
    instance_offset = 2.1 * (instance_offset - 0.5);

    output.Pos = float4((0.10 * local_offset) + instance_offset, 0.0, 1.0);

    return output;
}
