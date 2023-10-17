#version 450
/* Copyright (c) 2023, Google
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

#define LINKED_LIST_END_SENTINEL	0xFFFFFFFFU
#define SORTED_FRAGMENT_MAX_COUNT	16U

////////////////////////////////////////

layout(set = 0, binding = 0) uniform SceneConstants
{
	mat4 projection;
	mat4 view;
	uvec2 unused;
	uint sortFragments;
	uint fragmentMaxCount;
} sceneConstants;

layout(set = 0, binding = 2, r32ui) uniform uimage2D linkedListHeadTex;

layout(set = 0, binding = 3) buffer FragmentBuffer {
	uvec3 data[];
} fragmentBuffer;

layout(set = 0, binding = 4) buffer FragmentCounter {
	uint value;
} fragmentCounter;

////////////////////////////////////////

layout (location = 0) out vec4 outFragColor;

////////////////////////////////////////

void main()
{
    // Reset the atomic counter for the next frame
	// Note that we don't care about atomicity here, as all threads will write the same value
    fragmentCounter.value = 0;

    // Get the first fragment index in the linked list
	uint fragmentIndex = imageLoad(linkedListHeadTex, ivec2(gl_FragCoord.xy)).r;
	// Reset the list head for the next frame
	imageStore(linkedListHeadTex, ivec2(gl_FragCoord.xy), uvec4(LINKED_LIST_END_SENTINEL, 0, 0, 0));

    // Copy the fragments into local memory for sorting
    uvec2 sortedFragments[SORTED_FRAGMENT_MAX_COUNT];
    uint fragmentCount = 0U;
    while(fragmentIndex != LINKED_LIST_END_SENTINEL && fragmentCount < SORTED_FRAGMENT_MAX_COUNT)
    {
        const uvec3 fragment = fragmentBuffer.data[fragmentIndex];
        fragmentIndex = fragment.x;
        sortedFragments[fragmentCount] = fragment.yz;
        ++fragmentCount;
    }

	// Early return if there are no fragments
    if(fragmentCount == 0)
    {
		outFragColor = vec4(0.0f);
        return;
    }

    // Sort the fragments by depth (back to front)
	if(sceneConstants.sortFragments == 1U)
    {
        for(int i = 0; i < fragmentCount - 1; ++i)
        {
            for(int j = i + 1; j < fragmentCount; ++j)
            {
                if(sortedFragments[j].y < sortedFragments[i].y)
                {
                    const uvec2 tmp = sortedFragments[i];
                    sortedFragments[i] = sortedFragments[j];
                    sortedFragments[j] = tmp;
                }
            }
        }
    }

    // Merge the fragments to get the final color
    vec4 color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    for(int i = 0; i < fragmentCount; ++i)
    {
		const vec4 fragmentColor = unpackUnorm4x8(sortedFragments[i].x);
		color.rgb = mix(color.rgb, fragmentColor.rgb, fragmentColor.a);
    	color.a *= 1.0f - fragmentColor.a;
    }
    color.a = 1.0f - color.a;
    outFragColor = color;
}

