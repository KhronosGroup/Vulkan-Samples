/* Copyright (c) 2024, Sascha Willems
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

[[vk::input_attachment_index(0)]][[vk::binding(0)]] SubpassInput in_color_r;
[[vk::input_attachment_index(1)]][[vk::binding(1)]] SubpassInput in_color_g;
[[vk::input_attachment_index(2)]][[vk::binding(2)]] SubpassInput in_color_b;

float4 main() : SV_TARGET0
{
    float4 color_r = in_color_r.SubpassLoad();
    float4 color_g = in_color_g.SubpassLoad();
    float4 color_b = in_color_b.SubpassLoad();

    return color_r + color_g + color_b;
}
