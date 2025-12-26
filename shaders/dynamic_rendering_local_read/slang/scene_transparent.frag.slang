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

[[vk::input_attachment_index(0)]] SubpassInput positionDepthAttachment;

[[vk::binding(2, 0)]] Sampler2D samplerTexture;

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Color;
    float2 UV;
};

[[SpecializationConstant]] const float NEAR_PLANE = 0.1;
[[SpecializationConstant]] const float FAR_PLANE = 256.0;

float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));	
}

[shader("fragment")]
float4 main(VSOutput input)
{
	// Sample depth from deferred depth buffer and discard if obscured
	float depth = positionDepthAttachment.SubpassLoad().a;

    // Save the sampled texture color before discarding.
    // This is to avoid implicit derivatives in non-uniform control flow.
    float4 sampledColor = samplerTexture.Sample(input.UV);
	if ((depth != 0.0) && (linearDepth(input.Pos.z) < depth))
	{
//		discard;
	};

	return sampledColor;
}
