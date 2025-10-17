/* Copyright (c) 2025, Holochip Inc
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

// Minimal Slang compute shader demonstrating derivatives in compute.
// Note: ddx/ddy in compute require VK_KHR_compute_shader_derivatives at runtime.
// Slang will emit the appropriate SPIR-V instructions/capabilities when targeting SPIR-V.

// Result buffer: each invocation writes { v, ddx(v), ddy(v), padding }
[[vk::binding(0, 0)]] RWStructuredBuffer<float4> gResults : register(u0, space0);

// 4x4 local size so derivatives over 2x2 quads are well-defined in the group
[shader("compute")]
[numthreads(4, 4, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    float v = float(tid.x) + float(tid.y) * 4.0;

    // Compute derivatives across the derivative group (quads on most implementations)
    float dx = ddx(v);
    float dy = ddy(v);

    // Write results to SSBO for CPU-side printing
    uint index = tid.y * 4 + tid.x;
    gResults[index] = float4(v, dx, dy, 0.0);
}
