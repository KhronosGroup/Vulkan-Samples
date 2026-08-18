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

layout(location = 0) in vec3 aPos;

// Instanced attributes
layout (location = 1) in vec3 instancePos;
layout (location = 2) in vec4 inColor;
layout (location = 3) in float instanceScale;

layout (binding = 0) uniform UBO
{
    mat4 projection;
    mat4 camera;
} ubo;

layout (location = 0) out vec4 outColor;

void main()
{
    outColor = inColor;
    vec4 locPos = vec4(aPos, 1.0);
    float eps = 0.00001;
    vec4 Pos = vec4((locPos.xyz * instanceScale) + instancePos, 1.0);
    Pos.x -= eps;
    Pos.y -= eps;
    Pos.z -= eps;
    gl_Position = ubo.projection * ubo.camera * Pos;
}