/* Copyright (c) 2024, Sascha Willems
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

struct VSInput
{
    [[vk::location(0)]] float3 Pos : POSITION0;
    [[vk::location(1)]] float3 Normal : NORMAL0;
    [[vk::location(2)]] float2 UV : TEXCOORD0;
};

// In a fixed pipeline approach which you can still find in OpenGL implementations you could define 
// up to maximum 6 clipping half-spaces and a distance to each clipping half-space was stored in gl_ClipDistance.
// That's why gl_ClipDistance[] is an array which maximum size is limited by maxClipDistances.
//
// In our example we show you how to define such half-spaces using plane equation: Ax+By+Cz+D=0
// You could transmit information about values of A,B,C and D using UBO for example, but for simplicity we will use const declaration:

static float4 planeValues = float4(0.0, 1.0, 0.0, 0.0);

struct UBO
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
    float4 colorTransformation;
    int2 sceneTransformation;
    float usePrimitiveClipping;
};
[[vk::binding(0, 0)]]
ConstantBuffer<UBO> ubo : register(b0);

// Additionally we will show you that you can use more than half-spaces only. 
// We will use some more advanced math functions to calculate gl_ClipDistance.

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float ClipDistance : SV_ClipDistance0;
    [[vk::location(0)]] float3 Normal : NORMAL0;
};

// Cases 0 and 1 present how to use world space coordinates with more advanced functions
// Case 2 shows how to use half-space in world space coordinates
// Case 3 shows how to use half-space in clip space coordinates
// Cases 4-6 present how to use clip space coordinates with more advanced functions
// Cases 0,1,4,5 use sin() function to create strips in which values of gl_ClipDistance below 0 cause triangle primitives
// to be clipped according to Vulkan specification chapter 27.4
// Cases 6-8 use different types of distance functions from center of the screen ( Euclidean, Manhattan, Chebyshev )

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    float4 worldPosition = mul(ubo.model, float4(input.Pos, 1.0));

    output.Pos = mul(ubo.projection, mul(ubo.view,  worldPosition));

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
            clipResult = dot(output.Pos, planeValues);
            break;
        case 4:
            clipResult = sin(output.Pos.x / output.Pos.w * 3.0 * 2.0 * 3.1415);
            break;
        case 5:
            clipResult = sin(output.Pos.y / output.Pos.w * 3.0 * 2.0 * 3.1415);
            break;
        case 6:
            clipResult = (output.Pos.x*output.Pos.x + output.Pos.y*output.Pos.y) / (output.Pos.w*output.Pos.w) - distance*distance;
            break;
        case 7:
            clipResult = (abs(output.Pos.x) + abs(output.Pos.y)) / output.Pos.w - distance;
            break;
        case 8:
            clipResult = max(abs(output.Pos.x), abs(output.Pos.y)) / output.Pos.w - distance;
            break;
        }

        output.ClipDistance = clipResult * float(ubo.sceneTransformation.y);
    }

    output.Normal = normalize(mul((float4x3)mul(ubo.view, ubo.model), input.Normal).xyz);

    return output;    
}
