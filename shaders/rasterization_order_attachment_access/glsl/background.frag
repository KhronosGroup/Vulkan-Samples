#version 450
/* Copyright (c) 2026, Arm Limited and Contributors
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

layout (location = 0) in vec2 inUV;

layout (set = 0, binding = 0) uniform SceneConstants
{
	mat4  projection;
	mat4  view;
	float background_grayscale;
} sceneConstants;

layout (set = 0, binding = 2) uniform sampler2D backgroundTex;

layout (location = 0) out vec4 outColor;

void main()
{
	outColor.rgb = texture(backgroundTex, inUV).rgb * sceneConstants.background_grayscale;
	outColor.a = 1.0;
}

