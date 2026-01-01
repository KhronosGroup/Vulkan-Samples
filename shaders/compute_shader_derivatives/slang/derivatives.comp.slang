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

// Compute shader demonstrating derivatives in compute with a practical use case.
// This shader computes a procedural 2D function and uses derivatives to calculate
// gradient magnitude - demonstrating edge detection and spatial analysis capabilities.
// Note: ddx/ddy in compute require VK_KHR_compute_shader_derivatives at runtime.

// Output image for gradient magnitude visualization
[[vk::binding(0, 0)]] RWTexture2D<float4> gOutputImage : register(u0, space0);

// 8x8 local size for efficient compute with proper derivative quad coverage
[shader("compute")]
[numthreads(8, 8, 1)]
[DerivativeGroupQuad]  // Enable quad-based derivative computation
void main(uint3 tid : SV_DispatchThreadID)
{
    // Get image dimensions
    uint2 dims;
    gOutputImage.GetDimensions(dims.x, dims.y);
    
    if (tid.x >= dims.x || tid.y >= dims.y)
        return;
    
    // Normalized coordinates [0, 1]
    float2 uv = float2(tid.xy) / float2(dims - 1);
    
    // Create an interesting procedural function: radial gradient with modulation
    // This demonstrates a practical use case for derivatives in compute
    float2 center = float2(0.5, 0.5);
    float2 delta = uv - center;
    float dist = length(delta);
    
    // Procedural function with spatial variation
    float value = sin(dist * 10.0) * 0.5 + 0.5;
    value *= (1.0 - smoothstep(0.0, 0.7, dist));
    
    // Compute derivatives of the function - this is the key feature!
    // These derivatives tell us how quickly the function changes spatially
    float dx = ddx(value);
    float dy = ddy(value);
    
    // Gradient magnitude - useful for edge detection, LOD selection, and filtering
    // High gradient = rapid spatial changes (edges)
    float gradientMag = sqrt(dx * dx + dy * dy);
    
    // Visualize gradient magnitude as edge detection
    // Scale up the gradient for better visibility
    float edgeIntensity = saturate(gradientMag * 10.0);
    
    // Create a color visualization:
    // - Base pattern in blue channel
    // - Edges (high gradient) in red/yellow
    float3 color;
    color.r = edgeIntensity;                    // Red for edges
    color.g = edgeIntensity * 0.5;              // Some yellow for strong edges
    color.b = value * (1.0 - edgeIntensity);    // Blue for the base pattern
    
    // Write visualization to output image
    gOutputImage[tid.xy] = float4(color, 1.0);
}
