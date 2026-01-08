/* Copyright (c) 2026, Holochip Inc
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

// Compute shader demonstrating derivatives in compute (Linear mode).
// Many implementations only support computeDerivativeGroupLinear.
// Mapping 1D lanes to 2D coordinates for linear groups follows the DX SM 6.6 spec:
// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_Derivatives.html
// Intuition: lanes are ordered linearly across the group; hardware derives a
// consistent local 2D mapping to evaluate ddx/ddy even when quads aren't supported.
// Note: ddx/ddy in compute require VK_KHR_compute_shader_derivatives at runtime.
// Tooling note: If your toolchain emits SPV_NV_compute_shader_derivatives instead of SPV_KHR,
// please update to a newer Vulkan SDK, glslang, and SPIR-V Tools that support the KHR variant.

// Output image for gradient magnitude visualization
[[vk::binding(0, 0)]] RWTexture2D<float4> gOutputImage : register(u0, space0);

// 8x8 local size; derivative groups are defined by implementation in Linear mode
[shader("compute")]
[numthreads(8, 8, 1)]
[DerivativeGroupLinear]  // Enable linear-based derivative computation
void main(uint3 tid : SV_DispatchThreadID)
{
    // Get image dimensions
    uint2 dims;
    gOutputImage.GetDimensions(dims.x, dims.y);
    
    if (tid.x >= dims.x || tid.y >= dims.y)
        return;
    
    // Normalized coordinates [0, 1]
    float2 uv = float2(tid.xy) / float2(dims - 1);
    
    // Procedural function (same as Quad variant) to produce identical output
    float2 center = float2(0.5, 0.5);
    float2 delta = uv - center;
    float dist = length(delta);
    
    float value = sin(dist * 10.0) * 0.5 + 0.5;
    value *= (1.0 - smoothstep(0.0, 0.7, dist));
    
    // Derivatives of the function
    float dx = ddx(value);
    float dy = ddy(value);
    
    float gradientMag = sqrt(dx * dx + dy * dy);
    float edgeIntensity = saturate(gradientMag * 10.0);
    
    float3 color;
    color.r = edgeIntensity;
    color.g = edgeIntensity * 0.5;
    color.b = value * (1.0 - edgeIntensity);
    
    gOutputImage[tid.xy] = float4(color, 1.0);
}
