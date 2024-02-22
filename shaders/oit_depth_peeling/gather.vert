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

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;

layout (location = 0) out vec4 outColor;

////////////////////////////////////////

layout(set = 0, binding = 0) uniform SceneConstants
{
	mat4  model_view_projection;
	float background_grayscale;
	float object_alpha;
	int   front_layer_index;
	int   back_layer_index;
} sceneConstants;

////////////////////////////////////////

void main()
{
	gl_Position = sceneConstants.model_view_projection * vec4(inPos, 1.0f);
	outColor = vec4(inUV, 0.0f, sceneConstants.object_alpha);
}

