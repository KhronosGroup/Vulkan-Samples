/* Copyright (c) 2024-2025, Holochip Inc.
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
#extension GL_ARB_separate_shader_objects : enable

// Inputs from vertex shader
layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inConic;       // 2D conic parameters (inverse covariance)
layout(location = 2) in float inOpacity;
layout(location = 3) in vec2 inCoord;       // Quad coordinate for Gaussian evaluation

// Output
layout(location = 0) out vec4 outColor;

void main() {
    // Evaluate 2D Gaussian function
    // G(x,y) = exp(-0.5 * (x,y) * Sigma^-1 * (x,y)^T)
    // where Sigma^-1 is the inverse covariance matrix
    
    // For a symmetric 2x2 matrix [[a, b], [b, c]], the quadratic form is:
    // a*x^2 + 2*b*x*y + c*y^2
    // We pass conic = (c/det, -b/det) from vertex shader
    // and need to reconstruct the full inverse
    
    float x = inCoord.x;
    float y = inCoord.y;
    
    // Simplified Gaussian evaluation using distance from center
    // This is an approximation - full implementation would use the conic parameters
    float power = -0.5 * (x * x + y * y);
    
    // Gaussian falloff
    float alpha = exp(power);
    
    // Apply opacity
    float finalAlpha = alpha * inOpacity;
    
    // Discard fragments with very low alpha for performance
    if (finalAlpha < 0.004) {
        discard;
    }
    
    // Output with premultiplied alpha for proper blending
    outColor = vec4(inColor.rgb * finalAlpha, finalAlpha);
}
