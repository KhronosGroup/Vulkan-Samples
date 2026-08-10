/* Copyright (c) 2026, NVIDIA CORPORATION
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

layout (location = 0) out vec4 outColor;

// Push constants for window resolution, camera, and time
layout(push_constant) uniform PushConstants {
    vec2 resolution;
    vec3 position;
    vec3 color;
} pushConstants;

void main()
{
    vec2 uv = (gl_FragCoord.xy - 0.5 * pushConstants.resolution) / pushConstants.resolution.y;

    // Circle parameters
    vec2 pos = pushConstants.position.xy;
    float radius = 0.2;

    // 2D SDF for circle
    float dist = length(uv - pos) - radius;

    if (dist > 0.0)
    {
        discard;
    }

    outColor = vec4(pushConstants.color, 1.0);
}
