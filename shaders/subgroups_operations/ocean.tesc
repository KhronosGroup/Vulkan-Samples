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

layout (vertices = 3) out;

layout(location = 0) in vec2 inUv[];

layout(location = 0) out vec2 outUv[];

layout (binding = 3) uniform CameraPos
{
    vec4 position;
} cam;

float get_tesselllation_level(float dist0, float dist1)
{
    float avg_dist = (dist0 + dist1) / 2.0f;

    if (avg_dist <= 10.0f)
        return 3.0f;
    if (avg_dist <= 40.0f)
        return 2.0f;
    return 1.0f;
}

void main()
{
    vec3 p0 = vec3(-gl_in[0].gl_Position.x, gl_in[0].gl_Position.y, -gl_in[0].gl_Position.z);
    vec3 p1 = vec3(-gl_in[1].gl_Position.x, gl_in[1].gl_Position.y, -gl_in[1].gl_Position.z);
    vec3 p2 = vec3(-gl_in[2].gl_Position.x, gl_in[2].gl_Position.y, -gl_in[2].gl_Position.z);

    float dist_cam_v0 = distance(cam.position.xyz,  p0);
    float dist_cam_v1 = distance(cam.position.xyz,  p1);
    float dist_cam_v2 = distance(cam.position.xyz,  p2);

    gl_TessLevelOuter[0] = get_tesselllation_level(dist_cam_v1, dist_cam_v2);
    gl_TessLevelOuter[1] = get_tesselllation_level(dist_cam_v0, dist_cam_v2);
    gl_TessLevelOuter[2] = get_tesselllation_level(dist_cam_v0, dist_cam_v1);

    gl_TessLevelInner[0] = gl_TessLevelOuter[2];

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    outUv[gl_InvocationID] = inUv[gl_InvocationID];
}