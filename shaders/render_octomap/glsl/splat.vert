/* Copyright (c) 2024-2026, Holochip Inc.
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
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inRotation;
layout(location = 2) in vec3 inScale;
layout(location = 3) in float inOpacity;
layout(location = 4) in vec3 inColor;

// Uniform buffer — layout must match the C++ splat_ubo struct
layout(binding = 0) uniform UBO {
    mat4  projection;
    mat4  view;
    vec2  viewport;   // not used in this shader, kept for struct compatibility
    float focalX;     // not used
    float focalY;     // not used
} ubo;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 outConic;   // inverse 2D covariance (c/det, -b/det, a/det)
layout(location = 2) out float outOpacity;
layout(location = 3) out vec2 outCoord;   // view-space offset from splat center

const vec2 quadVertices[4] = vec2[4](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0,  1.0)
);

mat3 quaternionToMatrix(vec4 q) {
    float x = q.x, y = q.y, z = q.z, w = q.w;
    float x2 = x * 2.0, y2 = y * 2.0, z2 = z * 2.0;
    float xx = x * x2, xy = x * y2, xz = x * z2;
    float yy = y * y2, yz = y * z2, zz = z * z2;
    float wx = w * x2, wy = w * y2, wz = w * z2;
    return mat3(
        1.0 - (yy + zz), xy - wz,         xz + wy,
        xy + wz,         1.0 - (xx + zz), yz - wx,
        xz - wy,         yz + wx,         1.0 - (xx + yy)
    );
}

void main() {
    vec2 quadPos = quadVertices[gl_VertexIndex % 4];

    // Transform splat center to view space
    vec4 viewCenter = ubo.view * vec4(inPosition, 1.0);

    // There is no vertex-shader discard, so culling is done by placing the quad outside the
    // clip volume: with w = 1, clip coordinates of (2, 2, 2) divide down to NDC x/y = 2, which
    // is outside Vulkan's [-1, 1] NDC range on those axes alone and is therefore rejected by
    // clipping before the rasterizer ever sees it (NDC z = 2 is also outside Vulkan's [0, 1]
    // depth range, reinforcing the cull). The other outputs are zeroed only so a culled vertex
    // can't contribute visible color/opacity if it somehow survives (e.g. via flat interpolation).

    // Cull splats at or behind the near plane — they produce inverted/degenerate quads.
    if (viewCenter.z >= -0.001) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        outColor    = vec4(0.0);
        outConic    = vec3(0.0);
        outOpacity  = 0.0;
        outCoord    = vec2(0.0);
        return;
    }

    float splatDepth = -viewCenter.z; // positive distance in front of camera

    // Cull unconverged / degenerate splats whose world-scale exceeds their depth (same
    // outside-clip-volume technique as above).
    float maxScale = max(inScale.x, max(inScale.y, inScale.z));
    if (maxScale > splatDepth) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        outColor    = vec4(0.0);
        outConic    = vec3(0.0);
        outOpacity  = 0.0;
        outCoord    = vec2(0.0);
        return;
    }

    // Build 3D covariance from rotation and scale
    mat3 R    = quaternionToMatrix(inRotation);
    mat3 S    = mat3(inScale.x, 0.0, 0.0,
                     0.0, inScale.y, 0.0,
                     0.0, 0.0, inScale.z);
    mat3 RS   = R * S;
    mat3 cov3D = RS * transpose(RS);

    // Project 3D covariance into view space and take the top-left 2x2
    mat3 W         = mat3(ubo.view);
    mat3 cov3D_view = W * cov3D * transpose(W);
    cov3D_view[0][0] += 1e-4;
    cov3D_view[1][1] += 1e-4;

    // cov2D: a = [0][0], b = [0][1], c = [1][1]  (symmetric 2x2)
    float a = cov3D_view[0][0];
    float b = cov3D_view[0][1];
    float c = cov3D_view[1][1];

    // Per-axis billboard extents (3 sigma), clamped to depth to avoid screen-filling quads
    float extentX = min(3.0 * sqrt(max(1e-6, a)), splatDepth);
    float extentY = min(3.0 * sqrt(max(1e-6, c)), splatDepth);

    // View-space coordinate for this quad vertex
    outCoord = vec2(quadPos.x * extentX, quadPos.y * extentY);

    // Place quad vertex in view space and project
    vec3 quadViewPos = viewCenter.xyz + vec3(1.0, 0.0, 0.0) * outCoord.x
                                      + vec3(0.0, 1.0, 0.0) * outCoord.y;
    gl_Position = ubo.projection * vec4(quadViewPos, 1.0);

    // Inverse 2D covariance for fragment-shader Gaussian evaluation
    float det = a * c - b * b;
    if (det < 1e-14) det = 1e-14;
    outConic = vec3(c, -b, a) / det;   // (c/det, -b/det, a/det)

    // Color is stored in display space — pass through without gamma modification
    outColor   = vec4(inColor, 1.0);
    outOpacity = inOpacity;
}
