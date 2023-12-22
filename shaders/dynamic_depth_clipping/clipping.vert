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

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

out gl_PerVertex 
{
    vec4  gl_Position;
    float gl_ClipDistance[1];
};


layout (binding = 0) uniform Ubo
{
    mat4 projection;
    mat4 modelview;
    vec4 colorTransformation;
    ivec2 sceneTransformation;
} ubo;

layout (location = 0) out vec3 outNormal;

void main()
{
    gl_Position = ubo.projection * ubo.modelview * vec4(inPos, 1.0);

    float clipResult = 1.0;

    switch(ubo.sceneTransformation.x)
    {
    case 0:
        clipResult = (gl_Position.x*gl_Position.x + gl_Position.y*gl_Position.y) / (gl_Position.w*gl_Position.w) - 0.5;
        break;
    case 1:
        clipResult = sin(gl_Position.x / gl_Position.w * 3.0 * 2.0 * 3.1415);
        break;
    case 2:
        clipResult = sin(gl_Position.y / gl_Position.w * 3.0 * 2.0 * 3.1415);
        break;
    case 3:
        clipResult = (gl_Position.z / gl_Position.w) - 0.995;
        break;
    }

    gl_ClipDistance[0] = clipResult * float(ubo.sceneTransformation.y);

	outNormal = mat3(ubo.modelview) * inNormal;
}
