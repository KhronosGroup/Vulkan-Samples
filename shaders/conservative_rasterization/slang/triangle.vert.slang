/* Copyright (c) 2025, Sascha Willems
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
	float3 Color;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Color;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;
    output.Color = input.Color;
    output.Pos = mul(ubo.projection, mul(ubo.model, float4(input.Pos, 1.0)));
    return output;
}