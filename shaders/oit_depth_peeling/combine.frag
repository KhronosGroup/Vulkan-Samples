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

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

////////////////////////////////////////

layout(set = 0, binding = 0) uniform SceneConstants
{
	mat4  model_view_projection;
	float background_grayscale;
	float object_alpha;
	int   front_layer_index;
	int   back_layer_index;
} sceneConstants;

const int kLayersCount = 8;
layout(set = 0, binding = 2) uniform sampler2D layerTex[kLayersCount];

////////////////////////////////////////

void main()
{
	vec4 color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	const int front_layer_index = clamp(sceneConstants.front_layer_index, 0, kLayersCount - 1);
	const int back_layer_index = clamp(sceneConstants.back_layer_index, front_layer_index, kLayersCount - 1);
	for(int i = back_layer_index; i >= front_layer_index; --i)
	{
		const vec4 fragmentColor = texture(layerTex[i], inUV);
		color.rgb = mix(color.rgb, fragmentColor.rgb, fragmentColor.a);
		color.a *= 1.0f - fragmentColor.a;
	}
	color.a = 1.0f - color.a;
	outFragColor = color;
}

