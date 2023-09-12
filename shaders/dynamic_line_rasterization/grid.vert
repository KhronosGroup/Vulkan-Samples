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

layout(binding = 0) uniform Ubo {
    mat4 projection;
    mat4 view;
    mat4 model;
} ubo;

layout(location = 0) out vec3 nearPoint;
layout(location = 1) out vec3 farPoint;
layout(location = 2) out mat4 view;
layout(location = 6) out mat4 projection;


vec3 unprojectPoint(float x, float y, float z, mat4 viewProjectionInverse) {
    vec4 clipSpacePos = vec4(x, y, z, 1.0);
    vec4 eyeSpacePos = viewProjectionInverse * clipSpacePos;
    return eyeSpacePos.xyz / eyeSpacePos.w;
}

vec3 gridPlane[6] = vec3[](
    vec3(1, 1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
    vec3(-1, -1, 0), vec3(1, 1, 0), vec3(1, -1, 0)
);

void main() {
    vec3 pos = gridPlane[gl_VertexIndex].xyz;
    mat4 viewProjectionInverse = inverse(ubo.projection * ubo.view);

    nearPoint = unprojectPoint(pos.x, pos.y, 0.0, viewProjectionInverse);
    farPoint = unprojectPoint(pos.x, pos.y, 1.0, viewProjectionInverse);

    view = ubo.view;
    projection = ubo.projection;

    gl_Position = vec4(pos, 1.0);


}
