#version 450
/* Copyright (c) 2024, Mobica Limited
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


layout (location = 0) in vec3 inNormal;

layout (binding = 0) uniform Ubo
{
    mat4 projection;
    mat4 view;
    mat4 model;
    vec4 colorTransformation;
    ivec2 sceneTransformation;
} ubo;

layout (location = 0) out vec4 outColor;

void main()
{

    outColor = vec4( 0.5 * inNormal + vec3(0.5), 1);

    outColor.xyz = ubo.colorTransformation.x * outColor.xyz + vec3(ubo.colorTransformation.y);
}
