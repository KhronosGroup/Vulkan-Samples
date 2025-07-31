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

Texture2D texture : register(t2);
SamplerState samplerTexture : register(s2);

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Color : COLOR0;
    float2 UV : TEXCOORD0;
};

[[vk::constant_id(0)]] const float NEAR_PLANE = 0.1;
[[vk::constant_id(1)]] const float FAR_PLANE = 256.0;

float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));	
}

float4 main(VSOutput input) : SV_TARGET
{
	// Sample depth from deferred depth buffer and discard if obscured
	float depth = positionDepthAttachment.SubpassLoad().a;

    // Save the sampled texture color before discarding.
    // This is to avoid implicit derivatives in non-uniform control flow.
    // @todo: reversed depth
    float4 sampledColor = texture.Sample(samplerTexture, input.UV);
	if ((depth != 0.0) && (linearDepth(input.Pos.z) < depth))
	{
//		discard;
	};

	return sampledColor;
}
