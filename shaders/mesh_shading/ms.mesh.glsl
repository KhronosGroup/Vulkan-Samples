#version 450
#extension GL_EXT_mesh_shader : require

/* Copyright (c) 2023 Holochip Corporation
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

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_vertices = 3, max_primitives = 1) out;

void main()
{
  uint vertexCount = 3;
  uint triangleCount = 1;
  SetMeshOutputsEXT(vertexCount, triangleCount);
  gl_MeshVerticesEXT[0].gl_Position = vec4(0.5,-0.5, 0, 1);
  gl_MeshVerticesEXT[1].gl_Position = vec4(0.5, 0.5, 0, 1);
  gl_MeshVerticesEXT[2].gl_Position = vec4(-0.5, 0.5, 0, 1);
  gl_PrimitiveTriangleIndicesEXT[0] =  uvec3(0, 1, 2);
}
