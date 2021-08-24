#version 460

/* Copyright (c) 2021 Holochip Corporation
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

layout(location = 1) in vec2 in_uv;
layout(location = 2) flat in uint in_texture_index;

layout(location = 0) out vec4 o_color;

layout(binding = 1, set = 0) uniform sampler2D textures[256];

void main(void)
{
	o_color = vec4(texture(textures[uint(round(in_texture_index))], in_uv));
	o_color.rgb *= 1.5;
}