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
#version 450

layout (triangles) in;
layout (line_strip, max_vertices = 2) out;

layout (binding = 1) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout (location = 0) in vec3 inNormal[];
layout (location = 0) out vec3 outColor;

void main(void)
{
    float normalLength_f = 0.1;

    //middle point of triangle
    vec3 pos = (gl_in[0].gl_Position.xyz + gl_in[1].gl_Position.xyz + gl_in[2].gl_Position.xyz)/3;
    vec3 normal = inNormal[0].xyz;

    //line vertices
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0);
    outColor = vec3(1.0, 0.0, 0.0);
    EmitVertex();

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos + normal * normalLength_f, 1.0);
    outColor = vec3(0.0, 0.0, 1.0);
    EmitVertex();

    EndPrimitive();
}
