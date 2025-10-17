/* Copyright (c) 2025, Holochip Inc.
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
#extension GL_EXT_shader_quad_control : require

// Demonstrate quad control using the execution mode only. We avoid explicit
// subgroup quad operations to keep SPIR-V requirements minimal.
layout(full_quads) in;

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

void main()
{
    // Visualize UVs as color. With full_quads, helper invocations are managed
    // to form full quads which can affect implicit derivatives. For a simple
    // visualization, just output vUV.
    outColor = vec4(vUV, 0.5, 1.0);
}
