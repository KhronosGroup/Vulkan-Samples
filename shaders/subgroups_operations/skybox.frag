#version 450
/* Copyright (c) 2023, Mobica Limited
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

layout (location = 0) out vec4 outFragColor;

layout (location = 0) in vec3 inUV;

layout (set = 0, binding = 1) uniform samplerCube skybox_texture_map;

void main(void)
{
    vec3 normal = normalize(inUV);
    vec4 color = texture(skybox_texture_map, inUV);
    outFragColor = vec4(color.rgb, 1.0);
}
