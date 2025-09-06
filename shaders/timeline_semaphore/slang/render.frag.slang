/* Copyright (c) 2021-2025, Arm Limited and Contributors
 * Copyright (c) 2025, Sascha Willems
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

struct VSOutput
{
    float4 Pos : SV_Position;
    float2 UV;
}

Sampler2D in_image;

float4 convert_color(float4 value)
{
    float gray = dot(value.rgb, float3(0.3, 0.6, 0.1));
    return float4(lerp(float3(gray), value.rgb, value.a), 1.0);
}

[shader("fragment")]
float4 main(VSOutput input)
{
    return convert_color(in_image.SampleLevel(input.UV, 0.0));
}
