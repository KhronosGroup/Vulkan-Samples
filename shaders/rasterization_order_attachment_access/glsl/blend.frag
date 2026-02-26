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

// Input attachment for framebuffer fetch using VK_KHR_dynamic_rendering_local_read
layout (input_attachment_index = 0, binding = 3) uniform subpassInput colorAttachment;

layout (location = 0) in vec4 inColor;

layout (location = 0) out vec4 outColor;

void main()
{
	// Read current framebuffer value
	vec4 dst = subpassLoad(colorAttachment);
	vec4 src = inColor;

	// Alpha blending: src over dst
	outColor.rgb = src.rgb * src.a + dst.rgb * (1.0 - src.a);
	outColor.a = 1.0;
}

