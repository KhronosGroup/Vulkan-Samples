#version 450
/* Copyright (c) 2023, Mobica Limited
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

layout (binding = 1) uniform Col
{
        vec4 data[8];
} color;


layout (location = 0) in vec2 inUV;
layout (location = 1) flat in uint colorOffset;

layout (location = 0) out vec4 outColor;

vec4 sampleTexture(vec2 uv)
{
    vec4 c00 = color.data[0 + colorOffset];
    vec4 c01 = color.data[1 + colorOffset];
    vec4 c02 = color.data[2 + colorOffset];
    vec4 c03 = color.data[3 + colorOffset];

    vec4 b0 = mix(c00, c01, uv.x);
    vec4 b1 = mix(c02, c03, uv.x);

    vec4 p0 = mix(b0, b1, uv.y);

    return p0;
}

void main()
{
    outColor = sampleTexture(inUV);
}








