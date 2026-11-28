/* Copyright (c) 2020-2026, Arm Limited and Contributors
/* Copyright (c) 2026, Sascha Willems
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
    float4 position : SV_Position;
    [[vk::location(0)]] float2 outUV;
};

[shader("vertex")]
VSOutput main(uint vertexIndex : SV_VertexID)
{
    VSOutput output;
    output.outUV = float2((vertexIndex << 1) & 2, vertexIndex & 2);
    output.position = float4(output.outUV * 2.0f - 1.0f, 0.0f, 1.0f);
    return output;
}
