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

layout(location = 0) in vec3 in_color;

layout(location = 0) out vec4 out_color_r;
layout(location = 1) out vec4 out_color_g;
layout(location = 2) out vec4 out_color_b;

// The full color is copied to individual attachments.
// Each attachment has a single component bit (R, G, B) enabled
// via the blend_attachment in ColorWriteEnable::prepare_pipelines.
void main()
{
       out_color_r = vec4(in_color, 1.0f);
       out_color_g = vec4(in_color, 1.0f);
       out_color_b = vec4(in_color, 1.0f);
}
