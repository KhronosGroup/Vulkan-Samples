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

layout(location = 0) in vec4  inColor;
layout(location = 1) in vec3  inConic;    // (c/det, -b/det, a/det) of view-space cov2D
layout(location = 2) in float inOpacity;
layout(location = 3) in vec2  inCoord;   // view-space offset from splat center

layout(location = 0) out vec4 outColor;

void main() {
    float x = inCoord.x;
    float y = inCoord.y;

    // Full anisotropic 2D Gaussian: -0.5 * [x,y] * cov2D^-1 * [x,y]^T
    // inConic = (c/det, -b/det, a/det) for cov2D = [[a,b],[b,c]]
    float power = -0.5 * (inConic.x * x * x + 2.0 * inConic.y * x * y + inConic.z * y * y);

    if (power > 0.0) {
        discard;
    }

    float alpha = exp(power) * inOpacity;

    if (alpha < 0.01) {
        discard;
    }

    // Premultiplied alpha for correct blending
    outColor = vec4(inColor.rgb * alpha, alpha);
}
