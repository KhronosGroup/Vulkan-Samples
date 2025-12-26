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

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float4 Color;
    float3 WorldPos;
};

[[SpecializationConstant]] const float NEAR_PLANE = 0.1;
[[SpecializationConstant]] const float FAR_PLANE = 256.0;

float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));	
}

struct FSOutput
{
    float4 Color;
    float4 PositionDepth;
    float4 Normal;
    float4 Albedo;
};

[shader("fragment")]
FSOutput main(VSOutput input)
{
    FSOutput output;
	float3 N = normalize(input.Normal);
	N.y = -N.y;
	output.Normal = float4(N, 1.0);
	output.Albedo = input.Color;
	// Store linearized depth in alpha component
    output.PositionDepth.rgb = input.WorldPos;
    output.PositionDepth.a = linearDepth(input.Pos.z);
    // Write color attachments to avoid undefined behaviour (validation error)
    output.Color = (1.0).rrrr;
	return output;
}