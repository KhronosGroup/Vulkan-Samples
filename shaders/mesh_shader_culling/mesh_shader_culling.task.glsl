#version 450
/* Copyright (c) 2023, Holochip Corporation
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

#extension GL_EXT_mesh_shader: require
#extension GL_GOOGLE_include_directive: require
#extension GL_KHR_shader_subgroup_ballot : require                  // subgroupBallot, subgroupBallotBitCount, subgroupBallotExclusiveBitCount

#include "mesh_shader_shared.h"

// Use numTaskInvocationsX * numTaskInvocationsY task shader invocations per task shader workgroup,
// resulting in N * N * numTaskInvocationsX * numTaskInvocationsY invocations
layout(local_size_x = numTaskInvocationsX, local_size_y = numTaskInvocationsY, local_size_z = 1) in;

layout (binding = 0) uniform UBO
{
    float cull_center_x;
    float cull_center_y;
    float cull_radius;
    float meshlet_density;
} ubo;

taskPayloadSharedEXT SharedData sharedData;

float square(float x)
{
    return x * x;
}

float square(vec2 p )
{
    return p.x * p.x + p.y * p.y;
}

void main()
{
    // calculate size and offset of sub-rect per task shader invocation
    vec2 size = 2.0f / ( gl_NumWorkGroups.xy * gl_WorkGroupSize.xy );
    vec2 offset = gl_GlobalInvocationID.xy * size;

    // determine the four corners of the sub-rect and check if it's completely out of the culling circle
    // (ignoring the case that a sub-rect could completely include the culling circle...)

    vec2 position = vec2(ubo.cull_center_x, ubo.cull_center_y);
    vec2 p0 = position + offset;
    vec2 p1 = p0 + vec2(size.x, 0.0f);
    vec2 p2 = p0 + size;
    vec2 p3 = p0 + vec2(0.0f, size.y);
    float squaredRadius = square(ubo.cull_radius);
    bool isValid = ( square(p0) < squaredRadius )
                    && ( square(p1) < squaredRadius )
                    && ( square(p2) < squaredRadius)
                    && ( square(p3) < squaredRadius);
    // contribute a single bit by this task shader invocation
    uvec4 validVotes = subgroupBallot(isValid);

    if ( isValid )
    {
        // get the next free index into the offsets array
        uint index = subgroupBallotExclusiveBitCount(validVotes);
        sharedData.offsets[index] = offset;
    }
    if ( gl_LocalInvocationIndex == 0 )
    {
        // for just one task shader invocation we can emit mesh shaders
        sharedData.position = position;
        sharedData.size = size;
        // get the actual number of task shader invocations that are determined to display something
        uint validCount = subgroupBallotBitCount(validVotes);
        EmitMeshTasksEXT(validCount, 1, 1);
    }
}
