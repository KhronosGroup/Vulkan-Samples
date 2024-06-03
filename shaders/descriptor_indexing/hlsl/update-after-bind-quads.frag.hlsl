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

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float2 UV : TEXCOORD0;
};

[[vk::binding(0, 0)]]
Texture2D Textures[] : register(t0, space0);
[[vk::binding(0, 1)]]
SamplerState ImmutableSampler : register(s0, space1);

struct Registers
{
    [[vk::offset(4)]] uint table_offset;
};
[[vk::push_constant]] Registers registers;

float4 main(VSOutput input) : SV_TARGET0
{
    // This is a very common usage pattern for streamed descriptors with UPDATE_AFTER_BIND.
    // We only need to update push constants, and our materials can access new descriptors.
    // This avoids having to allocate and manage individual descriptor sets.
    // It does mean that the chance to introduce bugs is higher however ...

    // A push constant must be dynamically uniform over our draw call, so we do not have to do anything here.
    // This is simply considered dynamic indexing, which is a feature in core Vulkan 1.0.
    return Textures[registers.table_offset].Sample(ImmutableSampler, input.UV);
}
