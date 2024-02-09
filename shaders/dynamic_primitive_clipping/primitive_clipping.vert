#version 450
/* Copyright (c) 2024, Mobica Limited
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

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

// In a fixed pipeline approach which you can still find in OpenGL implementations you could define 
// up to maximum 6 clipping half-spaces and a distance to each clipping half-space was stored in gl_ClipDistance.
// That's why gl_ClipDistance[] is an array which maximum size is limited by maxClipDistances.
//
// In our example, instead of using half-spaces we will use some more advanced math functions to 
// calculate gl_ClipDistance and it's enough to use only one value from gl_ClipDistance[] array.

out gl_PerVertex 
{
    vec4  gl_Position;
    float gl_ClipDistance[1];
};

layout (binding = 0) uniform Ubo
{
    mat4 projection;
    mat4 view;
    mat4 model;
    vec4 colorTransformation;
    ivec2 sceneTransformation;
} ubo;

layout (location = 0) out vec3 outNormal;

// Cases 0 and 1 present how to use world space coordinates
// Cases 2-6 present how to use clip space coordinates
// Cases 0-3 use sin() function to create strips in which values of gl_ClipDistance below 0 cause triangle primitives
// to be clipped according to Vulkan specification chapter 27.4
// Cases 4-6 use different types of distance functions from center of the screen ( Euclidean, Manhattan, Chebyshev )

void main()
{
    vec4 worldPosition =  ubo.model * vec4(inPos, 1.0);
    gl_Position        = ubo.projection * ubo.view * worldPosition;

    float clipResult = 1.0, tmp;
    float distance = 0.4;

    switch(ubo.sceneTransformation.x)
    {
    case 0:
        clipResult = sin(worldPosition.x * 0.1 * 2.0 * 3.1415);
        break;
    case 1:
        clipResult = sin(worldPosition.y * 0.1 * 2.0 * 3.1415);
        break;
    case 2:
        clipResult = sin(gl_Position.x / gl_Position.w * 3.0 * 2.0 * 3.1415);
        break;
    case 3:
        clipResult = sin(gl_Position.y / gl_Position.w * 3.0 * 2.0 * 3.1415);
        break;
    case 4:
        clipResult = (gl_Position.x*gl_Position.x + gl_Position.y*gl_Position.y) / (gl_Position.w*gl_Position.w) - distance*distance;
        break;
    case 5:
        clipResult = (abs(gl_Position.x) + abs(gl_Position.y)) / gl_Position.w - distance;
        break;
    case 6:
        clipResult = max(abs(gl_Position.x), abs(gl_Position.y)) / gl_Position.w - distance;
        break;
    }

    gl_ClipDistance[0] = clipResult * float(ubo.sceneTransformation.y);

	outNormal = normalize(mat3(ubo.view * ubo.model) * inNormal);
}
