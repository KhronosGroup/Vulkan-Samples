#version 450
/* Copyright (c) 2023, Maximilien Dagois
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

layout(set = 0, binding = 0) uniform SceneConstants
{
	mat4 projection;
	mat4 view;
	uvec4 parameters;
} sceneConstants;
layout(set = 0, binding = 1, r32ui) uniform uimage2D linkedListHeadTex;
layout(set = 0, binding = 2) buffer FragmentBuffer {
	uvec4 data[];
} fragmentBuffer;
layout(set = 0, binding = 3) buffer AtomicCounter {
	uint value;
} atomicCounter;

layout (location = 0) in vec4 inColor;

void main()
{
    // Find the next fragment index
    const uint nextFragmentIndex = atomicAdd(atomicCounter.value, 1U);

    // Ignore the fragment if the fragment buffer is full
    if(nextFragmentIndex >= sceneConstants.parameters.x)
    {
        discard;
    }

    // Update the linked list head
    const uint previousFragmentIndex = imageAtomicExchange(linkedListHeadTex, ivec2(gl_FragCoord.xy), nextFragmentIndex);

    // Add the fragment to the buffer
    fragmentBuffer.data[nextFragmentIndex] = uvec4(packUnorm4x8(inColor), floatBitsToUint(gl_FragCoord.z), previousFragmentIndex, 0U);
}

