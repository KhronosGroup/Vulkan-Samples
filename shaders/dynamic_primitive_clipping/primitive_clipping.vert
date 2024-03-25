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
// In our example we show you how to define such half-spaces using plane equation: Ax+By+Cz+D=0
// You could transmit information about values of A,B,C and D using UBO for example, but for simplicity we will use const declaration:

const vec4 planeValues = vec4(0.0, 1.0, 0.0, 0.0);

// Additionally we will show you that you can use more than half-spaces only. 
// We will use some more advanced math functions to calculate gl_ClipDistance.

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
    float usePrimitiveClipping;
} ubo;

layout (location = 0) out vec3 outNormal;

// Cases 0 and 1 present how to use world space coordinates with more advanced functions
// Case 2 shows how to use half-space in world space coordinates
// Case 3 shows how to use half-space in clip space coordinates
// Cases 4-6 present how to use clip space coordinates with more advanced functions
// Cases 0,1,4,5 use sin() function to create strips in which values of gl_ClipDistance below 0 cause triangle primitives
// to be clipped according to Vulkan specification chapter 27.4
// Cases 6-8 use different types of distance functions from center of the screen ( Euclidean, Manhattan, Chebyshev )

void main()
{
    vec4 worldPosition =  ubo.model * vec4(inPos, 1.0);
    gl_Position        = ubo.projection * ubo.view * worldPosition;

    float clipResult = 1.0, tmp;
    float distance = 0.4;

    // Primitive clipping does not have any vkCmd* command that turns it off.
    // If we want to turn it off - we have to transfer this information to shader through UBO variable and
    // then use code similar to this:
    if(ubo.usePrimitiveClipping > 0.0)
    {
        switch(ubo.sceneTransformation.x)
        {
        case 0:
            clipResult = sin(worldPosition.x * 0.1 * 2.0 * 3.1415);
            break;
        case 1:
            clipResult = sin(worldPosition.y * 0.1 * 2.0 * 3.1415);
            break;
        case 2:
            clipResult = dot(worldPosition, planeValues );
            break;
        case 3:
            clipResult = dot(gl_Position, planeValues);
            break;
        case 4:
            clipResult = sin(gl_Position.x / gl_Position.w * 3.0 * 2.0 * 3.1415);
            break;
        case 5:
            clipResult = sin(gl_Position.y / gl_Position.w * 3.0 * 2.0 * 3.1415);
            break;
        case 6:
            clipResult = (gl_Position.x*gl_Position.x + gl_Position.y*gl_Position.y) / (gl_Position.w*gl_Position.w) - distance*distance;
            break;
        case 7:
            clipResult = (abs(gl_Position.x) + abs(gl_Position.y)) / gl_Position.w - distance;
            break;
        case 8:
            clipResult = max(abs(gl_Position.x), abs(gl_Position.y)) / gl_Position.w - distance;
            break;
        }

        gl_ClipDistance[0] = clipResult * float(ubo.sceneTransformation.y);
    }

    outNormal = normalize(mat3(ubo.view * ubo.model) * inNormal);
}
