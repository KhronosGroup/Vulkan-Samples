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
	float3 Position : POSITION0;
	float3 Normal;
};

RaytracingAccelerationStructure topLevelAS;

struct GlobalUniform
{
	float4x4 view;
	float4x4 proj;
	float4 camera_position;
	float4 light_position;
};
ConstantBuffer<GlobalUniform> global_uniform;

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float4 O_Pos;
    float3 O_Normal;
	float4 Scene_pos;	// scene with respect to BVH coordinates
};

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;
	// We want to be able to perform ray tracing, so don't apply any matrix to scene_pos
	output.Scene_pos = float4(input.Position, 1.0);  
	output.O_Pos = mul(global_uniform.view, float4(input.Position, 1.0));
	output.O_Normal = input.Normal;
	output.Pos = mul(global_uniform.proj, mul(global_uniform.view, float4(input.Position, 1.0)));   
	return output;
}
