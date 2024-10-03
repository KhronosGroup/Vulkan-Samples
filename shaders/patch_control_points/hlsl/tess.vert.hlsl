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

struct VSInput
{
    [[vk::location(0)]] float3 Pos : POSITION0;
};

struct UBO
{
	float4x4 projection;
	float4x4 view;
};
[[vk::binding(0, 0)]]
ConstantBuffer<UBO> ubo : register(b0);

struct PushConstants
{
	float3 direction;
};
[[vk::push_constant]] PushConstants push_constants;

struct VSOutput
{
    float4 Pos : SV_POSITION;
};

#define PI 3.14159

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

	float4x4 rotX = float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	rotX[1][1]    = cos(PI);
	rotX[1][2]    = sin(PI);
	rotX[2][1]    = -sin(PI);
	rotX[2][2]    = cos(PI);
	
	output.Pos = mul(rotX, float4(input.Pos + push_constants.direction, 1.0f));

    return output;
}