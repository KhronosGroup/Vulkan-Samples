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

#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform texture2D Textures[];
layout(set = 1, binding = 0) uniform sampler ImmutableSampler;

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 out_frag_color;

layout(push_constant) uniform Registers
{
    // Offset 0 is used by vertex.
    layout(offset = 4) uint table_offset;
} registers;

void main()
{
    // This is a very common usage pattern for streamed descriptors with UPDATE_AFTER_BIND.
    // We only need to update push constants, and our materials can access new descriptors.
    // This avoids having to allocate and manage individual descriptor sets.
    // It does mean that the chance to introduce bugs is higher however ...

    // A push constant must be dynamically uniform over our draw call, so we do not have to do anything here.
    // This is simply considered dynamic indexing, which is a feature in core Vulkan 1.0.
    out_frag_color = texture(sampler2D(Textures[registers.table_offset], ImmutableSampler), in_uv);
}
