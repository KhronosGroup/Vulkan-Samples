#version 450
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

// Should be zero, also rename in other shader
layout (input_attachment_index = 0, binding = 0) uniform subpassInput positionDepthAttachment;

layout (binding = 2) uniform sampler2D samplerTexture;

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (constant_id = 0) const float NEAR_PLANE = 0.1f;
layout (constant_id = 1) const float FAR_PLANE = 256.0f;

float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));	
}

void main () 
{
	// Sample depth from deferred depth buffer and discard if obscured
	float depth = subpassLoad(positionDepthAttachment).a;

	// Save the sampled texture color before discarding.
	// This is to avoid implicit derivatives in non-uniform control flow.
	// @todo: reversed depth
	vec4 sampledColor = texture(samplerTexture, inUV);
	if ((depth != 0.0) && (linearDepth(gl_FragCoord.z) < depth))
	{
//		discard;
	};

	outColor = sampledColor;
}
