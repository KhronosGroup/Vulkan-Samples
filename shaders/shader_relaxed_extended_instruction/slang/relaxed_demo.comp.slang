/* Copyright (c) 2026, Holochip Inc.
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

// This shader demonstrates a non-semantic extended instruction via
// printf. On SPIR-V this lowers to an OpExtInst in the
// NonSemantic.DebugPrintf instruction set, which interacts with
// SPV_KHR_non_semantic_info. VK_KHR_shader_relaxed_extended_instruction
// makes certain forward-reference cases in such extended instruction sets
// legal in SPIR-V.
//
// By using a struct with a method that calls printf, we create the pattern
// that should trigger circular dependencies in debug instructions, which
// would emit OpExtInstWithForwardRef instructions when compiled with rich
// debug info (e.g., NonSemantic.Shader.DebugInfo.100).
//
// Note: As of Slang in Vulkan SDK 1.4.328, the compiler does not yet emit
// the rich debug info needed to trigger OpExtInstWithForwardRef. This shader
// is structured correctly and will emit OpExtInstWithForwardRef once Slang's
// debug info support is enhanced.

struct A {
    void foo(uint3 gid) {
        printf("relaxed-ext-inst demo: gid = %u", gid.x);
    }
};

[shader("compute")]
[numthreads(1, 1, 1)]
void main(uint3 gid : SV_DispatchThreadID)
{
    A a;
    a.foo(gid);
}
