#version 460
/* Copyright (c) 2023, Qualcomm Innovation Center, Inc. All rights reserved.
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

layout(location = 0) in vec2 texCoord_frag;
layout(location = 1) in vec3 rayDirectionIn;

layout(location = 0) out vec4 o_color_0;
layout(location = 1) out vec4 o_color_1;
layout(location = 2) out vec4 rayDirectionOut;


layout(binding = 0) uniform sampler2D textureInput_0;
layout(binding = 1) uniform sampler2D textureInput_1;

void main(void)
{
    vec2 flipped = vec2( texCoord_frag.x, 1.0 - texCoord_frag.y );
	vec4 pixel_0 = texture(textureInput_0, flipped);
	// if (pixel_0.r == 0.0) discard;
	vec4 pixel_1 = texture(textureInput_1, flipped);
	o_color_0 = pixel_0;
	o_color_1 = pixel_1;
	
	rayDirectionOut.rgb = normalize(rayDirectionIn);
	rayDirectionOut.a = 1.0f;
	
	// For debugging only
	// o_color_0 = vec4( texCoord_frag.x, 0.0f, 0.0f, 1.0f);
}
