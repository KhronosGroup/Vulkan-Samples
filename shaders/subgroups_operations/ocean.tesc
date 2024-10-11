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

void main()
{
   if (gl_InvocationID == 0)
   {
       gl_TessLevelOuter[0] = 1.0f;
       gl_TessLevelOuter[1] = 1.0f;
       gl_TessLevelOuter[2] = 1.0f;

       gl_TessLevelInner[0] = 2.0f;
       gl_TessLevelInner[1] = 1.0f;
   }

   gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}