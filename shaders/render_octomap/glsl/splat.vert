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

// Per-splat attributes (instanced)
layout(location = 0) in vec3 inPosition;      // Splat center position
layout(location = 1) in vec4 inRotation;      // Quaternion rotation
layout(location = 2) in vec3 inScale;         // 3D scale factors
layout(location = 3) in float inOpacity;      // Opacity value
layout(location = 4) in vec3 inColor;         // RGB color

// Uniform buffer
layout(binding = 0) uniform UBO {
    mat4 projection;
    mat4 view;
    vec2 viewport;      // Viewport dimensions
    float focalX;       // Focal length X
    float focalY;       // Focal length Y
} ubo;

// Outputs to fragment shader
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outConic;       // 2D conic parameters for Gaussian
layout(location = 2) out float outOpacity;
layout(location = 3) out vec2 outCoord;       // Quad coordinate for Gaussian evaluation

// Quad vertices for billboard rendering
const vec2 quadVertices[4] = vec2[4](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0,  1.0)
);

// Convert quaternion to rotation matrix
mat3 quaternionToMatrix(vec4 q) {
    float x = q.x, y = q.y, z = q.z, w = q.w;
    return mat3(
        1.0 - 2.0*(y*y + z*z), 2.0*(x*y - w*z), 2.0*(x*z + w*y),
        2.0*(x*y + w*z), 1.0 - 2.0*(x*x + z*z), 2.0*(y*z - w*x),
        2.0*(x*z - w*y), 2.0*(y*z + w*x), 1.0 - 2.0*(x*x + y*y)
    );
}

void main() {
    // Get quad vertex for this instance
    int quadIdx = gl_VertexIndex % 4;
    vec2 quadPos = quadVertices[quadIdx];
    
    // Transform splat center to view space
    vec4 viewPos = ubo.view * vec4(inPosition, 1.0);
    
    // Build 3D covariance matrix from rotation and scale
    mat3 R = quaternionToMatrix(inRotation);
    mat3 S = mat3(
        inScale.x, 0.0, 0.0,
        0.0, inScale.y, 0.0,
        0.0, 0.0, inScale.z
    );
    mat3 M = R * S;
    mat3 cov3D = M * transpose(M);
    
    // Project 3D covariance to 2D screen space
    // Jacobian of perspective projection
    float z = viewPos.z;
    float z2 = z * z;
    mat3 J = mat3(
        ubo.focalX / z, 0.0, -ubo.focalX * viewPos.x / z2,
        0.0, ubo.focalY / z, -ubo.focalY * viewPos.y / z2,
        0.0, 0.0, 0.0
    );
    
    // 2D covariance in screen space
    mat3 viewRot = mat3(ubo.view);
    mat3 cov2D = J * viewRot * cov3D * transpose(viewRot) * transpose(J);
    
    // Extract 2D covariance parameters (symmetric matrix)
    float a = cov2D[0][0] + 0.3;  // Add small value for numerical stability
    float b = cov2D[0][1];
    float c = cov2D[1][1] + 0.3;
    
    // Compute eigenvalues for splat size
    float det = a * c - b * b;
    float trace = a + c;
    float gap = sqrt(max(0.0, trace * trace - 4.0 * det));
    float lambda1 = (trace + gap) * 0.5;
    float lambda2 = (trace - gap) * 0.5;
    
    // Splat radius (3 sigma covers 99.7% of Gaussian)
    float radius = 3.0 * sqrt(max(lambda1, lambda2));
    
    // Project center to screen
    vec4 clipPos = ubo.projection * viewPos;
    vec2 screenPos = clipPos.xy / clipPos.w;
    
    // Offset by quad position scaled by radius
    vec2 pixelOffset = quadPos * radius / ubo.viewport;
    
    // Final position
    gl_Position = vec4(screenPos + pixelOffset, clipPos.z / clipPos.w, 1.0);
    
    // Pass to fragment shader
    // Convert sRGB to linear (the GLTF specifies colorSpace: "BT.709-sRGB")
    vec3 linearColor = pow(inColor, vec3(2.2));
    outColor = vec4(linearColor, 1.0);
    outOpacity = inOpacity;
    outCoord = quadPos * radius;
    
    // Conic parameters for Gaussian evaluation (inverse of 2D covariance)
    float invDet = 1.0 / det;
    outConic = vec2(c * invDet, -b * invDet);  // Simplified for symmetric case
}
