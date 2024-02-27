#version 450
/* Copyright (c) 2024, Google
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

layout (location = 0) in vec4 inColor;

layout (location = 0) out vec4 outFragColor;

////////////////////////////////////////

layout(set = 0, binding = 1) uniform sampler2D depthTex;

////////////////////////////////////////

void main()
{
	// 'depthTex' contains the depth information of the previous layer.
	// That information is used to discard any fragment that belongs to
	// a layer that has already be rendered (i.e. a layer further in front).
	const float layerDepth = texelFetch(depthTex, ivec2(gl_FragCoord.xy), 0).r;
	if(gl_FragCoord.z >= layerDepth)
	{
		// NOTE
		// Remember that a reversed Z-buffer is used here.
		// Fragments closer to the camera have a higher z coordinate.
		discard;
	}

	outFragColor = inColor;
}

