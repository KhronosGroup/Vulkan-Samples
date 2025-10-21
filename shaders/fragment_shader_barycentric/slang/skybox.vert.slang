/* Copyright (c) 2023-2025, Mobica Limited
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

struct VSInput
{
    float3 Pos;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 UVW;
};

struct UBO {
    float4x4 projection;
    float4x4 modelview;
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;
    output.Pos = mul(ubo.projection, float4(mul((float3x3)ubo.modelview, input.Pos), 1.0));
    output.UVW = input.Pos;
    return output;
}