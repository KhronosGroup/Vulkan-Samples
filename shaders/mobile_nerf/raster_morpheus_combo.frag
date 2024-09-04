/* Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
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
 *
 * ------------------------------------------------------------------------
 *
 * THIS IS A MODIFIED VERSION OF THE ORIGINAL FILE
 *
 * The original file, along with the original Apache-2.0 LICENSE can be found at:
 * https://github.com/google-research/jax3d/tree/main/jax3d/projects/mobilenerf
 *
 * Modification details: Shader code was updated to work on Vulkan (originally
 * built for WebGL)
 * Contributor: (Qualcomm) Rodrigo Holztrattner - quic_rholztra@quicinc.com
 */
#version 460

layout(location = 0) in vec2 texCoord_frag;
layout(location = 1) in vec3 rayDirectionIn;

layout(location = 0) out vec4 o_color_0;
layout(location = 1) out vec4 o_color_1;
layout(location = 2) out vec4 rayDirectionOut;
layout(location = 3) out uint weights_idx_out;

layout(binding = 0) uniform sampler2D textureInput_0;
layout(binding = 1) uniform sampler2D textureInput_1;

layout(push_constant) uniform PushConstants {
	uint weights_idx;
} pc;

void main(void)
{
    vec2 flipped = vec2( texCoord_frag.x, 1.0 - texCoord_frag.y );
	vec4 pixel_0 = texture(textureInput_0, flipped);
	// if (pixel_0.r == 0.0) discard;
	vec4 pixel_1 = texture(textureInput_1, flipped);
	o_color_0 = vec4(pixel_0.xyz, pixel_0.w);
	o_color_1 = vec4(pixel_1.xyz, pixel_1.w);

	rayDirectionOut.rgb = normalize(rayDirectionIn);
	rayDirectionOut.a = 1.0f;

	weights_idx_out = pc.weights_idx;
}
