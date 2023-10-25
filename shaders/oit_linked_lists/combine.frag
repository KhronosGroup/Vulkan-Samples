#version 450
/* Copyright (c) 2023, Google
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LINKED_LIST_END_SENTINEL	0xFFFFFFFFU

// For performance reasons, this should be kept as low as result correctness allows.
#define SORTED_FRAGMENT_MAX_COUNT	16U

////////////////////////////////////////

layout(set = 0, binding = 0) uniform SceneConstants
{
	mat4  projection;
	mat4  view;
	float background_grayscale;
	uint  sortFragments;
	uint  fragmentMaxCount;
	uint  sortedFragmentCount;
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

// Blend two colors.
// The alpha channel keeps track of the amount of visibility of the background.
vec4 blendColors(uint packedSrcColor, vec4 dstColor)
{
	const vec4 srcColor = unpackUnorm4x8(packedSrcColor);
	return vec4(
		mix(dstColor.rgb, srcColor.rgb, srcColor.a),
		dstColor.a * (1.0f - srcColor.a));
}

// Sort and blend fragments from the linked list.
// For performance reasons, the maximum number of sorted fragments is limited.
// Approximations are used when the number of fragments is over the limit.
vec4 mergeWithSort(uint firstFragmentIndex)
{
	// Fragments are sorted from back to front.
	// e.g. sortedFragments[0] is the farthest from the camera.
	uvec2 sortedFragments[SORTED_FRAGMENT_MAX_COUNT];
	uint sortedFragmentCount = 0U;
	const uint kSortedFragmentMaxCount = min(sceneConstants.sortedFragmentCount, SORTED_FRAGMENT_MAX_COUNT);

	vec4 color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	uint fragmentIndex = firstFragmentIndex;
	while(fragmentIndex != LINKED_LIST_END_SENTINEL)
	{
		const uvec3 fragment = fragmentBuffer.data[fragmentIndex];
		fragmentIndex = fragment.x;

		if(sortedFragmentCount < kSortedFragmentMaxCount)
		{
			// There is still room in the sorted list.
			// Insert the fragment so that the list stay sorted.
			uint i = sortedFragmentCount;
			for(; (i > 0) && (fragment.z < sortedFragments[i - 1].y); --i)
			{
				sortedFragments[i] = sortedFragments[i - 1];
			}
			sortedFragments[i] = fragment.yz;
			++sortedFragmentCount;
		}
		else if(sortedFragments[0].y < fragment.z)
		{
			// The fragment is closer than the farthest sorted one.
			// First, make room by blending the farthest fragment from the sorted list.
			// Then, insert the fragment in the sorted list.
            // This is an approximation.
			color = blendColors(sortedFragments[0].x, color);
			uint i = 0;
			for(; (i < kSortedFragmentMaxCount - 1) && (sortedFragments[i + 1].y < fragment.z); ++i)
			{
				sortedFragments[i] = sortedFragments[i + 1];
			}
			sortedFragments[i] = fragment.yz;
		}
		else
		{
			// The next fragment is farther than any of the sorted ones.
			// Blend it early.
            // This is an approximation.
			color = blendColors(fragment.y, color);
		}
	}

	// Early return if there are no fragments.
	if(sortedFragmentCount == 0)
	{
		return vec4(0.0f);
	}

	// Blend the sorted fragments to get the final color.
	for(int i = 0; i < sortedFragmentCount; ++i)
	{
		color = blendColors(sortedFragments[i].x, color);
	}
	color.a = 1.0f - color.a;
	return color;
}

// Blend fragments from the linked list without sorting them.
vec4 mergeWithoutSort(uint firstFragmentIndex)
{
	vec4 color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	uint fragmentIndex = firstFragmentIndex;
	while(fragmentIndex != LINKED_LIST_END_SENTINEL)
	{
		const uvec3 fragment = fragmentBuffer.data[fragmentIndex];
		fragmentIndex = fragment.x;
		color = blendColors(fragment.y, color);
	}
	color.a = 1.0f - color.a;
	return color;
}

void main()
{
	// Reset the atomic counter for the next frame.
	// Note that we don't care about atomicity here, as all threads will write the same value.
	fragmentCounter.value = 0;

	// Get the first fragment index in the linked list.
	uint fragmentIndex = imageLoad(linkedListHeadTex, ivec2(gl_FragCoord.xy)).r;
	// Reset the list head for the next frame.
	imageStore(linkedListHeadTex, ivec2(gl_FragCoord.xy), uvec4(LINKED_LIST_END_SENTINEL, 0, 0, 0));

	// Compute the final color.
	outFragColor = (sceneConstants.sortFragments == 1U) ? mergeWithSort(fragmentIndex) : mergeWithoutSort(fragmentIndex);
}

